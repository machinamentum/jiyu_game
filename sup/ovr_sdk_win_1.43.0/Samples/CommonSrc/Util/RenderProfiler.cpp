/************************************************************************************

Filename    :   RenderProfiler.cpp
Content     :   Profiling for render.
Created     :   March 10, 2014
Authors     :   Caleb Leak

Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*************************************************************************************/

#include "RenderProfiler.h"

using namespace OVR;

RenderProfiler::RenderProfiler()
{
    memset(SampleHistory, 0, sizeof(SampleHistory));
    memset(SampleAverage, 0, sizeof(SampleAverage));
    SampleCurrentFrame = 0;
}

void RenderProfiler::RecordSample(SampleType sampleType)
{
    if (sampleType == Sample_FrameStart)
    {
        // Recompute averages and subtract off frame start time.
        for (int sample = 1; sample < Sample_LAST; sample++)
        {
            SampleHistory[SampleCurrentFrame][sample] -= SampleHistory[SampleCurrentFrame][0];

            // Recompute the average for the current sample type.
            SampleAverage[sample] = 0.0;
            for (int frame = 0; frame < NumFramesOfTimerHistory; frame++)
            {
                SampleAverage[sample] += SampleHistory[frame][sample];
            }
            SampleAverage[sample] /= NumFramesOfTimerHistory;
        }

        SampleCurrentFrame = ((SampleCurrentFrame + 1) % NumFramesOfTimerHistory);
    }

    SampleHistory[SampleCurrentFrame][sampleType] = ovr_GetTimeInSeconds();
}

const double* RenderProfiler::GetLastSampleSet() const
{
    return SampleHistory[(SampleCurrentFrame - 1 + NumFramesOfTimerHistory) % NumFramesOfTimerHistory];
}

// Returns rendered bounds
Recti RenderProfiler::DrawOverlay(RenderDevice* prender, float centerX, float centerY, float textHeight)
{
    char buf[256 * Sample_LAST];
    OVR_strcpy ( buf, sizeof(buf), "Timing stats" );     // No trailing \n is deliberate.

    /*int timerLastFrame = TimerCurrentFrame - 1;
    if ( timerLastFrame < 0 )
    {
        timerLastFrame = NumFramesOfTimerHistory - 1;
    }*/
    // Timer 0 is always the time at the start of the frame.

    const double* averages = GetAverages();
    const double* lastSampleSet = GetLastSampleSet();

    static_assert((Sample_FrameStart == 0) && (Sample_AfterGameProcessing == 1) && (Sample_AfterPresent + 1 == Sample_LAST), "The following code depends on SampleType enum values.");

    for ( int timerNum = Sample_AfterGameProcessing; timerNum < Sample_LAST; timerNum++ )
    {
        char const *pName = "";
        switch ( timerNum )
        {
        case Sample_AfterGameProcessing:     pName = "AfterGameProcessing"; break;
        case Sample_AfterEyeRender     :     pName = "AfterEyeRender     "; break;
      //case Sample_BeforeDistortion   :     pName = "BeforeDistortion   "; break;  This enumerant is currently disabled in the enumeration declaration.
      //case Sample_AfterDistortion    :     pName = "AfterDistortion    "; break;
        case Sample_AfterPresent       :     pName = "AfterPresent       "; break;
      //case Sample_AfterFlush         :     pName = "AfterFlush         "; break;
        }
        char bufTemp[256];
        snprintf( bufTemp, sizeof(bufTemp), "\nRaw: %.2lfms\t400Ave: %.2lfms\t800%s",
                        lastSampleSet[timerNum] * 1000.0, averages[timerNum] * 1000.0, pName );
        OVR_strcat ( buf, sizeof(buf), bufTemp );
    }

    return DrawTextBox(prender, centerX, centerY, textHeight, buf, DrawText_Center);
}
