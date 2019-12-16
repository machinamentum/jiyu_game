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

namespace TESTTV
{
    /// Variables / Demo controls
    /// =========================
    /// '1' recentres
    /// '2' just centre one
    /// '3' hold to have full 3D of nausea layer
    /// '4' hold to expand TVs
    /// '5' hold to turn on transparent visible static layer
    /// '6' hold to static more green than world
    /// '7' hold to suspend TVs (although fake distance still active without '3')
    /// '9' hold 9 for lurch
    /// 'FG' will vary artificial 2D background advancement.


    DWORD col = 0xffffffff;
    float HALF_HEIGHT[2] = { 0.55f, 0.65f };
    float HALF_WIDTH[2] = { 0.55f, 0.65f };
    static bool INGAME_IS_FLAT = true;
    #define NUM_TVS_HORIZ 10
    #define NUM_TVS_VERTI 3
    #define NUM_TVS ((NUM_TVS_HORIZ * NUM_TVS_VERTI)+2)  // The extra two are floor and ceiling
    #define DO_TVS_HAVE_EDGES true
    #define SPEED_OF_ROTATION 0 // 0.0005f
    float holodeckInstead;
    bool STATIC_IS_GREEN;
    #define SHOW_CROSSHAIRS false
    #define TRANSPARENCY_OF_STATIC_VISIBLE 0.1f
    bool forceJustCentreOne;
    bool forceLarge;
    bool force3D;
    bool forceColourChange;

    // Local
    Model * TVmodel[NUM_TVS][2];
    Model * whichRoom;
    bool firstTime = true;
}
using namespace TESTTV;

//-------------------------------
int(*TESTTV_Init(bool argHolodeckInstead, bool argForceJustCentreOne, bool argForceLarge, bool argForce3D, bool argForceColourChange))(void)
{
    holodeckInstead = argHolodeckInstead;
    forceJustCentreOne = argForceJustCentreOne;
    forceLarge = argForceLarge;
    force3D = argForce3D;
    forceColourChange = argForceColourChange;

    Layer0 = standardLayer0;

    if (holodeckInstead) whichRoom = holoModel;
    else                  whichRoom = roomModelNoFurniture;

    *mainCam = Camera(&Vector3(0.0f, 2.0f, -105.0f), &Quat4());
    *localCam = Camera(&Vector3(0.0f, 2.0f, 5.0f), &Quat4());

    if (firstTime)
    {
        firstTime = false;
        // Lets make a TV object.
        Texture * wallTex = new Texture(false, 512, 512, DO_TVS_HAVE_EDGES ? Texture::AUTO_GRID_GREENY : Texture::AUTO_GREENY, 1, 8);
		char ordinaryPShader[] =
			"Texture2D Texture   : register(t0); SamplerState Linear : register(s0); "
			"float4 main(in float4 Position : SV_Position, in float4 mainCol: COLOR0, in float2 TexCoord : TEXCOORD0) : SV_Target"
			"{                                                       "
			"    float4 texCol = Texture.Sample(Linear, TexCoord);                        "
			"    float4 c = texCol*mainCol;												  "
			"    return (c);                                                              "
			"};                                                      ";

        Shader * obShader = new Shader("ShaderOrdinary", 0, 3, 0, ordinaryPShader);
        Material * overBrightWallMat = new Material(wallTex, obShader, Material::MAT_WRAP | Material::MAT_TRANS | Material::MAT_NOCULL);

        TriangleSet t, double_t;
        for (int k = 0; k < 2; k++)
        {
            t.Empty();
            t.AddQuad(Vertex(Vector3(-HALF_WIDTH[k], HALF_HEIGHT[k], 0), col, 0, 0),
                Vertex(Vector3(HALF_WIDTH[k], HALF_HEIGHT[k], 0), col, 1, 0),
                Vertex(Vector3(-HALF_WIDTH[k], -HALF_HEIGHT[k], 0), col, 0, 1),
                Vertex(Vector3(HALF_WIDTH[k], -HALF_HEIGHT[k], 0), col, 1, 1));
            double_t.Empty();
            double_t.AddQuad(Vertex(Vector3(-2 * HALF_WIDTH[k], 2 * HALF_HEIGHT[k], 0), col, 0, 0),
                Vertex(Vector3(2 * HALF_WIDTH[k], 2 * HALF_HEIGHT[k], 0), col, 1, 0),
                Vertex(Vector3(-2 * HALF_WIDTH[k], -2 * HALF_HEIGHT[k], 0), col, 0, 1),
                Vertex(Vector3(2 * HALF_WIDTH[k], -2 * HALF_HEIGHT[k], 0), col, 1, 1));

            Quat4 rotToSend;
            int curr = 0;
            for (int hh = 0; hh < NUM_TVS_HORIZ; hh++)
            for (int vv = 0; vv < NUM_TVS_VERTI; vv++)
            {
                TVmodel[curr][k] = new Model(&t, Vector3(0, 0, 0), rotToSend, overBrightWallMat);
                curr++;
            }

            // Floor
            TVmodel[curr][k] = new Model(&double_t, Vector3(0, 0, 0), rotToSend, overBrightWallMat);
            curr++;
            // Ceiling
            TVmodel[curr][k] = new Model(&double_t, Vector3(0, 0, 0), rotToSend, overBrightWallMat);
            curr++;
        }

        // Useful to see crosshairs
        if (SHOW_CROSSHAIRS)    ovr_SetInt(session, OVR_DEBUG_HUD_STEREO_MODE, int(ovrDebugHudStereo_CrosshairAtInfinity));
        else                    ovr_SetInt(session, OVR_DEBUG_HUD_STEREO_MODE, int(ovrDebugHudStereo_Off));
    }

    return(TESTTV_Process_And_Render);
}

//----------------------------------
int TESTTV_Process_And_Render()
{
    APPCAMERA_SmoothedMove(mainCam, 0.1f, 0.02f);

    if (DIRECTX.Key['9'])
        APPCAMERA_ApplyLurchAfterCameraMove(mainCam, 0.1f, 0.1f);
    if ((DIRECTX.Key['3']) || (force3D)) INGAME_IS_FLAT = false;
    else                  INGAME_IS_FLAT = true;

    if ((DIRECTX.Key['6']) || (forceColourChange)) STATIC_IS_GREEN = true;
    else                  STATIC_IS_GREEN = false;
    
    // Get the current pose, for standard eye offset, for general use.
    double sensorSampleTime = 0;

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
        float range = 2;
        float centreZ = 5;
        float centreY = 2.0f;

        for (int hh = 0; hh < NUM_TVS_HORIZ; hh++)
        for (int vv = 0; vv < NUM_TVS_VERTI; vv++)
        {
            if (vv == 1) range = 2;
            if (vv == 0) range = 2.0f;
            float angleDiff = (360 / Maths::RTOD()) / NUM_TVS_HORIZ;
            float angleRound = (hh * angleDiff) + ((vv == 0) ? SPEED_OF_ROTATION : -SPEED_OF_ROTATION)*frameIndex;
            float angleUp = (vv - 1) * angleDiff;

            float horizDist = range*cos(angleUp);
            float vertiDist = range*sin(angleUp);

            TVmodel[curr][k]->Pos = Vector3(horizDist*sin(angleRound), vertiDist + centreY, horizDist*cos(angleRound) + centreZ);
            TVmodel[curr][k]->Rot = Quat4(-angleUp, angleRound, 0);
            curr++;
        }
        // Floor
        TVmodel[curr][k]->Pos = Vector3(0, (-0.999f*range) + centreY, centreZ);
        TVmodel[curr][k]->Rot = Quat4(90 / Maths::RTOD(), 0, 0);
        curr++;
        // Ceiling
        TVmodel[curr][k]->Pos = Vector3(0, (+0.999f*range) + centreY, centreZ);
        TVmodel[curr][k]->Rot = Quat4(90 / Maths::RTOD(), 0, 0);
        curr++;
    }

    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        // Layer0
        Layer0->PrepareToRender(session, eye, currPoseFlat[eye], sensorSampleTime, 0, 0, 0, 1, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.2f, 1000.0f, ovrProjection_None));
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer0->ld.RenderPose[eye]);

        // Lets move the viewport a little
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

        // We need to render the corridor first, that's the game world
        corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);

        // Decide if actually want to suspend static background
        if (!DIRECTX.Key['7'])
        {
            // Then clear the z buffer.
            DIRECTX.Context->ClearDepthStencilView(Layer0->pEyeDepthBuffer[eye]->TexDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);

            // Set camera to local
            viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(localCam, Layer0->ld.RenderPose[eye]);

            // Use normal pose
            Layer0->PrepareToRender(session, eye, currPoseNormal[eye], sensorSampleTime, 0, 0, 0, 1, false);
            viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(localCam, currPoseNormal[eye]);

            // Then draw special 3D objects (TV screens) that bring parts forward.  Basically, just transparent objects.
            for (int tv = 0; tv < NUM_TVS; tv++)
            {
                if (((DIRECTX.Key['2'] || forceJustCentreOne) && ((tv - 0) != (NUM_TVS / 2)))) continue;
                TVmodel[tv][(DIRECTX.Key['4'] || forceLarge) ? 1 : 0]->Render(&(viewMatrix*projMatrix), 1, 1, 1, 1, true);
            }
            // Then render room
            if (STATIC_IS_GREEN) whichRoom->Render(&(viewMatrix * projMatrix), 0, 1, 0, 1.0f, true);
            else                 whichRoom->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1.0f, true); /// Can change this alpha, but muddies waters of static layer

            // This is an optional seeing the faded background on the holding of the 5 button
            if (DIRECTX.Key['5'])
            {
                // OK, lets zero z buffer again, and rerender room at faded amount.
                DIRECTX.Context->ClearDepthStencilView(Layer0->pEyeDepthBuffer[eye]->TexDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
                whichRoom->Render(&(viewMatrix * projMatrix), 1, 1, 1, TRANSPARENCY_OF_STATIC_VISIBLE, true);
            }
        }

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);

        Layer0->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(1);
}

