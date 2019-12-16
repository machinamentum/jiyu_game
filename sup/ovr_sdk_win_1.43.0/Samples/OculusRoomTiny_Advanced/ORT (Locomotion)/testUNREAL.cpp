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

namespace TESTUNREAL
{
    /// Variables / Demo controls
    /// =========================
    /// This demo shows an unreal backdrop vs a real cockpit
    /// Hmm...largely unconvincing at present.
    /// '1' recentres
    /// '3' turns off effect
    /// '4' for bigger cockpit
    /// '6' hold for a counter lurch on the cockpit
    /// '7' hold for a double counter lurch on the cockpit
    /// '8' hold for a quad counter lurch on the cockpit - PARTICULARLY NOTICE LACK OF APPARENT MOVEMENT IN VR
    /// '9' hold 9 for lurch
    bool extendedCockpit;
    bool forceEffectOff;

    // Local
    Model * cockpitModel;
    Model * cockpitModelRegular = 0;
    Model * cockpitModelExtended = 0;
}
using namespace TESTUNREAL;


//-------------------------------
int(*TESTUNREAL_Init(bool argExtendedCockpit, bool argForceEffectOff))(void)
{
    extendedCockpit = argExtendedCockpit;
    forceEffectOff = argForceEffectOff;

    Layer0 = standardLayer0;

    if (!cockpitModelRegular) cockpitModelRegular = SCENE_CreateCockpitModel(false);
    if (!cockpitModelExtended) cockpitModelExtended = SCENE_CreateCockpitModel(true);
    if (extendedCockpit) cockpitModel = cockpitModelExtended;
    else                  cockpitModel = cockpitModelRegular;

    *mainCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());
    localCam = new Camera(&Vector3(0.0f, 0.0f, 0.0f), &Quat4());

    return(TESTUNREAL_Process_And_Render);
}

//----------------------------------
int TESTUNREAL_Process_And_Render()
{
    APPCAMERA_SmoothedMove(mainCam, 0.1f, 0.02f);

    if (DIRECTX.Key['9'])
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0.05f, 0.05f);
    if (DIRECTX.Key['6']) APPCAMERA_ApplyLurchAfterCameraMove(localCam, -1 * 0.05f, -1 * 0.05f);
    if (DIRECTX.Key['7']) APPCAMERA_ApplyLurchAfterCameraMove(localCam, -2 * 0.05f, -2 * 0.05f);
    if (DIRECTX.Key['8']) APPCAMERA_ApplyLurchAfterCameraMove(localCam, -4 * 0.05f, -4 * 0.05f);

    // Generate new text
    if (frameIndex < 600)
    {
        INFOTEXT_GetModelToAdd()->Col(0xff000000);
        INFOTEXT_GetModelToAdd()->Add(1, "UNREAL PROTOTYPE")->CR();
        INFOTEXT_GetModelToAdd()->Add(1, "Correct cockpit and text - unreal 3D element")->CR();
        INFOTEXT_GetModelToAdd()->Add(1, "Press '1' if this isn't in front of you");
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
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer0->ld.RenderPose[eye]);

        // Seriously corrupt view
        float offx = 150;
        float offy = 300;
        if ((!(DIRECTX.Key['3'])) && (!forceEffectOff))
        {
            DIRECTX.SetViewport((float)Layer0->ld.Viewport[eye].Pos.x + 0.5f*offx, (float)Layer0->ld.Viewport[eye].Pos.y + 0.5f*offy,
                (float)Layer0->ld.Viewport[eye].Size.w - offx, (float)Layer0->ld.Viewport[eye].Size.h - offy);
        }

        corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        skyModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Correct Viewport
        DIRECTX.SetViewport((float)Layer0->ld.Viewport[eye].Pos.x, (float)Layer0->ld.Viewport[eye].Pos.y,
            (float)Layer0->ld.Viewport[eye].Size.w, (float)Layer0->ld.Viewport[eye].Size.h);

        Matrix44 localViewMatrix = VRCONVERT_GetViewMatrixWithEyePose(localCam, Layer0->ld.RenderPose[eye]);
        cockpitModel->Render(&(localViewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);

        Layer0->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(1);
}


