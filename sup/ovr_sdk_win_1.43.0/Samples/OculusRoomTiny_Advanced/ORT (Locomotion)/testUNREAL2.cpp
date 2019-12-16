/************************************************************************************
Content     :   Locomotion research application for Oculus Rift
Created     :   23rd Dec 2017
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

namespace TESTUNREAL2
{
    /// Variables / Demo controls
    /// =========================
    /// This demo shows how the cockpit can become 'more real' relative 
	/// to the main VR world, by dropping the framerate of the VR world.
    /// Two methods are exhibited, one where even the rotation of the HMD shows low framerate
	/// and the other just performing the potentially discomforting locomotion at lower framerate.
	/// '1' recentres
	/// '3' disables the effect
	/// '9' hold 9 for lurch
    bool extendedCockpit;
	int framesToSkip;
	int framesToSkipJustCamera;
	bool smoothMove;

    // Local
    Model * cockpitModel;
    Model * cockpitModelRegular = 0;
    Model * cockpitModelExtended = 0;
}
using namespace TESTUNREAL2;


//-------------------------------
int(*TESTUNREAL2_Init(bool argExtendedCockpit, int argFramesToSkip, bool argSmoothMove, int argFramesToSkipJustCamera))(void)
{
    extendedCockpit = argExtendedCockpit;
	framesToSkip = argFramesToSkip;
	framesToSkipJustCamera = argFramesToSkipJustCamera;
	smoothMove = argSmoothMove;

    Layer0 = standardLayer0;

    if (!cockpitModelRegular) cockpitModelRegular = SCENE_CreateCockpitModel(false);
    if (!cockpitModelExtended) cockpitModelExtended = SCENE_CreateCockpitModel(true);
    if (extendedCockpit) cockpitModel = cockpitModelExtended;
    else                  cockpitModel = cockpitModelRegular;

    *mainCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());
    localCam = new Camera(&Vector3(0.0f, 0.0f, 0.0f), &Quat4());

    return(TESTUNREAL2_Process_And_Render);
}

//----------------------------------
int TESTUNREAL2_Process_And_Render()
{
 	if (smoothMove)
		APPCAMERA_VerySmoothedMoveForTilt(mainCam, 0.01f, 0.02f);
	else
		APPCAMERA_SmoothedMove(mainCam, 0.1f, 0.02f);


    if (DIRECTX.Key['9'])
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0.05f, 0.05f);
 
     // Get the current pose, for standard eye offset, for general use.
    double sensorSampleTime = 0;
    ovrPosef currPose[2];

	ovrPosef HmdToEyePose[2];
	HmdToEyePose[0].Position = VRCONVERT_ovrVector3f(Vector3(-0.032f, 0, 0));
	HmdToEyePose[1].Position = VRCONVERT_ovrVector3f(Vector3(+0.032f, 0, 0));
	HmdToEyePose[0].Orientation = VRCONVERT_ovrQuatf(Quat4());
	HmdToEyePose[1].Orientation = VRCONVERT_ovrQuatf(Quat4());
	ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, currPose, &sensorSampleTime);


	// Lets adjust framerate of main camera
	Camera mainCamToUse;
	static Camera savedMainCam;
	if (((frameIndex % framesToSkipJustCamera) == 0) || (DIRECTX.Key['3']))
		savedMainCam = *mainCam;
	mainCamToUse = savedMainCam;


    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
		// Set viewport
		DIRECTX.SetViewport((float)Layer0->ld.Viewport[eye].Pos.x, (float)Layer0->ld.Viewport[eye].Pos.y,
			(float)Layer0->ld.Viewport[eye].Size.w, (float)Layer0->ld.Viewport[eye].Size.h);
		
		// Layer0
        Layer0->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        Matrix44 correctViewMatrix = VRCONVERT_GetViewMatrixWithEyePose(&mainCamToUse, Layer0->ld.RenderPose[eye]);
		
		// Use lower framerate matrix, unless '3' is pressed
		Matrix44 viewMatrixToUse;
		static Matrix44 savedViewMatrix[2];
		if (((frameIndex % framesToSkip) == 0) || (DIRECTX.Key['3']))
			savedViewMatrix[eye] = correctViewMatrix;
		viewMatrixToUse = savedViewMatrix[eye];

        corridorModel->Render(&(viewMatrixToUse * projMatrix), 1, 1, 1, 1, true);
        skyModel->Render(&(viewMatrixToUse * projMatrix), 1, 1, 1, 1, true);

 		// Use correct matrix for cockpit
        Matrix44 localViewMatrix = VRCONVERT_GetViewMatrixWithEyePose(localCam, Layer0->ld.RenderPose[eye]);
        cockpitModel->Render(&(localViewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);

        Layer0->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(1);
}


