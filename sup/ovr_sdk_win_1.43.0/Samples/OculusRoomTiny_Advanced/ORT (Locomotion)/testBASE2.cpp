/************************************************************************************
Content     :   Locomotion research application for Oculus Rift
Created     :   19th July 2017
Authors     :   Tom Heath
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

#include "general.h"

namespace TESTBASE2
{
    /// Variables / Demo controls
    /// =========================
    /// This has accelerating control, and attempts to tilt the world 
    /// in anti-nausea response to that acceleration
    /// '1' recentres
}
using namespace TESTBASE2;

//-------------------------------
int(*TESTBASE2_Init())(void)
{
    Layer0 = standardLayer0;
    *mainCam = Camera(&Vector3(0.0f, 2.0f, 0.0f), &Quat4());

    return(TESTBASE2_Process_And_Render);
}

//----------------------------------
int TESTBASE2_Process_And_Render()
{
    APPCAMERA_DefaultMove(mainCam, 0.05f, 0.02f);

    if (DIRECTX.Key['9'])
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0.1f, 0.1f);

    // Get the current pose, for standard eye offset, for general use.
    double sensorSampleTime;
    ovrPosef currPose[2];

#ifdef BUILD_FOR_OLD_SDK
    ovrVector3f HmdToEyeOffset[2] = { { -0.032f, 0, 0 }, { +0.032f, 0, 0 } };
    ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, currPose, &sensorSampleTime);
#else
    ovrPosef HmdToEyePose[2];
    HmdToEyePose[0].Position = VRCONVERT_ovrVector3f(Vector3(-0.032f, 0, 0));
    HmdToEyePose[1].Position = VRCONVERT_ovrVector3f(Vector3(+0.032f, 0, 0));
    HmdToEyePose[0].Orientation = VRCONVERT_ovrQuatf(Quat4());
    HmdToEyePose[1].Orientation = VRCONVERT_ovrQuatf(Quat4());
    ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, currPose, &sensorSampleTime);
#endif

    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        // Layer0
        Layer0->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer0->ld.RenderPose[eye]);

        roomModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);

        corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);
        Layer0->FinishRendering(session, eye);
    }

    return(1); // Layers to submit to compositor
}

