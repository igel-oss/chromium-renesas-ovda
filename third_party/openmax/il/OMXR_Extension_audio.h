/**********************************************************************
 *
 * PURPOSE
 *   Media Component Library Header File
 *
 * AUTHOR
 *   Renesas Electronics Corporation
 *
 * DATE
 *   2013/06/17
 *
 **********************************************************************/
/*
 * Copyright (C) Renesas Electronics Corporation 2013
 * RENESAS ELECTRONICS CONFIDENTIAL AND PROPRIETARY
 * This program must be used solely for the purpose for which
 * it was furnished by Renesas Electronics Corporation.
 * No part of this program may be reproduced or disclosed to
 * others, in any form, without the prior written permission
 * of Renesas Electronics Corporation.
 *
 **********************************************************************/

#ifndef OMXR_EXTENSION_AUDIO_H
#define OMXR_EXTENSION_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/***************************************************************************/
/*    Include Header Files                                                 */
/***************************************************************************/
#include "OMXR_Extension.h"
/***************************************************************************/
/*    Macro Definitions                                                    */
/***************************************************************************/
/*******************/
/* Extended Index. */
/*******************/
enum {
    OMXR_MC_IndexParamAudioPortSettingMask           = (OMXR_MC_IndexVendorBaseAudio + 0x00000000),  /* OMX.RENESAS.INDEX.PARAM.AUDIO.PORTSETTINGSEVENTMASK */
    OMXR_MC_IndexParamAudioOutputUnit                = (OMXR_MC_IndexVendorBaseAudio + 0x00000001)  /* OMX.RENESAS.INDEX.PARAM.AUDIO.OUTPUTUNIT */
};

#define OMXR_MC_AUDIO_EVENTMASK_SAMPLINGRATE   ((OMX_U32)0x00000001U)
#define OMXR_MC_AUDIO_EVENTMASK_CHANNELS       ((OMX_U32)0x00000002U)
#define OMXR_MC_AUDIO_EVENTMASK_CHANNELMAPPING ((OMX_U32)0x00000004U)

/**************************/
/* Extended Unit Type. */
/**************************/
typedef enum tagOMXR_MC_AUIDO_UNITTYPE {
    OMXR_MC_AUDIO_UnitFrame      = 0x00,
    OMXR_MC_AUDIO_UnitFull       = 0x01,
    OMXR_MC_AUDIO_UnitPayload    = 0x02,
    OMXR_MC_AUDIO_UintMax        = 0x7FFFFFFF
} OMXR_MC_AUIDO_UNITTYPE;


/***************************************************************************/
/*    Type  Definitions                                                    */
/***************************************************************************/
/******************************/
/* Extended Output Unit Type. */
/******************************/
typedef struct tagOMXR_MC_AUDIO_PARAM_OUTPUTUNITTYPE {
    OMX_U32                 nSize;
    OMX_VERSIONTYPE         nVersion;
    OMX_U32                 nPortIndex;
    OMXR_MC_AUIDO_UNITTYPE  eUnit;
} OMXR_MC_AUDIO_PARAM_OUTPUTUNITTYPE;

/*******************************************/
/* Extended PortSettingChenges Event Mask. */
/*******************************************/
typedef struct tagOMXR_MC_AUDIO_PARAM_PORTSETTINGSEVENTMASKTYPE {
    OMX_U32             nSize;
    OMX_VERSIONTYPE     nVersion;
    OMX_U32             nPortIndex;
    OMX_U32             nMaskedBits;
} OMXR_MC_AUDIO_PARAM_PORTSETTINGSEVENTMASKTYPE;

/***************************************************************************/
/* End of module                                                           */
/***************************************************************************/
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* OMXR_EXTENSION_AUDIO_H */
