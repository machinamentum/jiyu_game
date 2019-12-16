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

namespace TESTDESENS
{
    /// Variables / Demo controls
    /// =========================
    /// This demo shows a static environment in the periphery
    /// '1' recentres
    /// '9' hold 9 for lurch
    float frequency;
}
using namespace TESTDESENS;

//-------------------------------
int(*TESTDESENS_Init(float argFrequency))(void)
{
    frequency = argFrequency;

    Layer0 = standardLayer0;
    Layer1 = standardLayer1;

    *mainCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());
    *counterCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());

    return(TESTDESENS_Process_And_Render);
}

//----------------------------------
int TESTDESENS_Process_And_Render()
{
    APPCAMERA_SmoothedMove(mainCam, 0.1f, 0.02f);

    // Counter cam is the same
    counterCam->Pos = mainCam->Pos;
    counterCam->idealPos = mainCam->idealPos;
    counterCam->smoothedPos = mainCam->smoothedPos;
    counterCam->Rot = mainCam->Rot;
    counterCam->OptionalYaw = mainCam->OptionalYaw;

    // Now add counter lurching
    if (DIRECTX.Key['9'])
    {
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0, 0.15f,frequency);
        APPCAMERA_ApplyLurchAfterCameraMove(counterCam, 0, -0.15f,frequency);
    }
    else
    {
        // Reset to straight ahead
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0, 0);
        APPCAMERA_ApplyLurchAfterCameraMove(counterCam, -0, -0);
    }

    // Get the current pose, for standard eye offset, for general use.
    double sensorSampleTime = 0;
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
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(counterCam, Layer0->ld.RenderPose[eye]);
        corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        skyModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        Layer0->FinishRendering(session, eye);

        // Layer1
        Layer1->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer1->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer1->ld.RenderPose[eye]);

        float amountOfTrans = 0.25f;
        corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, amountOfTrans, true);
        skyModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, amountOfTrans, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);

        Layer1->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(2);

}


