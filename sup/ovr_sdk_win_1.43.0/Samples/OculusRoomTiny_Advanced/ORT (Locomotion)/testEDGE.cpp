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

namespace TESTEDGE
{
    /// Variables / Demo controls
    /// =========================
    /// This demo shows a static environment in the periphery
    /// '1' recentres
    /// '2' hold for debug trigger of edge coming in.
    /// '3' hold to get it not to fade out
    /// '4' hold to get different holodeck and colours of main scene
    /// '9' hold 9 for lurch
    bool equaliseHorizFovs;  
    #define INWARD_SPEED 0.01f
    #define OUTWARD_SPEED 0.005f
    bool noTrans;

    // Local
    VRLayer * variableLayer = 0;

}
using namespace TESTEDGE;

//-------------------------------
int(*TESTEDGE_Init(bool argEqualiseHorizFovs, bool argNoTrans))(void)
{
    equaliseHorizFovs = argEqualiseHorizFovs;
    noTrans = argNoTrans;

    Layer0 = standardLayer0;
    if (!variableLayer) variableLayer = new VRLayer(session, hmdDesc.DefaultEyeFov[0], hmdDesc.DefaultEyeFov[1], 1.0f, true); 
    Layer1 = variableLayer; 

    *mainCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());
    *localCam = Camera(&Vector3(0.0f, 2.0f, 5.0f), &Quat4());

    return(TESTEDGE_Process_And_Render);
}

//----------------------------------
int TESTEDGE_Process_And_Render()
{
    APPCAMERA_SmoothedMove(mainCam, 0.1f, 0.02f);

    if (DIRECTX.Key['9'])
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0.1f, 0.1f);

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

    GAME_Process(mainCam->Pos, VRCONVERT_Quat4(currPose[0].Orientation)* mainCam->Rot);
    
    // Lets see if in nausea danger
    // Just simply on any motion button press for now
    bool nauseaDanger = false;
    if ((DIRECTX.Key['A'])
     || (DIRECTX.Key['S'])
     || (DIRECTX.Key['D'])
     || (DIRECTX.Key['W'])
     || (DIRECTX.Key[VK_UP])
     || (DIRECTX.Key[VK_DOWN])
     || (DIRECTX.Key[VK_LEFT])
     || (DIRECTX.Key[VK_RIGHT]))
     nauseaDanger = true;

    // Lets allow a debug trigger too. 
    if (DIRECTX.Key['2'])
        nauseaDanger = true;

    // Press '2' to live-vary Layer1 FOV, assymetrically
    static float amountIn = 0;
    if (nauseaDanger)    amountIn += INWARD_SPEED;
    else                amountIn -= OUTWARD_SPEED;
    Maths::Limit(&amountIn, 0, 0.5f);

    ovrFovPort newFov[2] = { hmdDesc.DefaultEyeFov[0], hmdDesc.DefaultEyeFov[1] };

    // Lets try for symmetrically handling it, at least horizontally for fusion.
    if (equaliseHorizFovs)
    {
        newFov[1].LeftTan = newFov[1].RightTan;
        newFov[0].RightTan = newFov[0].LeftTan;
    }

    float degreeIn = amountIn;

    newFov[0].UpTan    -= amountIn;
    newFov[0].DownTan -= amountIn;
    newFov[0].LeftTan -= amountIn;
    newFov[0].RightTan -= amountIn;
    newFov[1].UpTan -= amountIn;
    newFov[1].DownTan -= amountIn;
    newFov[1].LeftTan -= amountIn;
    newFov[1].RightTan -= amountIn;

    Layer1->ld.Fov[0] = newFov[0];
    Layer1->ld.Fov[1] = newFov[1];

    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        // Layer0
        Layer0->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(localCam, Layer0->ld.RenderPose[eye]);

        if (DIRECTX.Key['4'])
            holoModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        else
            roomModelNoFurniture->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        Layer0->FinishRendering(session, eye);

        // Layer1
        Layer1->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer1->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer1->ld.RenderPose[eye]);

        float amountOfTrans = 1.0f - (amountIn);
        if ((DIRECTX.Key['3']) || (noTrans)) amountOfTrans = 1.0f;
        if (DIRECTX.Key['4']) corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, amountOfTrans, true);
        else                  corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 0, amountOfTrans, true);
        skyModel->Render(&(viewMatrix * projMatrix), 1, 1, 0, amountOfTrans, true);

        GAME_Render(&(viewMatrix * projMatrix));

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);

        Layer1->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(2);

}

