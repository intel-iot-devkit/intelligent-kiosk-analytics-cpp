/******************************************************************************\
Copyright (c) 2005-2018, Intel Corporation
All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

This sample was distributed or derived from the Intel's Media Samples package.
The original version of this sample may be obtained from https://software.intel.com/en-us/intel-media-server-studio
or https://software.intel.com/en-us/media-client-solutions-support.
\**********************************************************************************/

#include "main.hpp"
#include "mfx_samples_config.h"

#include "pipeline_decode.h"
#include <sstream>
#include "version.h"
#include <wait.h>

#ifndef MFX_VERSION
#error MFX_VERSION not defined
#endif


// Takes the video 
int media_sdk(const char* fileName)
{
    sInputParams        Params;   // input parameters from command line
    CDecodingPipeline   Pipeline; // pipeline for decoding, includes input file reader, decoder and output file writer
    char codec[5] = "h265";
    mfxStatus sts = MFX_ERR_NONE; // return value check

    msdk_opt_read(fileName, Params.strSrcFile);

    Params.memType = D3D9_MEMORY;
    Params.mode = MODE_RENDERING;
    Params.libvaBackend = MFX_LIBVA_X11;

    //Params.bUseHWLib = false;

    msdk_opt_read("0", Params.nRenderWinX);
    msdk_opt_read("0", Params.nRenderWinY);
    msdk_opt_read("700", Params.Width);
    msdk_opt_read("400", Params.Height);
    Params.bUseHWLib = true;
    if (Params.nAsyncDepth == 0)
    {
        Params.nAsyncDepth = 4; //set by default;
    }

    sts = StrFormatToCodecFormatFourCC(codec , Params.videoType);

    if (sts != MFX_ERR_NONE)
    {
        std::cout<<"Unknown codec"<<std::endl;
        return MFX_ERR_UNSUPPORTED;
    }
    if (!IsDecodeCodecSupported(Params.videoType))
    {
        std::cout<<"Unsupported codec"<<std::endl;
        return MFX_ERR_UNSUPPORTED;
    }
    if (Params.videoType == CODEC_MVC)
    {
        Params.videoType = MFX_CODEC_AVC;
        Params.bIsMVC = true;
    }

    if (Params.videoType == CODEC_MVC)
    {
        std::cout<<"CODEC_MVC "<<std::endl;
        Params.videoType = MFX_CODEC_AVC;
        Params.bIsMVC = true;
    }
    // use d3d9 rendering by default
    if (SYSTEM_MEMORY == Params.memType)
        Params.memType = D3D9_MEMORY;
    if (sts == MFX_ERR_ABORTED)
    {
        // No error, just need to close app normally
        return MFX_ERR_NONE;
    }
    MSDK_CHECK_PARSE_RESULT(sts, MFX_ERR_NONE, 1);

    if (Params.bIsMVC)
        Pipeline.SetMultiView();

    sts = Pipeline.Init(&Params);
    MSDK_CHECK_STATUS(sts, "Pipeline.Init failed");

    // print stream info
    // Pipeline.PrintInfo();

    msdk_printf(MSDK_STRING("Decoding started\n"));

    mfxU64 prevResetBytesCount = 0xFFFFFFFFFFFFFFFF;
    for (;;)
    {
        sts = Pipeline.RunDecoding();
        if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts || MFX_ERR_DEVICE_LOST == sts || MFX_ERR_DEVICE_FAILED == sts)
        {
            if (prevResetBytesCount == Pipeline.GetTotalBytesProcessed())
            {
                msdk_printf(MSDK_STRING("\nERROR: No input data was consumed since last reset. Quitting to avoid looping forever.\n"));
                break;
            }
            prevResetBytesCount = Pipeline.GetTotalBytesProcessed();


            if (MFX_ERR_INCOMPATIBLE_VIDEO_PARAM == sts)
            {
                msdk_printf(MSDK_STRING("\nERROR: Incompatible video parameters detected. Recovering...\n"));
            }
            else
            {
                msdk_printf(MSDK_STRING("\nERROR: Hardware device was lost or returned unexpected error. Recovering...\n"));
                sts = Pipeline.ResetDevice();
                MSDK_CHECK_STATUS(sts, "Pipeline.ResetDevice failed");
            }

            sts = Pipeline.ResetDecoder(&Params);
            MSDK_CHECK_STATUS(sts, "Pipeline.ResetDecoder failed");
            continue;
        }
        else
        {
            MSDK_CHECK_STATUS(sts, "Pipeline.RunDecoding failed");
            break;
        }
    }

    msdk_printf(MSDK_STRING("\nDecoding finished\n"));

    return 0;
}


