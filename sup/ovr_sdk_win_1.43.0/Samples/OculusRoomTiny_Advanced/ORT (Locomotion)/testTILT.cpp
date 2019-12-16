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

namespace TESTTILT
{
    /// Variables / Demo controls
    /// =========================
    /// This has accelerating control, and attempts to tilt the world 
    ///in anti-nausea response to that acceleration.
    /// Use strafe keys to test in one way
    /// Use arrows and 'drive' to test another - note the subtle tilting
    /// '1' recentres
    /// '2' kills the effect
    bool extraSmoothCamera;  // Having this is slightly more nauseous, but creates a smoother impression for testing the method
    // It appears that the more instantaneous accelerations and snaps to tilts are more comfortable. 
    float proportionOfTiltToAdopt = 1.0f;  // 1 = full amount.
    bool linearApproach;  // Other approach is with trig and gravity component, but ingame accel are too great in practice and make tilt too much
}
using namespace TESTTILT;

//-------------------------------
int(*TESTTILT_Init(bool argLinearApproach, bool argExtraSmoothCamera))(void)
{
    linearApproach = argLinearApproach;
    extraSmoothCamera = argExtraSmoothCamera;

    Layer0 = standardLayer0;
    *mainCam = Camera(&Vector3(0.0f, 2.0f, 0.0f), &Quat4());

    return(TESTTILT_Process_And_Render);
}

//----------------------------------
int TESTTILT_Process_And_Render()
{
    if (extraSmoothCamera)
        APPCAMERA_VerySmoothedMoveForTilt(mainCam, linearApproach ? 0.01f : 0.003f, 0.02f);
    else 
        APPCAMERA_SmoothedMove(mainCam, 0.1f, 0.02f);

    // Generate new text
    if (frameIndex < 300)
    {
        INFOTEXT_GetModelToAdd()->Add(1, "LOCOMOTION PROTOTYPES")->CR();
        INFOTEXT_GetModelToAdd()->Add(1, "This is the second line")->CR();
        INFOTEXT_GetModelToAdd()->Add(1, "Frame index = %d", frameIndex)->CR();
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

    // Find acceleration direction
    static Vector3 pos2ago = Vector3(0, 0, 0);
    static Vector3 pos1ago = Vector3(0, 0, 0);
    Vector3 pos0ago = mainCam->Pos;
    Vector3 vel1ago = pos1ago - pos2ago;
    Vector3 vel0ago = pos0ago - pos1ago;
    Vector3 acc = vel0ago - vel1ago;

    // ...we are done so upgrade them
    pos2ago = pos1ago;
    pos1ago = pos0ago;

    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        // Layer0
        Layer0->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer0->ld.RenderPose[eye]);

        // Lets make a tilt for the world models.
        Quat4 desiredTilt;
        if (linearApproach)
            desiredTilt = Quat4(-90.0f*proportionOfTiltToAdopt*acc.z, 0, 90.0f*proportionOfTiltToAdopt* acc.x);
        else
        {
            float adjustForScale = 90.0f;
            desiredTilt = Quat4(-proportionOfTiltToAdopt*(atan(adjustForScale*adjustForScale*acc.z / 9.81f)), 0, proportionOfTiltToAdopt* (atan(adjustForScale*adjustForScale*acc.x / 9.81f)));
        }
        if (DIRECTX.Key['2'])
            desiredTilt = Quat4();  // Turn off effect
        Matrix44 tiltMat = desiredTilt.ToRotationMatrix44();
        Vector3 translationThere = mainCam->Pos;
        Vector3 translationBack = mainCam->Pos * -1.0f;
        Matrix44 transMatThere = translationThere.ToTranslationMatrix44();
        Matrix44 transMatBack = translationBack.ToTranslationMatrix44();

        roomModel->Render(&(transMatBack*tiltMat*transMatThere * viewMatrix * projMatrix), 1, 1, 1, 1, true);
        corridorModel->Render(&(transMatBack*tiltMat*transMatThere * viewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);
        Layer0->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(1);
}

