/************************************************************************************

Filename    :   RenderProfiler.h
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

#ifndef OVR_RenderProfiler_h
#define OVR_RenderProfiler_h

#include "Kernel/OVR_Types.h"
#include "Kernel/OVR_Allocator.h"
#include "Kernel/OVR_RefCount.h"
#include "Kernel/OVR_System.h"
#include "Kernel/OVR_Nullptr.h"
#include "Kernel/OVR_Timer.h"
#include "Extras/OVR_Math.h"

// TODO: Refactor option menu so dependencies are in a separate file.
#include "OptionMenu.h"

#include <vector>
#include <string>

//-------------------------------------------------------------------------------------
// ***** RenderProfiler

// Tracks reported timing sample in a frame and dislays them an overlay from DrawOverlay().
class RenderProfiler
{
public:
    enum { NumFramesOfTimerHistory = 10 };

    enum SampleType
    {
        Sample_FrameStart            ,
        Sample_AfterGameProcessing   ,
        Sample_AfterEyeRender        ,
      //Sample_BeforeDistortion      ,
      //Sample_AfterDistortion       ,
        Sample_AfterPresent          ,
      //Sample_AfterFlush            ,

        Sample_LAST
    };

    RenderProfiler();

    // Records the current time for the given sample type.
    void          RecordSample(SampleType sampleType);

    const double* GetAverages() const { return SampleAverage; } 
    const double* GetLastSampleSet() const;

    // Returns rendered bounds.
    Recti          DrawOverlay(RenderDevice* prender, float centerX, float centerY, float textHeight);

private:

    double      SampleHistory[NumFramesOfTimerHistory][Sample_LAST];
    double      SampleAverage[Sample_LAST];
    int         SampleCurrentFrame;
};

#endif // OVR_RenderProfiler_h
