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

// Note : Please disregard this flag, and any code under it - it is to temporarily maintain code internally 
// #define BUILD_FOR_OLD_SDK

#ifdef BUILD_FOR_OLD_SDK // USES LOCAL SDK, BUT CURRENT LIBEXTRA AND CODE. 
#include "C:\\Users\\tomheath\\Desktop\\Depot\\depot\\User\\TomH\\LOCOMOTION\\OculusSDK\\LibOVR\\Include\\OVR_CAPI_D3D.h"  // Oculus SDK - note libExtra is independent of it.      
#include "C:\\Users\\tomheath\\Desktop\\Depot\\depot\\Software\\OculusSDK\\PC\\Main\\Samples\\OculusRoomTiny_Advanced\\Common\\Win32_LibExtra\\Header\\libextra.h"  // Oculus SDK - note libExtra is independent of it.      
#else
#include "../../../LibOVR/Include/OVR_CAPI_D3D.h"  // Oculus SDK - note libExtra is independent of it.      
#include "../common/Win32_Libextra/Header/libextra.h"   // Had to add extra to find it
#endif

//----------------------------------------------------------
void APPCAMERA_DefaultMove(Camera * cam, float linearScale, float rotateScale);
void APPCAMERA_SmoothedMove(Camera * cam, float linearScale, float rotateScale);
void APPCAMERA_SmoothedMoveAndCounterMove(Camera * cam, Camera * counterCam, bool doWeSync);
void APPCAMERA_ApplyLurchAfterCameraMove(Camera * cam, float scalePitch, float scaleRoll, float frequency = 0.05f);
void APPCAMERA_VerySmoothedMoveForTilt(Camera * cam, float linearScale, float rotateScale);

//-----------------------------------------------
void INFOTEXT_Init();
void INFOTEXT_StartAddingText();
TextModel * INFOTEXT_GetModelToAdd();
void INFOTEXT_RenderWithOwnCameraAndReset(Matrix44 projMatrix, ovrPosef pose);

//-----------------------------------------------
void GAME_Init();
void GAME_Reset();
void GAME_Process(Vector3 bulletOrigin, Quat4 bulletDir);
void GAME_Render(Matrix44 * projView);

//-------------------------------------------------------
Model * SCENE_CreateOriginalORTModel(bool includeFurniture);
Model * SCENE_CreateCorridorModel(bool rebuild);
Model * SCENE_CreateSkyBoxModel();
Model * SCENE_CreateHolodeckModel();
Model * SCENE_CreateCockpitModel(bool extendedLength);
Model * SCENE_CreateTouchModel(float poleLength);

//----------------------------------------------
Matrix44    VRCONVERT_Matrix44(ovrMatrix4f ovrm);
Quat4       VRCONVERT_Quat4(ovrQuatf v);
ovrQuatf    VRCONVERT_ovrQuatf(Quat4 v);
Vector3     VRCONVERT_Vector3(ovrVector3f v);
ovrVector3f VRCONVERT_ovrVector3f(Vector3 v);
Matrix44    VRCONVERT_GetViewMatrixWithEyePose(Camera * cam, ovrPosef pose);

//------------------------------------
int(*TESTBASE2_Init())(void);
int(*TESTTV_Init(bool argHolodeckInstead, bool argForceJustCentreOne, bool argForceLarge, bool argForce3D, bool forceColourChange))(void);
int(*TESTPORTAL_Init(bool forceFull3D, bool argForceFullSize, bool argForceGreenReverse))(void);
int(*TESTEDGE_Init(bool argEqualiseHorizFovs, bool argNoTrans))(void);
int(*TESTCOUNTER_Init(int argFrameCountsForAutoSync, bool argUseHologrid, bool argPermanentEffect))(void);
int(*TESTCOUNTER2_Init(bool argPermanentEffect, int argOption, float argDegree))(void);
int(*TESTDESENS_Init(float argFrequency))(void);
int(*TESTUNREAL_Init(bool argExtendedCockpit, bool argForceEffectOff))(void);
int(*TESTUNREAL2_Init(bool argExtendedCockpit, int argFramesToSkip, bool argSmoothMove, int argFramesToSkipJustCamera))(void);
int(*TESTINVISIBLE_Init(bool argSmoothMove))(void);
int(*TESTPOLE_Init(bool argLastTouchHasMomentum, bool argAllowTouchTwistRotate, float argDirectionOfTwistRotate))(void);
int(*TESTTILT_Init(bool argLinearApproach, bool argExtraSmoothCamera))(void);

int TESTBASE2_Process_And_Render();
int TESTTV_Process_And_Render();
int TESTPORTAL_Process_And_Render();
int TESTEDGE_Process_And_Render();
int TESTCOUNTER_Process_And_Render();
int TESTCOUNTER2_Process_And_Render();
int TESTDESENS_Process_And_Render();
int TESTUNREAL_Process_And_Render();
int TESTUNREAL2_Process_And_Render();
int TESTINVISIBLE_Process_And_Render();
int TESTPOLE_Process_And_Render();
int TESTTILT_Process_And_Render();

//------------------------------------------------------------
struct VRTexture
{
    ovrSession               hmd;
    ovrTextureSwapChain      TextureSet;
    std::vector<ID3D11RenderTargetView*> TexRtvN;
    int                      SizeW, SizeH;
    VRTexture();
    bool Init(ovrSession _hmd, int sizeW, int sizeH);
    ~VRTexture();
    ID3D11RenderTargetView* GetRTV();
    void Commit();
};

//------------------------------------------------------------
struct VRLayer
{
    ID3D11RenderTargetView*  TexRtv[2][10];
    DepthBuffer    * pEyeDepthBuffer[2];
    ovrLayerEyeFov ld; // Its OK to use this type as 'common' for now, as long as we only use Layer types which follow the same pattern and fields.

    // MSAA, just in case
    bool useMSAA;
    Texture     * MSAATexture[2];
    DepthBuffer * MSAADepthBuffer[2];

    VRLayer(ovrSession session, ovrFovPort leftFOV, ovrFovPort rightFOV, float pixelsPerDisplayPixel, bool argUseMSAA); // EyeFov
    VRLayer(ovrSession session, ovrSizei resolution);   // Direct
    void PrepareToRender(ovrSession session, int eye, ovrPosef poseWeUsed, double sensorSampleTime, float R, float G, float B, float A, bool doWeClear);
    void ClearDepth(ovrSession session, int eye);
    void FinishRendering(ovrSession session, int eye);
    void SetUseMSAA(bool isItEnabled);
    void SetShuntedViewport(int eye, int xShunt, int yShunt);

private:
    void CommonInit(ovrSession session, int eye, ovrSizei idealSize, bool argUseMSAA);
};

//---------------------------------------------------------
void VRMIRROR_Init(ovrSession session, VRLayer * layerForUndistortedLeft);
void VRMIRROR_Render(ovrSession session, int distortedboth0_undistortedleft1_independent2, void(*independentRenderFunc)(void));

//-----Globals-from-main-that-every-test-uses----------------
extern DirectX11 DIRECTX;
extern Files File;
extern Control CONTROL;
extern ovrSession session;
extern ovrHmdDesc hmdDesc;
extern long frameIndex;

extern Camera * mainCam;
extern Camera * localCam;
extern Camera * counterCam;
extern Model * roomModel;
extern Model * roomModelNoFurniture;
extern Model * corridorModel;
extern Model * skyModel;
extern Model * holoModel;
extern VRLayer * Layer0;
extern VRLayer * Layer1;
extern VRLayer * standardLayer0;
extern VRLayer * standardLayer1;



