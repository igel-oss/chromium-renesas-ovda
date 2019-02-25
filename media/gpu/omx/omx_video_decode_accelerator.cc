// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "media/gpu/omx/omx_video_decode_accelerator.h"

#include <libdrm/drm_fourcc.h>
#include "base/bind.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "media/base/bitstream_buffer.h"
#include "media/video/picture.h"
#include "third_party/openmax/il/OMXR_Extension_vdcmn.h"
#include "ui/gl/egl_util.h"

#include "media/gpu/omx/omx_stubs.h"

#define PAGE_SIZE 4096

#define VLOGF(level) VLOG(level) << __func__ << "(): "

using media_gpu_omx::kModuleOmx;
using media_gpu_omx::kModuleMmngr;
using media_gpu_omx::kModuleMmngrbuf;
using media_gpu_omx::InitializeStubs;
using media_gpu_omx::StubPathMap;

static const base::FilePath::CharType kOMXLib[] =
    FILE_PATH_LITERAL("/usr/lib/libomxr_core.so");

static const base::FilePath::CharType kMMNGRLib[] =
    FILE_PATH_LITERAL("/usr/lib/libmmngr.so.1");

static const base::FilePath::CharType kMMNGRBufLib[] =
    FILE_PATH_LITERAL("/usr/lib/libmmngrbuf.so.1");

namespace media {

enum { kNumPictureBuffers = 8 };

// Delay between polling for texture sync status. 5ms feels like a good
// compromise, allowing some decoding ahead (up to 3 frames/vsync) to compensate
// for more difficult frames.
enum { kSyncPollDelayMs = 5 };

OmxVideoDecodeAccelerator::BitstreamBufferRef::BitstreamBufferRef(
    const media::BitstreamBuffer &buf,
    scoped_refptr<base::SingleThreadTaskRunner> tr,
    base::WeakPtr<Client> cl)
    : task_runner(tr),
      client(cl) {
  id = buf.id();
  size = buf.size();
  shm = std::make_unique<base::SharedMemory> (buf.handle(), true);
  shm->Map(size);
  memory = shm->memory();
}

OmxVideoDecodeAccelerator::BitstreamBufferRef::~BitstreamBufferRef() {
    if (id < 0)
        return;
    task_runner->PostTask(FROM_HERE, base::Bind(
     &Client::NotifyEndOfBitstreamBuffer, client, id));
}

OmxVideoDecodeAccelerator::OutputPicture::OutputPicture(
  const OmxVideoDecodeAccelerator &dec,
  media::PictureBuffer pbuffer,
  OMX_BUFFERHEADERTYPE* obuffer,
  EGLImageKHR eimage,
  struct MmngrBuffer mbuf)
  : decoder(dec), picture_buffer(pbuffer),
    omx_buffer_header(obuffer),
    egl_image(eimage), mmngr_buf(mbuf),
    at_component(false),
    allocated(false) {}

OMX_ERRORTYPE OmxVideoDecodeAccelerator::OutputPicture::FreeOMXHandle() {
  OMX_BUFFERHEADERTYPE* obuffer = omx_buffer_header;
  if (!obuffer)
    return OMX_ErrorNone;

  omx_buffer_header = NULL;
  return OMX_FreeBuffer(decoder.component_handle_, decoder.output_port_, obuffer);
}

OmxVideoDecodeAccelerator::OutputPicture::~OutputPicture() {

    VLOGF(1) << "Deleting picture " << picture_buffer.id();

    FreeOMXHandle();

    mmngr_export_end_in_user_ext(mmngr_buf.dmabuf_id);
    mmngr_free_in_user_ext(mmngr_buf.mem_id);
    eglDestroyImageKHR(decoder.egl_display_, egl_image);

    if (decoder.client_)
      decoder.client_->DismissPictureBuffer(picture_buffer.id());
}

// Maps h264-related Profile enum values to OMX_VIDEO_AVCPROFILETYPE values.
static OMX_U32 MapH264ProfileToOMXAVCProfile(uint32_t profile) {
  switch (profile) {
    case media::H264PROFILE_BASELINE:
      return OMX_VIDEO_AVCProfileBaseline;
    case media::H264PROFILE_MAIN:
      return OMX_VIDEO_AVCProfileMain;
    case media::H264PROFILE_EXTENDED:
      return OMX_VIDEO_AVCProfileExtended;
    case media::H264PROFILE_HIGH:
      return OMX_VIDEO_AVCProfileHigh;
    case media::H264PROFILE_HIGH10PROFILE:
      return OMX_VIDEO_AVCProfileHigh10;
    case media::H264PROFILE_HIGH422PROFILE:
      return OMX_VIDEO_AVCProfileHigh422;
    case media::H264PROFILE_HIGH444PREDICTIVEPROFILE:
      return OMX_VIDEO_AVCProfileHigh444;
    // Below enums don't have equivalent enum in Openmax.
    case media::H264PROFILE_SCALABLEBASELINE:
    case media::H264PROFILE_SCALABLEHIGH:
    case media::H264PROFILE_STEREOHIGH:
    case media::H264PROFILE_MULTIVIEWHIGH:
      // Nvidia OMX video decoder requires the same resources (as that of the
      // High profile) in every profile higher to the Main profile.
      return OMX_VIDEO_AVCProfileHigh444;
    default:
      NOTREACHED();
      return OMX_VIDEO_AVCProfileMax;
  }
}

// Helper macros for dealing with failure.  If |result| evaluates false, emit
// |log| to ERROR, register |error| with the decoder, and return |ret_val|
// (which may be omitted for functions that return void).
#define RETURN_ON_FAILURE(result, log, error, ret_val)             \
  do {                                                             \
    if (!(result)) {                                               \
      DLOG(ERROR) << log;                                          \
      StopOnError(error);                                          \
      return ret_val;                                              \
    }                                                              \
  } while (0)

// OMX-specific version of RETURN_ON_FAILURE which compares with OMX_ErrorNone.
#define RETURN_ON_OMX_FAILURE(omx_result, log, error, ret_val)          \
  RETURN_ON_FAILURE(                                                    \
      ((omx_result) == OMX_ErrorNone),                                  \
      log << ", OMX result: 0x" << std::hex << omx_result,              \
      error, ret_val)

OmxVideoDecodeAccelerator::OmxVideoDecodeAccelerator(
    EGLDisplay egl_display,
    const base::Callback<bool(void)>& make_context_current)
    : child_task_runner_(base::ThreadTaskRunnerHandle::Get()),
      component_handle_(NULL),
      weak_this_factory_(this),
      init_begun_(false),
      init_done_cond_(&init_lock_),
      client_state_(OMX_StateMax),
      current_state_change_(NO_TRANSITION),
      input_buffer_count_(0),
      input_buffer_size_(0),
      input_port_(0),
      input_buffers_at_component_(0),
      first_input_buffer_sent_(false),
      previous_frame_has_data_(false),
      output_port_(0),
      output_buffers_at_component_(0),
      egl_display_(egl_display),
      make_context_current_(make_context_current),
      codec_(UNKNOWN),
      h264_profile_(OMX_VIDEO_AVCProfileMax) {
  weak_this_ = weak_this_factory_.GetWeakPtr();
  static bool omx_functions_initialized = PostSandboxInitialization();
  RETURN_ON_FAILURE(omx_functions_initialized,
                    "Failed to load openmax library", PLATFORM_FAILURE,);
  RETURN_ON_OMX_FAILURE(OMX_Init(), "Failed to init OpenMAX core",
                        PLATFORM_FAILURE,);
}

OmxVideoDecodeAccelerator::~OmxVideoDecodeAccelerator() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK(free_input_buffers_.empty());
  DCHECK_EQ(0, input_buffers_at_component_);
  DCHECK_EQ(0, output_buffers_at_component_);
  DCHECK(pictures_.empty());
}

// This is to initialize the OMX data structures to default values.
template <typename T>
static void InitParam(T* param) {
  memset(param, 0, sizeof(T));
  param->nVersion.nVersion = 0x00000101;
  param->nSize = sizeof(T);
}

static const VideoCodecProfile kSupportedProfiles[] = {
    H264PROFILE_BASELINE,
    H264PROFILE_MAIN,
    H264PROFILE_HIGH,
    VP8PROFILE_ANY
};

VideoDecodeAccelerator::SupportedProfiles
OmxVideoDecodeAccelerator::GetSupportedProfiles() {
    VideoDecodeAccelerator::SupportedProfiles profiles;

    for (const auto& profile : kSupportedProfiles) {
        const auto kMinSize = gfx::Size(130,98);
        const auto kMaxSize = gfx::Size(1920,1080);
        VideoDecodeAccelerator::SupportedProfile supp_profile;
        supp_profile.profile = profile;
        supp_profile.min_resolution = kMinSize;
        supp_profile.max_resolution = kMaxSize;
        supp_profile.encrypted_only = false;
        profiles.push_back(supp_profile);
    }
    return profiles;
}

bool OmxVideoDecodeAccelerator::Initialize(const Config& config, Client* client) {
  auto profile = config.profile;
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (profile >= media::H264PROFILE_MIN && profile <= media::H264PROFILE_MAX) {
    codec_ = H264;
    h264_profile_ = MapH264ProfileToOMXAVCProfile(profile);
    RETURN_ON_FAILURE(h264_profile_ != OMX_VIDEO_AVCProfileMax,
                      "Unexpected profile", INVALID_ARGUMENT, false);
    h264_parser_.reset(new H264Parser);
  } else if (profile >= media::VP8PROFILE_MIN && profile <= media::VP8PROFILE_MAX) {
    codec_ = VP8;
  } else {
    RETURN_ON_FAILURE(false, "Unsupported profile: " << profile,
                      INVALID_ARGUMENT, false);
  }

  // Make sure that we have a context we can use for EGL image binding.
  RETURN_ON_FAILURE(make_context_current_.Run(),
                    "Failed make context current",
                    PLATFORM_FAILURE,
                    false);

  client_ptr_factory_.reset(new base::WeakPtrFactory<Client>(client));
  client_ = client_ptr_factory_->GetWeakPtr();

  if (!decode_task_runner_) {
    decode_task_runner_ = child_task_runner_;
    decode_client_ = client_;
  }

  if (!config.supported_output_formats.empty() &&
      !base::ContainsValue(config.supported_output_formats,
        PIXEL_FORMAT_NV12))
    return false;

  RETURN_ON_FAILURE(gl::GLFence::IsSupported(),
                    "Platform does not support GL fences",
                    PLATFORM_FAILURE,
                    false);

  if (!CreateComponent())  // Does its own RETURN_ON_FAILURE dances.
    return false;
  if (!DecoderSpecificInitialization())  // Does its own RETURN_ON_FAILURE dances.
    return false;

  deferred_init_allowed_ = config.is_deferred_initialization_allowed;

  VLOGF(1) << "Deferred initialization " << (deferred_init_allowed_ ? "allowed" : "not allowed");

  DCHECK_EQ(current_state_change_, NO_TRANSITION);
  current_state_change_ = INITIALIZING;
  BeginTransitionToState(OMX_StateIdle);

  if (!AllocateInputBuffers())  // Does its own RETURN_ON_FAILURE dances.
    return false;
  if (!AllocateFakeOutputBuffers())  // Does its own RETURN_ON_FAILURE dances.
    return false;

  init_begun_ = true;
  input_buffer_offset_ = 0;


  if (deferred_init_allowed_)
    return true;

  /* Wait until we reach executing if deferred init is not allowed */
  /* TODO(dhobsong): timeout */

  base::AutoLock auto_lock_(init_lock_);
  while (current_state_change_ == INITIALIZING) {
    init_done_cond_.Wait();
  }
  VLOGF(1) << "Sync Initialization complete";
  return true;

}

bool OmxVideoDecodeAccelerator::CreateComponent() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  OMX_CALLBACKTYPE omx_accelerator_callbacks = {
    &OmxVideoDecodeAccelerator::EventHandler,
    &OmxVideoDecodeAccelerator::EmptyBufferCallback,
    &OmxVideoDecodeAccelerator::FillBufferCallback
  };

  OMX_STRING role_name = codec_ == H264 ?
      const_cast<OMX_STRING>("video_decoder.avc") :
      const_cast<OMX_STRING>("video_decoder.vpx");

  // Get the first component for this role and set the role on it.

  OMX_U32 num_components = 1;
  char *component;
  component = new char[OMX_MAX_STRINGNAME_SIZE];

  OMX_ERRORTYPE result = OMX_GetComponentsOfRole(
      role_name, &num_components,
      reinterpret_cast<OMX_U8**>(&component));
  RETURN_ON_OMX_FAILURE(result, "Unsupported role: " << role_name,
                        PLATFORM_FAILURE, false);
  RETURN_ON_FAILURE(num_components == 1, "No components for: " << role_name,
                    PLATFORM_FAILURE, false);

  VLOG(1) << "Got component " << component << " for role: " << role_name;

  // Get the handle to the component.
  result = OMX_GetHandle(
      &component_handle_,
      reinterpret_cast<OMX_STRING>(component),
      this, &omx_accelerator_callbacks);
  delete[] component;

  RETURN_ON_OMX_FAILURE(result,
                        "Failed to OMX_GetHandle on: " << component,
                        PLATFORM_FAILURE, false);
  client_state_ = OMX_StateLoaded;

  // Get the port information. This will obtain information about the number of
  // ports and index of the first port.
  OMX_PORT_PARAM_TYPE port_param;
  InitParam(&port_param);
  result = OMX_GetParameter(component_handle_, OMX_IndexParamVideoInit,
                            &port_param);
  RETURN_ON_FAILURE(result == OMX_ErrorNone && port_param.nPorts == 2,
                    "Failed to get Port Param: " << result << ", "
                    << port_param.nPorts,
                    PLATFORM_FAILURE, false);

  input_port_ = port_param.nStartPortNumber;
  output_port_ = input_port_ + 1;

  // Set role for the component because components can have multiple roles.
  OMX_PARAM_COMPONENTROLETYPE role_type;
  InitParam(&role_type);
  base::strlcpy(reinterpret_cast<char*>(role_type.cRole),
                role_name,
                OMX_MAX_STRINGNAME_SIZE);

  result = OMX_SetParameter(component_handle_,
                            OMX_IndexParamStandardComponentRole,
                            &role_type);
  RETURN_ON_OMX_FAILURE(result, "Failed to Set Role",
                        PLATFORM_FAILURE, false);

  // Populate input-buffer-related members based on input port data.
  OMX_PARAM_PORTDEFINITIONTYPE port_format;
  InitParam(&port_format);
  port_format.nPortIndex = input_port_;
  result = OMX_GetParameter(component_handle_,
                            OMX_IndexParamPortDefinition,
                            &port_format);
  RETURN_ON_OMX_FAILURE(result,
                        "GetParameter(OMX_IndexParamPortDefinition) failed",
                        PLATFORM_FAILURE, false);
  RETURN_ON_FAILURE(OMX_DirInput == port_format.eDir, "Expected input port",
                    PLATFORM_FAILURE, false);

  input_buffer_count_ = port_format.nBufferCountActual;
  input_buffer_size_ = port_format.nBufferSize;

  // Verify output port conforms to our expectations.
  InitParam(&port_format);
  port_format.nPortIndex = output_port_;
  result = OMX_GetParameter(component_handle_,
                            OMX_IndexParamPortDefinition,
                            &port_format);
  RETURN_ON_OMX_FAILURE(result,
                        "GetParameter(OMX_IndexParamPortDefinition) failed",
                        PLATFORM_FAILURE, false);
  RETURN_ON_FAILURE(OMX_DirOutput == port_format.eDir, "Expect Output Port",
                    PLATFORM_FAILURE, false);

  // Set output port parameters.
  port_format.nBufferCountActual = kNumPictureBuffers;
  port_format.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;

  // Force an OMX_EventPortSettingsChanged event to be sent once we know the
  // stream's real dimensions (which can only happen once some Decode() work has
  // been done).
  port_format.format.video.nFrameWidth = 128;
  port_format.format.video.nFrameHeight = 96;
  port_format.format.video.nStride = 128;
  port_format.format.video.nSliceHeight = 96;

  port_format.nBufferSize = port_format.format.video.nStride *
        port_format.format.video.nSliceHeight * 3 / 2;
  output_buffer_size_ = port_format.nBufferSize;
  result = OMX_SetParameter(component_handle_,
                            OMX_IndexParamPortDefinition,
                            &port_format);
  RETURN_ON_OMX_FAILURE(result,
                        "SetParameter(OMX_IndexParamPortDefinition) failed",
                        PLATFORM_FAILURE, false);
  return true;
}

bool OmxVideoDecodeAccelerator::DecoderSpecificInitialization() {
  OMXR_MC_VIDEO_PARAM_REORDERTYPE param_reorder;
  InitParam(&param_reorder);

  param_reorder.nPortIndex = output_port_;
  param_reorder.bReorder = OMX_FALSE;

  OMX_ERRORTYPE result = OMX_SetParameter(component_handle_,
                            static_cast<OMX_INDEXTYPE> (OMXR_MC_IndexParamVideoReorder),
                            &param_reorder);

  RETURN_ON_OMX_FAILURE(result,
                        "SetParameter(OMXR_MC_IndexParamVideoReorder) failed",
                        PLATFORM_FAILURE, false);

  // Set up timestamps to be returned in decode order (i.e. don't adjust
  // values to make them come out in ascneding order)

  OMXR_MC_VIDEO_PARAM_TIME_STAMP_MODETYPE param_ts;
  InitParam(&param_ts);

  param_ts.nPortIndex = output_port_;
  param_ts.eTimeStampMode = OMXR_MC_VIDEO_TimeStampModeDecodeOrder;

  result = OMX_SetParameter(component_handle_,
                            static_cast<OMX_INDEXTYPE> (OMXR_MC_IndexParamVideoTimeStampMode),
                            &param_ts);

  RETURN_ON_OMX_FAILURE(result,
                        "SetParameter(OMXR_MC_IndexParamVideoTimeStampMode) failed",
                        PLATFORM_FAILURE, false);

  // Enable dynamic video resizing up to FHD resolution

  OMXR_MC_VIDEO_PARAM_DYNAMIC_PORT_RECONF_IN_DECODINGTYPE param_dynamic;
  InitParam(&param_dynamic);

  param_dynamic.nPortIndex = output_port_;
  param_dynamic.bEnable = OMX_TRUE;

  result = OMX_SetParameter(component_handle_,
                            static_cast<OMX_INDEXTYPE> (OMXR_MC_IndexParamVideoDynamicPortReconfInDecoding),
                            &param_dynamic);

  RETURN_ON_OMX_FAILURE(result,
                        "SetParameter(OMXR_MC_IndexParamVideoDynamicPortReconfInDecoding) failed",
                        PLATFORM_FAILURE, false);

  OMXR_MC_VIDEO_PARAM_MAXIMUM_DECODE_CAPABILITYTYPE param_maxdecode;
  InitParam(&param_maxdecode);

  param_maxdecode.nPortIndex = output_port_;
  param_maxdecode.nMaxDecodedWidth = 1920;
  param_maxdecode.nMaxDecodedHeight = 1088;
  param_maxdecode.eMaxLevel = OMX_VIDEO_AVCLevel5;
  param_maxdecode.bForceEnable = OMX_TRUE;

  result = OMX_SetParameter(component_handle_,
                            static_cast<OMX_INDEXTYPE> (OMXR_MC_IndexParamVideoMaximumDecodeCapability),
                            &param_maxdecode);
  RETURN_ON_OMX_FAILURE(result,
                        "SetParameter(OMXR_MC_IndexParamVideoMaximumDecodeCapability) failed",
                        PLATFORM_FAILURE, false);
  return true;
}

void OmxVideoDecodeAccelerator::Decode(
    const media::BitstreamBuffer& bitstream_buffer) {
  TRACE_EVENT2("Video Decoder", "OVDA::Decode",
               "Buffer id", bitstream_buffer.id(),
               "Component input buffers", input_buffers_at_component_ + 1);
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  VLOGF(2) << "buffer id:" << bitstream_buffer.id();

  auto buffer = std::make_unique<BitstreamBufferRef>(bitstream_buffer, decode_task_runner_, decode_client_);
  RETURN_ON_FAILURE(buffer->memory != NULL || buffer->id < 0,
                    "Failed to map bistream buffer memory", UNREADABLE_INPUT,);

  DecodeBuffer(std::move(buffer));
}

void OmxVideoDecodeAccelerator::DecodeBuffer(std::unique_ptr<struct BitstreamBufferRef> input_buffer) {
  if (current_state_change_ == RESETTING ||
      current_state_change_ == INITIALIZING ||
      !queued_bitstream_buffers_.empty() ||
      free_input_buffers_.empty()) {
    queued_bitstream_buffers_.push_back(std::move(input_buffer));
    return;
  }

  RETURN_ON_FAILURE((current_state_change_ == NO_TRANSITION ||
                     current_state_change_ == RESIZING ||
                     current_state_change_ == FLUSHING) &&
                    (client_state_ == OMX_StateIdle ||
                     client_state_ == OMX_StateExecuting),
                    "Call to Decode() during invalid state or transition: "
                    << current_state_change_ << ", " << client_state_,
                    ILLEGAL_STATE,);

  OMX_BUFFERHEADERTYPE* omx_buffer = free_input_buffers_.front();

  if (input_buffer->id == -1) {
    // Cook up an empty buffer w/ EOS set and feed it to OMX.
    if (input_buffer_offset_) {
      first_input_buffer_sent_ = true;
      // Give this buffer to OMX.
      free_input_buffers_.pop();
      OMX_ERRORTYPE result = OMX_EmptyThisBuffer(component_handle_, omx_buffer);
      RETURN_ON_OMX_FAILURE(result, "OMX_EmptyThisBuffer() failed",
                        PLATFORM_FAILURE,);

      input_buffer_size_ = 0;
      input_buffer_offset_ = 0;
      input_buffers_at_component_++;
      if (free_input_buffers_.empty()) {
        VLOGF(2) << "No more buffers available, returning bistream buffer to queue";
        queued_bitstream_buffers_.push_back(std::move(input_buffer));
        return;
      }
      omx_buffer = free_input_buffers_.front();
    }

    omx_buffer->nFilledLen = 0;
    omx_buffer->nAllocLen = omx_buffer->nFilledLen;
    omx_buffer->nFlags = OMX_BUFFERFLAG_EOS;
    omx_buffer->nTimeStamp = -2;
    free_input_buffers_.pop();
    OMX_ERRORTYPE result = OMX_EmptyThisBuffer(component_handle_, omx_buffer);
    RETURN_ON_OMX_FAILURE(result, "OMX_EmptyThisBuffer() failed",
                          PLATFORM_FAILURE,);
    input_buffer_offset_ = 0;
    input_buffers_at_component_++;
    return;
  }

  // Setup |omx_buffer|.

  DCHECK(!omx_buffer->pAppPrivate);


  OMX_U8 *data = static_cast<OMX_U8*>(input_buffer->memory);

  bool send_frame = false;
  int size = input_buffer->size;
  if (codec_ == H264) {
    h264_parser_->SetStream(data, size);

    bool has_data = false;
    bool new_frame = false;
    H264Parser::Result res;
    H264NALU nal;
    while ((res = h264_parser_->AdvanceToNextNALU(&nal)) != H264Parser::kEOStream) {

      RETURN_ON_FAILURE(res == H264Parser::kOk, "Parsing H264 stream failed",
                          PLATFORM_FAILURE,);

      switch (nal.nal_unit_type) {
         case H264NALU::kNonIDRSlice:
         case H264NALU::kIDRSlice:
            //check if first-mb-in-slice is 0 (i.e. first NAL in picture)
            if (nal.size > 1 && nal.data[1] & 0x80) {
                DCHECK_EQ(has_data, false);
                new_frame = true;
            }
            has_data = true;
            break;
         case H264NALU::kAUD:
         case H264NALU::kEOSeq:
         case H264NALU::kEOStream:
         case H264NALU::kSEIMessage:
         case H264NALU::kSPS:
         case H264NALU::kPPS:
              new_frame = true;
              break;
         default:
            LOG(WARNING) << "Got an unrecognized NAL unit: " << nal.nal_unit_type;
      };

    }

    send_frame = new_frame && previous_frame_has_data_;
    previous_frame_has_data_ = has_data;
  }

  if (send_frame && omx_buffer->nFilledLen) {
      first_input_buffer_sent_ = true;
      // Give this buffer to OMX.
      free_input_buffers_.pop();
      OMX_ERRORTYPE result = OMX_EmptyThisBuffer(component_handle_, omx_buffer);
      RETURN_ON_OMX_FAILURE(result, "OMX_EmptyThisBuffer() failed",
                        PLATFORM_FAILURE,);

      input_buffer_size_ = 0;
      input_buffer_offset_ = 0;
      input_buffers_at_component_++;

      if (free_input_buffers_.empty()) {
        VLOGF(2) << "No more buffers available, returning bistream buffer to queue";
        queued_bitstream_buffers_.push_back(std::move(input_buffer));
        return;
      }
      omx_buffer = free_input_buffers_.front();
  }

  // Abuse the header's nTimeStamp field to propagate the bitstream buffer ID to
  // the output buffer's nTimeStamp field, so we can report it back to the
  // client in PictureReady().
  omx_buffer->nTimeStamp = input_buffer->id;

  memcpy(omx_buffer->pBuffer + input_buffer_offset_, data, size);
  input_buffer_offset_ += size;

  omx_buffer->nFlags = OMX_BUFFERFLAG_ENDOFFRAME;
  omx_buffer->nFilledLen = input_buffer_offset_;
  omx_buffer->nAllocLen = omx_buffer->nFilledLen;

  //processed |input_buffer|s go out of scope here and return to client.
}

void OmxVideoDecodeAccelerator::AssignPictureBuffers(
    const std::vector<media::PictureBuffer>& buffers) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  // If we are resetting/destroying/erroring, don't bother, as
  // OMX_FillThisBuffer will fail anyway. In case we're in the middle of
  // closing, this will put the Accelerator in ERRORING mode, which has the
  // unwanted side effect of not going through the OMX_FreeBuffers path and
  // leaks memory.
  if (current_state_change_ == RESETTING ||
      current_state_change_ == DESTROYING ||
      current_state_change_ == ERRORING)
    return;

  RETURN_ON_FAILURE(CanFillBuffer(), "Can't fill buffer", ILLEGAL_STATE,);
  RETURN_ON_FAILURE((kNumPictureBuffers <= buffers.size()),
      "Failed to provide requested picture buffers. (Got " << buffers.size() <<
      ", requested " << kNumPictureBuffers << ")", INVALID_ARGUMENT,);

  DCHECK_EQ(output_buffers_at_component_, 0);
  DCHECK_EQ(fake_output_buffers_.size(), 0U);
  DCHECK_EQ(pictures_.size(), 0U);

  if (!make_context_current_.Run())
    return;

  OMX_ERRORTYPE result;
  OMX_PARAM_PORTDEFINITIONTYPE port_format;

  InitParam(&port_format);
  port_format.nPortIndex = output_port_;

  result = OMX_GetParameter(component_handle_,
                            OMX_IndexParamPortDefinition,
                            &port_format);

  port_format.nBufferCountActual = buffers.size();

  result = OMX_SetParameter(component_handle_,
                            OMX_IndexParamPortDefinition,
                            &port_format);
  RETURN_ON_OMX_FAILURE(result,
                        "SetParameter(OMX_IndexParamPortDefinition) failed",
                        PLATFORM_FAILURE,);

  for (size_t i = 0; i < buffers.size(); ++i) {
    void *dummy;
    EGLImageKHR egl_image;
    struct MmngrBuffer mbuf;
    int alloc_size = (port_format.nBufferSize + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1);

    gfx::Size size = buffers[i].size();
    DCHECK_EQ(picture_buffer_dimensions_.width(), size.width());
    DCHECK_EQ(picture_buffer_dimensions_.height(), size.height());

    int ret = mmngr_alloc_in_user_ext(&mbuf.mem_id, alloc_size,
            &mbuf.hard_addr, &dummy, MMNGR_PA_SUPPORT, NULL);

    RETURN_ON_FAILURE(!ret, "Cannot allocate output buffer memory" << ret,
        PLATFORM_FAILURE,);

    ret = mmngr_export_start_in_user_ext(&mbuf.dmabuf_id, alloc_size,
        mbuf.hard_addr, &mbuf.dmabuf_fd, NULL);
    /* Make EGLImage */

    std::vector<EGLint> attrs;
    attrs.push_back(EGL_WIDTH);
    attrs.push_back(size.width());
    attrs.push_back(EGL_HEIGHT);
    attrs.push_back(size.height());
    attrs.push_back(EGL_LINUX_DRM_FOURCC_EXT);
    attrs.push_back(DRM_FORMAT_NV12);

    static const int plane_count = 2; // NV12 has 2 planes

    size_t plane_offset = 0;
    for (size_t plane = 0; plane < plane_count; ++plane) {
      attrs.push_back(EGL_DMA_BUF_PLANE0_FD_EXT + plane * 3);
      attrs.push_back(mbuf.dmabuf_fd);
      attrs.push_back(EGL_DMA_BUF_PLANE0_OFFSET_EXT + plane * 3);
      attrs.push_back(plane_offset);
      attrs.push_back(EGL_DMA_BUF_PLANE0_PITCH_EXT + plane * 3);
      attrs.push_back(port_format.format.video.nStride);

      plane_offset += port_format.format.video.nStride *
                      port_format.format.video.nSliceHeight;
    }

    attrs.push_back(EGL_NONE);

    uint32_t texture_id = buffers[i].service_texture_ids()[0];

    egl_image = eglCreateImageKHR(
        egl_display_, EGL_NO_CONTEXT, EGL_LINUX_DMA_BUF_EXT, NULL, &attrs[0]);
    RETURN_ON_FAILURE((egl_image != EGL_NO_IMAGE_KHR), "Cannot create EGLImage " << ui::GetLastEGLErrorString(),
          PLATFORM_FAILURE,);

    glBindTexture(GL_TEXTURE_EXTERNAL_OES, texture_id);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_EXTERNAL_OES, egl_image);

    VLOGF(1) << "Creating picture buffer. id = " << buffers[i].id();

    pictures_.insert(std::make_pair(buffers[i].id(),
        std::make_unique<OutputPicture>(*this, buffers[i], nullptr, egl_image, mbuf)));
  }

  if (!SendCommandToPort(OMX_CommandPortEnable, output_port_))
    return;
  if (!AllocateOutputBuffers(port_format.nBufferSize))
    return;
  VLOGF(1) << "Resize complete";
  current_state_change_ = NO_TRANSITION;
}

void OmxVideoDecodeAccelerator::ReusePictureBuffer(int32_t picture_buffer_id) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT1("Video Decoder", "OVDA::ReusePictureBuffer",
               "Picture id", picture_buffer_id);

  RETURN_ON_FAILURE(make_context_current_.Run(),
                    "Failed to make context current",
                    PLATFORM_FAILURE,);

  auto picture_sync_fence = gl::GLFence::Create();

  // Start checking sync status periodically.
  CheckPictureStatus(picture_buffer_id, std::move(picture_sync_fence));
}

void OmxVideoDecodeAccelerator::CheckPictureStatus(
    int32_t picture_buffer_id,
    std::unique_ptr<gl::GLFence> fence_obj
    ) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  TRACE_EVENT1("Video Decoder", "OVDA::CheckPictureStatus",
               "Picture id", picture_buffer_id);

  // It's possible for this task to never run if the message loop is
  // stopped. In that case we may never call QueuePictureBuffer().
  // This is fine though, because all pictures, irrespective of their state,
  // are in pictures_ map and that's what will be used to do the clean up.
  if (!fence_obj->HasCompleted()) {
    child_task_runner_->PostDelayedTask(FROM_HERE, base::Bind(
        &OmxVideoDecodeAccelerator::CheckPictureStatus, weak_this_,
        picture_buffer_id, base::Passed(&fence_obj)),
        base::TimeDelta::FromMilliseconds(kSyncPollDelayMs));
    return;
  }
  // Synced successfully. Queue the buffer for reuse.
  QueuePictureBuffer(picture_buffer_id);
}

void OmxVideoDecodeAccelerator::QueuePictureBuffer(int32_t picture_buffer_id) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (picture_buffer_id < 0)
     return;

  // During port-flushing, do not call OMX FillThisBuffer.
  if (current_state_change_ == RESETTING) {
    queued_picture_buffer_ids_.push_back(picture_buffer_id);
    return;
  }

  // We might have started destroying while waiting for the picture. It's safe
  // to drop it here, because we will free all the pictures regardless of their
  // state using the pictures_ map.
  if (!CanFillBuffer())
    return;

  OutputPictureById::iterator it = pictures_.find(picture_buffer_id);
  RETURN_ON_FAILURE(it != pictures_.end(),
                    "Missing picture buffer id: " << picture_buffer_id,
                    INVALID_ARGUMENT,);
  OutputPicture& output_picture = *it->second;

  if (!output_picture.omx_buffer_header) {
    VLOGF(1) << "Releasing picture for freed OMX buffer. Picture ID = " << picture_buffer_id;
    pictures_.erase(it);
    return;
  }

  ++output_buffers_at_component_;
  output_picture.at_component = true;
  TRACE_EVENT2("Video Decoder", "OVDA::QueuePictureBuffer",
               "Picture id", picture_buffer_id,
               "At component", output_buffers_at_component_);
  OMX_ERRORTYPE result =
      OMX_FillThisBuffer(component_handle_, output_picture.omx_buffer_header);
  RETURN_ON_OMX_FAILURE(result, "OMX_FillThisBuffer() failed",
                        PLATFORM_FAILURE,);
}

void OmxVideoDecodeAccelerator::Flush() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(current_state_change_, NO_TRANSITION);
  DCHECK_EQ(client_state_, OMX_StateExecuting);
  current_state_change_ = FLUSHING;

  Decode(media::BitstreamBuffer(-1, base::SharedMemoryHandle(), 0));
}

void OmxVideoDecodeAccelerator::OnReachedEOSInFlushing() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(client_state_, OMX_StateExecuting);

  current_state_change_ = NO_TRANSITION;
  if (client_)
    client_->NotifyFlushDone();
}

void OmxVideoDecodeAccelerator::FlushIOPorts() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  // Flush input port first.
  if (!SendCommandToPort(OMX_CommandFlush, input_port_))
    return;
}

void OmxVideoDecodeAccelerator::InputPortFlushDone() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(input_buffers_at_component_, 0);
  if (!SendCommandToPort(OMX_CommandFlush, output_port_))
    return;
}

void OmxVideoDecodeAccelerator::OutputPortFlushDone() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_EQ(output_buffers_at_component_, 0);
  BeginTransitionToState(OMX_StateExecuting);
}

void OmxVideoDecodeAccelerator::Reset() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK(current_state_change_ == NO_TRANSITION ||
        current_state_change_ == FLUSHING);
  DCHECK_EQ(client_state_, OMX_StateExecuting);
  if (first_input_buffer_sent_) {
    current_state_change_ = RESETTING;
    queued_bitstream_buffers_.clear();
    BeginTransitionToState(OMX_StatePause);
  } else {
     input_buffer_offset_ = 0;
     first_input_buffer_sent_ = false;

     child_task_runner_->PostTask(FROM_HERE, base::Bind(
        &Client::NotifyResetDone, client_));
  }
}

void OmxVideoDecodeAccelerator::Destroy() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  std::unique_ptr<OmxVideoDecodeAccelerator> deleter(this);
  client_ptr_factory_->InvalidateWeakPtrs();

  if (current_state_change_ == ERRORING ||
      current_state_change_ == DESTROYING) {
    return;
  }

  DCHECK(current_state_change_ == NO_TRANSITION ||
         current_state_change_ == FLUSHING ||
         current_state_change_ == RESETTING) << current_state_change_;

  // If we were never initializeed there's no teardown to do.
  if (client_state_ == OMX_StateMax)
    return;
  // If we can already call OMX_FreeHandle, simply do so.
  if (client_state_ == OMX_StateInvalid || client_state_ == OMX_StateLoaded) {
    ShutdownComponent();
    return;
  }
  DCHECK(client_state_ == OMX_StateExecuting ||
         client_state_ == OMX_StateIdle ||
         client_state_ == OMX_StatePause);
  current_state_change_ = DESTROYING;
  BeginTransitionToState(OMX_StateIdle);
  BusyLoopInDestroying(std::move(deleter));
}

bool OmxVideoDecodeAccelerator::TryToSetupDecodeOnSeparateThread(
    const base::WeakPtr<Client>& decode_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner) {

    return false;
}

void OmxVideoDecodeAccelerator::BeginTransitionToState(
    OMX_STATETYPE new_state) {
  VLOGF(1) << "new_state = " << new_state;
  DCHECK(new_state == OMX_StateInvalid ||
         current_state_change_ != NO_TRANSITION);

  if (current_state_change_ == ERRORING)
    return;
  OMX_ERRORTYPE result = OMX_SendCommand(
      component_handle_, OMX_CommandStateSet, new_state, 0);
  RETURN_ON_FAILURE(result == OMX_ErrorNone || new_state == OMX_StateInvalid,
                        "SendCommand(OMX_CommandStateSet) failed",
                        PLATFORM_FAILURE,);
}

void OmxVideoDecodeAccelerator::OnReachedIdleInInitializing() {
  VLOGF(1);
  DCHECK_EQ(client_state_, OMX_StateLoaded);
  client_state_ = OMX_StateIdle;
  BeginTransitionToState(OMX_StateExecuting);
}

void OmxVideoDecodeAccelerator::OnReachedExecutingInInitializing() {
  VLOGF(1);
  DCHECK_EQ(client_state_, OMX_StateIdle);
  base::AutoLock auto_lock_(init_lock_);
  client_state_ = OMX_StateExecuting;
  current_state_change_ = NO_TRANSITION;

  // Request filling of our fake buffers to trigger decode processing.  In
  // reality as soon as any data is decoded these will get dismissed due to
  // dimension mismatch.
  for (std::set<OMX_BUFFERHEADERTYPE*>::iterator it =
           fake_output_buffers_.begin();
       it != fake_output_buffers_.end(); ++it) {
    OMX_BUFFERHEADERTYPE* buffer = *it;
    OMX_ERRORTYPE result = OMX_FillThisBuffer(component_handle_, buffer);
    RETURN_ON_OMX_FAILURE(result, "OMX_FillThisBuffer()", PLATFORM_FAILURE,);
    ++output_buffers_at_component_;
  }
  if (deferred_init_allowed_ && client_) {
    client_->NotifyInitializationComplete(true);
     // Drain queues of input & output buffers held during the init.
    VLOGF(1) << "Deferred Initialization complete";
    DecodeQueuedBitstreamBuffers();
  } else {
    init_done_cond_.Signal();
  }
}

void OmxVideoDecodeAccelerator::OnReachedPauseInResetting() {
  DCHECK_EQ(client_state_, OMX_StateExecuting);
  client_state_ = OMX_StatePause;
  FlushIOPorts();
}

void OmxVideoDecodeAccelerator::DecodeQueuedBitstreamBuffers() {
  BitstreamBufferList buffers;
  buffers.swap(queued_bitstream_buffers_);
  if (current_state_change_ == DESTROYING ||
      current_state_change_ == ERRORING) {
    return;
  }
  for (size_t i = 0; i < buffers.size(); ++i)
    DecodeBuffer(std::move(buffers[i]));
}

void OmxVideoDecodeAccelerator::OnReachedExecutingInResetting() {
  DCHECK_EQ(client_state_, OMX_StatePause);
  client_state_ = OMX_StateExecuting;
  current_state_change_ = NO_TRANSITION;
  if (!client_)
    return;

  // Drain queues of input & output buffers held during the reset.
  DecodeQueuedBitstreamBuffers();
  for (size_t i = 0; i < queued_picture_buffer_ids_.size(); ++i)
    QueuePictureBuffer(queued_picture_buffer_ids_[i]);
  queued_picture_buffer_ids_.clear();

  client_->NotifyResetDone();
}

// Alert: HORROR ahead!  OMX shutdown is an asynchronous dance but our clients
// enjoy the fire-and-forget nature of a synchronous Destroy() call that
// ensures no further callbacks are made.  Since the interface between OMX
// callbacks and this class is a MessageLoop, we need to ensure the loop
// outlives the shutdown dance, even during process shutdown.  We do this by
// repeatedly enqueuing a no-op task until shutdown is complete, since
// MessageLoop's shutdown drains pending tasks.
// TODO(dhobsong): Yes indeed.  Review this.
void OmxVideoDecodeAccelerator::BusyLoopInDestroying(
    std::unique_ptr<OmxVideoDecodeAccelerator> self) {
  if (!component_handle_) return;
  // Can't use PostDelayedTask here because MessageLoop doesn't drain delayed
  // tasks.  Instead we sleep for 5ms.  Really.
  base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(5));
  self->child_task_runner_->PostTask(FROM_HERE, base::Bind(
      &OmxVideoDecodeAccelerator::BusyLoopInDestroying,
      base::Unretained(this), base::Passed(&self)));
}

void OmxVideoDecodeAccelerator::OnReachedIdleInDestroying() {
  DCHECK(client_state_ == OMX_StateExecuting ||
         client_state_ == OMX_StateIdle ||
         client_state_ == OMX_StatePause);
  client_state_ = OMX_StateIdle;

  // Note that during the Executing -> Idle transition, the OMX spec guarantees
  // buffers have been returned to the client, so we don't need to do an
  // explicit FlushIOPorts().

  BeginTransitionToState(OMX_StateLoaded);

  FreeOMXBuffers();
}

void OmxVideoDecodeAccelerator::OnReachedLoadedInDestroying() {
  DCHECK_EQ(client_state_, OMX_StateIdle);
  client_state_ = OMX_StateLoaded;
  current_state_change_ = NO_TRANSITION;
  ShutdownComponent();
}

void OmxVideoDecodeAccelerator::OnReachedInvalidInErroring() {
  client_state_ = OMX_StateInvalid;
  FreeOMXBuffers();
  ShutdownComponent();
}

void OmxVideoDecodeAccelerator::ShutdownComponent() {
  OMX_ERRORTYPE result = OMX_FreeHandle(component_handle_);
  if (result != OMX_ErrorNone)
    DLOG(ERROR) << "OMX_FreeHandle() error. Error code: " << result;
  client_state_ = OMX_StateMax;
  OMX_Deinit();
  // Allow BusyLoopInDestroying to exit and delete |this|.
  component_handle_ = NULL;
}

void OmxVideoDecodeAccelerator::StopOnError(
    media::VideoDecodeAccelerator::Error error) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (current_state_change_ == ERRORING)
    return;

  if (client_ && init_begun_)
    client_->NotifyError(error);
  client_ptr_factory_->InvalidateWeakPtrs();

  if (client_state_ == OMX_StateInvalid || client_state_ == OMX_StateMax)
      return;

  BeginTransitionToState(OMX_StateInvalid);
  current_state_change_ = ERRORING;
}

bool OmxVideoDecodeAccelerator::AllocateInputBuffers() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  VLOG(1) << __func__ << ": Allocating " << input_buffer_count_ << " buffers of size: " << input_buffer_size_;
  for (int i = 0; i < input_buffer_count_; ++i) {
    OMX_BUFFERHEADERTYPE* buffer;
    OMX_ERRORTYPE result =
        OMX_AllocateBuffer(component_handle_, &buffer, input_port_,
                      NULL, /* pAppPrivate gets set in Decode(). */
                      input_buffer_size_);
    RETURN_ON_OMX_FAILURE(result, "OMX_AllocateBuffer() Input buffer error",
                          PLATFORM_FAILURE, false);
    buffer->nInputPortIndex = input_port_;
    buffer->nOffset = 0;
    buffer->nFlags = 0;
    free_input_buffers_.push(buffer);
  }
  return true;
}

bool OmxVideoDecodeAccelerator::AllocateFakeOutputBuffers() {
  // Fill the component with fake output buffers.
  VLOG(1) << __func__ << ": Allocating " << kNumPictureBuffers << " buffers of size: " << output_buffer_size_;
  for (unsigned int i = 0; i < kNumPictureBuffers; ++i) {
    OMX_BUFFERHEADERTYPE* buffer;
    OMX_ERRORTYPE result;
    result = OMX_AllocateBuffer(component_handle_, &buffer, output_port_,
                                NULL, output_buffer_size_);
    RETURN_ON_OMX_FAILURE(result, "OMX_AllocateBuffer failed",
                          PLATFORM_FAILURE, false);
    buffer->pAppPrivate = NULL;
    buffer->nTimeStamp = -1;
    buffer->nOutputPortIndex = output_port_;
    CHECK(fake_output_buffers_.insert(buffer).second);
  }
  return true;
}

bool OmxVideoDecodeAccelerator::AllocateOutputBuffers(int size) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  DCHECK(!pictures_.empty());
  for (OutputPictureById::iterator it = pictures_.begin();
       it != pictures_.end(); ++it) {
    OutputPicture *output_picture = it->second.get();
    if (output_picture->allocated)
        continue;

    OMX_BUFFERHEADERTYPE** omx_buffer = &output_picture->omx_buffer_header;
    uint32_t hard_addr = output_picture->mmngr_buf.hard_addr;
    DCHECK(!*omx_buffer);

    OMX_ERRORTYPE result = OMX_UseBuffer(
        component_handle_, omx_buffer, output_port_, output_picture, size,
        reinterpret_cast<OMX_U8*>(hard_addr));

    RETURN_ON_OMX_FAILURE(result, "OMX_UseBuffer", PLATFORM_FAILURE, false);
  }
  return true;
}

void OmxVideoDecodeAccelerator::FreeOMXBuffers() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  bool failure_seen = false;
  while (!free_input_buffers_.empty()) {
    OMX_BUFFERHEADERTYPE* omx_buffer = free_input_buffers_.front();
    free_input_buffers_.pop();
    OMX_ERRORTYPE result =
        OMX_FreeBuffer(component_handle_, input_port_, omx_buffer);
    if (result != OMX_ErrorNone) {
      DLOG(ERROR) << "OMX_FreeBuffer failed: 0x" << std::hex << result;
      failure_seen = true;
    }
  }

  pictures_.clear();

  // Delete pending fake_output_buffers_ //TODO(dhobsong): still not liking these
  for (std::set<OMX_BUFFERHEADERTYPE*>::iterator it =
           fake_output_buffers_.begin();
       it != fake_output_buffers_.end(); ++it) {
    OMX_BUFFERHEADERTYPE* buffer = *it;
    OMX_ERRORTYPE result =
        OMX_FreeBuffer(component_handle_, output_port_, buffer);
    if (result != OMX_ErrorNone) {
      DLOG(ERROR) << "OMX_FreeBuffer failed: 0x" << std::hex << result;
      failure_seen = true;
    }
  }
  fake_output_buffers_.clear();

  // Dequeue pending queued_picture_buffer_ids_
  if (client_) {
    for (size_t i = 0; i < queued_picture_buffer_ids_.size(); ++i)
      client_->DismissPictureBuffer(queued_picture_buffer_ids_[i]);
  }
  queued_picture_buffer_ids_.clear();

  RETURN_ON_FAILURE(!failure_seen, "OMX_FreeBuffer", PLATFORM_FAILURE,);
}

void OmxVideoDecodeAccelerator::OnPortSettingsChanged() {
  VLOGF(1) << "Port settings changed received";
  SendCommandToPort(OMX_CommandPortDisable, output_port_);
  current_state_change_ = RESIZING;

  for (OutputPictureById::iterator it = pictures_.begin();
           it != pictures_.end(); ++it) {
    if (!it->second->at_component) {
      OMX_ERRORTYPE result = it->second->FreeOMXHandle();
      RETURN_ON_OMX_FAILURE(result, "OMX_FreeBuffer", PLATFORM_FAILURE,);
    }
  }
}

void OmxVideoDecodeAccelerator::OnOutputPortDisabled() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  OMX_PARAM_PORTDEFINITIONTYPE port_format;
  InitParam(&port_format);
  port_format.nPortIndex = output_port_;
  OMX_ERRORTYPE result = OMX_GetParameter(
      component_handle_, OMX_IndexParamPortDefinition, &port_format);
  RETURN_ON_OMX_FAILURE(result, "OMX_GetParameter", PLATFORM_FAILURE,);
  DCHECK_LE(port_format.nBufferCountMin, kNumPictureBuffers);

  // TODO(fischman): to support mid-stream resize, need to free/dismiss any
  // |pictures_| we already have.  Make sure that the shutdown-path agrees with
  // this (there's already freeing logic there, which should not be duplicated).

  // Request picture buffers to be handed to the component.
  // ProvidePictureBuffers() will trigger AssignPictureBuffers, which ultimately
  // assigns the textures to the component and re-enables the port.
  const OMX_VIDEO_PORTDEFINITIONTYPE& vformat = port_format.format.video;
  picture_buffer_dimensions_.SetSize(vformat.nFrameWidth,
                                                    vformat.nFrameHeight);
  if (client_) {
    client_->ProvidePictureBuffers(
        kNumPictureBuffers,
        PIXEL_FORMAT_NV12,
        1,
        picture_buffer_dimensions_,
        GL_TEXTURE_EXTERNAL_OES);
  }
}

void OmxVideoDecodeAccelerator::OnOutputPortEnabled() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());

  if (current_state_change_ == RESETTING) {
    for (OutputPictureById::iterator it = pictures_.begin();
         it != pictures_.end(); ++it) {
      queued_picture_buffer_ids_.push_back(it->first);
    }
    return;
  }

  if (!CanFillBuffer()) {
    StopOnError(ILLEGAL_STATE);
    return;
  }

  // Provide output buffers to decoder.
  for (OutputPictureById::iterator it = pictures_.begin();
       it != pictures_.end(); ++it) {
    if (it->second->allocated)
        continue;
    OMX_BUFFERHEADERTYPE* omx_buffer = it->second->omx_buffer_header;
    DCHECK(omx_buffer);
    // Clear EOS flag.
    omx_buffer->nFlags &= ~OMX_BUFFERFLAG_EOS;
    omx_buffer->nOutputPortIndex = output_port_;
    ++output_buffers_at_component_;
    it->second->at_component = true;
    OMX_ERRORTYPE result = OMX_FillThisBuffer(component_handle_, omx_buffer);
    RETURN_ON_OMX_FAILURE(result, "OMX_FillThisBuffer() failed",
                          PLATFORM_FAILURE,);
    it->second->allocated = true;
  }
}

void OmxVideoDecodeAccelerator::FillBufferDoneTask(
    OMX_BUFFERHEADERTYPE* buffer) {
  VLOGF(2);
  OutputPicture *output_picture =
      reinterpret_cast<OutputPicture*>(buffer->pAppPrivate);

  int picture_buffer_id = output_picture ? output_picture->picture_buffer.id() : -1;

  TRACE_EVENT2("Video Decoder", "OVDA::FillBufferDoneTask",
               "Buffer id", buffer->nTimeStamp,
               "Picture id", picture_buffer_id);
  DCHECK(decode_task_runner_->BelongsToCurrentThread());
  DCHECK_GT(output_buffers_at_component_, 0);
  --output_buffers_at_component_;

  // If we are destroying and then get a fillbuffer callback, calling into any
  // openmax function will put us in error mode, so bail now. In the RESETTING
  // case we still need to enqueue the picture ids but have to avoid giving
  // them to the client (this is handled below).
  if (current_state_change_ == DESTROYING ||
      current_state_change_ == ERRORING)
    return;

  // During the transition from Executing to Idle, and during port-flushing, all
  // pictures are sent back through here.  Avoid giving them to the client.
  if (current_state_change_ == RESETTING) {
    queued_picture_buffer_ids_.push_back(picture_buffer_id);
    return;
  }

  if (fake_output_buffers_.size() && fake_output_buffers_.count(buffer)) {
    size_t erased = fake_output_buffers_.erase(buffer);
    DCHECK_EQ(erased, 1U);
    OMX_ERRORTYPE result =
        OMX_FreeBuffer(component_handle_, output_port_, buffer);
    RETURN_ON_OMX_FAILURE(result, "OMX_FreeBuffer failed", PLATFORM_FAILURE,);
    return;
  }
  DCHECK(!fake_output_buffers_.size());

  output_picture->at_component = false;

  if (current_state_change_ == RESIZING) {
    VLOGF(1) << "Freeing buffer during resize. remaining at component: " << output_buffers_at_component_;
    pictures_.erase(output_picture->picture_buffer.id());
    return;
  }

  // When the EOS picture is delivered back to us, notify the client and reuse
  // the underlying picturebuffer.
  if (buffer->nFlags & OMX_BUFFERFLAG_EOS) {
    buffer->nFlags &= ~OMX_BUFFERFLAG_EOS;
    OnReachedEOSInFlushing();
    QueuePictureBuffer(picture_buffer_id);
    return;
  }


  //TODO(dhobsong): Set up colorspace (BT.601 vs BT.709)*/
  media::Picture picture(picture_buffer_id, buffer->nTimeStamp,
            gfx::Rect(picture_buffer_dimensions_), gfx::ColorSpace(), false);

  // See Decode() for an explanation of this abuse of nTimeStamp.
  if (decode_client_)
    decode_client_->PictureReady(picture);
}

void OmxVideoDecodeAccelerator::EmptyBufferDoneTask(
    OMX_BUFFERHEADERTYPE* buffer) {
  TRACE_EVENT1("Video Decoder", "OVDA::EmptyBufferDoneTask",
               "Buffer id", buffer->nTimeStamp);
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  DCHECK_GT(input_buffers_at_component_, 0);
  free_input_buffers_.push(buffer);
  input_buffers_at_component_--;
  if (buffer->nFlags & OMX_BUFFERFLAG_EOS)
    return;

  DecodeQueuedBitstreamBuffers();
}

void OmxVideoDecodeAccelerator::DispatchStateReached(OMX_STATETYPE reached) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  switch (current_state_change_) {
    case INITIALIZING:
      switch (reached) {
        case OMX_StateIdle:
          OnReachedIdleInInitializing();
          return;
        case OMX_StateExecuting:
          OnReachedExecutingInInitializing();
          return;
        default:
          NOTREACHED() << "Unexpected state in INITIALIZING: " << reached;
          return;
      }
    case RESETTING:
      switch (reached) {
        case OMX_StatePause:
          OnReachedPauseInResetting();
          return;
        case OMX_StateExecuting:
          OnReachedExecutingInResetting();
          return;
        default:
          NOTREACHED() << "Unexpected state in RESETTING: " << reached;
          return;
      }
    case DESTROYING:
      switch (reached) {
        case OMX_StatePause:
        case OMX_StateExecuting:
          // Because Destroy() can interrupt an in-progress Reset(),
          // we might arrive at these states after current_state_change_ was
          // overwritten with DESTROYING.  That's fine though - we already have
          // the state transition for Destroy() queued up at the component, so
          // we treat this as a no-op.
          return;
        case OMX_StateIdle:
          OnReachedIdleInDestroying();
          return;
        case OMX_StateLoaded:
          OnReachedLoadedInDestroying();
          return;
        default:
          NOTREACHED() << "Unexpected state in DESTROYING: " << reached;
          return;
      }
    case ERRORING:
      switch (reached) {
        case OMX_StateInvalid:
          OnReachedInvalidInErroring();
          return;
        default:
          NOTREACHED() << "Unexpected state in ERRORING: " << reached;
          return;
      }
    default:
      NOTREACHED() << "Unexpected state in " << current_state_change_
                   << ": " << reached;
  }
}

void OmxVideoDecodeAccelerator::HandleSyncronousInit(OMX_EVENTTYPE event,
                                                         OMX_U32 data1,
                                                         OMX_U32 data2) {

  VLOGF(1) << "event = " << event;
  switch (event) {
    case OMX_EventCmdComplete:
      if (data1 == OMX_CommandStateSet) {
        if (data2 == OMX_StateIdle) {
          OnReachedIdleInInitializing();
          return;
        } else if (data2 == OMX_StateExecuting) {
          OnReachedExecutingInInitializing();
          return;
        }
      }
      //fall through
    default:
      RETURN_ON_FAILURE(false, __func__ << "Unexpected unhandled event: " << event,
                        PLATFORM_FAILURE,);
  }
}

void OmxVideoDecodeAccelerator::EventHandlerCompleteTask(OMX_EVENTTYPE event,
                                                         OMX_U32 data1,
                                                         OMX_U32 data2) {
  VLOGF(1) << "event = " << event;
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  switch (event) {
    case OMX_EventCmdComplete:
      switch (data1) {
        case OMX_CommandPortDisable:
          DCHECK_EQ(data2, output_port_);
          OnOutputPortDisabled();
          return;
        case OMX_CommandPortEnable:
          DCHECK_EQ(data2, output_port_);
          OnOutputPortEnabled();
          return;
        case OMX_CommandStateSet:
          DispatchStateReached(static_cast<OMX_STATETYPE>(data2));
          return;
        case OMX_CommandFlush:
          if (current_state_change_ == DESTROYING ||
              current_state_change_ == ERRORING) {
            return;
          }
          DCHECK_EQ(current_state_change_, RESETTING);
          if (data2 == input_port_)
            InputPortFlushDone();
          else if (data2 == output_port_)
            OutputPortFlushDone();
          else
            NOTREACHED() << "Unexpected port flushed: " << data2;
          return;
        default:
          RETURN_ON_FAILURE(false, "Unknown command completed: " << data1,
                            PLATFORM_FAILURE,);
      }
      return;
    case OMX_EventError:
      if (current_state_change_ != DESTROYING &&
          current_state_change_ != ERRORING) {
        RETURN_ON_FAILURE(false, "EventError: 0x" << std::hex << data1,
                          PLATFORM_FAILURE,);
      }
      return;
    case OMX_EventPortSettingsChanged:
      if (data1 == output_port_ &&
          data2 == OMX_IndexParamPortDefinition) {
        // This event is only used for output resize; kick off handling that by
        // pausing the output port.
        OnPortSettingsChanged();
      } else if (data1 == output_port_ &&
                 data2 == OMX_IndexConfigCommonOutputCrop) {
        // TODO(vjain): Handle video crop rect.
      } else if (data1 == output_port_ &&
                 data2 == OMX_IndexConfigCommonScale) {
        // TODO(ashokm@nvidia.com): Handle video SAR change.
      } else {
        RETURN_ON_FAILURE(false,
                          "Unexpected EventPortSettingsChanged: "
                          << data1 << ", " << data2,
                          PLATFORM_FAILURE,);
      }
      return;
    case OMX_EventBufferFlag:
      if (data1 == output_port_) {
        // In case of Destroy() interrupting Flush().
        if (current_state_change_ == DESTROYING)
          return;
        DCHECK_EQ(current_state_change_, FLUSHING);
        // Do nothing; rely on the EOS picture delivery to notify the client.
      } else {
        RETURN_ON_FAILURE(false,
                          "Unexpected OMX_EventBufferFlag: "
                          << data1 << ", " << data2,
                          PLATFORM_FAILURE,);
      }
      return;
    default:
      RETURN_ON_FAILURE(false, "Unexpected unhandled event: " << event,
                        PLATFORM_FAILURE,);
  }
}

// static
bool OmxVideoDecodeAccelerator::PostSandboxInitialization() {
  StubPathMap paths;
  paths[kModuleOmx].push_back(kOMXLib);
  paths[kModuleMmngr].push_back(kMMNGRLib);
  paths[kModuleMmngrbuf].push_back(kMMNGRBufLib);

  return InitializeStubs(paths);
}

// static
OMX_ERRORTYPE OmxVideoDecodeAccelerator::EventHandler(OMX_HANDLETYPE component,
                                                      OMX_PTR priv_data,
                                                      OMX_EVENTTYPE event,
                                                      OMX_U32 data1,
                                                      OMX_U32 data2,
                                                      OMX_PTR event_data) {

  VLOGF(1);
  // Called on the OMX thread.
  OmxVideoDecodeAccelerator* decoder =
      static_cast<OmxVideoDecodeAccelerator*>(priv_data);
  DCHECK_EQ(component, decoder->component_handle_);

  if (!decoder->deferred_init_allowed_ &&
         decoder->current_state_change_ == INITIALIZING) {
      decoder->HandleSyncronousInit(event, data1, data2);
      return OMX_ErrorNone;
  }

  if (event == OMX_EventPortSettingsChanged) {
    if (decoder->current_state_change_ == RESIZING) {
        VLOGF(1) << "Dropping resize during resize";
        return OMX_ErrorNone;
    }
    VLOGF(1) << "Changing state to resize";
    decoder->current_state_change_ = RESIZING;
  }

  decoder->child_task_runner_->PostTask(FROM_HERE, base::Bind(
      &OmxVideoDecodeAccelerator::EventHandlerCompleteTask,
      decoder->weak_this(), event, data1, data2));
  return OMX_ErrorNone;
}

// static
OMX_ERRORTYPE OmxVideoDecodeAccelerator::EmptyBufferCallback(
    OMX_HANDLETYPE component,
    OMX_PTR priv_data,
    OMX_BUFFERHEADERTYPE* buffer) {
  TRACE_EVENT1("Video Decoder", "OVDA::EmptyBufferCallback",
               "Buffer id", buffer->nTimeStamp);
  // Called on the OMX thread.
  OmxVideoDecodeAccelerator* decoder =
      static_cast<OmxVideoDecodeAccelerator*>(priv_data);
  DCHECK_EQ(component, decoder->component_handle_);
  decoder->decode_task_runner_->PostTask(FROM_HERE, base::Bind(
      &OmxVideoDecodeAccelerator::EmptyBufferDoneTask, decoder->weak_this(),
      buffer));
  return OMX_ErrorNone;
}

// static
OMX_ERRORTYPE OmxVideoDecodeAccelerator::FillBufferCallback(
    OMX_HANDLETYPE component,
    OMX_PTR priv_data,
    OMX_BUFFERHEADERTYPE* buffer) {
  media::Picture* picture =
      reinterpret_cast<media::Picture*>(buffer->pAppPrivate);
  int picture_buffer_id = picture ? picture->picture_buffer_id() : -1;
  TRACE_EVENT2("Video Decoder", "OVDA::FillBufferCallback",
               "Buffer id", buffer->nTimeStamp,
               "Picture id", picture_buffer_id);
  // Called on the OMX thread.
  OmxVideoDecodeAccelerator* decoder =
      static_cast<OmxVideoDecodeAccelerator*>(priv_data);
  DCHECK_EQ(component, decoder->component_handle_);
  decoder->decode_task_runner_->PostTask(FROM_HERE, base::Bind(
      &OmxVideoDecodeAccelerator::FillBufferDoneTask, decoder->weak_this(),
      buffer));
  return OMX_ErrorNone;
}

bool OmxVideoDecodeAccelerator::CanFillBuffer() {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  const CurrentStateChange csc = current_state_change_;
  const OMX_STATETYPE cs = client_state_;
  return (csc != DESTROYING && csc != ERRORING && csc != RESETTING) &&
      (cs == OMX_StateIdle || cs == OMX_StateExecuting || cs == OMX_StatePause);
}

bool OmxVideoDecodeAccelerator::SendCommandToPort(
    OMX_COMMANDTYPE cmd, int port_index) {
  DCHECK(child_task_runner_->BelongsToCurrentThread());
  OMX_ERRORTYPE result = OMX_SendCommand(component_handle_,
                                         cmd, port_index, 0);
  RETURN_ON_OMX_FAILURE(result, "SendCommand() failed" << cmd,
                        PLATFORM_FAILURE, false);
  return true;
}

}  // namespace content
