// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_MEDIA_OMX_VIDEO_DECODE_ACCELERATOR_H_
#define CONTENT_COMMON_GPU_MEDIA_OMX_VIDEO_DECODE_ACCELERATOR_H_

#include <dlfcn.h>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/compiler_specific.h"
#include "base/logging.h"
#include "base/memory/shared_memory.h"
#include "base/message_loop/message_loop.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/condition_variable.h"
#include "content/common/content_export.h"
#include "media/video/h264_parser.h"
#include "media/video/video_decode_accelerator.h"
#include "third_party/mmngr/mmngr_user_public.h"
#include "third_party/mmngr/mmngr_buf_user_public.h"
#include "third_party/openmax/il/OMX_Component.h"
#include "third_party/openmax/il/OMX_Core.h"
#include "third_party/openmax/il/OMX_Video.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_fence.h"

namespace media {

// Class to wrap OpenMAX IL accelerator behind VideoDecodeAccelerator interface.
// The implementation assumes an OpenMAX IL 1.1.2 implementation conforming to
// http://www.khronos.org/registry/omxil/specs/OpenMAX_IL_1_1_2_Specification.pdf
//
// This class lives on a single thread (the GPU process ChildThread) and DCHECKs
// that it is never accessed from any other.  OMX callbacks are trampolined from
// the OMX component's thread to maintain this invariant, using |weak_this()|.
class CONTENT_EXPORT OmxrVideoDecodeAccelerator :
    public VideoDecodeAccelerator {
 public:
  // Does not take ownership of |client| which must outlive |*this|.
  OmxrVideoDecodeAccelerator(
      EGLDisplay egl_display,
      const base::Callback<bool(void)>& make_context_current);
  virtual ~OmxrVideoDecodeAccelerator();

  // media::VideoDecodeAccelerator implementation.
  bool Initialize(const Config& config, Client* client) override;
  void Decode(const media::BitstreamBuffer& bitstream_buffer) override;
  virtual void AssignPictureBuffers(
      const std::vector<media::PictureBuffer>& buffers) override;
  void ReusePictureBuffer(int32_t picture_buffer_id) override;
  void Flush() override;
  void Reset() override;
  void Destroy() override;
  bool TryToSetupDecodeOnSeparateThread(const base::WeakPtr<Client>& decode_client,
   const scoped_refptr<base::SingleThreadTaskRunner>& decode_task_runner) override;

  base::WeakPtr<OmxrVideoDecodeAccelerator> weak_this() { return weak_this_; }

  static VideoDecodeAccelerator::SupportedProfiles GetSupportedProfiles();
  // Do any necessary initialization before the sandbox is enabled.
  static void PreSandboxInitialization();

 private:
  // Because OMX state-transitions are described solely by the "state reached"
  // (3.1.2.9.1, table 3-7 of the spec), we track what transition was requested
  // using this enum.  Note that it is an error to request a transition while
  // |*this| is in any state other than NO_TRANSITION, unless requesting
  // DESTROYING or ERRORING.
  enum CurrentStateChange {
    NO_TRANSITION,  // Not in the middle of a transition.
    INITIALIZING,
    FLUSHING,
    RESETTING,
    RESIZING,
    DESTROYING,
    ERRORING,  // Trumps all other transitions; no recovery is possible.
  };

  // Add codecs as we get HW that supports them (and which are supported by SW
  // decode!).
  enum Codec {
    UNKNOWN,
    H264,
    VP8,
    CODEC_MAX,
  };

  struct CodecInfo {
    Codec codec;
    const char *role;
    char *component;
  };

  // Helper struct for keeping track of MMNGR buffer metadata
  struct MmngrBuffer {
    MMNGR_ID mem_id;
    uint32_t hard_addr;
    int dmabuf_id;
    int dmabuf_fd;
  };

  // Helper struct for keeping track of all output buffer metadata
  // buffer and the PictureBuffer it points to.
  struct OutputPicture {
    OutputPicture(
          const OmxrVideoDecodeAccelerator &dec,
          media::PictureBuffer pbuffer,
          OMX_BUFFERHEADERTYPE* obuffer,
          EGLImageKHR eimage,
          struct MmngrBuffer mbuf);
    virtual ~OutputPicture();

    OMX_ERRORTYPE FreeOMXHandle();

    const OmxrVideoDecodeAccelerator &decoder;
    media::PictureBuffer picture_buffer;
    OMX_BUFFERHEADERTYPE* omx_buffer_header;
    EGLImageKHR egl_image;
    struct MmngrBuffer mmngr_buf;
    bool at_component;
    bool allocated;
  };

  class OmxrProfileManager {
  public:
    static const OmxrProfileManager &Get();

    OmxrProfileManager();
    ~OmxrProfileManager() = default;

    const struct CodecInfo getCodecForProfile(VideoCodecProfile profile) const;
    const std::vector<VideoCodecProfile> & getSupportedProfiles() const { return supported_profiles_;}

  private:
    void InitOMXLibs(void);

  private:
    std::vector<std::pair<struct CodecInfo, std::vector<VideoCodecProfile>>> possible_profiles_ = {
        {{H264, "video_decoder.avc", nullptr}, {H264PROFILE_BASELINE, H264PROFILE_MAIN, H264PROFILE_HIGH}},
        {{VP8, "video_decoder.vp9", nullptr}, {VP8PROFILE_ANY}}
    };
    std::vector<VideoCodecProfile> supported_profiles_;
  };

  struct BitstreamBufferRef {
    BitstreamBufferRef(
      const media::BitstreamBuffer &buf,
      scoped_refptr<base::SingleThreadTaskRunner> tr,
      base::WeakPtr<Client> cl);
    virtual ~BitstreamBufferRef();

    std::unique_ptr<base::SharedMemory> shm;
    scoped_refptr<base::SingleThreadTaskRunner> task_runner;
    base::WeakPtr<Client> client;
    int32_t id;
    size_t size;
    void *memory;
  };

  typedef std::map<int32_t, std::unique_ptr<OutputPicture>> OutputPictureById;

  scoped_refptr<base::SingleThreadTaskRunner> child_task_runner_;

  OMX_HANDLETYPE component_handle_;

  // Create the Component for OMX. Handles all OMX initialization.
  bool CreateComponent(const struct CodecInfo &cinfo);
  // Do any decoder specific initialization not covered in the standard OMX spec
  bool DecoderSpecificInitialization();

  // Buffer allocation/free methods for input and output buffers.
  bool AllocateInputBuffers();
  bool AllocateFakeOutputBuffers();
  bool AllocateOutputBuffers(int size);
  void FreeOMXBuffers();

  // Methods to handle OMX state transitions.  See section 3.1.1.2 of the spec.
  // Request transitioning OMX component to some other state.
  void BeginTransitionToState(OMX_STATETYPE new_state);
  // The callback received when the OMX component has transitioned.
  void DispatchStateReached(OMX_STATETYPE reached);
  // Callbacks handling transitioning to specific states during state changes.
  // These follow a convention of OnReached<STATE>In<CurrentStateChange>(),
  // requiring that each pair of <reached-state>/CurrentStateChange is unique
  // (i.e. the source state is uniquely defined by the pair).
  void OnReachedIdleInInitializing();
  void OnReachedExecutingInInitializing();
  void OnReachedPauseInResetting();
  void OnReachedExecutingInResetting();
  void OnReachedIdleInDestroying();
  void OnReachedLoadedInDestroying();
  void OnReachedEOSInFlushing();
  void OnReachedInvalidInErroring();
  void ShutdownComponent();
  void BusyLoopInDestroying(std::unique_ptr<OmxrVideoDecodeAccelerator> self);

  // Port-flushing helpers.
  void FlushIOPorts();
  void InputPortFlushDone();
  void OutputPortFlushDone();

  // Stop the component when any error is detected.
  void StopOnError(media::VideoDecodeAccelerator::Error error);

  // Determine whether we can issue fill buffer to the decoder based on the
  // current state (and outstanding state change) of the component.
  bool CanFillBuffer();

  // Whenever port settings change, the first thing we must do is disable the
  // port (see Figure 3-18 of the OpenMAX IL spec linked to above).  When the
  // port is disabled, the component will call us back here.  We then re-enable
  // the port once we have textures, and that's the second method below.
  void OnOutputPortDisabled();
  void OnOutputPortEnabled();
  void OnPortSettingsChanged();

  // Do the Decode() heavy lifting.
  void DecodeBuffer(std::unique_ptr<struct BitstreamBufferRef> input_buffer);
  // Decode bitstream buffers that were queued (see queued_bitstream_buffers_).
  void DecodeQueuedBitstreamBuffers();

  // Weak pointer to |this|; used to safely trampoline calls from the OMX thread
  // to the ChildThread.  Since |this| is kept alive until OMX is fully shut
  // down, only the OMX->Child thread direction needs to be guarded this way.
  base::WeakPtr<OmxrVideoDecodeAccelerator> weak_this_;
  base::WeakPtrFactory<OmxrVideoDecodeAccelerator> weak_this_factory_;

  // True once Initialize() has returned true; before this point there's never a
  // point in calling client_->NotifyError().
  bool init_begun_;

  base::Lock init_lock_;
  base::ConditionVariable init_done_cond_;

  // IL-client state.
  OMX_STATETYPE client_state_;
  // See comment on CurrentStateChange above.
  CurrentStateChange current_state_change_;

  // Following are input port related variables.
  int input_buffer_count_;
  int input_buffer_size_;
  OMX_U32 input_port_;
  int input_buffers_at_component_;

  std::unique_ptr<H264Parser> h264_parser_;
  int input_buffer_offset_;
  bool first_input_buffer_sent_;
  bool previous_frame_has_data_;

  // Following are output port related variables.
  OMX_U32 output_port_;
  int output_buffer_size_;
  int output_buffers_at_component_;
  int page_size_;

  gfx::Size picture_buffer_dimensions_;

  /* Helpers to handle restrictions on Reset() timing*/
  bool reset_pending_;
  void FinishReset();

  // NOTE: someday there may be multiple contexts for a single decoder.  But not
  // today.
  // TODO(fischman,vrk): handle lost contexts?
  EGLDisplay egl_display_;
  EGLContext egl_context_;
  base::Callback<bool(void)> make_context_current_;

  // Free input OpenMAX buffers that can be used to take bitstream from demuxer.
  std::queue<OMX_BUFFERHEADERTYPE*> free_input_buffers_;

  // For output buffer recycling cases.
  OutputPictureById pictures_;

  // To kick the component from Loaded to Idle before we know the real size of
  // the video (so can't yet ask for textures) we populate it with fake output
  // buffers.  Keep track of them here.
  // TODO(fischman): do away with this madness.
  std::set<OMX_BUFFERHEADERTYPE*> fake_output_buffers_;

  // Encoded bitstream buffers awaiting decode, queued while the decoder was
  // unable to accept them.
  typedef std::vector<std::unique_ptr<BitstreamBufferRef>> BitstreamBufferList;
  BitstreamBufferList queued_bitstream_buffers_;
  // Available output picture buffers released during Reset() and awaiting
  // re-use once Reset is done.  Is empty most of the time and drained right
  // before NotifyResetDone is sent.
  std::vector<int> queued_picture_buffer_ids_;

  // To expose client callbacks from VideoDecodeAccelerator.
  // NOTE: all calls to these objects *MUST* be executed on
  // message_loop_.
  std::unique_ptr<base::WeakPtrFactory<Client>> client_ptr_factory_;
  base::WeakPtr<Client> client_;


  // The following are used when performing Docode() tasks from
  // a separate thread.  Otherwise they will be set to
  // |child_task_runner_| and |client_| respectively

  scoped_refptr<base::SingleThreadTaskRunner> decode_task_runner_;
  base::WeakPtr<Client> decode_client_;

  // These members are only used during Initialization.
  Codec codec_;
  bool deferred_init_allowed_;

  // Handle syncronous transition to EXECUTING state when deferred init is
  // not available.
  void HandleSyncronousInit(OMX_EVENTTYPE event,
                                OMX_U32 data1,
                                OMX_U32 data2);
  // Method to handle events
  void EventHandlerCompleteTask(OMX_EVENTTYPE event,
                                OMX_U32 data1,
                                OMX_U32 data2);

  // Method to receive buffers from component's input port
  void EmptyBufferDoneTask(OMX_BUFFERHEADERTYPE* buffer);

  // Method to receive buffers from component's output port
  void FillBufferDoneTask(OMX_BUFFERHEADERTYPE* buffer);

  // Send a command to an OMX port.  Return false on failure (after logging and
  // setting |this| to ERRORING state).
  bool SendCommandToPort(OMX_COMMANDTYPE cmd, int port_index);

  // Callback methods for the OMX component.
  // When these callbacks are received, the
  // call is delegated to the three internal methods above.
  static OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE component,
                                    OMX_PTR priv_data,
                                    OMX_EVENTTYPE event,
                                    OMX_U32 data1, OMX_U32 data2,
                                    OMX_PTR event_data);
  static OMX_ERRORTYPE EmptyBufferCallback(OMX_HANDLETYPE component,
                                           OMX_PTR priv_data,
                                           OMX_BUFFERHEADERTYPE* buffer);
  static OMX_ERRORTYPE FillBufferCallback(OMX_HANDLETYPE component,
                                          OMX_PTR priv_data,
                                          OMX_BUFFERHEADERTYPE* buffer);

  // When we get a texture back via ReusePictureBuffer(), we want to ensure
  // that its contents have been read out by rendering layer, before we start
  // overwriting it with the decoder. Use a GPU fence and CheckPicutreStatus()
  // to poll for the fence completion before sending it to the decoder.
  void CheckPictureStatus(int32_t picture_buffer_id,
            std::unique_ptr<gl::GLFence> fence_obj);

  // Queue a picture for use by the decoder, either by sending it directly to it
  // via OMX_FillThisBuffer, or by queueing it for later if we are RESETTING.
  void QueuePictureBuffer(int32_t picture_buffer_id);

};

}  // namespace content

#endif  // CONTENT_COMMON_GPU_MEDIA_OMX_VIDEO_DECODE_ACCELERATOR_H_
