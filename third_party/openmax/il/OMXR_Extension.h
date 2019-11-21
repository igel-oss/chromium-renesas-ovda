/*
 * Copyright(C) 2015-2017 Renesas Electronics Corporation. All Rights Reserved.
 * RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY
 * This program must be used solely for the purpose for which
 * it was furnished by Renesas Electronics Corporation.
 * No part of this program may be reproduced or disclosed to
 * others, in any form, without the prior written permission
 * of Renesas Electronics Corporation.
 */
/**
 * The OMXR Extension header contains vendor-defined extension api and definitions.
 *
 * \file OMXR_Extension.h
 */

#ifndef OMXR_EXTENSION_H
#define OMXR_EXTENSION_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Files                                                        */
/***************************************************************************/
#include "OMX_Types.h"
#include "OMX_Core.h"
#include "OMX_Component.h"
#include "OMX_Index.h"
#include "OMX_Audio.h"
#include "OMX_Video.h"
#include "OMX_Image.h"
#include "OMX_IVCommon.h"
#include "OMX_Other.h"

/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/

/**
 * Base offset of the vendor-defined extensions for each domain (OMX_INDEXTYPE)
 */
enum {
    OMXR_MC_IndexVendorBaseVideo                    = (OMX_IndexVendorStartUnused + 0x00000000),    /**< Video domain */
    OMXR_MC_IndexVendorBaseAudio                    = (OMX_IndexVendorStartUnused + 0x00010000),    /**< Audio domain */
    OMXR_MC_IndexVendorBaseImage                    = (OMX_IndexVendorStartUnused + 0x00020000),    /**< Image domain */
    OMXR_MC_IndexVendorBaseOther                    = (OMX_IndexVendorStartUnused + 0x00030000)     /**< other */
};

/**
 * Base offset of the vendor-defined extensions for each domain (OMX_EVENTTYPE)
 */
enum {
    OMXR_MC_EventVendorBaseVideo                    = (OMX_EventVendorStartUnused + 0x00000000),    /**< Video domain */
    OMXR_MC_EventVendorBaseAudio                    = (OMX_EventVendorStartUnused + 0x00010000),    /**< Audio domain */
    OMXR_MC_EventVendorBaseImage                    = (OMX_EventVendorStartUnused + 0x00020000),    /**< Image domain */
    OMXR_MC_EventVendorBaseOther                    = (OMX_EventVendorStartUnused + 0x00030000)     /**< other */
};

/**
 * Base offset of the vendor-defined extensions for each domain (OMX_ERRORTYPE)
 */
enum {
    OMXR_ErrorBase                                  = (OMX_ErrorVendorStartUnused + 0x00000000),    /**< Common */
    OMXR_UTIL_ErrorBase                             = (OMX_ErrorVendorStartUnused + 0x00008000),    /**< Utility */
    OMXR_MC_ErrorBaseVideo                          = (OMX_ErrorVendorStartUnused + 0x00010000),    /**< Video domain */
    OMXR_MC_ErrorBaseAudio                          = (OMX_ErrorVendorStartUnused + 0x00020000),    /**< Audio domain */
    OMXR_MC_ErrorBaseImage                          = (OMX_ErrorVendorStartUnused + 0x00030000),    /**< Image domain */
    OMXR_MC_ErrorBaseOther                          = (OMX_ErrorVendorStartUnused + 0x00040000)     /**< other */
};

/* Error codes */
#define OMXR_ErrorFileNotFound                      (OMX_S32)(OMXR_ErrorBase + 0x00000000)      /**< file is not found */
#define OMXR_ErrorFileRead                          (OMX_S32)(OMXR_ErrorBase + 0x00000001)      /**< It isn't able to read the file */
#define OMXR_ErrorIllegalConfig                     (OMX_S32)(OMXR_ErrorBase + 0x00000002)      /**< Illigal configuration file */

/* Log message level */
#define OMXR_LOG_LEVEL_NONE     0x0         /**< (not support) */
#define OMXR_LOG_LEVEL_ERROR    0x1         /**< (not support) */
#define OMXR_LOG_LEVEL_HIGH     0x2         /**< (not support) */
#define OMXR_LOG_LEVEL_LOW      0x4         /**< (not support) */
#define OMXR_LOG_LEVEL_DEBUG    0x8         /**< (not support) */
#define OMXR_LOG_LEVEL_COMM     0x10        /**< (not support) */

#define OMXR_BASE_LOG_LEVEL_ERROR    0x1    /**< bit pattern of error log level */
#define OMXR_BASE_LOG_LEVEL_WARN     0x3    /**< bit pattern of warning log level */
#define OMXR_BASE_LOG_LEVEL_INFO     0x7    /**< bit pattern of information log level */
#define OMXR_BASE_LOG_LEVEL_DEBUG    0xf    /**< bit pattern of debug log level */

#define OMXR_LOG_LEVEL_SHIFT_CORE     8     /**< core module */
#define OMXR_LOG_LEVEL_SHIFT_UTIL    12     /**< utility module */
#define OMXR_LOG_LEVEL_SHIFT_CMN     16     /**< common module */
#define OMXR_LOG_LEVEL_SHIFT_VIDEO   20     /**< video module */
#define OMXR_LOG_LEVEL_SHIFT_AUDIO   24     /**< audio module */
#define OMXR_LOG_LEVEL_SHIFT_CNV     28     /**< video converter module */

/* Log level for OMX Core module */
#define OMXR_CORE_LOG_LEVEL_ERROR	(OMXR_BASE_LOG_LEVEL_ERROR << OMXR_LOG_LEVEL_SHIFT_CORE)    /**< error log level for OMX Core */
#define OMXR_CORE_LOG_LEVEL_WARN	(OMXR_BASE_LOG_LEVEL_WARN  << OMXR_LOG_LEVEL_SHIFT_CORE)    /**< warning log level for OMX Core */
#define OMXR_CORE_LOG_LEVEL_INFO	(OMXR_BASE_LOG_LEVEL_INFO  << OMXR_LOG_LEVEL_SHIFT_CORE)    /**< information log level for OMX Core */
#define OMXR_CORE_LOG_LEVEL_DEBUG	(OMXR_BASE_LOG_LEVEL_DEBUG << OMXR_LOG_LEVEL_SHIFT_CORE)    /**< debug log level for OMX Core */

/* Log level for OMX Utility(os/system depend) module */
#define OMXR_UTIL_LOG_LEVEL_ERROR	(OMXR_BASE_LOG_LEVEL_ERROR << OMXR_LOG_LEVEL_SHIFT_UTIL)    /**< error log level for OMX Utility */
#define OMXR_UTIL_LOG_LEVEL_WARN	(OMXR_BASE_LOG_LEVEL_WARN  << OMXR_LOG_LEVEL_SHIFT_UTIL)    /**< warning log level for OMX Utility */
#define OMXR_UTIL_LOG_LEVEL_INFO	(OMXR_BASE_LOG_LEVEL_INFO  << OMXR_LOG_LEVEL_SHIFT_UTIL)    /**< information log level for OMX Utility */
#define OMXR_UTIL_LOG_LEVEL_DEBUG	(OMXR_BASE_LOG_LEVEL_DEBUG << OMXR_LOG_LEVEL_SHIFT_UTIL)    /**< debug log level for OMX Utility */

/* Log level for OMX MC Common module */
#define OMXR_CMN_LOG_LEVEL_ERROR	(OMXR_BASE_LOG_LEVEL_ERROR << OMXR_LOG_LEVEL_SHIFT_CMN)     /**< error log level for OMX MC Common */
#define OMXR_CMN_LOG_LEVEL_WARN		(OMXR_BASE_LOG_LEVEL_WARN  << OMXR_LOG_LEVEL_SHIFT_CMN)     /**< warning log level for OMX MC Common */
#define OMXR_CMN_LOG_LEVEL_INFO		(OMXR_BASE_LOG_LEVEL_INFO  << OMXR_LOG_LEVEL_SHIFT_CMN)     /**< information log level for OMX MC Common */
#define OMXR_CMN_LOG_LEVEL_DEBUG	(OMXR_BASE_LOG_LEVEL_DEBUG << OMXR_LOG_LEVEL_SHIFT_CMN)     /**< debug log level for OMX MC Common */

/* Log level for OMX MC Video module */
#define OMXR_VIDEO_LOG_LEVEL_ERROR	(OMXR_BASE_LOG_LEVEL_ERROR << OMXR_LOG_LEVEL_SHIFT_VIDEO)   /**< error log level for OMX MC Video */
#define OMXR_VIDEO_LOG_LEVEL_WARN	(OMXR_BASE_LOG_LEVEL_WARN  << OMXR_LOG_LEVEL_SHIFT_VIDEO)   /**< warning log level for OMX MC Video */
#define OMXR_VIDEO_LOG_LEVEL_INFO	(OMXR_BASE_LOG_LEVEL_INFO  << OMXR_LOG_LEVEL_SHIFT_VIDEO)   /**< information log level for OMX MC Video */
#define OMXR_VIDEO_LOG_LEVEL_DEBUG	(OMXR_BASE_LOG_LEVEL_DEBUG << OMXR_LOG_LEVEL_SHIFT_VIDEO)   /**< debug log level for OMX MC Video */

/* Log level for OMX MC Audio module */
#define OMXR_AUDIO_LOG_LEVEL_ERROR	(OMXR_BASE_LOG_LEVEL_ERROR << OMXR_LOG_LEVEL_SHIFT_AUDIO)   /**< error log level for OMX MC Audio */
#define OMXR_AUDIO_LOG_LEVEL_WARN	(OMXR_BASE_LOG_LEVEL_WARN  << OMXR_LOG_LEVEL_SHIFT_AUDIO)   /**< warning log level for OMX MC Audio */
#define OMXR_AUDIO_LOG_LEVEL_INFO	(OMXR_BASE_LOG_LEVEL_INFO  << OMXR_LOG_LEVEL_SHIFT_AUDIO)   /**< information log level for OMX MC Audio */
#define OMXR_AUDIO_LOG_LEVEL_DEBUG	(OMXR_BASE_LOG_LEVEL_DEBUG << OMXR_LOG_LEVEL_SHIFT_AUDIO)   /**< debug log level for OMX MC Audio */

/* Log level for OMX MC Video Converter module */
#define OMXR_CNV_LOG_LEVEL_ERROR	(OMXR_BASE_LOG_LEVEL_ERROR << OMXR_LOG_LEVEL_SHIFT_CNV)     /**< error log level for OMX MC Video Converter */
#define OMXR_CNV_LOG_LEVEL_WARN		(OMXR_BASE_LOG_LEVEL_WARN  << OMXR_LOG_LEVEL_SHIFT_CNV)     /**< warning log level for OMX MC Video Converter */
#define OMXR_CNV_LOG_LEVEL_INFO		(OMXR_BASE_LOG_LEVEL_INFO  << OMXR_LOG_LEVEL_SHIFT_CNV)     /**< information log level for OMX MC Video Converter */
#define OMXR_CNV_LOG_LEVEL_DEBUG	(OMXR_BASE_LOG_LEVEL_DEBUG << OMXR_LOG_LEVEL_SHIFT_CNV)     /**< debug log level for OMX MC Video Converter */

/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/

/***************************************************************************/
/*    Function Prototypes                                                  */
/***************************************************************************/
/**
 * Set log mode
 *
 * This function sets Log mode. It will change the display pattern of the log by the Log mode.
 *
 * \param[in]	u32LogMode	log mode.
 * \return		OMX_ERRORTYPE
 */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMXR_SetLogMode(OMX_IN OMX_U32 u32LogMode);

/**
 * Get configuraion file name
 *
 * This function returns the configuration definition file name stored in OMX Core.
 *
 * \param[out]		cConfigName	Configuration file name
 * \param[in,out]	*pu32Length	Buffer size / Length of configuration file name
 * \return		OMX_ERRORTYPE
 */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMXR_GetConfiguration(OMX_OUT OMX_STRING cConfigName, OMX_U32* pu32Length);

/**
 * Set configuraion file name
 *
 * This function sets the configuration definition file name to OMX Core.
 *
 * \param[in]	cConfigName	Configuration file name
 * \return		OMX_ERRORTYPE
 */
OMX_API OMX_ERRORTYPE OMX_APIENTRY OMXR_SetConfiguration(OMX_IN OMX_STRING cConfigName);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_H */
