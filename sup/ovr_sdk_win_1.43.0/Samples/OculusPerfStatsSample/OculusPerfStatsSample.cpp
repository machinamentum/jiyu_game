/************************************************************************************
Filename    :   OculusPerfStatsCollectorSample.cpp
Content     :   Performance statistics collector application for Oculus Rift
Created     :   15th Feb 2017
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
/// This sample demonstrates how you can gather any Oculus Rift enabled application's
/// performance statistics via the provided SDK API.

#include "OVR_CAPI.h"   // Include the Oculus SDK
#include <assert.h>
#include <thread>
#include <stdio.h>

//-------------------------------------------------------------------------------------

int main()
{
    // Initializes LibOVR, and the Rift
    ovrInitParams initParams = { ovrInit_Invisible | ovrInit_RequestVersion, OVR_MINOR_VERSION, NULL, 0, 0 };
    ovrResult result = ovr_Initialize(&initParams);
    assert(OVR_SUCCESS(result)); // Failed to initialize libOVR

    printf("Oculus Rift Performance Stats Viewer Sample\n"
           "-------------------------------------------\n");

    {
        ovrSession session;
        ovrGraphicsLuid luid;
        ovrResult result = ovr_Create(&session, &luid);
        if (!OVR_SUCCESS(result))
        {
            printf("ERROR: Failed to create session.\n");
            return 1;
        }
        
        ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);
        float hmdRefreshRate = hmdDesc.DisplayRefreshRate;

        ovrPerfStats lastPerfStats;
        ovrPerfStatsPerCompositorFrame& lastFrameStats = lastPerfStats.FrameStats[0]; // [0] contains the most recent stats
        lastFrameStats.HmdVsyncIndex = -1;  // reset to make sure we know it's not updated via the SDK yet
        lastPerfStats.VisibleProcessId = -1;

        ovrPerfStats perfStats;

        bool waitingForStatsMessaged = false;

        // Main loop
        while (1)
        {
            ovrSessionStatus sessionStatus;
            ovr_GetSessionStatus(session, &sessionStatus);
            if (sessionStatus.ShouldQuit || sessionStatus.DisplayLost || !sessionStatus.HmdPresent )
            {
                break;
            }
            assert(!sessionStatus.IsVisible);   // this session should never be visible since it's not submitting any frames

            ovr_GetPerfStats(session, &perfStats);

            if (lastPerfStats.VisibleProcessId == -1)
            {
            }

            // Did we get any valid stats?
            if (perfStats.FrameStatsCount > 0)
            {
                // Did we process a frame before?
                if (lastFrameStats.HmdVsyncIndex > 0)
                {
                    // Are we still looking at the same app, or did focus shift?
                    if (lastPerfStats.VisibleProcessId == perfStats.VisibleProcessId)
                    {
                        int framesSinceLastInterval = perfStats.FrameStats[0].HmdVsyncIndex - lastFrameStats.HmdVsyncIndex;

                        int appFramesSinceLastInterval = perfStats.FrameStats[0].AppFrameIndex - lastFrameStats.AppFrameIndex;
                        int compFramesSinceLastInterval = perfStats.FrameStats[0].CompositorFrameIndex - lastFrameStats.CompositorFrameIndex;

                        float appFrameRate = (float)appFramesSinceLastInterval / framesSinceLastInterval * hmdRefreshRate;
                        float compFrameRate = (float)compFramesSinceLastInterval / framesSinceLastInterval * hmdRefreshRate;

                        printf("App PID: %d\tApp FPS: %0.0f\tCompositor FPS: %0.0f\n", perfStats.VisibleProcessId, appFrameRate, compFrameRate);
                    }
                    else
                    {
                        printf("Focus shifted to another VR app. Resetting perf stats...\n");
                        result = ovr_ResetPerfStats(session);
                        if (!OVR_SUCCESS(result))
                        {
                            printf("ERROR: Failed to reset perf stats.\n");
                            return 1;
                        }
                    }
                }

                // save off values for next interval's calculations
                lastPerfStats = perfStats;
                waitingForStatsMessaged = false;
            }
            else if(!waitingForStatsMessaged)
            {
                // This can happen if there are no VR applications currently in focus, or if the HMD has been
                // static for a certain amount of time, causing the SDK to put the HMD display to sleep.
                printf("Waiting for performance stats...\n");
                waitingForStatsMessaged = true;
            }

            // In this app, since we don't care about individual frame time values and only looking at frame rate,
            // we can update the frame rate once a second (i.e. 1000 ms) and look at the number of frames rendered.
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }

        // Release resources
        ovr_Destroy(session);
    }

    ovr_Shutdown();
    return 0;
}
