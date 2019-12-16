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

namespace TESTPORTAL
{
    /// Variables / Demo controls
    /// =========================
    /// '1' recentres
    /// '3' hold to have full 3D of nausea layer
    /// '4' hold to expand TVs
    /// '6' hold to reverse green colour
    /// '7' hold to take away portals
    /// '9' hold 9 for lurch
    /// A note - seems important to have portal rims coloured the same as the main world
    /// A note - lurch with full spread corridor portal works well

    DWORD col = 0xffffffff;
    float HALF_HEIGHT[2] = { 1.00f, 1.50f }; // The two sizes toggleable
    float HALF_WIDTH[2] = { 0.50f, 0.75f };
    static bool INGAME_IS_FLAT = true;
    bool forceFull3D;
    bool forceFullSize;
    bool forceGreenReverse;
    #define NUM_TVS_HORIZ 20
    #define NUM_TVS_VERTI 3
    #define NUM_TVS ((NUM_TVS_HORIZ * NUM_TVS_VERTI)) 
    #define DO_TVS_HAVE_EDGES true
    bool STATIC_IS_GREEN; // Otherwise, world is green
    #define SHOW_CROSSHAIRS false

    // Local
    Model * TVmodel[NUM_TVS][2];
    Model * loadsOfCubes;
    bool firstTime = true;
}
using namespace TESTPORTAL;

//-------------------------------
int(*TESTPORTAL_Init(bool argForceFull3D, bool argForceFullSize, bool argForceGreenReverse))(void)
{
    forceFull3D = argForceFull3D;
    forceFullSize = argForceFullSize;
    forceGreenReverse = argForceGreenReverse;

    // Make the VR layer, with all its ingredients 
    Layer0 = standardLayer0;

    *mainCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());
    *localCam = Camera(&Vector3(0.0f, 2.0f, 5.0f), &Quat4());

    if (firstTime)
    {
        firstTime = false;

        // Lets make a TV object.
        Texture * wallTex;
        wallTex = new Texture(false, 512, 512, DO_TVS_HAVE_EDGES ? Texture::AUTO_DOUBLEGRID_CLEAR : Texture::AUTO_CLEAR, 1, 8);
		char ordinaryPShader[] =
			"Texture2D Texture   : register(t0); SamplerState Linear : register(s0); "
			"float4 main(in float4 Position : SV_Position, in float4 mainCol: COLOR0, in float2 TexCoord : TEXCOORD0) : SV_Target"
			"{                                                       "
			"    float4 texCol = Texture.Sample(Linear, TexCoord);                        "
			"    float4 c = texCol*mainCol;												  "
			"    return (c);                                                              "
			"};                                                      ";
		Shader * obShader = new Shader("ShaderOrdinary", 0, 3, 0, ordinaryPShader);
        Material * overBrightWallMat = new Material(wallTex, obShader, Material::MAT_WRAP | Material::MAT_TRANS);

        TriangleSet t;
        Quat4 rotToSend;
        for (int k = 0; k < 2; k++)
        {
            t.Empty();
            t.AddQuad(Vertex(Vector3(-HALF_WIDTH[k], HALF_HEIGHT[k], 0), col, 0, 0),
                Vertex(Vector3(HALF_WIDTH[k], HALF_HEIGHT[k], 0), col, 1, 0),
                Vertex(Vector3(-HALF_WIDTH[k], -HALF_HEIGHT[k], 0), col, 0, 1),
                Vertex(Vector3(HALF_WIDTH[k], -HALF_HEIGHT[k], 0), col, 1, 1));
            int curr = 0;
            for (int hh = 0; hh < NUM_TVS_HORIZ; hh++)
            for (int vv = 0; vv < NUM_TVS_VERTI; vv++)
            {
                TVmodel[curr][k] = new Model(&t, Vector3(0, 0, 0), rotToSend, overBrightWallMat);
                curr++;
            }
        }

        // Make a smattering of cubes
        t.Empty();
        for (int k = 0; k < 50; k++)
        {
            float DISTANCE_ALONG_CORRIDOR = -130;
            Vector3 loc = Vector3(2 * (Maths::Rand01() - Maths::Rand01()), 1.0f + 2.0f*Maths::Rand01(), DISTANCE_ALONG_CORRIDOR + (-2.0f*k));
            float edge = 0.3f;
            t.AddCube(loc + Vector3(-edge, -edge, -edge), loc + Vector3(edge, edge, edge), 0xffff00ff);
        }
        loadsOfCubes = new Model(&t, Vector3(0, 0, 0), rotToSend, overBrightWallMat);
    }

    // Useful to see crosshairs
    if (SHOW_CROSSHAIRS)    ovr_SetInt(session, OVR_DEBUG_HUD_STEREO_MODE, int(ovrDebugHudStereo_CrosshairAtInfinity));
    else                    ovr_SetInt(session, OVR_DEBUG_HUD_STEREO_MODE, int(ovrDebugHudStereo_Off));

    return(TESTPORTAL_Process_And_Render);
}

//----------------------------------
int TESTPORTAL_Process_And_Render()
{
    APPCAMERA_SmoothedMove(mainCam, 0.1f, 0.02f);

    if (DIRECTX.Key['9'])
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0.1f, 0.1f);

    if ((DIRECTX.Key['6']) || (forceGreenReverse)) STATIC_IS_GREEN = true;
    else                  STATIC_IS_GREEN = false;

    if ((DIRECTX.Key['3']) || (forceFull3D)) INGAME_IS_FLAT = false;
    else                                     INGAME_IS_FLAT = true;

    // Get the current pose, for standard eye offset, for general use.
    double sensorSampleTime=0;

#ifdef BUILD_FOR_OLD_SDK
    ovrPosef currPoseNormal[2];
    ovrVector3f HmdToEyeOffsetNormal[2] = { { -0.032f, 0, 0 }, { +0.032f, 0, 0 } };
    ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffsetNormal, currPoseNormal, &sensorSampleTime);
    ovrPosef currPoseFlat[2];
    ovrVector3f HmdToEyeOffsetFlat[2] = { { 0, 0, 0 }, { 0, 0, 0 } };
    ovr_GetEyePoses(session, frameIndex, ovrTrue, INGAME_IS_FLAT ? HmdToEyeOffsetFlat : HmdToEyeOffsetNormal, currPoseFlat, &sensorSampleTime);
#else
    ovrPosef currPoseNormal[2];
    ovrPosef HmdToEyePoseNormal[2];
    HmdToEyePoseNormal[0].Position = VRCONVERT_ovrVector3f(Vector3(-0.032f, 0, 0));
    HmdToEyePoseNormal[1].Position = VRCONVERT_ovrVector3f(Vector3(+0.032f, 0, 0));
    HmdToEyePoseNormal[0].Orientation = VRCONVERT_ovrQuatf(Quat4());
    HmdToEyePoseNormal[1].Orientation = VRCONVERT_ovrQuatf(Quat4());
    ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePoseNormal, currPoseNormal, &sensorSampleTime);

    ovrPosef currPoseFlat[2];
    ovrPosef HmdToEyePoseFlat[2];
    HmdToEyePoseFlat[0].Position = VRCONVERT_ovrVector3f(Vector3(0, 0, 0));
    HmdToEyePoseFlat[1].Position = VRCONVERT_ovrVector3f(Vector3(0, 0, 0));
    HmdToEyePoseFlat[0].Orientation = VRCONVERT_ovrQuatf(Quat4());
    HmdToEyePoseFlat[1].Orientation = VRCONVERT_ovrQuatf(Quat4());
    ovr_GetEyePoses(session, frameIndex, ovrTrue, INGAME_IS_FLAT ? HmdToEyePoseFlat : HmdToEyePoseNormal, currPoseFlat, &sensorSampleTime);
#endif

    // Lets rotate the TVs.
    for (int k = 0; k < 2; k++)
    {
        Quat4 rotToSend;
        int curr = 0;
        float centreZ = -100;
        float centreY = 2.0f;

        for (int hh = 0; hh < NUM_TVS_HORIZ; hh++)
        for (int vv = 0; vv < NUM_TVS_VERTI; vv++)
        {
            if (vv == 0)
            {
                TVmodel[curr][k]->Pos = Vector3(-1.98f + (vv * 3.96f), 0 + centreY, (hh - (NUM_TVS_HORIZ / 2)) * 2.0f + centreZ);
                TVmodel[curr][k]->Rot = Quat4(0, 90.0f / Maths::RTOD(), 0);
            }
            if (vv == 1)
            {
                TVmodel[curr][k]->Pos = Vector3(-1.98f + (vv * 3.96f), 0 + centreY, (hh - (NUM_TVS_HORIZ / 2)) * 2.0f + centreZ);
                TVmodel[curr][k]->Rot = Quat4(0, -90.0f / Maths::RTOD(), 0);
            }
            if (vv == 2)
            {
                // These are the floor panels.
                TVmodel[curr][k]->Pos = Vector3(0, -1.98f + centreY, (hh - (NUM_TVS_HORIZ / 2)) * 2.0f + centreZ);
                TVmodel[curr][k]->Rot = Quat4(-90.0f / Maths::RTOD(), 90.0f / Maths::RTOD(), 0);

            }
            curr++;
        }
    }

    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        // First render static ORT world
        Layer0->PrepareToRender(session, eye, currPoseNormal[eye], sensorSampleTime, 0, 0, 0, 1, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(localCam, Layer0->ld.RenderPose[eye]);

        if (STATIC_IS_GREEN) roomModelNoFurniture->Render(&(viewMatrix * projMatrix), 0.5f, 1, 0.5f, 1, true);
        else                 roomModelNoFurniture->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Then clear the z buffer, because we don't want the static model fighting with the 3D world.
        DIRECTX.Context->ClearDepthStencilView(Layer0->pEyeDepthBuffer[eye]->TexDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);


        // Re-set it all up, with a flat pose (optional)
        Layer0->PrepareToRender(session, eye, currPoseFlat[eye], sensorSampleTime, 0, 0, 0, 1, false);

        // This is artificial shunt to get flat to appear about in plane of TVs
        static float offy = 15.1f; 
        if (DIRECTX.Key['F']) offy -= 0.05f;
        if (DIRECTX.Key['G']) offy += 0.05f;

        if (!INGAME_IS_FLAT) // No viewport shunt for if simul normal 3D for distance
            DIRECTX.SetViewport((float)Layer0->ld.Viewport[eye].Pos.x, (float)Layer0->ld.Viewport[eye].Pos.y,
            (float)Layer0->ld.Viewport[eye].Size.w, (float)Layer0->ld.Viewport[eye].Size.h);
        else
            DIRECTX.SetViewport((float)Layer0->ld.Viewport[eye].Pos.x + (eye ? -offy : offy), (float)Layer0->ld.Viewport[eye].Pos.y,
            (float)Layer0->ld.Viewport[eye].Size.w, (float)Layer0->ld.Viewport[eye].Size.h);

        // Now we draw the portals with our world camera
        viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer0->ld.RenderPose[eye]);

        // Ie draw special 3D objects (TV screens) that bring parts forward.  Basically, just transparent objects.
        for (int tv = 0; tv < NUM_TVS; tv++)
        {
            if (DIRECTX.Key['7']) continue;
            if (STATIC_IS_GREEN) TVmodel[tv][DIRECTX.Key['4'] || forceFullSize ? 1 : 0]->Render(&(viewMatrix*projMatrix), 1, 1, 1, 1, true);
            else                 TVmodel[tv][DIRECTX.Key['4'] || forceFullSize ? 1 : 0]->Render(&(viewMatrix*projMatrix), 0.5f, 1, 0.5f, 1, true);
        }
        loadsOfCubes->Render(&(viewMatrix*projMatrix), 1, 1, 1, 1, true);

        // We need to render the corridor first, that's the game world
        if (STATIC_IS_GREEN) corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        else                 corridorModel->Render(&(viewMatrix * projMatrix), 0.5f, 1, 0.5f, 1, true);

        // Lets also render a skybox to avoid seeing the static world through the wholes in the model  
        skyModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);

        Layer0->FinishRendering(session, eye);
    }
    
    // Submit all the layers to the compositor
    return(1);
}

