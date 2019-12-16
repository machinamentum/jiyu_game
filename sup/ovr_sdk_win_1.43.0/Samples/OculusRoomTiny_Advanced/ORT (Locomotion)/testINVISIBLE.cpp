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

namespace TESTINVISIBLE
{
    /// Variables / Demo controls
    /// =========================
    /// This demo shows how one can use a shader to have the overall brightness to be uniform
	/// such that the periphery becomes less visible to the cones in the periphery of
	/// your eye.
	/// '1' recentres
	/// '9' hold 9 for lurch
	bool smoothMove;
	Model * alternateCorridorModel;
	Model * alternateSkyModel;
}
using namespace TESTINVISIBLE;


//--------------------------------------------
void SetModelMaterialRecursively(Model * model, Material * material)
{
	model->Mat = material;
	if (model->SubModel) SetModelMaterialRecursively(model->SubModel, material);
}
//--------------------------------------------
void SetModelShaderRecursively(Model * model, Shader * shader)
{
	if (model->Mat) model->Mat->TheShader = shader;
	if (model->SubModel) SetModelShaderRecursively(model->SubModel, shader);
}


//-------------------------------
int(*TESTINVISIBLE_Init(bool argSmoothMove))(void)
{
	smoothMove = argSmoothMove;

    Layer0 = standardLayer0;

    *mainCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());

	// Make new shader
	char invisibleShader[] =
		"Texture2D Texture   : register(t0); SamplerState Linear : register(s0); "
		"float4 main(in float4 Position : SV_Position, in float4 mainCol: COLOR0, in float2 TexCoord : TEXCOORD0) : SV_Target"
		"{                                                       "
		"    float4 texCol = Texture.Sample(Linear, TexCoord);                        "
		"    mainCol.rgb *= 2;														  "
		"    float4 c = texCol*mainCol;												  "
		"    if (mainCol.r > 1) c.r = lerp(texCol.r, 1, (mainCol.r - 1.0f));	  "
		"    if (mainCol.g > 1) c.g = lerp(texCol.g, 1, (mainCol.g - 1.0f));	  "
		"    if (mainCol.b > 1) c.b = lerp(texCol.b, 1, (mainCol.b - 1.0f));	  "
		"    float totalCol = c.r + c.g + c.b;"
		"    float multiplier = 1.5f/totalCol;"
		"    c *= multiplier;"
		"    return (c);                                                              "
		"}                                                       ";

	Shader * newShader = new Shader("InvisibleShader", 0, 3, 0, invisibleShader);

	//Make alternate models for use here, so we don't modify all the other tests
	alternateCorridorModel = SCENE_CreateCorridorModel(false); 
	alternateSkyModel = SCENE_CreateSkyBoxModel();

	//Swap in new shader recursively
	Material * floorMat = alternateCorridorModel->SubModel->Mat;
	SetModelMaterialRecursively(alternateCorridorModel, floorMat);

	SetModelShaderRecursively(alternateCorridorModel, newShader);
	SetModelShaderRecursively(alternateSkyModel, newShader);

    return(TESTINVISIBLE_Process_And_Render);
}



//----------------------------------
int TESTINVISIBLE_Process_And_Render()
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

    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
		// Set viewport
		DIRECTX.SetViewport((float)Layer0->ld.Viewport[eye].Pos.x, (float)Layer0->ld.Viewport[eye].Pos.y,
			(float)Layer0->ld.Viewport[eye].Size.w, (float)Layer0->ld.Viewport[eye].Size.h);
		
		// Layer0
        Layer0->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer0->ld.RenderPose[eye]);

		alternateCorridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
		alternateSkyModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);

        Layer0->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(1);
}


