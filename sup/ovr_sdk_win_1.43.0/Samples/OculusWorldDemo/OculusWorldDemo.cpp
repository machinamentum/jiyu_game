/************************************************************************************

Filename    :   OculusWorldDemo.cpp
Content     :   First-person view test application for Oculus Rift - Implementation
Created     :   October 4, 2012
Authors     :   Michael Antonov, Andrew Reisse, Steve LaValle, Dov Katz
                Peter Hoff, Dan Goodman, Bryan Croteau

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

#include "OculusWorldDemo.h"
#include "Kernel/OVR_Threads.h"
#include "Kernel/OVR_Hash.h"
#include "Util/Util_SystemGUI.h"
#include <algorithm>
#include <string>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds
#include "Kernel/OVR_Log.h"


// Use WaitFrame/BeginFrame/EndFrame in place of SubmitFrame
#define USE_WAITFRAME 1
// Keeps submitting frame index 0, forcing the SDK manage the frame index (only valid when using ovr_SubmitFrame)
#define USE_FRAME_INDEX_ZERO  (!USE_WAITFRAME && 1)

OVR_DISABLE_MSVC_WARNING(4996) // "scanf may be unsafe"
OVR_DISABLE_MSVC_WARNING(4351) // elements of array will be default initialize







  #define DefaultWindowWidth 1600
  #define DefaultWindowHeight 900


bool IsMixedRealityCaptureMode = false;
bool IsMRCDemoMode = false;

Posef CockpitPanelPose[5];
Vector2f CockpitPanelSize[5];
Recti CockpitClipRect[5];
int32_t HapticsMode = 0;
const char* HapticsModeDesc[] = {
    "[CONST] Off",
    "[CONST] Low-Latency:  per-touch (mid trigger) amplitude",
    "[BUFFR] Low-Latency:  per-touch (mid trigger) amplitude",
    "[BUFFR] High-Latency: per-touch (mid trigger) amplitude",
    "[BUFFR] Low-Latency:  LR-touch (v++)%256 wave (ButtonA)",
    "[BUFFR] Low-Latency:  LR-touch 1 sample   (ButtonA)",
    "[BUFFR] Low-Latency:  LR-touch 4 samples  (ButtonA)",
    "[BUFFR] Low-Latency:  LR-touch 20 samples (ButtonA)",
    "[BUFFR] High-Latency: LR-touch 2 samples [0, 800ms] (ButtonA)",
    "[BUFFR] Low-Latency:  LR-touch 4 interleaved samples (ButtonA)",
    "[BUFFR] Low-Latency:  Audio WAV playback (ButtonA)",
    "XBox controller rumble - Hold mid-trigger to adjust amp/freq",
};
const int32_t kMaxHapticsModes = _countof(HapticsModeDesc);
int32_t LowLatencyBufferSizeInSamples[2] = { 10, 10 }; // original value used for Rift


const bool OculusWorldDemoApp::AllowMsaaTargets[OculusWorldDemoApp::Rendertarget_LAST] =
{
    true,  //Rendertarget_Left,
    true,  //Rendertarget_Right,
    true,  //Rendertarget_BothEyes,
    true,  //Rendertarget_Left1,
    true,  //Rendertarget_Right1,
    true,  //Rendertarget_Left2,
    true,  //Rendertarget_Right2,
    true,  //Rendertarget_Left3,
    true,  //Rendertarget_Right3,
    false, //Rendertarget_Hud,
    false, //Rendertarget_Menu
};

const bool OculusWorldDemoApp::UseDepth[OculusWorldDemoApp::Rendertarget_LAST] =
{
    true,  //Rendertarget_Left,
    true,  //Rendertarget_Right,
    true,  //Rendertarget_BothEyes,
    true,  //Rendertarget_Left1,
    true,  //Rendertarget_Right1,
    true,  //Rendertarget_Left2,
    true,  //Rendertarget_Right2,
    true,  //Rendertarget_Left3,
    true,  //Rendertarget_Right3,
    false, //Rendertarget_Hud,
    false, //Rendertarget_Menu
};



//-------------------------------------------------------------------------------------
// ***** OculusWorldDemoApp

OculusWorldDemoApp::OculusWorldDemoApp() :
    OVR_ExceptionHandler(),
    OVR_GUIExceptionListener(),
    OwdScript(this),

    Argc(0),
    Argv(nullptr),

    pRender(0),
    RenderParams(),
    WindowSize(DefaultWindowWidth,DefaultWindowHeight),
    HmdDisplayAcquired(false),
    FirstScreenInCycle(0),
    SupportsSrgbSwitching(true),        // May be proven false below.
    SupportsMultisampling(true),        // May be proven false below.
    SupportsDepthSubmit(true),         // May be proven false below.
    ActiveGammaCurve(1.0f),
    SrgbGammaCurve(2.2f),
    MirrorIsSrgb(false),
    EyeTextureFormat(EyeTextureFormat_RGBA8_SRGB),
    TrackingOriginType(ovrTrackingOrigin_FloorLevel),
    MenuUserEyeHeight(1.6f),    // initialized from profile later

    //RenderTargets()
    //DrawEyeTargets(),
    Session(0),
    HmdDesc(),
    //EyeRenderDesc[2];
    //Projection[2];          // Projection matrix for eye.
    //OrthoProjection[2];     // Projection for 2D.
    //EyeRenderPose[2];       // Poses we used for rendering.
    //EyeTexture[2];
    //EyeRenderSize[2];       // Saved render eye sizes; base for dynamic sizing.
    UsingDebugHmd(false),

    SecondsOfFpsMeasurement(1.0f),
    FrameCounter(0),
    TotalFrameCounter(0),
    SecondsPerFrame(0.f),
    FPS(0.f),
    LastFpsUpdate(0.0),
    LastUpdate(0.0),

    TouchHapticsDesc{},
    TouchHapticsPlayIndex(0),

    MainFilePath(),
    CollisionModels(),
    GroundCollisionModels(),

    LoadingState(LoadingState_Frame0),

    InteractiveMode(true),
    HaveHMDVisionTracking(false),
    HaveBothControllersVisionTracking(false),
    HavePositionTracker(false),
    HaveHMDConnected(false),
    HaveSync(true),

    Recording(false),
    Replaying(false),
    LastSyncTime(0.0),

    LastControllerState(),
    LastControllerType(ovrControllerType_None),

    ThePlayer(),
    CurrentCameraID(-1),

    MainScene(),
    SmallGreenCube(),
    SmallOculusCube(),
    SmallOculusGreenCube(),
    SmallOculusRedCube(),

    OculusCubesScene(),
    GreenCubesScene(),
    RedCubesScene(),
    YellowCubesScene(),

    TextureOculusCube(),

    BoundaryScene(),

    CockpitPanelTexture(),

    LoadingTexture(),

    CubemapRenderTexture(),
    CubemapLoadTexture(),

    HmdFrameTiming(),
    HmdStatus(0),
    EnableTrackingStateLog(false),

    NotificationTimeout(0.0),

    HmdSettingsChanged(false),

    RendertargetIsSharedByBothEyes(false),
    FovStencilType(-1),  // -1 means disable
    FovStencilMeshes(),
    FovStencilLinesX(),
    FovStencilLinesY(),
    FovStencilLineCount(0),
    FovStencilColor(FovStencilColor_Black),
    ResolutionScalingMode(ResolutionScalingMode_Off),
    AlternatingEyeResScale(0.8f),


    ShutterType("Unknown"),

    HasInputState(false),
    ConnectedControllerTypes(0),
    InterAxialDistance(0.0f),
    MonoscopicRenderMode(Mono_Off),
    PositionTrackingScale(1.0f),
    ScaleAffectsEyeHeight(false),
    DesiredPixelDensity(1.0f),
    AdaptivePixelDensity(1.0f),
    FovScaling(0.0f),
    FovPortOverrideEnable(false),
    FovPortOverrideResetToDefault(true),  // reset the first time app starts up
    FovPortOverride(),
    NearClip(0.01f),
    FarClip(10000.0f),
    DepthModifier(NearLessThanFar),
    DepthFormatOption(DepthFormatOption_D32f),
    SceneRenderCountType(SceneRenderCount_FixedLow),
    SceneRenderCountLow(1),
    SceneRenderCountHigh(10),
    SceneRenderWasteCpuTimePreTrackingQueryMs(0),
    SceneRenderWasteCpuTimePreRenderMs(0),
    SceneRenderWasteCpuTimeEachRenderMs(0),
    SceneRenderWasteCpuTimePreSubmitMs(0),
    DrawFlushMode(DrawFlush_Off),
    DrawFlushCount(10),
    MenuHudMovementMode(MenuHudMove_FixedToFace),
    MenuHudMovementRadius(36.0f),
    MenuHudMovementDistance(0.35f),
    MenuHudMovementRotationSpeed(400.0f),
    MenuHudMovementTranslationSpeed(4.0f),
    MenuHudTextPixelHeight(22.0f),
    MenuHudDistance(0.8f),
    MenuHudSize(1.25f),     // default = 1.0f / MenuHudDistance
    MenuHudMaxPitchToOrientToHeadRoll(45.0f),
    MenuHudAlwaysOnMirrorWindow(false),

    TimewarpRenderIntervalEnabled(true),
    TimewarpRenderIntervalInSeconds(0.0f),
    NeverRenderedIntoEyeTextures(false),
    EnableTimewarpOnMainLayer(true),
    PredictionEnabled(true),
    FreezeEyeUpdate(false),

    DiscreteHmdRotationEnable(false),
    DiscreteHmdRotationNeedsInit(true),
    DiscreteEyePoses{},
    DiscreteRotationThresholdAngle(5.0f),
    DiscreteRotationFovMult(1.0f),
    DiscreteRotationScissorTech(DRScissorTechnique_ProjectCorners),
    DiscreteRotationSubFovPort(),
    DiscreteRotationScissor{ Recti(0, 0, 0, 0), Recti(0, 0, 0, 0) },
    DiscreteRotationScissorMult(1.0f),
    DiscreteRotationScissorMenuViewSavings{},
    DiscreteRotationScissorMenuViewPerfHit{},
    DiscreteRotationScissorIdealRenderSize{ Sizei(0, 0), Sizei(0, 0) },

    SubpixelJitterEnable(false),
    SubpixelJitterSampleCount(3),
    SubpixelJitterSamplePolarOffset(1.0f),
    SubpixelJitterSamplePolarDistMult(1.0f),
    SubpixelJitterBaseCamFovPort{},
    SubpixelJitterCurrentIndex(0),


#ifdef OVR_OS_MS
    LayersEnabled(true),             // Use layers at all
#else
    LayersEnabled(false),
#endif
    Layer0HighQuality(true),
    Layer0GenMipCount(0), // let's SDK generate all mips - good test
    Layer0Depth(true),
    Layer1Enabled(false),
    Layer1HighQuality(true),
    Layer2Enabled(false),
    Layer2HighQuality(true),
    Layer3Enabled(false),
    Layer3HighQuality(true),
    Layer4Enabled(false),
    Layer4HighQuality(true),
    LayerHdcpEnabled(false),
    Layer234Size(1.0f),
    LayerDirectEnabled(false),
    LayerCockpitEnabled(0),
    LayerCockpitHighQuality(true),
    LayerHudMenuEnabled(true),
    LayerHudMenuHighQuality(true),
    LayerLoadingHighQuality(true),
    LayerCubemap(Cubemap_Off),
    LayerCubemapSaveOnRender(false),
    LayerCubemapHighQuality(true),
    LayerCylinderEnabled(false),
    LayerCylinderHighQuality(true),
    LayerCylinderRadius(1.0f),
    LayerCylinderAspectRatio(1.0f),
    LayerCylinderAngle(3.14159f),
    LayerCylinderHeadlocked(false),

    HdcpTexture(),
    CenterPupilDepthMeters(0.05f),
    ForceZeroHeadMovement(false),
    MultisampleRequested(true),
    MultisampleEnabled(true),
    SrgbRequested(true),
    AnisotropicSample(true),
    SceneBrightnessGrayscale(true),
    SceneBrightness(255.0f),
    SceneBlack(false),
    TextureOriginAtBottomLeft(false),
#if defined(OVR_OS_LINUX)
    LinuxFullscreenOnDevice(false),
#endif
    PositionTrackingEnabled(true),
    MirrorToWindow(true),
    MirrorNativeResScale(1.0f),

    DistortionClearBlue(0),

    ShiftDown(false),
    CtrlDown(false),
    IsLogging(false),

    PerfHudMode(ovrPerfHud_Off),
    DebugHudStereoPresetMode(DebugHudStereoPreset_Free),
    DebugHudStereoMode(ovrDebugHudStereo_Off),
    DebugHudStereoGuideInfoEnable(true),
    DebugHudStereoGuidePosition(0.0f, 0.0f, -1.5f),
    DebugHudStereoGuideSize(1.0f, 1.0f),
    DebugHudStereoGuideYawPitchRollDeg(0.0f, 0.0f, 0.0f),
    DebugHudStereoGuideYawPitchRollRad(0.0f, 0.0f, 0.0f),
    DebugHudStereoGuideColor(1.0f, 0.5f, 0.1f, 0.8f),

    SceneMode(Scene_World),
    GridDisplayMode(GridDisplay_None),
    GridMode(Grid_Lens),
    GridColor(GridColor_Default),
    TextScreen(Text_None),
    ComfortTurnMode(ComfortTurn_Off),
    SceneAnimationEnabled(true),
    SceneAnimationTime(0.0),
    BlocksShowType(0),
    BlocksShowMeshType(0),
    BlocksSpeed(1.0f),
    BlocksCenter(0.0f, 0.0f, 0.0f),
    BlockScale(20.0f, 20.0f, 20.0f),
    BlocksHowMany(10),
    BlocksMovementRadius(1.0f),
    BlocksMovementType(0),
    BlocksMovementScale(0.5f),
    BlockModelSize(0.004f),

    // Sit/stand height adjustment and floor rendering: Key_O
    VisualizeSeatLevel(false),
    VisualizeTracker(false),
    Sitting(false),
    SittingAutoSwitch(false),
    ExtraSittingAltitude(0.4f),
    TransitionToSitTime(0.0), TransitionToStandTime(0.0),
    DonutFloorVisual(false),
    ShowCalibratedOrigin(false),
    CalibratedTrackingOrigin(),
    ProvidedTrackingOriginTranslationYaw(),

    CurrentMirrorOptionStage(MirrorOptionStage_Rectilinear),
    CurrentMirrorOptionEyes(MirrorOptionEyes_Both),
    CurrentMirrorOptionCaptureGuardian(true),
    CurrentMirrorOptionCaptureNotifications(true),
    CurrentMirrorOptionCaptureSystemGui(false),
    CurrentMirrorOptionForceSymmetricFov(false),
    CurrentMirrorOptionsFormat(0),
    MirrorOptionsChanged(false),

    Menu(),
    ShortcutChangeMessageEnable(true),
    MenuPopulated(false),

    Profiler(),
    IsVisionLogging(false),
    SensorSampleTimestamp(0.0),
    EyeLayer(),
    Layer1(),
    Layer2(),
    Layer3(),
    Layer4(),
    LayerHdcp(),
    CockpitLayer(),
    HudLayer(),
    MenuLayer(),
    MenuPose(),
    MenuIsRotating(false),
    MenuIsTranslating(false)
{
    OVR_ExceptionHandler.SetExceptionListener(&OVR_GUIExceptionListener, 0); // Use GUI popup?
    OVR_ExceptionHandler.SetPathsFromNames("Oculus", "OculusWorldDemo"); // File dump?
    OVR_ExceptionHandler.EnableReportPrivacy(true); // Dump less info?
    OVR_ExceptionHandler.Enable(true); // Enable handler


    for ( int i = 0; i < CamRenderPose_Count; i++ )
    {
        CamRenderSize[i] = Sizei(0);
        // TODO - init the others of size CamRenderPose_Count.
    }

    EyeFromWorld[0].SetIdentity();
    EyeFromWorld[1].SetIdentity();

      
CurrentMirrorOptionsFormat = ConvertMirrorOptionsToMirrorFormatFlags();



    // Touch Haptics
    memset(&TouchHapticsClip, 0, sizeof(TouchHapticsClip));

    for ( int i = 0; i < Rendertarget_LAST; i++ )
    {
        DrawEyeTargets[i] = nullptr;
    }

    // EyeRenderDesc[], EyeTexture[] : Initialized in CalculateHmdValues()

    memset(LayerList, 0, sizeof(LayerList));
    for (int i = 0; i < OVR_ARRAY_COUNT(GenMipCount); i++)
    {
        GenMipCount[i] = -1;    // will make sure it inits the first time the RT is used
    }

    // Set up the "cockpit"
    Vector3f CockpitCenter = Vector3f ( 7.75f, 1.6f, -1.75f );
    // Lower middle
    CockpitPanelPose[0] = Posef ( Quatf ( Axis_X, -0.5f ), Vector3f (  0.0f, -0.4f, 0.0f ) + CockpitCenter );
    CockpitPanelSize[0] = Vector2f ( 0.5f, 0.3f );
    CockpitClipRect [0] = Recti ( 0, 373, 387, 512-373 );
    // Left side
    CockpitPanelPose[1] = Posef ( Quatf ( Axis_Y,  0.7f ), Vector3f ( -0.6f, -0.4f, 0.2f ) + CockpitCenter );
    CockpitPanelSize[1] = Vector2f ( 0.5f, 0.3f );
    CockpitClipRect [1] = Recti ( 272, 0, 512-272, 103 );
    CockpitPanelPose[2] = Posef ( Quatf ( Axis_Y,  0.5f ), Vector3f ( -0.4f,  0.2f, 0.0f ) + CockpitCenter );
    CockpitPanelSize[2] = Vector2f ( 0.3f, 0.4f );
    CockpitClipRect [2] = Recti ( 0, 0, 132, 339 );
    // Right side
    CockpitPanelPose[3] = Posef ( Quatf ( Axis_Y, -0.7f ), Vector3f ( 0.6f, -0.4f, 0.2f ) + CockpitCenter );
    CockpitPanelSize[3] = Vector2f ( 0.5f, 0.3f );
    CockpitClipRect [3] = Recti ( 272, 0, 512-272, 103 );
    CockpitPanelPose[4] = Posef ( Quatf ( Axis_Y, -0.5f ), Vector3f ( 0.4f,  0.2f, 0.0f ) + CockpitCenter );
    CockpitPanelSize[4] = Vector2f ( 0.3f, 0.4f );
    CockpitClipRect [4] = Recti ( 132, 0, 258-132, 173 );


    HandStatus[0] = HandStatus[1] = 0;

    // Users can set the OWD_DEFAULT_SCENE_RENDER_MODE environment variable to
    // set the default scene mode.  The value of the environment variable must
    // be an unsigned integer corresponding to a SceneRenderMode value.
    char EnvDefaultSceneRenderMode[16];
    size_t EnvVarLen = 0;
    errno_t EnvErr = getenv_s(&EnvVarLen, EnvDefaultSceneRenderMode, OVR_ARRAY_COUNT(EnvDefaultSceneRenderMode),
                              "OWD_DEFAULT_SCENE_RENDER_MODE");
    if (!EnvErr && EnvDefaultSceneRenderMode[0])
    {
        unsigned long EnvRenderModeAsLong = OVR_strtoul(EnvDefaultSceneRenderMode, nullptr, 0);

        if (EnvRenderModeAsLong < (unsigned long)Scene_MAX)
        {
            WriteLog("[OculusWorldDemoApp] Setting SceneMode to %i based on environment variable OWD_DEFAULT_SCENE_RENDER_MODE",
                            EnvRenderModeAsLong);
            SceneMode = (SceneRenderMode)EnvRenderModeAsLong;
        }
        else
        {
            WriteLog("[OculusWorldDemoApp] Warning: Unable to set OWD_DEFAULT_SCENE_RENDER_MODE to %i. "
                            "Value must be between 0 and %i.\n",
                            EnvRenderModeAsLong, (unsigned long)Scene_MAX);
        }
    }

    const float cv1TexelsPerRadian = 802.20f;
    const float cv1RadiansPerTexel = 1.0f / cv1TexelsPerRadian;
    const float cylinderTexWidth   = 715.0f;
    const float cylinderTexHeight  = 715.0f;

    // 715x715 is the viewport width/height the target texture.
    LayerCylinderAngle = cylinderTexWidth * cv1RadiansPerTexel;
    LayerCylinderAspectRatio = cylinderTexWidth / cylinderTexHeight;

    ExternalCameraNameOpenBits.fill(false);
    OldExternalCameraNameOpenBits.fill(false);

}

OculusWorldDemoApp::~OculusWorldDemoApp()
{
  ovr_Shutdown();
}

void OculusWorldDemoApp::DestroyFovStencil()
{
  for (int curEye = 0; curEye < ovrEye_Count; curEye++)
  {
    for (int curMode = 0; curMode < FovStencilMeshMode_Count; curMode++)
    {
      FovStencilMeshes[curEye][curMode].Clear();
    }
    delete[] FovStencilLinesX[curEye];
    FovStencilLinesX[curEye] = nullptr;
    delete[] FovStencilLinesY[curEye];
    FovStencilLinesY[curEye] = nullptr;
    FovStencilLineCount = 0;
  }
}

void OculusWorldDemoApp::DestroyRendering()
{
    CleanupDrawTextFont();

    if (Session)
    {
        // Delete any render targets and the mirror window, which may be
        // using ovrSwapTextureSet or ovrTexture underneath and need to
        // call in the Session to be deleted (since we're about to destroy
        // the hmd).
        for (size_t i = 0; i < OVR_ARRAY_COUNT(RenderTargets); ++i)
        {
            RenderTargets[i].pColorTex.Clear();
            RenderTargets[i].pDepthTex.Clear();
        }

        for (size_t i = 0; i < OVR_ARRAY_COUNT(CamTexture); ++i)
        {
            CamTexture[i].Clear();
            CamDepthTexture[i].Clear();
        }

        MirrorTexture.Clear();


        // Need to explicitly clean these up because they can contain SwapTextureSets,
        // and we need to release those before we destroy the HMD connection.
        CockpitPanelTexture.Clear();
        HdcpTexture.Clear();
        TextureOculusCube.Clear();
        LoadingTexture.Clear();
        CubemapRenderTexture.Clear();
        CubemapLoadTexture.Clear();

        OculusRoundFloor[0].Clear();
        OculusRoundFloor[1].Clear();
        pRoundFloorModel[0].Clear();
        pRoundFloorModel[1].Clear();

        for (size_t i = 0; i < TrackingCamera_Count; ++i) {
          PositionalTrackers[i].Clear();
        }

        pPlatform->DestroyGraphics();
        pRender = nullptr;

        ovr_Destroy(Session);
        Session = nullptr;
    } // if Session
    CollisionModels.shrink_to_fit();
    GroundCollisionModels.shrink_to_fit();

    DestroyFovStencil();

    // Setting this will make sure OnIdle() doesn't continue rendering until we reacquire hmd
    HmdDisplayAcquired = false;
}

static void OVR_CDECL LogCallback(uintptr_t /*userData*/, int /*level*/, const char* message)
{
    WriteLog(message);
}

auto Dequote = [](std::string& str) -> void
{
  if (!str.empty() && (str[0] == '\"'))
    str.erase(str.begin());
  if (!str.empty() && (str.back() == '\"'))
    str.pop_back();
};

// Return 0 upon success, else non-zero.
int OculusWorldDemoApp::OnStartup(int argc, const char** argv)
{
    Argc = argc;
    Argv = argv;

    OVR::Thread::SetCurrentThreadName("OWDMain");



    // *** Oculus HMD Initialization

    // Example use of ovr_Initialize() to specify a log callback.
    // The log callback can be called from other threads until ovr_Shutdown() completes.
    ovrInitParams params = {
      0,
      0, nullptr, 0, 0, OVR_ON64("")};


    std::string replayFile;

    for (int i = 1; i < argc; i++)
    {
        const char* argStr = argv[i];

        // command line arguments get passed in with double quotes around them
        // when OWD is executed by other programs; let's get rid of them
        std::string str(argStr);
        Dequote(str);
        const char* argStrClean = str.c_str();

        while (*argStrClean == '-' || *argStrClean == '/')
        {
          ++argStrClean;
        }


        if (!OVR_stricmp(argStrClean, "scriptfile"))
        {
            if ( i <= argc - 1 ) // next arg is the script file path
            {
                bool result = OwdScript.LoadScriptFile(argv[ i + 1 ]);
                if (!result)
                    WriteLog("[OculusWorldDemoApp] Failed to load script %s.", argv[ i + 1 ]);
                ++i; // move past the file path
            }
        }

        if (!OVR_stricmp(argStrClean, "scriptoutputpath"))
        {
          if (i <= argc - 1) // next arg is the script output path
          {
            bool result = OwdScript.SetScriptOutputPath(argv[i + 1]);
            if (!result)
              WriteLog("[OculusWorldDemoApp] Failed to set script output path %s.", argv[i + 1]);
            ++i; // move past the file path
          }
        }

        if (!OVR_stricmp(argStrClean, "automation"))
        {
            InteractiveMode = false;
        }

        if (!OVR_stricmp(argStrClean, "disableexceptionhandler"))
        {
          WriteLog("[OculusWorldDemoApp] Disabling exception handler.");
          OVR_ExceptionHandler.Enable(false);
        }

        if (!OVR_stricmp(argStrClean, "replay"))
        {
            if ( i <= argc - 1 ) // next arg is the filename
            {
               replayFile = argv[ i + 1 ];
               ++i; // move past the file path
            }
        }

    }


    // Declare that we are aware of focus functionality. This means we need to keep drawing when we
    // lose focus, but trying to simulate and handle input, and hide any representation of hands.
    params.Flags |= ovrInit_FocusAware;

    params.LogCallback = LogCallback;
    ovrResult error = ovr_Initialize(&params);


    ovr_TraceMessage(ovrLogLevel_Info, "Oculus World Demo OnStartup");

    if (error != ovrSuccess)
    {
        DisplayLastErrorMessageBox("ovr_Initialize failure.");
        return 1;
    }

    if ( !replayFile.empty() )
    {
        ovr_SetString( nullptr, "server:replayInit", replayFile.c_str() );
    }

    int res = InitializeRendering(true);

    ovrControllerType touchController[] = { ovrControllerType_LTouch, ovrControllerType_RTouch };
    for (size_t t = 0; t < 2; ++t) {
      ovrTouchHapticsDesc& hapticsDesc = TouchHapticsDesc[t];
      hapticsDesc = ovr_GetTouchHapticsDesc(Session, touchController[t]);
      // For low latency, sample count should be (safe Haptics latency = TouchHapticsDesc.QueueMinSizeToAvoidStarvation) + (safe frame latency = 4 @90hz, 6 @60hz)
      // We'll use 5 here as a middle ground between 90hz and 60hz
      int32_t bufferSizeInSamples = hapticsDesc.QueueMinSizeToAvoidStarvation + 5;

      if (bufferSizeInSamples < hapticsDesc.SubmitMinSamples) {
        bufferSizeInSamples = hapticsDesc.SubmitMinSamples;
      }
      else if (bufferSizeInSamples > hapticsDesc.SubmitOptimalSamples) {
        bufferSizeInSamples = hapticsDesc.SubmitOptimalSamples;
      }
      LowLatencyBufferSizeInSamples[t] = bufferSizeInSamples;
    }

    return res;
}

void OculusWorldDemoApp::OnShutdown()
{
  ClearScene();
  DestroyRendering();
}

int OculusWorldDemoApp::InitializeRendering(bool firstTime)
{
    OVR_ASSERT(!HmdDisplayAcquired);

    ovrGraphicsLuid luid;
    ovrResult error = ovr_Create(&Session, &luid);

    if (error != ovrSuccess)
    {
        if (firstTime)
        {
            DisplayLastErrorMessageBox("Unable to create HMD.");
            return 1;
        }
        else
        {
            HmdDisplayAcquired = false;
            return 0;
        }
    }


    // Did we end up with a debug HMD?
    HmdDesc = ovr_GetHmdDesc(Session);
    UsingDebugHmd = (HmdDesc.DefaultHmdCaps & ovrHmdCap_DebugDevice) != 0;


    // In Direct App-rendered mode, we can use smaller window size,
    // as it can have its own contents and isn't tied to the buffer.
    WindowSize = Sizei(DefaultWindowWidth, DefaultWindowHeight);//Sizei(960, 540); avoid rotated output bug.

    // ***** Setup System Window & rendering.

    if (!SetupWindowAndRendering(firstTime, Argc, Argv, luid))
    {
        if (firstTime)
        {
            DisplayLastErrorMessageBox("Unable to initialize rendering");
            DestroyRendering();
            return 1;
        }
        else
        {
            HmdDisplayAcquired = false;
            GetPlatformCore()->SetNotificationOverlay(0, 28, 8, "Unable to initialize rendering");
            DestroyRendering();
            return 0;
        }
    }

    NotificationTimeout = ovr_GetTimeInSeconds() + 10.0f;

    PositionTrackingEnabled = (HmdDesc.DefaultTrackingCaps & ovrTrackingCap_Position) ? true : false;

    // *** Configure HMD Stereo settings.

    error = CalculateHmdValues();
    if (error != ovrSuccess)
    {
        if (firstTime)
        {
            DisplayLastErrorMessageBox("Unable to initialize HMD with values");
            DestroyRendering();
            return 1;
        }
        else
        {
            GetPlatformCore()->SetNotificationOverlay(0, 28, 8, "Unable to initialize HMD with values");

            // Destroy what was initialized
            HandleOvrError(error);

            DestroyRendering();

            return 0;
        }
    }

    // Query eye height - and set it (will be used layer to calc an offset if needed)
    ThePlayer.ProfileStandingEyeHeight = ovr_GetFloat(Session, OVR_KEY_EYE_HEIGHT, ThePlayer.UserStandingEyeHeight);
    EyeHeightChange(nullptr);   // updates ThePlayer's standing user eye height
    MenuUserEyeHeight = ThePlayer.ProfileStandingEyeHeight;

    ovr_SetTrackingOriginType(Session, TrackingOriginType);

    // Center pupil for customization; real game shouldn't need to adjust this.
    CenterPupilDepthMeters  = ovr_GetFloat(Session, "CenterPupilDepth", 0.0f);


    ThePlayer.bMotionRelativeToBody = false;  // Default to head-steering for DK1

    if (UsingDebugHmd)
        Menu.SetPopupMessage("NO HMD DETECTED");
    else
        Menu.SetPopupMessage("Please put on Rift");

    // Give first message 10 sec timeout, add border lines.
    Menu.SetPopupTimeout(10.0f, true);

    if (!MenuPopulated)
    {
        // Incase rendering is re-initialized, avoid repopulating the menu
        PopulateOptionMenu();
        MenuPopulated = true;
    }

    // *** Identify Scene File & Prepare for Loading

    InitMainFilePath();
    PopulatePreloadScene();

    // Load Haptics audio WAV file
    FILE* file = fopen("./song1.wav", "rb");
    if (file != nullptr)
    {
        // Read all WAV file data
        fseek(file, 0, SEEK_END);
        int32_t fileSize = ftell(file);
        fseek(file, 0, SEEK_SET);
        std::unique_ptr<uint8_t[]> wavData(new uint8_t[fileSize]);
        fread(wavData.get(), fileSize, 1, file);

        // Get audio channel data
        ovrAudioChannelData channelData;
        ovr_ReadWavFromBuffer(&channelData, wavData.get(), fileSize, 0);

        // Generate haptics data from audio channel
        ovr_GenHapticsFromAudioData(&TouchHapticsClip, &channelData, ovrHapticsGenMode_PointSample);
        ovr_ReleaseAudioChannelData(&channelData);
    }


    SetCommandLineMenuOption(Argc, Argv);

    LastUpdate = ovr_GetTimeInSeconds();


    // Create a layer list. This is slightly gratuitous - this app had only one thread,
    // so it really only needs the default list. So we're creating and using one just to
    // demonstrate/test the API.
    //OVR_ASSERT ( pCockpitLayerList == nullptr );
    //pCockpitLayerList = ovr_CreateLayerList ( Session );

    HmdDisplayAcquired = true;


    return 0;
}

// This function sets parameters based on command line arguments
// The argument should have the format --m:FeatureName=Value
// Feature supported:
//   scenecontent.drawrepeat.lowcount
//   scenecontent.renderscene
void OculusWorldDemoApp::SetCommandLineMenuOption(int argc, const char** argv)
{
    for (int iTok = 0; iTok < argc; ++iTok)
    {
        // Looking for the --m argument tag
        std::string token(argv[iTok]);
        if (token.compare("--m:"))
        {
            size_t start = token.find_first_of(":") + 1;
            size_t seperator = token.find_last_of("=");

            if (start != std::string::npos && seperator != std::string::npos)
            {
                OptionMenuItem *menuItem = nullptr;

                std::string label = token.substr(start, seperator - start);
                std::string value = token.substr(seperator + 1);

                // Lower case to make the string comparison case independent
                std::transform(label.begin(), label.end(), label.begin(), [](const char& c) { return (char)::tolower(c); });

                // Looking for the menu item to set
                if (label.compare("scenecontent.drawrepeat.lowcount") == 0)
                {
                    menuItem = Menu.FindMenuItem("Scene Content.Draw Repeat.Low count");
                }
                else if (label.compare("scenecontent.renderscene") == 0)
                {
                    menuItem = Menu.FindMenuItem("Scene Content.Rendered Scene ';");
                }

                // Setting the value of the feature
                if (menuItem != nullptr)
                {
                    menuItem->SetValue(std::string(value.c_str()));
                }
                else
                {
                    WriteLog("[OculusWorldDemoApp::SetCommandLineMenuOption] Warning: Didn't find label for -> %s\n", label.c_str());
                }
            }
        }
    }
}


bool OculusWorldDemoApp::SetupWindowAndRendering(bool firstTime, int argc, const char** argv, ovrGraphicsLuid luid)
{
    // *** Window creation

    // Don't recreate the window if we already created it
    if (firstTime)
    {
        void* windowHandle = pPlatform->SetupWindow(WindowSize.w, WindowSize.h);

        if (!windowHandle)
            return false;
    }

    // Report relative mouse motion in OnMouseMove
    pPlatform->SetMouseMode(Mouse_Relative);

    // *** Initialize Rendering

#if defined(OVR_OS_MS)
    const char* graphics = "d3d11";  //Default to DX11. Can be overridden below.
#else
    const char* graphics = "GL";
#endif

    // Select renderer based on command line arguments.
    // Example usage: App.exe -r d3d9 -fs
    // Example usage: App.exe -r GL
    // Example usage: App.exe -r GL -GLVersion 4.1 -GLCoreProfile -DebugEnabled
    for(int i = 1; i < argc; i++)
    {
        const bool lastArg = (i == (argc - 1)); // False if there are any more arguments after this.

        if(!OVR_stricmp(argv[i], "-r") && !lastArg)  // Example: -r GL
        {
            graphics = argv[++i];
        }
        else if(!OVR_stricmp(argv[i], "-MultisampleDisabled")) // Example: -MultisampleDisabled
        {
            MultisampleRequested = false;
        }
        else if(!OVR_stricmp(argv[i], "-DebugEnabled")) // Example: -DebugEnabled
        {
            RenderParams.DebugEnabled = true;
        }
        else if(!OVR_stricmp(argv[i], "-GLVersion") && !lastArg) // Example: -GLVersion 3.2
        {
            const char* version = argv[++i];
            sscanf(version, "%d.%d", &RenderParams.GLMajorVersion, &RenderParams.GLMinorVersion);
        }
        else if(!OVR_stricmp(argv[i], "-GLCoreProfile")) // Example: -GLCoreProfile
        {
            RenderParams.GLCoreProfile = true;
        }
        else if(!OVR_stricmp(argv[i], "-GLCoreCompatibilityProfile")) // Example: -GLCompatibilityProfile
        {
            RenderParams.GLCompatibilityProfile = true;
        }
        else if(!OVR_stricmp(argv[i], "-GLForwardCompatibleProfile")) // Example: -GLForwardCompatibleProfile
        {
            RenderParams.GLForwardCompatibleProfile = true;
        }
    }

    // Setup RenderParams.RenderAPI
    if (OVR_stricmp(graphics, "GL") == 0)
    {
        RenderParams.RenderAPI = RenderAPI_OpenGL;
        TextureOriginAtBottomLeft = true;
    }
#if defined(OVR_OS_MS)
    else if (OVR_stricmp(graphics, "d3d11") == 0)
    {
        RenderParams.RenderAPI = RenderAPI_D3D11;
    }
#endif

    char title[128];
    sprintf(title, "Oculus World Demo %s : %s", graphics, HmdDesc.ProductName[0] ? HmdDesc.ProductName : "<unknown device>");
    pPlatform->SetWindowTitle(title);

    SrgbRequested = ShouldLoadedTexturesBeSrgb(EyeTextureFormat);

    // Ideally we would use the created OpenGL context to determine multisamping support,
    // but that creates something of a chicken-and-egg problem which is easier to solve in
    // practice by the code below, as it's the only case where multisamping isn't supported
    // in modern computers of interest to us.
    #if defined(OVR_OS_MAC)
        if (RenderParams.GLMajorVersion < 3)
            SupportsMultisampling = false;
    #endif

    // Make sure the back buffer format is also sRGB correct, otherwise the blit of the mirror into the back buffer will be wrong
    RenderParams.SrgbBackBuffer = SrgbRequested;
    RenderParams.Resolution  = WindowSize;
#if defined(OVR_BUILD_DEBUG)
    RenderParams.DebugEnabled = true;
#endif

    pRender = pPlatform->SetupGraphics(Session, OVR_DEFAULT_RENDER_DEVICE_SET,
                                       graphics, RenderParams, luid); // To do: Remove the graphics argument to SetupGraphics, as RenderParams already has this info.
    return (pRender != nullptr);
}

// Custom formatter for Timewarp interval message.
static std::string FormatTimewarp(OptionVar* var)
{
    char    buff[64];
    float   timewarpInterval = *var->AsFloat();
    sprintf(buff, "%.1fms, %.1ffps",
                timewarpInterval * 1000.0f,
                ( timewarpInterval > 0.000001f ) ? 1.0f / timewarpInterval : 10000.0f);
    return std::string(buff);
}

static ovrFovPort g_EyeFov[2];  // store the current FOV globally for access by static display function

static std::string FormatMaxFromSideTan(OptionVar* var)
{
    OVR_UNUSED(var);
    char   buff[64];
    float horiz = (atan(g_EyeFov[0].LeftTan) + atan(g_EyeFov[1].RightTan)) * (180.0f / MATH_FLOAT_PI);
    float vert = (atan(g_EyeFov[0].UpTan) + atan(g_EyeFov[1].DownTan)) * (180.0f / MATH_FLOAT_PI);
    sprintf(buff, "%.1f x %.1f deg", horiz, vert);
    return std::string(buff);
}

static std::string FormatTanAngleToDegs(OptionVar* var)
{
  char   buff[64];
  float degs = atan(*var->AsFloat()) * (180.0f / MATH_FLOAT_PI);
  sprintf(buff, "%.1f deg", degs);
  return std::string(buff);
}



void OculusWorldDemoApp::PopulateOptionMenu()
{
    // For shortened function member access.
    typedef OculusWorldDemoApp OWD;

    // *** Scene Content Sub-Menu

    // Test
    /*
        Menu.AddEnum("Scene Content.EyePoseMode", &FT_EyePoseState).AddShortcutKey(Key_Y).
        AddEnumValue("Separate Pose",  0).
        AddEnumValue("Same Pose",      1).
        AddEnumValue("Same Pose+TW",   2);
    */

    Menu.AddEnum("Scene Content.Rendered Scene ';'", &SceneMode).AddShortcutKey(Key_Semicolon).
                 AddEnumValue("World",        Scene_World).
                 AddEnumValue("Cubes",        Scene_Cubes).
                 AddEnumValue("Oculus Cubes", Scene_OculusCubes)
                 ;


    // Toggle grid
    Menu.AddEnum("Scene Content.Grid Display 'G'",  &GridDisplayMode).AddShortcutKey(Key_G).
                 AddEnumValue("No Grid",                    GridDisplay_None).
                 AddEnumValue("Grid Only",                  GridDisplay_GridOnly).
                 AddEnumValue("Grid Direct (not stereo!)",  GridDisplay_GridDirect).
                 AddEnumValue("Grid And Scene",             GridDisplay_GridAndScene).
                            SetNotify(this, &OWD::HmdSettingChange); // GridDisplay_GridDirect needs to disable MSAA.
    Menu.AddEnum("Scene Content.Grid Mode 'H'",     &GridMode).AddShortcutKey(Key_H).
                 AddEnumValue("4-pixel RT-centered", Grid_Rendertarget4).
                 AddEnumValue("16-pixel RT-centered",Grid_Rendertarget16).
                 AddEnumValue("Lens-centered grid",  Grid_Lens);
    Menu.AddEnum("Scene Content.Grid Color", &GridColor).
      AddEnumValue("Default", GridColor_Default).
      AddEnumValue("White"  , GridColor_White).
      AddEnumValue("Red"    , GridColor_Red).
      AddEnumValue("Green"  , GridColor_Green).
      AddEnumValue("Blue"   , GridColor_Blue).
      AddEnumValue("Yellow" , GridColor_Yellow).
      AddEnumValue("Magenta", GridColor_Magenta).
      AddEnumValue("Cyan"   , GridColor_Cyan);

    Menu.AddBool("Scene Content.Anisotropic Sampling", &AnisotropicSample).SetNotify(this, &OWD::ForceAssetReloading); // same sRGB function works fine

    Menu.AddBool("Scene Content.Tint.Grayscale Tint", &SceneBrightnessGrayscale);
    Menu.AddFloat("Scene Content.Tint.R (Full=255)", &SceneBrightness.x, 0.0f, 1000.0f, 0.5f, "%.1f");
    Menu.AddFloat("Scene Content.Tint.G (Full=255)", &SceneBrightness.y, 0.0f, 1000.0f, 0.5f, "%.1f");
    Menu.AddFloat("Scene Content.Tint.B (Full=255)", &SceneBrightness.z, 0.0f, 1000.0f, 0.5f, "%.1f");
    Menu.AddBool ("Scene Content.Black screen 'Shift+B'", &SceneBlack).AddShortcutKey(Key_B, ShortcutKey::Shift_RequireOn);
    Menu.AddBool ("Scene Content.Animation Enabled", &SceneAnimationEnabled);

    // Animating blocks
    Menu.AddEnum("Scene Content.Animated Blocks.Movement Type 'B'", &BlocksShowType).
                 AddShortcutKey(Key_B, ShortcutKey::Shift_RequireOff).SetNotify(this, &OWD::BlockShowChange).
                 AddEnumValue("None",  0).
                 AddEnumValue("Horizontal Circle", 1).
                 AddEnumValue("Vertical Circle",   2).
                 AddEnumValue("Bouncing Blocks",   3);
    Menu.AddEnum("Scene Content.Animated Blocks.Movement Pattern", &BlocksMovementType).
                 AddEnumValue("Circle",             0).
                 AddEnumValue("Circle+Sine",        1).
                 AddEnumValue("Circle+Triangle",    2);
    Menu.AddFloat("Scene Content.Animated Blocks.Movement Speed", &BlocksSpeed, 0.0f, 10.0f, 0.1f, "%.1f");
    Menu.AddFloat("Scene Content.Animated Blocks.Movement Radius", &BlocksMovementRadius, 0.1f, 100.0f, 0.1f, "%.2f");
    Menu.AddFloat("Scene Content.Animated Blocks.Movement Scale",  &BlocksMovementScale, 0.1f, 10.0f, 0.05f, "%.2f");
    Menu.AddEnum("Scene Content.Animated Blocks.Block Type", &BlocksShowMeshType).
                 AddEnumValue("Green",       0).
                 AddEnumValue("Oculus",      1).
                 AddEnumValue("Oculus Green",2).
                 AddEnumValue("Oculus Red",  3);
    Menu.AddInt  ("Scene Content.Animated Blocks.Block Count", &BlocksHowMany, 1, 50, 1);
    Menu.AddFloat("Scene Content.Animated Blocks.Block Size X", &BlockScale.x, 0.0f, 100.0f, 1.0f, "%.0f");
    Menu.AddFloat("Scene Content.Animated Blocks.Block Size Y", &BlockScale.y, 0.0f, 100.0f, 1.0f, "%.0f");
    Menu.AddFloat("Scene Content.Animated Blocks.Block Size Z", &BlockScale.z, 0.0f, 100.0f, 1.0f, "%.0f");


    Menu.AddInt ("Scene Content.Draw Repeat.Low count",  &SceneRenderCountLow, 1, 1000000, 1 );
    Menu.AddInt ("Scene Content.Draw Repeat.High count", &SceneRenderCountHigh, 1, 1000000, 1 );
    Menu.AddEnum("Scene Content.Draw Repeat.Load type",  &SceneRenderCountType).
                AddEnumValue("Fixed low",       SceneRenderCount_FixedLow).
                AddEnumValue("Sine wave 10s",   SceneRenderCount_SineTenSec).
                AddEnumValue("Square wave 10s", SceneRenderCount_SquareTenSec).
                AddEnumValue("Spikes",          SceneRenderCount_Spikes);

    Menu.AddEnum("Scene Content.Draw Flush.Flush Mode", &DrawFlushMode).
        AddEnumValue("Off", DrawFlush_Off).
        AddEnumValue("After Each Eye Render", DrawFlush_AfterEachEyeRender).
        AddEnumValue("After Eye Pair Render", DrawFlush_AfterEyePairRender);
    Menu.AddInt("Scene Content.Draw Flush.Max Flushes Per-Frame", &DrawFlushCount, 0, 100, 1);

    Menu.AddFloat("Scene Content.Waste CPU Time.Before Tracking Query (ms)", &SceneRenderWasteCpuTimePreTrackingQueryMs, 0.0f, 20.0f, 0.1f);
    Menu.AddFloat("Scene Content.Waste CPU Time.Before Scene Draw (ms)", &SceneRenderWasteCpuTimePreRenderMs, 0.0f, 20.0f, 0.1f);
    Menu.AddFloat("Scene Content.Waste CPU Time.After Each Repeated Draw (ms)", &SceneRenderWasteCpuTimeEachRenderMs, 0.0f, 20.0f, 0.1f);
    Menu.AddFloat("Scene Content.Waste CPU Time.Before Frame Submit (ms)", &SceneRenderWasteCpuTimePreSubmitMs, 0.0f, 20.0f, 0.1f);

    // Render target menu
    Menu.AddBool( "Render Target.Share RenderTarget",  &RendertargetIsSharedByBothEyes).
                                                        AddShortcutKey(Key_F8).SetNotify(this, &OWD::HmdSettingChangeFreeRTs);
    Menu.AddEnum( "Render Target.Resolution Scaling", &ResolutionScalingMode).
                AddEnumValue("Off",                             ResolutionScalingMode_Off).
                AddEnumValue("Dynamic",                         ResolutionScalingMode_Dynamic).
                AddEnumValue("Adaptive",                        ResolutionScalingMode_Adaptive).
                AddEnumValue("AlternatingEyes",                 ResolutionScalingMode_AlternatingEyes).
                                                        AddShortcutKey(Key_F8, ShortcutKey::Shift_RequireOn).
                                                        SetNotify(this, &OWD::RendertargetResolutionModeChange);
    Menu.AddFloat("Render Target.Alternating Eye Scale", &AlternatingEyeResScale, 0.1f, 1.0f, 0.01f);
    Menu.AddEnum("Render Target.Monoscopic Render 'F7'", &MonoscopicRenderMode).
                 AddEnumValue("Off",                            Mono_Off).
                 AddEnumValue("Zero IPD - !!nausea caution!!",  Mono_ZeroIpd).
                 AddEnumValue("Zero player scale",              Mono_ZeroPlayerScale).
                                                        AddShortcutKey(Key_F7).
                                                        SetNotify(this, &OWD::HmdSettingChangeFreeRTs);
    Menu.AddFloat("Render Target.Limit Fov",           &FovScaling, -1.0f, 1.0f, 0.02f,
                                                        "%.1f Degrees", 1.0f, &FormatMaxFromSideTan).
                                                        SetNotify(this, &OWD::HmdSettingChange);

    Menu.AddBool("Render Target.Fov Override.Enable",  &FovPortOverrideEnable).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddBool("Render Target.Fov Override.Reset To Default",  &FovPortOverrideResetToDefault).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Fov Override.Up",     &FovPortOverride.UpTan, 0.1f, 10.0f, 0.01f,
                                                        "%.1f Degrees", 1.0f, &FormatTanAngleToDegs).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Fov Override.Down",   &FovPortOverride.DownTan, 0.1f, 10.0f, 0.01f,
                                                        "%.1f Degrees", 1.0f, &FormatTanAngleToDegs).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Fov Override.Temporal", &FovPortOverride.LeftTan, 0.1f, 10.0f, 0.01f,
                                                        "%.1f Degrees", 1.0f, &FormatTanAngleToDegs).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Fov Override.Nasal",  &FovPortOverride.RightTan, 0.1f, 10.0f, 0.01f,
                                                        "%.1f Degrees", 1.0f, &FormatTanAngleToDegs).
                                                        SetNotify(this, &OWD::HmdSettingChange);

    Menu.AddFloat("Render Target.Max Pixel Density",   &DesiredPixelDensity, 0.1f, 2.5, 0.025f, "%3.2f", 1.0f).
                                                        SetNotify(this, &OWD::HmdSettingChange);
    if (SupportsMultisampling)
    {
        Menu.AddBool("Render Target.MultiSample 'F4'", &MultisampleRequested).AddShortcutKey(Key_F4).SetNotify(this, &OWD::RendertargetFormatChange);
    }

    Menu.AddEnum("Render Target.Eye Texture Format", &EyeTextureFormat).
        //AddEnumValue("B5G6R5",      EyeTextureFormat_B5G6R5).
        //AddEnumValue("BGR5A1",      EyeTextureFormat_BGR5A1).
        //AddEnumValue("BGRA4",       EyeTextureFormat_BGRA4).
        AddEnumValue("RGBA8",       EyeTextureFormat_RGBA8).
        //AddEnumValue("BGRA8",       EyeTextureFormat_BGRA8).
        //AddEnumValue("BGRX8",       EyeTextureFormat_BGRX8).
        AddEnumValue("RGBA16F",     EyeTextureFormat_RGBA16F).
        AddEnumValue("R11G11B10F",  EyeTextureFormat_R11G11B10F).
        AddEnumValue("RGBA8_SRGB",  EyeTextureFormat_RGBA8_SRGB).
        //AddEnumValue("BGRA8_SRGB",  EyeTextureFormat_BGRA8_SRGB).
        //AddEnumValue("BGRX8_SRGB",  EyeTextureFormat_BGRX8_SRGB).
        SetNotify(this, &OWD::RendertargetFormatChange);

    Menu.AddEnum( "Render Target.FOV Stencil Type",  &FovStencilType).
      AddEnumValue("Off", -1).
      AddEnumValue("Hidden Area", ovrFovStencil_HiddenArea).
      AddEnumValue("Visible Area", ovrFovStencil_VisibleArea).
      AddEnumValue("Visible Rectangle", ovrFovStencil_VisibleRectangle).
      AddEnumValue("Border Line", ovrFovStencil_BorderLine).
      SetNotify(this, &OWD::HmdSettingChangeFreeRTs);
    Menu.AddEnum("Render Target.FOV Stencil Color", &FovStencilColor).
      AddEnumValue("Black", FovStencilColor_Black).
      AddEnumValue("Green", FovStencilColor_Green).
      AddEnumValue("Red",   FovStencilColor_Red).
      AddEnumValue("White", FovStencilColor_White).
      SetNotify(this, &OWD::HmdSettingChangeFreeRTs);


    Menu.AddEnum("Render Target.Depth.Format", &DepthFormatOption).
                                                AddEnumValue("Depth32f",            DepthFormatOption_D32f).
                                                AddEnumValue("Depth24_Stencil8",    DepthFormatOption_D24_S8).
                                                AddEnumValue("Depth16",             DepthFormatOption_D16).
                                                AddEnumValue("Depth32f_Stencil8",   DepthFormatOption_D32f_S8).
                                                SetNotify(this, &OWD::HmdSettingChange);



    Menu.AddFloat("Render Target.Depth.Near Clipping", &NearClip,
                                                        0.0001f, 10.00f, 0.0001f, "%.4f").
                                                        SetNotify(this, &OWD::HmdSettingChange);

    Menu.AddFloat("Render Target.Depth.Far Clipping", &FarClip,
                                                        1.0f, 100000.00f, 10.0f, "%.0f").
                                                        SetNotify(this, &OWD::HmdSettingChange);

    Menu.AddEnum( "Render Target.Depth.Depth Modifier", &DepthModifier).
                                                        AddEnumValue("Near < Far", NearLessThanFar).
                                                        AddEnumValue("Far < Near", FarLessThanNear).
                                                        AddEnumValue("Far < Near & Far Clip at infinity", FarLessThanNearAndInfiniteFarClip).
                                                        SetNotify(this, &OWD::HmdSettingChange);





    Menu.AddBool("Render Target.Discrete Rotation.Enable",             &DiscreteHmdRotationEnable).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddEnum("Render Target.Discrete Rotation.Scisscor Technique", &DiscreteRotationScissorTech).
      AddEnumValue("None", DRScissorTechnique_None).
      AddEnumValue("Project Corners", DRScissorTechnique_ProjectCorners).
      AddEnumValue("Project Edges", DRScissorTechnique_ProjectEdges);
    Menu.AddFloat("Render Target.Discrete Rotation.Threshold Angle",   &DiscreteRotationThresholdAngle, 0.0f, 30.0f, 0.01f).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Discrete Rotation.Scissor Grow Mult", &DiscreteRotationScissorMult, -10.0f, 10.0f, 0.01f).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Discrete Rotation.Debug FOV Grow Mult", &DiscreteRotationFovMult, -10.0f, 10.0f, 0.01f).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddFloat("Render Target.Discrete Rotation.Read Only - Scissor Cull % - Eye L", &DiscreteRotationScissorMenuViewSavings[ovrEye_Left], 0.0f, 1.0f, 0.01f, "%3.0f%%");
    Menu.AddFloat("Render Target.Discrete Rotation.Read Only - Scissor Cull % - Eye R", &DiscreteRotationScissorMenuViewSavings[ovrEye_Right], 0.0f, 1.0f, 0.01f, "%3.0f%%");

    Menu.AddFloat("Render Target.Discrete Rotation.Read Only - Pixel Perf Hit % - Eye L", &DiscreteRotationScissorMenuViewPerfHit[ovrEye_Left], 0.0f, 10.0f, 0.01f, "%3.1f%%");
    Menu.AddFloat("Render Target.Discrete Rotation.Read Only - Pixel Perf Hit % - Eye R", &DiscreteRotationScissorMenuViewPerfHit[ovrEye_Right], 0.0f, 10.0f, 0.01f, "%3.1f%%");

    Menu.AddInt("Render Target.Discrete Rotation.Read Only - Scissor Width  - Eye L", &DiscreteRotationScissor[ovrEye_Left].w, 0, 10000, 1);
    Menu.AddInt("Render Target.Discrete Rotation.Read Only - Scissor Height - Eye L", &DiscreteRotationScissor[ovrEye_Left].h, 0, 10000, 1);
    Menu.AddInt("Render Target.Discrete Rotation.Read Only - Scissor Width  - Eye R", &DiscreteRotationScissor[ovrEye_Right].w, 0, 10000, 1);
    Menu.AddInt("Render Target.Discrete Rotation.Read Only - Scissor Height - Eye R", &DiscreteRotationScissor[ovrEye_Right].h, 0, 10000, 1);
    
    Menu.AddBool("Render Target.Sub-pixel Jitter.Enable", &SubpixelJitterEnable).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddInt("Render Target.Sub-pixel Jitter.Sample Count", &SubpixelJitterSampleCount, 1, 10, 1);
    Menu.AddFloat("Render Target.Sub-pixel Jitter.Sample Polar Offset", &SubpixelJitterSamplePolarOffset, 0.0f, MATH_FLOAT_TWOPI, 0.01f);
    Menu.AddFloat("Render Target.Sub-pixel Jitter.Sample Spread", &SubpixelJitterSamplePolarDistMult, 0.01f, 10.0f, 0.01f);

    Menu.AddBool( "Timewarp.Freeze Eye Update 'C'",         &FreezeEyeUpdate).AddShortcutKey(Key_C);
    Menu.AddBool( "Timewarp.Enable Timewarp on Main Layer", &EnableTimewarpOnMainLayer);
    Menu.AddBool( "Timewarp.Render Interval Enabled 'I'",   &TimewarpRenderIntervalEnabled).AddShortcutKey(Key_I);
    Menu.AddFloat("Timewarp.Render Interval",               &TimewarpRenderIntervalInSeconds,
                                                             0.0001f, 1.00f, 0.0001f, "%.1f", 1.0f, &FormatTimewarp).
                                                             AddShortcutUpKey(Key_J).AddShortcutDownKey(Key_U);
    Menu.AddBool("Timewarp.Enable Prediction",              &PredictionEnabled);


    // Layers menu
    Menu.AddBool( "Layers.Layers Enabled 'Shift+L'",      &LayersEnabled).AddShortcutKey(Key_L, ShortcutKey::Shift_RequireOn).SetNotify(this, &OWD::HmdSettingChange);
    // Layer 0 is the main scene - can't turn it off!
    Menu.AddBool( "Layers.Main Layer.HQ",           &Layer0HighQuality).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddInt(  "Layers.Main Layer.Gen-Mips (0=SDK-gen)",  &Layer0GenMipCount, 0, 12).SetNotify(this, &OWD::HmdSettingChange);
    Menu.AddBool( "Layers.Main Layer.Depth",        &Layer0Depth).SetNotify(this, &OWD::RendertargetFormatChange);
    Menu.AddBool( "Layers.World Layer Enabled",     &Layer1Enabled);
    // Removing UI clutter - Menu.AddBool( "Layers.World Layer HQ",          &Layer1HighQuality);
    Menu.AddBool( "Layers.Torso Layer Enabled",     &Layer2Enabled);
    // Removing UI clutter - Menu.AddBool( "Layers.Torso Layer HQ",          &Layer2HighQuality);
    Menu.AddBool( "Layers.Face Layer Enabled",      &Layer3Enabled);
    // Removing UI clutter - Menu.AddBool( "Layers.Face Layer HQ",           &Layer3HighQuality);
    Menu.AddBool( "Layers.Face2 Layer Enabled",     &Layer4Enabled);
    Menu.AddBool( "Layers.Protected Layer Enabled", &LayerHdcpEnabled).SetNotify(this, &OWD::TriggerHdcpLoad);
    // Removing UI clutter - Menu.AddBool( "Layers.Face2 Layer HQ",          &Layer4HighQuality);
    Menu.AddFloat("Layers.Torso+Face Layer Size",   &Layer234Size, 0.0f, 10.0f, 0.05f);
    Menu.AddInt ( "Layers.Cockpit.Enable Bitfield", &LayerCockpitEnabled, 0, 31, 1);
    Menu.AddBool( "Layers.Cockpit.HighQuality",     &LayerCockpitHighQuality);
    Menu.AddBool( "Layers.HUD/Menu HighQuality",    &LayerHudMenuHighQuality);
    Menu.AddBool( "Layers.Cylinder.Enabled",        &LayerCylinderEnabled);
    Menu.AddBool( "Layers.Cylinder.HQ",             &LayerCylinderHighQuality);
    Menu.AddFloat("Layers.Cylinder.Radius",         &LayerCylinderRadius, 0.0f, 5.0f, 0.05f);
    Menu.AddFloat("Layers.Cylinder.Aspect Ratio",   &LayerCylinderAspectRatio, 0.01f, 10.0f, 0.05f);
    // 0.2 is subtracted off the angular range because the non-double-sided cylinder
    // exhibits artifacts at larger angles.
    Menu.AddFloat("Layers.Cylinder.Angle",          &LayerCylinderAngle, 0.01f, 2.0f*3.1415926f - 0.2f, 0.05f);
    Menu.AddBool("Layers.Cylinder.Headlocked", &LayerCylinderHeadlocked);

    Menu.AddEnum("Layers.Cubemap", &LayerCubemap).
        AddEnumValue("Disabled", Cubemap_Off).
        AddEnumValue("Rendered", Cubemap_Render).
        AddEnumValue("Texture", Cubemap_Load);
    Menu.AddBool("Layers.Layer Cubemap Save on Render", &LayerCubemapSaveOnRender);
    Menu.AddBool("Layers.Cubemap HQ", &LayerCubemapHighQuality);

    // Test pattern menu.


    // Player menu
    Menu.AddBool("Player.Seat Level.Visualize Floor 'O'", &VisualizeSeatLevel).AddShortcutKey(Key_O);
    Menu.AddBool("Player.Seat Level.Sitting Auto Switch 'Shift+O'", &SittingAutoSwitch).AddShortcutKey(Key_O, ShortcutKey::Shift_RequireOn);
    Menu.AddBool("Player.Seat Level.Sitting State",					&Sitting);
    Menu.AddBool("Player.Seat Level.Donut Floor Visual",			&DonutFloorVisual);


    Menu.AddEnum("Player.Tracking Origin", &TrackingOriginType).AddEnumValue("Eye Level", ovrTrackingOrigin_EyeLevel).
                                                                AddEnumValue("Floor Level", ovrTrackingOrigin_FloorLevel).
                                                                SetNotify(this, &OWD::UpdatedTrackingOriginType);

    Menu.AddFloat("Player.Position Tracking Scale", &PositionTrackingScale, 0.00f, 50.0f, 0.05f).
                                                    SetNotify(this, &OWD::EyeHeightChange);
    Menu.AddBool("Player.Scale Affects Player Height", &ScaleAffectsEyeHeight).SetNotify(this, &OWD::EyeHeightChange);
    Menu.AddFloat("Player.Standing Eye Height",     &MenuUserEyeHeight, 0.2f, 2.5f, 0.02f,
                                        "%4.2f m").SetNotify(this, &OWD::EyeHeightChange);
    Menu.AddFloat("Player.Center Pupil Depth",      &CenterPupilDepthMeters, 0.0f, 0.2f, 0.001f,
                                        "%4.3f m").SetNotify(this, &OWD::CenterPupilDepthChange);

    Menu.AddBool("Player.Body Relative Motion",     &ThePlayer.bMotionRelativeToBody).AddShortcutKey(Key_E);
    Menu.AddBool("Player.Zero Head Movement",       &ForceZeroHeadMovement) .AddShortcutKey(Key_F7, ShortcutKey::Shift_RequireOn);
    Menu.AddEnum("Player.Comfort Turn",             &ComfortTurnMode).
                 AddEnumValue("Off",                 ComfortTurn_Off).
                 AddEnumValue("30 degrees",          ComfortTurn_30Degrees).
                 AddEnumValue("45 degrees",          ComfortTurn_45Degrees);

    Menu.AddBool("Player.Show Calibrated Tracking Origin",   &ShowCalibratedOrigin);

    Menu.AddFloat(  "Player.Specified Tracking Origin.Move X (m)",    &ProvidedTrackingOriginTranslationYaw.x, -10.0f, 10.0f, 0.01f);
    Menu.AddFloat(  "Player.Specified Tracking Origin.Move Y (m)",    &ProvidedTrackingOriginTranslationYaw.y, -10.0f, 10.0f, 0.01f);
    Menu.AddFloat(  "Player.Specified Tracking Origin.Move Z (m)",    &ProvidedTrackingOriginTranslationYaw.z, -10.0f, 10.0f, 0.01f);
    Menu.AddFloat(  "Player.Specified Tracking Origin.Yaw (degrees)", &ProvidedTrackingOriginTranslationYaw.w, -180.0f, 180.0f, 1.0f);
    Menu.AddTrigger("Player.Specified Tracking Origin.Apply Tracking Origin").SetNotify(this, &OWD::SpecifyTrackingOrigin);

    // Tracking menu
	Menu.AddTrigger("Tracking.Recenter Tracking Origin 'R'").AddShortcutKey(Key_R).SetNotify(this, &OWD::ResetHmdPose);
	Menu.AddBool("Tracking.Visualize Tracker", &VisualizeTracker);


    // Display menu
    Menu.AddBool("Display.Mirror Window.Enabled",    &MirrorToWindow).AddShortcutKey(Key_M).SetNotify(this, &OWD::MirrorSettingChange);
    Menu.AddInt("Display.Mirror Window.Width",       &WindowSize.w, 100, 4000).SetNotify(this, &OWD::WindowSizeChange);
    Menu.AddInt("Display.Mirror Window.Height",      &WindowSize.h, 100, 4000).SetNotify(this, &OWD::WindowSizeChange);
    Menu.AddTrigger("Display.Mirror Window.Set To Native HMD Res").SetNotify(this, &OWD::WindowSizeToNativeResChange);
    Menu.AddFloat("Display.Mirror Window.Native Res Mirror Scale", &MirrorNativeResScale, 0.05f, 4.0f, 0.05f);

    Menu.AddEnum("Display.Mirror Window.Type",       &CurrentMirrorOptionStage).
        AddEnumValue("Rectilinear", MirrorOptionStage_Rectilinear).
        AddEnumValue("Post Distortion", MirrorOptionStage_PostDistortion).SetNotify(this, &OWD::MirrorSettingChange);
    Menu.AddEnum("Display.Mirror Window.Rectilinear.Eye",       &CurrentMirrorOptionEyes).
        AddEnumValue("Both", MirrorOptionEyes_Both).
        AddEnumValue("Left-Only", MirrorOptionEyes_LeftOnly).
        AddEnumValue("Right-Only", MirrorOptionEyes_RightOnly).SetNotify(this, &OWD::MirrorSettingChange);
    Menu.AddBool("Display.Mirror Window.Rectilinear.Symmetric FOV", &CurrentMirrorOptionForceSymmetricFov).SetNotify(this, &OWD::MirrorSettingChange);
    Menu.AddBool("Display.Mirror Window.Rectilinear.Capture Guardian", &CurrentMirrorOptionCaptureGuardian).SetNotify(this, &OWD::MirrorSettingChange);
    Menu.AddBool("Display.Mirror Window.Rectilinear.Capture Notifications", &CurrentMirrorOptionCaptureNotifications).SetNotify(this, &OWD::MirrorSettingChange);
    Menu.AddBool("Display.Mirror Window.Rectilinear.Capture System GUI", &CurrentMirrorOptionCaptureSystemGui).SetNotify(this, &OWD::MirrorSettingChange);
      
    Menu.AddEnum( "Display.Border Clear Color",      &DistortionClearBlue).
                                                      SetNotify(this, &OWD::DistortionClearColorChange).
                                                      AddEnumValue("Black",  0).
                                                      AddEnumValue("Blue", 1);



#if defined(OVR_OS_WIN32) || defined(OVR_OS_WIN64)
#endif



    // Menu menu.
    Menu.AddBool( "Menu.Menu Change Messages",       &ShortcutChangeMessageEnable);
    Menu.AddBool( "Menu.Menu Visible 'Shift+Tab'",   &LayerHudMenuEnabled).AddShortcutKey(Key_Tab, ShortcutKey::Shift_RequireOn);
    Menu.AddBool( "Menu.Menu Always on Mirror Window", &MenuHudAlwaysOnMirrorWindow);
    Menu.AddTrigger("Menu.Trace all menu options").SetNotify(this, &OWD::TraceAllMenuOptions);

    Menu.AddFloat("Menu.HUD/Menu Text Pixel Height", &MenuHudTextPixelHeight, 3.0f, 75.0f, 0.5f);
    Menu.AddFloat("Menu.HUD/Menu Distance (meters)", &MenuHudDistance, 0.1f, 10.0f, 0.05f);
    Menu.AddFloat("Menu.HUD/Menu Scale",             &MenuHudSize, 0.1f, 10.0f, 0.1f);
    Menu.AddEnum( "Menu.Movement Mode",              &MenuHudMovementMode).
                AddEnumValue("Fixed to face",         MenuHudMove_FixedToFace).
                AddEnumValue("Drag at edge",          MenuHudMove_DragAtEdge).
                AddEnumValue("Recenter at edge",      MenuHudMove_RecenterAtEdge);
    Menu.AddFloat("Menu.Movement Radius (degrees)",  &MenuHudMovementRadius, 1.0f, 50.0f, 1.0f );
    Menu.AddFloat("Menu.Movement Position",          &MenuHudMovementDistance, 0.0f, 10.0f, 0.01f);
    Menu.AddFloat("Menu.Movement Rotation Speed (deg/sec)", &MenuHudMovementRotationSpeed, 10.0f, 2000.0f, 10.0f);
    Menu.AddFloat("Menu.Movement Translation Speed (m/sec)", &MenuHudMovementTranslationSpeed, 0.01f, 10.0f, 0.01f);
    Menu.AddFloat("Menu.Movement Pitch Zone For Roll (deg)", &MenuHudMaxPitchToOrientToHeadRoll, 0.0f, 80.0f);


    // Add DK2 options to menus only for that headset.
    if (HmdDesc.AvailableTrackingCaps & ovrTrackingCap_Position)
    {
        Menu.AddBool("Tracking.Positional Tracking 'X'",            &PositionTrackingEnabled).
                                                                     AddShortcutKey(Key_X).SetNotify(this, &OWD::HmdSettingChange);
    }

    Menu.AddEnum("SDK HUD.Perf HUD Mode 'F11'", &PerfHudMode).  AddEnumValue("Off", ovrPerfHud_Off).
                                                                AddEnumValue("Performance Summary", ovrPerfHud_PerfSummary).
                                                                AddEnumValue("Latency Timing", ovrPerfHud_LatencyTiming).
                                                                AddEnumValue("App Render Timing", ovrPerfHud_AppRenderTiming).
                                                                AddEnumValue("Comp Render Timing", ovrPerfHud_CompRenderTiming).
                                                                AddEnumValue("ASW Stats", ovrPerfHud_AswStats).
                                                                AddEnumValue("Version Info", ovrPerfHud_VersionInfo).
                                                                AddShortcutKey(Key_F11).
                                                                SetNotify(this, &OWD::PerfHudSettingChange);

    Menu.AddEnum("SDK HUD.Stereo Guide Mode", &DebugHudStereoMode). AddEnumValue("Off", ovrDebugHudStereo_Off).
                                                                    AddEnumValue("Quad", ovrDebugHudStereo_Quad).
                                                                    AddEnumValue("Quad with crosshair", ovrDebugHudStereo_QuadWithCrosshair).
                                                                    AddEnumValue("Crosshair at Infinity", ovrDebugHudStereo_CrosshairAtInfinity).
                                                                    SetNotify(this, &OWD::DebugHudSettingModeChange);

    Menu.AddEnum("SDK HUD.Stereo Settings.Stereo Guide Presets", &DebugHudStereoPresetMode).
                                                                    AddEnumValue("No preset", DebugHudStereoPreset_Free).
                                                                    AddEnumValue("Stereo guide off", DebugHudStereoPreset_Off).
                                                                    AddEnumValue("One meter square two meters away", DebugHudStereoPreset_OneMeterTwoMetersAway).
                                                                    AddEnumValue("Huge and bright", DebugHudStereoPreset_HugeAndBright).
                                                                    AddEnumValue("Huge and dim", DebugHudStereoPreset_HugeAndDim).
                                                                    SetNotify(this, &OWD::DebugHudSettingQuadPropChange);



    Menu.AddBool ("SDK HUD.Stereo Settings.Show Info", &DebugHudStereoGuideInfoEnable).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Width (m)", &DebugHudStereoGuideSize.x, 0.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Height (m)", &DebugHudStereoGuideSize.y, 0.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Position X (m)", &DebugHudStereoGuidePosition.x, -100.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Position Y (m)", &DebugHudStereoGuidePosition.y, -100.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Position Z (m)", &DebugHudStereoGuidePosition.z, -100.0f, 100.0f, 0.05f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Yaw (deg)",  &DebugHudStereoGuideYawPitchRollDeg.x, -180.0f, 180.0f, 1.0f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Pitch (deg)",&DebugHudStereoGuideYawPitchRollDeg.y, -180.0f, 180.0f, 1.0f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Roll (deg)", &DebugHudStereoGuideYawPitchRollDeg.z, -180.0f, 180.0f, 1.0f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

    Menu.AddFloat("SDK HUD.Stereo Settings.Color R", &DebugHudStereoGuideColor.x, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Color G", &DebugHudStereoGuideColor.y, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Color B", &DebugHudStereoGuideColor.z, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);
    Menu.AddFloat("SDK HUD.Stereo Settings.Color A", &DebugHudStereoGuideColor.w, 0.0f, 1.0f, 0.01f).SetNotify(this, &OWD::DebugHudSettingQuadPropChange);

}


void OculusWorldDemoApp::TraceAllMenuOptions(OptionVar*)
{
    // Trace the entire menu tree.
    // Must be called after the menu has been populated.

    std::string optionVarLines;
    Menu.Enumerate(optionVarLines);

    WriteLog("[OculusWorldDemoApp] *** Begin menu item trace ***\n"
             "[OculusWorldDemoApp] %s"
             "[OculusWorldDemoApp] *** End menu item trace ***\n", optionVarLines.c_str());

    const char* fileName = "OculusWorldDemo Menu Options.txt";
    FILE* file = fopen(fileName, "w");

    if (file == nullptr)
        WriteLog("[OculusWorldDemoApp] Failed to write menu trace to file: %s\n", fileName);
    else
    {
        fwrite(optionVarLines.c_str(), 1, optionVarLines.length(), file);
        fclose(file);
    }
}




// Return value is true on success.
// menuFullName = the path without the keyboard shortcut (e.g. "Perf HUD.Mode 'F11'" -> "Perf HUD.Mode")
// All comparisons are case insensitive.
// Enum types will look for the enum that matches newValue.
// Bool types will regard "", "0" and "false" as false, anything else as true.
// Trigger types ignore newValue. All menu items will trigger on setting.
// So for example these all work:
// SetMenuValue ( "perf hud.mODe", "veRsion INFO" );
// SetMenuValue ( "layers.cockpit ENABLE Bitfield", "31" );
// SetMenuValue ( "layers.cockpit HighQuality", "trUE" );
// SetMenuValue ( "Display.Mirror Window.width", "1285" );
// SetMenuValue ( "Render Target.piXEL Density", "0.75" );
bool OculusWorldDemoApp::SetMenuValue ( std::string menuFullName, std::string newValue )
{
    int menuFullNameLen = (int) menuFullName.length();
    OVR_ASSERT ( menuFullNameLen > 0 );
    if ( menuFullName[menuFullNameLen-1] == '\'' )
    {
        OVR_ASSERT ( "Don't include the keyboard shortcut in the menu name" );
    }

    OptionMenuItem *menuItem = Menu.FindMenuItem ( menuFullName );
    if ( menuItem != nullptr )
    {
        OVR_ASSERT ( !menuItem->IsMenu() );
        return menuItem->SetValue ( newValue );
    }
    else
    {
        // Probably didn't want that to happen.
        OVR_ASSERT ( false );
        return false;
    }
}

uint32_t RoundToMultiple(uint32_t value, uint32_t multiple)
{
    return (value + (multiple - 1)) & ~(multiple - 1);
}

static Sizei RoundToMultiple(Sizei dims, uint32_t multiple)
{
    OVR_ASSERT(dims.w >= 0 && dims.h >= 0);
    dims.w = (int32_t)RoundToMultiple((uint32_t)dims.w, multiple);
    dims.h = (int32_t)RoundToMultiple((uint32_t)dims.h, multiple);
    return dims;
}

ovrResult OculusWorldDemoApp::UpdateFovStencilMeshes()
{
    ovrResult error = ovrSuccess;

    // The data in there isn't ref counted so we clear out old data before allocating new ones.
    DestroyFovStencil();

    for (int curEye = 0; curEye < ovrEye_Count; curEye++)
    {
      if (FovStencilType >= 0)
      {
        ovrFovStencilMeshBuffer stencilMesh = {};

        ovrFovStencilDesc stencilDesc = {};
        stencilDesc.StencilType = (ovrFovStencilType)FovStencilType;
        stencilDesc.StencilFlags = 0;
        stencilDesc.Eye = (ovrEyeType)curEye;
        stencilDesc.FovPort = g_EyeFov[curEye];
        stencilDesc.HmdToEyeRotation = EyeRenderDesc[curEye].HmdToEyePose.Orientation;

        // First call will query the vertex count
        error = ovr_GetFovStencil(Session, &stencilDesc, &stencilMesh);
        
        if (stencilMesh.UsedVertexCount > 0 && error == ovrSuccess)
        {
          stencilMesh.VertexBuffer = new ovrVector2f[stencilMesh.UsedVertexCount];
          stencilMesh.IndexBuffer = new uint16_t[stencilMesh.UsedIndexCount];
          stencilMesh.AllocVertexCount = stencilMesh.UsedVertexCount;
          stencilMesh.AllocIndexCount = stencilMesh.UsedIndexCount;
          
          error = ovr_GetFovStencil(Session, &stencilDesc, &stencilMesh);
          OVR_ASSERT(stencilMesh.UsedVertexCount > 0);
          OVR_ASSERT_AND_UNUSED(OVR_SUCCESS(error), error);

          // Our model type doesn't support lines, so we will just hold onto the lines, and 
          // draw them later using RenderLines() calls
          if (stencilDesc.StencilType == ovrFovStencil_BorderLine)
          {
            FovStencilLinesX[curEye] = new float[stencilMesh.UsedIndexCount];
            FovStencilLinesY[curEye] = new float[stencilMesh.UsedIndexCount];
            FovStencilLineCount = stencilMesh.UsedIndexCount / 2;
            uint16_t* iBuffer = stencilMesh.IndexBuffer;
            ovrVector2f* vBuffer = stencilMesh.VertexBuffer;

            for (int curIndex = 0; curIndex < stencilMesh.UsedIndexCount; ++curIndex)
            {
              FovStencilLinesX[curEye][curIndex] = vBuffer[iBuffer[curIndex]].x;
              FovStencilLinesY[curEye][curIndex] = vBuffer[iBuffer[curIndex]].y;
            }
          }
          else
          {
            // Create mesh using data provided by SDK
            FovStencilMeshes[curEye][FovStencilMeshMode_WriteDepth] = *new Model(Prim_Triangles, "ViewStencilMesh_WriteDepth");
            FovStencilMeshes[curEye][FovStencilMeshMode_UndoDepth] = *new Model(Prim_Triangles, "ViewStencilMesh_UndoDepth");

            Color vertColor = 0;
            switch (FovStencilColor)
            {
              case FovStencilColor_Black: vertColor = 0xFF000000; break;
              case FovStencilColor_Green: vertColor = 0xFF00FF00; break;
              case FovStencilColor_Red:   vertColor = 0xFFFF0000; break;
              case FovStencilColor_White: vertColor = 0xFFFFFFFF; break;
              default: OVR_FAIL(); break;
            }

            for (int curMode = 0; curMode < FovStencilMeshMode_Count; curMode++)
            {
              float meshVertexDepth = 0.0f;
              switch (DepthModifier)
              {
              case OculusWorldDemoApp::NearLessThanFar:
                meshVertexDepth = (curMode == FovStencilMeshMode_WriteDepth) ? 0.0f : 1.0f;
                break;
              case OculusWorldDemoApp::FarLessThanNear:
              case OculusWorldDemoApp::FarLessThanNearAndInfiniteFarClip:
                meshVertexDepth = (curMode == FovStencilMeshMode_WriteDepth) ? 1.0f : 0.0f;
                break;
              default:
                OVR_FAIL();
                break;
              }

              for (int curVertIdx = 0; curVertIdx < stencilMesh.UsedVertexCount; ++curVertIdx)
              {
                FovStencilMeshes[curEye][curMode]->AddVertex(
                  stencilMesh.VertexBuffer[curVertIdx].x, stencilMesh.VertexBuffer[curVertIdx].y, meshVertexDepth,
                  vertColor, 0.0f, 0.0f);
              }

              OVR_ASSERT(stencilMesh.UsedIndexCount % 3 == 0); // Make sure we have the right number of indices for a tri-strip
              int numTris = stencilMesh.UsedIndexCount / 3;
              for (int triIdx = 0; triIdx < numTris; ++triIdx)
              {
                int curIndex = triIdx * 3;
                FovStencilMeshes[curEye][curMode]->AddTriangle(
                  (uint16_t)stencilMesh.IndexBuffer[curIndex],
                  (uint16_t)stencilMesh.IndexBuffer[curIndex + 1],
                  (uint16_t)stencilMesh.IndexBuffer[curIndex + 2]);
              }

              FovStencilMeshes[curEye][curMode]->Fill = pRender->GetSimpleFill();
            }
          }

          delete[] stencilMesh.VertexBuffer;
          delete[] stencilMesh.IndexBuffer;
        }
      }
    }


    return ovrSuccess;
}

ovrResult OculusWorldDemoApp::CalculateHmdValues()
{
    MultisampleEnabled = MultisampleRequested;
    if ( !SupportsMultisampling )
    {
        MultisampleEnabled = false;
    }
    if(!SupportsDepthSubmit)
    {
        Layer0Depth = false;
    }
    if ( GridDisplayMode == GridDisplay_GridDirect )
    {
        // Direct is direct 1:1 pixel mapping please.
        MultisampleEnabled = false;
    }


    // Initialize eye rendering information.
    // The viewport sizes are re-computed in case RenderTargetSize changed due to HW limitations.
    g_EyeFov[0] = HmdDesc.DefaultEyeFov[0];
    g_EyeFov[1] = HmdDesc.DefaultEyeFov[1];

    if (DiscreteHmdRotationEnable)
    {
      DiscreteRotationScissorIdealRenderSize[ovrEye_Left]  = ovr_GetFovTextureSize(Session, ovrEye_Left, g_EyeFov[0], DesiredPixelDensity);
      DiscreteRotationScissorIdealRenderSize[ovrEye_Right] = ovr_GetFovTextureSize(Session, ovrEye_Right, g_EyeFov[1], DesiredPixelDensity);

      // Save off for scissor calculation later
      DiscreteRotationSubFovPort[ovrEye_Left] = g_EyeFov[ovrEye_Left];
      DiscreteRotationSubFovPort[ovrEye_Right] = g_EyeFov[ovrEye_Right];

      float fovGrowAngle = OVR::DegreeToRad(DiscreteRotationThresholdAngle);
      fovGrowAngle *= DiscreteRotationFovMult;
      g_EyeFov[0] = OVR::FovPort::Expand(g_EyeFov[0], fovGrowAngle);
      g_EyeFov[1] = OVR::FovPort::Expand(g_EyeFov[1], fovGrowAngle);

      DiscreteHmdRotationNeedsInit = true;
    }

    if (FovPortOverrideResetToDefault)
    {
      FovPortOverride = g_EyeFov[0];
      FovPortOverrideResetToDefault = false;
    }

    // Adjustable FOV
    // Most apps should use the default, but reducing Fov does reduce rendering cost
    // Increasing FOV can create a slightly more expansive view in the periphery at a performance penalty
    if (FovPortOverrideEnable)
    {
        g_EyeFov[0] = FovPortOverride;
        g_EyeFov[1] = FovPortOverride;
        // FovPortOverride is for the left eye, so we reflect the values for the right eye
        g_EyeFov[1].LeftTan = FovPortOverride.RightTan;
        g_EyeFov[1].RightTan = FovPortOverride.LeftTan;
    }
    else if (FovScaling > 0)
    {   // Scalings above zero lerps between the Default and Max FOV values
        g_EyeFov[0]. LeftTan = (HmdDesc.MaxEyeFov[0]. LeftTan - g_EyeFov[0]. LeftTan) * FovScaling + g_EyeFov[0]. LeftTan;
        g_EyeFov[0].RightTan = (HmdDesc.MaxEyeFov[0].RightTan - g_EyeFov[0].RightTan) * FovScaling + g_EyeFov[0].RightTan;
        g_EyeFov[0].   UpTan = (HmdDesc.MaxEyeFov[0].   UpTan - g_EyeFov[0].   UpTan) * FovScaling + g_EyeFov[0].   UpTan;
        g_EyeFov[0]. DownTan = (HmdDesc.MaxEyeFov[0]. DownTan - g_EyeFov[0]. DownTan) * FovScaling + g_EyeFov[0]. DownTan;

        g_EyeFov[1]. LeftTan = (HmdDesc.MaxEyeFov[1]. LeftTan - g_EyeFov[1]. LeftTan) * FovScaling + g_EyeFov[1]. LeftTan;
        g_EyeFov[1].RightTan = (HmdDesc.MaxEyeFov[1].RightTan - g_EyeFov[1].RightTan) * FovScaling + g_EyeFov[1].RightTan;
        g_EyeFov[1].   UpTan = (HmdDesc.MaxEyeFov[1].   UpTan - g_EyeFov[1].   UpTan) * FovScaling + g_EyeFov[1].   UpTan;
        g_EyeFov[1]. DownTan = (HmdDesc.MaxEyeFov[1]. DownTan - g_EyeFov[1]. DownTan) * FovScaling + g_EyeFov[1]. DownTan;
    }
    else if (FovScaling < 0)
    {   // Scalings below zero lerp between Default and an arbitrary minimum FOV.
        // but making sure to shrink to a symmetric FOV first
        float min_fov = 0.176f;  // 10 degrees minimum
        float max_fov = Alg::Max(g_EyeFov[0].LeftTan, Alg::Max(g_EyeFov[0].RightTan, Alg::Max(g_EyeFov[0].UpTan, g_EyeFov[0].DownTan)));
        max_fov = (max_fov - min_fov) * FovScaling + max_fov;
        g_EyeFov[0]. LeftTan = Alg::Min(max_fov, g_EyeFov[0]. LeftTan);
        g_EyeFov[0].RightTan = Alg::Min(max_fov, g_EyeFov[0].RightTan);
        g_EyeFov[0].   UpTan = Alg::Min(max_fov, g_EyeFov[0].   UpTan);
        g_EyeFov[0]. DownTan = Alg::Min(max_fov, g_EyeFov[0]. DownTan);

        max_fov = Alg::Max(g_EyeFov[1].LeftTan, Alg::Max(g_EyeFov[1].RightTan, Alg::Max(g_EyeFov[1].UpTan, g_EyeFov[1].DownTan)));
        max_fov = (max_fov - min_fov) * FovScaling + max_fov;
        g_EyeFov[1]. LeftTan = Alg::Min(max_fov, g_EyeFov[1]. LeftTan);
        g_EyeFov[1].RightTan = Alg::Min(max_fov, g_EyeFov[1].RightTan);
        g_EyeFov[1].   UpTan = Alg::Min(max_fov, g_EyeFov[1].   UpTan);
        g_EyeFov[1]. DownTan = Alg::Min(max_fov, g_EyeFov[1]. DownTan);
    }


    // Shutter type. Useful for debugging.
    const char *pcShutterType = ovr_GetString(Session, "server:ShutterType", "Unknown");
    ShutterType = pcShutterType;

    // If using our driver, display status overlay messages.
    if (NotificationTimeout != 0.0f)
    {
        GetPlatformCore()->SetNotificationOverlay(0, 28, 8,
           "Rendering to the HMD - Please put on your Rift");
        GetPlatformCore()->SetNotificationOverlay(1, 24, -8,
            MirrorToWindow ? "'M' - Mirror to Window [On]" : "'M' - Mirror to Window [Off]");
    }

    ovrResult error = ovrSuccess;

    // Normally, the cameras live at their respective eyes.
    CamFromEye[0] = Posef::Identity();
    CamFromEye[1] = Posef::Identity();

    if (MonoscopicRenderMode != Mono_Off)
    {
        // MonoscopicRenderMode does two things here:
        //  1) Sets FOV to maximum symmetrical FOV based on both eyes
        //  2) Uses only the Left texture for rendering.
        //  (it also plays with IPD and/or player scaling, but that is done elsewhere)

        g_EyeFov[0] = FovPort::Uncant(g_EyeFov[0], EyeRenderDesc[0].HmdToEyePose.Orientation);
        g_EyeFov[1] = FovPort::Uncant(g_EyeFov[1], EyeRenderDesc[1].HmdToEyePose.Orientation);
        g_EyeFov[0] = FovPort::Max(g_EyeFov[0], g_EyeFov[1]);
        g_EyeFov[1] = g_EyeFov[0];

        CamFovPort[0] = g_EyeFov[0];
        CamFovPort[1] = g_EyeFov[1];

        Sizei recommendedTexSize = ovr_GetFovTextureSize(Session, ovrEye_Left, g_EyeFov[0], DesiredPixelDensity);

        Sizei textureSize = EnsureRendertargetAtLeastThisBig(Rendertarget_Left, recommendedTexSize, (Layer0GenMipCount != 1), Layer0GenMipCount, error);
        if (error != ovrSuccess)
            return error;

        CamRenderSize[0] = Sizei::Min(textureSize, recommendedTexSize);
        CamRenderSize[1] = CamRenderSize[0];

        // Store texture pointers that will be passed for rendering.
        CamTexture[0] = RenderTargets[Rendertarget_Left].pColorTex;
        // Right eye is the same.
        CamTexture[1] = CamTexture[0];

        // Store texture pointers that will be passed for rendering.
        CamDepthTexture[0] = RenderTargets[Rendertarget_Left].pDepthTex;
        // Right eye is the same.
        CamDepthTexture[1] = CamDepthTexture[0];

        CamRenderViewports[0] = Recti(Vector2i(0), CamRenderSize[0]);
        CamRenderViewports[1] = CamRenderViewports[0];
        CamRenderPoseCount = 1;
    }
    else
    {
        // Configure Stereo settings. Default pixel density is 1.0f.
        Sizei recommendedTex0Size = ovr_GetFovTextureSize(Session, ovrEye_Left, g_EyeFov[0], DesiredPixelDensity);
        Sizei recommendedTex1Size = ovr_GetFovTextureSize(Session, ovrEye_Right, g_EyeFov[1], DesiredPixelDensity);
        CamRenderPoseCount = 2;

        // Here, the cameras still live at the eye pos+orn.
        CamFromEye[0] = Posef::Identity();
        CamFromEye[1] = Posef::Identity();
        CamFovPort[0] = g_EyeFov[0];
        CamFovPort[1] = g_EyeFov[1];

        if (RendertargetIsSharedByBothEyes)
        {
            Sizei  rtSize(recommendedTex0Size.w + recommendedTex1Size.w,
                Alg::Max(recommendedTex0Size.h, recommendedTex1Size.h));

            // Use returned size as the actual RT size may be different due to HW limits.
            rtSize = EnsureRendertargetAtLeastThisBig(Rendertarget_BothEyes, rtSize, (Layer0GenMipCount != 1), Layer0GenMipCount, error);
            if (error != ovrSuccess)
                return error;

            // Don't draw more then recommended size; this also ensures that resolution reported
            // in the overlay HUD size is updated correctly for FOV/pixel density change.
            CamRenderSize[0] = Sizei::Min(Sizei(rtSize.w / 2, rtSize.h), recommendedTex0Size);
            CamRenderSize[1] = Sizei::Min(Sizei(rtSize.w / 2, rtSize.h), recommendedTex1Size);

            // Store texture pointers that will be passed for rendering.
            // Same texture is used, but with different viewports.
            CamTexture[0] = RenderTargets[Rendertarget_BothEyes].pColorTex;
            CamDepthTexture[0] = RenderTargets[Rendertarget_BothEyes].pDepthTex;
            CamRenderViewports[0] = Recti(Vector2i(0), CamRenderSize[0]);
            CamTexture[1] = RenderTargets[Rendertarget_BothEyes].pColorTex;
            CamDepthTexture[1] = RenderTargets[Rendertarget_BothEyes].pDepthTex;
            CamRenderViewports[1] = Recti(Vector2i((rtSize.w + 1) / 2, 0), CamRenderSize[1]);
        }
        else if (GridDisplayMode == GridDisplay_GridDirect)
        {
            Sizei rtSize = HmdDesc.Resolution;

            rtSize = EnsureRendertargetAtLeastThisBig(Rendertarget_BothEyes, rtSize, (Layer0GenMipCount != 1), Layer0GenMipCount, error);
            if (HandleOvrError(error))
                return error;

            CamRenderSize[0] = rtSize;
            CamRenderSize[1] = rtSize;

            // Store texture pointers that will be passed for rendering.
            // Same texture is used, but with different viewports.
            CamTexture[0]         = RenderTargets[Rendertarget_BothEyes].pColorTex;
            CamDepthTexture[0]    = RenderTargets[Rendertarget_BothEyes].pDepthTex;
            CamRenderViewports[0] = rtSize;
            CamTexture[1]         = RenderTargets[Rendertarget_BothEyes].pColorTex;
            CamDepthTexture[1]    = RenderTargets[Rendertarget_BothEyes].pDepthTex;
            CamRenderViewports[1] = rtSize;
        }
        else
        {
            Sizei tex0Size = EnsureRendertargetAtLeastThisBig(Rendertarget_Left, recommendedTex0Size, (Layer0GenMipCount != 1), Layer0GenMipCount, error);
            if (error != ovrSuccess)
                return error;

            Sizei tex1Size = EnsureRendertargetAtLeastThisBig(Rendertarget_Right, recommendedTex1Size, (Layer0GenMipCount != 1), Layer0GenMipCount, error);
            if (error != ovrSuccess)
                return error;

            CamRenderSize[0] = Sizei::Min(tex0Size, recommendedTex0Size);
            CamRenderSize[1] = Sizei::Min(tex1Size, recommendedTex1Size);

            // Store texture pointers and viewports that will be passed for rendering.
            CamTexture[0] = RenderTargets[Rendertarget_Left].pColorTex;
            CamDepthTexture[0] = RenderTargets[Rendertarget_Left].pDepthTex;
            CamRenderViewports[0] = Recti(CamRenderSize[0]);
            CamTexture[1] = RenderTargets[Rendertarget_Right].pColorTex;
            CamDepthTexture[1] = RenderTargets[Rendertarget_Right].pDepthTex;
            CamRenderViewports[1] = Recti(CamRenderSize[1]);

        }
    }

    // Select the MSAA or non-MSAA version of each render target
    for (int rtIdx = 0; rtIdx < Rendertarget_LAST; rtIdx++)
    {
        DrawEyeTargets[rtIdx] = &RenderTargets[rtIdx];
    }

	  // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyePose) may change at runtime.
    EyeRenderDesc[0] = ovr_GetRenderDesc(Session, ovrEye_Left, g_EyeFov[0]);
    EyeRenderDesc[1] = ovr_GetRenderDesc(Session, ovrEye_Right, g_EyeFov[1]);

    unsigned int projectionModifier = createProjectionModifier();

    // Calculate projections
    // TODO: move this earlier I think!
    for ( int camNum = 0; camNum < CamRenderPose_Count; camNum++ )
    {
        int eyeNum = camNum & 1;

        ovrEyeRenderDesc camRenderDesc = ovr_GetRenderDesc(Session, (eyeNum==0) ? ovrEye_Left : ovrEye_Right, CamFovPort[camNum]);
        CamFovPort[camNum] = camRenderDesc.Fov;
        CamProjection[camNum] = ovrMatrix4f_Projection(CamFovPort[camNum], NearClip, FarClip, projectionModifier);

        SubpixelJitterBaseCamFovPort[camNum] = CamFovPort[camNum];  // Save off for later manipulation
    }
    PosTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(CamProjection[0], projectionModifier);





    UpdateFovStencilMeshes();

    // all done
    HmdSettingsChanged = false;

    return ovrSuccess;
}

bool OculusWorldDemoApp::ShouldLoadedTexturesBeSrgb(EyeTextureFormatOpt eyeTextureFormat)
{
    switch (eyeTextureFormat)
    {
    case EyeTextureFormat_RGBA8_SRGB:   // fall-thru
    //case EyeTextureFormat_BGRA8_SRGB:   // fall-thru
    //case EyeTextureFormat_BGRX8_SRGB:   // fall-thru
    case EyeTextureFormat_RGBA16F:      return true;    // works as if it's an sRGB format
    case EyeTextureFormat_R11G11B10F:   return true;    // works as if it's an sRGB format

    //case EyeTextureFormat_B5G6R5:       // fall-thru
    //case EyeTextureFormat_BGR5A1:       // fall-thru
    //case EyeTextureFormat_BGRA4:        // fall-thru
    //case EyeTextureFormat_BGRX8:        // fall-thru
    //case EyeTextureFormat_BGRA8:        // fall-thru
    case EyeTextureFormat_RGBA8:        return false;
    }

    OVR_FAIL();
    return false;
}

int OculusWorldDemoApp::GetRenderDeviceTextureFormatForEyeTextureFormat(EyeTextureFormatOpt eyeTextureFormat)
{
    switch (eyeTextureFormat)
    {
    //case EyeTextureFormat_B5G6R5:       return Texture_B5G6R5;
    //case EyeTextureFormat_BGR5A1:       return Texture_BGR5A1;
    //case EyeTextureFormat_BGRA4:        return Texture_BGRA4;
    case EyeTextureFormat_RGBA8:        return Texture_RGBA8;
    case EyeTextureFormat_RGBA8_SRGB:   return Texture_RGBA8 | Texture_SRGB;
    //case EyeTextureFormat_BGRA8:        return Texture_BGRA8;
    //case EyeTextureFormat_BGRA8_SRGB:   return Texture_BGRA8 | Texture_SRGB;
    //case EyeTextureFormat_BGRX8:        return Texture_BGRX;
    //case EyeTextureFormat_BGRX8_SRGB:   return Texture_BGRX | Texture_SRGB;
    case EyeTextureFormat_RGBA16F:      return Texture_RGBA16f;
    case EyeTextureFormat_R11G11B10F:   return Texture_R11G11B10f;
    }

    OVR_FAIL();
    return 0;
}

// Returns the actual size present.
// genMipCount == 0 generates mips by the SDK
// genMipCount > 1 generates mips by OWD.
Sizei OculusWorldDemoApp::EnsureRendertargetAtLeastThisBig(int rtNum, Sizei requestedSize, bool genMips, int genMipCount, ovrResult& error)
{
    OVR_ASSERT((rtNum >= 0) && (rtNum < Rendertarget_LAST));

    Sizei currentSize;
    // Texture size that we already have might be big enough.
    Sizei newRTSize;

    RenderTarget& rt = RenderTargets[rtNum];
    if (!rt.pColorTex)
    {
        // Hmmm... someone nuked my texture. Rez change or similar. Make sure we reallocate.
        currentSize = Sizei(0);
        newRTSize = requestedSize;
    }
    else
    {
        currentSize = newRTSize = Sizei(rt.pColorTex->GetWidth(), rt.pColorTex->GetHeight());
    }

    // %50 linear growth each time is a nice balance between being too greedy
    // for a 2D surface and too slow to prevent fragmentation.
    while ( newRTSize.w < requestedSize.w )
    {
        newRTSize.w += newRTSize.w/2;
    }
    while ( newRTSize.h < requestedSize.h )
    {
        newRTSize.h += newRTSize.h/2;
    }

    // Put some sane limits on it. 4k x 4k is fine for most modern video cards.
    // Nobody should be messing around with surfaces smaller than 64x64 pixels these days.
    newRTSize = Sizei::Max(Sizei::Min(newRTSize, Sizei(4096)), Sizei(64));

    // Since mips will be generated for this layer, we want to make sure it's a power of two
    // in both dimensions or the results won't look as sexy
    if (genMips)
    {
        if (genMipCount > 1)
        {
            // limit mip count to a logical value given the RTs dimensions
            int maxMipCount = Render::GetNumMipLevels(newRTSize.w, newRTSize.h);
            genMipCount = maxMipCount < genMipCount ? maxMipCount : genMipCount;

            // if we're not going to generate all the mip levels, then just make sure
            // it's a multiple of 2^mips
            newRTSize = RoundToMultiple(newRTSize, 1 << genMipCount);
        }
    }
    else
    {
        genMipCount = 1;    // do nothing
    }

    int allocatedDepthFormat = (rt.pDepthTex != NULL ? rt.pDepthTex->GetFormat() & Texture_DepthMask : 0);

    uint64_t desiredDepthFormat = Texture_Depth32f;
    switch (DepthFormatOption)
    {
    case DepthFormatOption_D32f:    desiredDepthFormat = Texture_Depth32f; break;
    case DepthFormatOption_D24_S8:  desiredDepthFormat = Texture_Depth24Stencil8; break;
    case DepthFormatOption_D16:     desiredDepthFormat = Texture_Depth16; break;
    case DepthFormatOption_D32f_S8: desiredDepthFormat = Texture_Depth32fStencil8; break;
    default:    OVR_ASSERT(false);
    }
    bool depthFormatChanged = UseDepth[rtNum] && (allocatedDepthFormat != desiredDepthFormat);

    // Does that require actual reallocation?
    if (currentSize != newRTSize ||
        depthFormatChanged ||
        genMipCount != GenMipCount[rtNum])
    {
        GenMipCount[rtNum] = genMipCount;

        desiredDepthFormat |= Texture_SampleDepth;
        uint64_t colorFormat = GetRenderDeviceTextureFormatForEyeTextureFormat(EyeTextureFormat);
        colorFormat |= Texture_RenderTarget;

        // We need to render at least one frame to the newly-allocated texture, otherwise we just get junk.
        NeverRenderedIntoEyeTextures = true;

        bool willUseMsaa = MultisampleEnabled && SupportsMultisampling && AllowMsaaTargets[rtNum];
        int msaaRate = willUseMsaa ? 4 : 1;

        int genMipsFlag = 0;

        // In OWD, mip count == 0 means let the SDK generate the mips
        if (genMipCount == 0) {
          genMipsFlag = Texture_GenMipmapsBySdk;
          genMipCount = 1; // SDK will be generating the mips so we clear it here
        }

        if (willUseMsaa)
        {
            genMipCount = 0;  // we can't gen mips on MSAA textures unless done by the SDK (above)
        }
        else if (genMipCount > 1)
        {
            genMipsFlag = Texture_GenMipmaps;
        }

        // Now create the textures, color first, then depth
        rt.pColorTex = *pRender->CreateTexture(colorFormat | msaaRate | genMipsFlag | Texture_SwapTextureSet,
                                                newRTSize.w, newRTSize.h, NULL, genMipCount, &error);

        if (HandleOvrError(error))
            return Sizei(0, 0);

        rt.pColorTex->SetSampleMode(Sample_ClampBorder | Sample_Linear);

        if (UseDepth[rtNum] && Layer0Depth)
        {
            rt.pDepthTex = *pRender->CreateTexture(desiredDepthFormat | msaaRate | Texture_SwapTextureSet,
                                                    newRTSize.w, newRTSize.h, NULL, 1, &error);
            if (HandleOvrError(error))
                return Sizei(0, 0);

            rt.pDepthTex->SetSampleMode(Sample_ClampBorder | Sample_Linear);
        }
        else
        {
            rt.pDepthTex = nullptr; // it will be created and assigned on SetRenderTarget
        }

    }

    return newRTSize;
}


//-----------------------------------------------------------------------------
// ***** Message Handlers

void OculusWorldDemoApp::OnResize(int width, int height)
{
    // Do not resize if in the mixed reality capture mode. If non-interactive,
    // do not resize to ensure consistent output mirror window texture size
    // when using scripted screen capture.
    if (!IsMixedRealityCaptureMode && InteractiveMode)
    {
        WindowSize = Sizei(width, height);
        HmdSettingsChanged = true;
        WindowSizeChange();
    }
}

void OculusWorldDemoApp::OnMouseMove(int x, int y, int modifiers)
{
    OVR_UNUSED(y);
    if(modifiers & Mod_MouseRelative)
    {
        // Get Delta
        int dx = x;

        // Apply to rotation. Subtract for right body frame rotation,
        // since yaw rotation is positive CCW when looking down on XZ plane.
        ThePlayer.AdjustBodyYaw((Sensitivity * dx) / -360.0f);
    }
}


void OculusWorldDemoApp::OnKey(OVR::KeyCode key, int chr, bool down, int modifiers)
{
    switch (key)
    {
    case Key_F6:
        if (!down)
        {
            IsMixedRealityCaptureMode = !IsMixedRealityCaptureMode;
            if (IsMixedRealityCaptureMode)
                IsMixedRealityCaptureMode = SetupMixedRealityCapture();
            if (!IsMixedRealityCaptureMode)
                CleanupMixedRealityCapture();
        }
        return;

    case Key_F12:
        if (!down)
        {
            // 0: none 1: hands 2: touches  3: both
            // cycles through 0->1->2->3
            RenderControllerFlag = RenderControllerType((RenderControllerFlag + 1) % RenderController_Count);
        }
        return;
    }


    if (Menu.OnKey(key, chr, down, modifiers))
        return;

    // Handle player movement keys.
    if (ThePlayer.HandleMoveKey(key, down))
        return;

    switch(key)
    {
    case Key_F2:
        if (!down && (modifiers == 0))
        {
            Recording = !Recording;
            if (modifiers == 0)
            {
                ovr_SetBool(Session, "server:RecordingEnabled", Recording);
            }
            else if ((modifiers & Mod_Control))
            {
                ovr_SetBool(Session, "server:RecordingWithImagesEnabled", Recording);
            }
        }
        break;

    case Key_F3:
        if (!down && (modifiers==0))
        {
            Replaying = !Replaying;
            ovr_SetBool(Session, "server:ReplayEnabled", Replaying);
        }
        break;

    case Key_Q:
        if (down && (modifiers & Mod_Control))
            pPlatform->Exit(0);
        break;

    case Key_Space:
        if (!down)
        {
          if (modifiers & Mod_Shift)
            TextScreen = (TextScreenType)((TextScreen - 1) < 0 ? (Text_Count - 1) : (TextScreen - 1));
          else
            TextScreen = (TextScreenType)((TextScreen + 1) % Text_Count);
        }
        break;

    // Distortion correction adjustments
    case Key_Backslash:
        break;
        // Holding down Shift key accelerates adjustment velocity.
    case Key_Shift:
        ShiftDown = down;
        break;
    case Key_Control:
        CtrlDown = down;
        break;

       // Reset the camera position in case we get stuck
    case Key_T:
        if (down)
        {
            struct {
                float  x, z;
                float  YawDegrees;
            }  Positions[] =
            {
               // x         z           Yaw
                { 7.7f,     -1.0f,      0.0f },         // The starting position.
                { 10.0f,    10.0f,      90.0f  },      // Outside, looking at some trees.
                { 19.26f,   5.43f,      22.0f  },      // Outside, looking at the fountain.
                { 24.26f,   -2.0f,      90.0f },       // Looking at house
            };

            static int nextPosition = 0;
            nextPosition = (nextPosition + 1) % (sizeof(Positions)/sizeof(Positions[0]));

            ThePlayer.SetBodyPos(Vector3f(Positions[nextPosition].x, 0.0f, Positions[nextPosition].z), true);
            ThePlayer.BodyYaw = DegreeToRad( Positions[nextPosition].YawDegrees );
        }
        break;

    case Key_BracketLeft: // Control-Shift-[  --> Test OVR_ASSERT
        if(down && (modifiers & Mod_Control) && (modifiers & Mod_Shift))
            OVR_ASSERT(key != Key_BracketLeft);
        break;

    case Key_BracketRight: // Control-Shift-]  --> Test exception handling
        if(down && (modifiers & Mod_Control) && (modifiers & Mod_Shift))
            OVR::CreateException(OVR::kCETAccessViolation);
        break;


    case Key_F:
        if (down)
        {
            if (TextScreen == Text_BoundaryInfo)
            {
                TextScreen = Text_None;
            }
            else
            {
                TextScreen = Text_BoundaryInfo;
            }
        }

     default:
        break;
    }
}

//-----------------------------------------------------------------------------


Matrix4f OculusWorldDemoApp::CalculateViewFromPose(const Posef& pose)
{
    Posef worldPose = ThePlayer.VirtualWorldTransformfromRealPose(pose, TrackingOriginType);

    // Rotate and position View Camera
    Vector3f up      = worldPose.Rotation.Rotate(UpVector);
    Vector3f forward = worldPose.Rotation.Rotate(ForwardVector);

    // Transform the position of the center eye in the real world (i.e. sitting in your chair)
    // into the frame of the player's virtual body.

    Vector3f viewPos = ForceZeroHeadMovement ? ThePlayer.GetBodyPos(TrackingOriginType) : worldPose.Translation;

    Matrix4f view = Matrix4f::LookAtRH(viewPos, viewPos + forward, up);
    return view;
}

static void WasteCpuTime(float timeToWasteMs)
{
    // keep looping until we've wasted enough CPU time
    double completionTime = ovr_GetTimeInSeconds() + (timeToWasteMs * 0.001);

    while (ovr_GetTimeInSeconds() < completionTime)
        OVR_PROCESSOR_PAUSE();
}

void OculusWorldDemoApp::FlushIfApplicable(DrawFlushModeEnum flushMode, int& currDrawFlushCount)
{
    if (DrawFlushMode == flushMode && currDrawFlushCount < DrawFlushCount)
    {
        pRender->Flush();
        currDrawFlushCount++;
    }
}


void OculusWorldDemoApp::HandleBoundaryControls()
{
    switch (TextScreen)
    {
    case Text_TouchState:
    case Text_GamepadState:
    case Text_ActiveControllerState:
      return; // Don't switch to boundary info if we're in any of the gamepad state menus
    }

    // Check if Touch B button was pressed to activate menu
    bool buttonPressed_B = ((InputState.Buttons & ovrButton_B) != 0);
    bool wasButtonPressed_B = ((LastInputState.Buttons & ovrButton_B) != 0);
    bool justPressedB = (!wasButtonPressed_B && buttonPressed_B);
    if (justPressedB)
    {
        if (TextScreen != Text_BoundaryInfo)
        {
            TextScreen = Text_BoundaryInfo;
        }
        else
        {
            TextScreen = Text_None;
        }
    }

    // If menu isn't present, ignore other input
    if (TextScreen != Text_BoundaryInfo) return;

    bool buttonPressed_X = ((InputState.Buttons & ovrButton_X) != 0);
    bool buttonPressed_A = ((InputState.Buttons & ovrButton_A) != 0);
    bool wasButtonPressed_X = ((LastInputState.Buttons & ovrButton_X) != 0);
    bool wasButtonPressed_A = ((LastInputState.Buttons & ovrButton_A) != 0);
    bool justPressedX = (!wasButtonPressed_X && buttonPressed_X);
    bool justPressedA = (!wasButtonPressed_A && buttonPressed_A);


    // Touch A Button, toggles force boundary visible
    if (justPressedA)
    {
        static ovrBool ForceBoundaryVisible = false;
        ForceBoundaryVisible = !ForceBoundaryVisible;
        ovr_RequestBoundaryVisible(Session, ForceBoundaryVisible);
    }

    // Touch X button, reset color to default
    if (justPressedX)
    {
        ovr_ResetBoundaryLookAndFeel(Session);
    }
}

void OculusWorldDemoApp::HandleHaptics()
{
    // The engine can queue up to 800ms (256 samples on Rift, 400 samples on LCON), longer queue yields higher latency

    // A Haptics sample represents a wave amplitude from [0, 100]% as a uint8_t [0, 255]
    // A Haptics buffer can contain up to TouchHapticsDesc.SubmitMaxSamples samples, which are sent to the HW in packets
    // Total Haptics latency, from submit to when it plays on Touch, varies between [9, 18]ms

    // For low latency, sample count should be (safe Haptics latency = TouchHapticsDesc.QueueMinSizeToAvoidStarvation) + (safe frame latency = 4 @90hz, 6 @60hz)
    //const int32_t kLowLatencyBufferSizeInSamples = 5 + 6;

    // Samples to be filled by different haptics modes
    static uint8_t amplitude = 0;

    ovrResult result;
    bool buttonPressed_A = ((InputState.Buttons & ovrButton_A) != 0);
    bool buttonPressed_X = ((InputState.Buttons & ovrButton_X) != 0);
    bool wasButtonPressed_A = ((LastInputState.Buttons & ovrButton_A) != 0);
    bool wasButtonPressed_X = ((LastInputState.Buttons & ovrButton_X) != 0);
    float handTrigger[] = {
        std::max(0.0f, InputState.HandTrigger[0] - 0.02f),
        std::max(0.0f, InputState.HandTrigger[1] - 0.02f) };
    float lastHandTrigger[] = {
        std::max(0.0f, LastInputState.HandTrigger[0] - 0.02f),
        std::max(0.0f, LastInputState.HandTrigger[1] - 0.02f) };

    // Touch X Button, changes haptics mode
    if (!wasButtonPressed_X && buttonPressed_X)
    {
        HapticsMode = (HapticsMode + 1) % kMaxHapticsModes;
    }

    // Send haptics to controllers
    for (size_t t = 0; t < 2; ++t)
    {
        const char* kTouchStr[] = { "LeftTouch", "RightTouch" };
        ovrControllerType touchController[] = { ovrControllerType_LTouch, ovrControllerType_RTouch };

        std::vector<uint8_t> samples;
        ovrHapticsPlaybackState state;
        memset(&state, 0, sizeof(state));

        // Multiple haptics modes to test
        switch (HapticsMode)
        {
        case 0:
            continue;
        // Constant Vibration
        case 1:
            if (lastHandTrigger[t] == 0.0f && handTrigger[t] == 0.0f)
                continue;

            ovr_SetControllerVibration(Session, touchController[t], 1.0f, handTrigger[t] * 1.025f);
            continue;
        break;

        // Low-latency, Short-queue, per-touch amplitude sent by the Hand Trigger
        case 2:
            if (handTrigger[t] == 0.0f)
                continue;
            amplitude = (uint8_t)std::min(255, (int32_t)round(handTrigger[t] * 255) + 8);

            result = ovr_GetControllerVibrationState(Session, touchController[t], &state);
            if (result != ovrSuccess || state.SamplesQueued >= LowLatencyBufferSizeInSamples[t])
            {
                WriteLog("[OculusWorldDemoApp] %s Haptics skipped. Queue size %d", kTouchStr[t], state.SamplesQueued);
                continue;
            }

            for (int32_t i = 0; i < LowLatencyBufferSizeInSamples[t]; ++i)
                samples.push_back(amplitude);
        break;

        // High-latency, Long-queue, per-touch amplitude sent by the Hand Trigger
        case 3:
            if (handTrigger[t] == 0.0f)
                continue;
            amplitude = (uint8_t)std::min(255, (int32_t)round(handTrigger[t] * 255) + 8);

            result = ovr_GetControllerVibrationState(Session, touchController[t], &state);
            if (result != ovrSuccess)
            {
                continue;
            }

            for (int32_t i = 0; i < LowLatencyBufferSizeInSamples[t]; ++i)
                samples.push_back(amplitude);
            break;

        // Low-latency, short-queue, both-touches increase mod amplitude
        case 4:
            if (!buttonPressed_A)
                continue;

            if (!wasButtonPressed_A && buttonPressed_A)
                amplitude = 0;
            amplitude++;

            result = ovr_GetControllerVibrationState(Session, touchController[t], &state);
            if (result != ovrSuccess || state.SamplesQueued >= LowLatencyBufferSizeInSamples[t])
            {
                WriteLog("[OculusWorldDemoApp] %s Haptics skipped. Queue size %d", kTouchStr[t], state.SamplesQueued);
                continue;
            }

            for (int32_t i = 0; i < LowLatencyBufferSizeInSamples[t]; ++i)
                samples.push_back(amplitude);
            break;

        // Low latency 1 click SHORT
        case 5:
            if (!wasButtonPressed_A && buttonPressed_A)
                for (int32_t i = 0; i < 1; ++i)
                    samples.push_back(255);
            break;

        // Low latency 1 click LONG
        case 6:
            if (!wasButtonPressed_A && buttonPressed_A)
                for (int32_t i = 0; i < 4; ++i)
                    samples.push_back(255);
            break;

        // Full Package
        case 7:
            if (!wasButtonPressed_A && buttonPressed_A)
                for (int32_t i = 0; i < 20; ++i)
                    samples.push_back(255);
            break;

        // First and Last buffer samples
        case 8:
            if (!wasButtonPressed_A && buttonPressed_A)
            {
                samples.push_back(255);
                // NOTE: Max buffer size for Remote Rendering is 25
                for (int32_t i = 0; i < 25-2; ++i)
                    samples.push_back(0);
                samples.push_back(255);
            }
            break;

        // Test Frequency/4
        case 9:
            if (!wasButtonPressed_A && buttonPressed_A)
                for (int32_t i = 0; i < 20; ++i)
                    samples.push_back( (i/4 % 2) * 255 );
            break;

        // XBox gamepad rumble
        case 11:
          ovr_SetControllerVibration(Session, ovrControllerType_XBox, handTrigger[0] * 1.025f, handTrigger[1] * 1.025f);
          break;

        // Test ovrHapticsClip playback
        case 10:
            if (!buttonPressed_A || TouchHapticsClip.Samples == nullptr || TouchHapticsClip.SamplesCount <= 0)
                continue;

            result = ovr_GetControllerVibrationState(Session, touchController[t], &state);
            if (result != ovrSuccess || state.SamplesQueued >= LowLatencyBufferSizeInSamples[t])
            {
                WriteLog("[OculusWorldDemoApp] %s Haptics skipped. Queue size %d", kTouchStr[t], state.SamplesQueued);
                continue;
            }

            const uint8_t* hapticsSamples = (const uint8_t*)TouchHapticsClip.Samples;
            for (int32_t i = 0; i < LowLatencyBufferSizeInSamples[t]; ++i)
            {
                int32_t hapticsPlayIndex = (TouchHapticsPlayIndex + i) % TouchHapticsClip.SamplesCount;
                samples.push_back(*(hapticsSamples + hapticsPlayIndex));
            }
            TouchHapticsPlayIndex = (TouchHapticsPlayIndex + (int32_t)samples.size()) % TouchHapticsClip.SamplesCount;
            break;
        }

        if (samples.size() > 0)
        {
            ovrHapticsBuffer buffer;
            buffer.SubmitMode = ovrHapticsBufferSubmit_Enqueue;
            buffer.SamplesCount = (uint32_t)samples.size();
            buffer.Samples = samples.data();
            result = ovr_SubmitControllerVibration(Session, touchController[t], &buffer);
            if (result != ovrSuccess)
            {
                // Something bad happened
                WriteLog("[OculusWorldDemoApp] %s: Haptics submit failed %d", kTouchStr[t], result);
            }
        }
    }
}

void OculusWorldDemoApp::Exit(int exitCode)
{
    WriteLog("[OculusWorldDemoApp] Exiting with code %d", exitCode);
    OnQuitRequest(exitCode);
}

void OculusWorldDemoApp::OnIdle()
{
    // Check to see if we are being asked to quit.
    ovrSessionStatus sessionStatus = {};
    ovr_GetSessionStatus(Session, &sessionStatus);

    if (sessionStatus.ShouldQuit)
    {
        Exit();
        return;
    }

    // T36991758: Long term solution to this problem is to make sure OWD is
    // always visible during automation.
    //if (sessionStatus.IsVisible) {
        // Run a single step of the OWD Script handler. It will do nothing if there is no script executing.
        // Don't execute while not visible, because currently we don't handle the case of switching 
        // textures while not visible.
        OwdScript.Step();
    //}
    if (sessionStatus.ShouldRecenter)
    {
        ResetHmdPose();
    }

    if (!HmdDisplayAcquired)
    {
        int result = InitializeRendering(false);

        // There were cases where InitializeRendering would fail but
        // HmdDisplayAcquired was still set to true.
        OVR_ASSERT(!((result == 1) && HmdDisplayAcquired));

        if (!HmdDisplayAcquired || (result == 1))
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return;
        }
    }


    const double curtime = ovr_GetTimeInSeconds();

    // If running slower than 10fps, clamp. Helps when debugging, because then dt can be minutes!
    const float dt = Alg::Min<float>(float(curtime - LastUpdate), 0.1f);
    LastUpdate     = curtime;

    if (SceneAnimationEnabled)
        SceneAnimationTime = curtime;

    Profiler.RecordSample(RenderProfiler::Sample_FrameStart);

    if (HmdSettingsChanged)
    {
        ovrResult error = CalculateHmdValues();
        if (HandleOvrError(error))
        {
            return;
        }
    }


    if (LoadingState == LoadingState_DoLoad)
    {
        PopulateScene(MainFilePath.c_str());
        LoadingState = LoadingState_Finished;
        return;
    }

    // Kill overlays in non-mirror mode after timeout.
    if ((NotificationTimeout != 0.0) && (curtime > NotificationTimeout))
    {
        if (MirrorToWindow)
        {
            GetPlatformCore()->SetNotificationOverlay(0,0,0,0);
            GetPlatformCore()->SetNotificationOverlay(1,0,0,0);
        }
        NotificationTimeout = 0.0;
    }

    ovrResult error = ovrSuccess;

#if USE_FRAME_INDEX_ZERO
    int frameIndex = 0;
#else
    int frameIndex = TotalFrameCounter;
#endif

#if USE_WAITFRAME
    error = ovr_WaitToBeginFrame(Session, frameIndex);
    if (HandleOvrError(error))
      return;

    if (PredictionEnabled)
    {
      HmdFrameTiming = ovr_GetPredictedDisplayTime(Session, frameIndex);
    }
    else
      HmdFrameTiming = 0.0;

    error = ovr_BeginFrame(Session, frameIndex);
    if (HandleOvrError(error))
      return;
#else
    HmdFrameTiming = ovr_GetPredictedDisplayTime(Session, frameIndex);
#endif //USE_WAITFRAME

    InterAxialDistance = ovr_GetFloat(Session, "server:InterAxialDistance", -1.0f );

    ovrTrackingState trackState = ovr_GetTrackingState(Session, HmdFrameTiming, ovrFalse);
    ovrTrackerPose trackerPose = ovr_GetTrackerPose(Session, 0);
    CalibratedTrackingOrigin = trackState.CalibratedOrigin;

    // Update tracking 'y' height based on Sitting/Standing state.
    UpdateTrackingStateForSittingHeight(&trackState);

    HmdStatus = trackState.StatusFlags;
    TrackingTrackerCount = ovr_GetInt(Session, "TrackingTrackerCountForHmd", 0);
    ConnectedTrackerCount = ovr_GetTrackerCount(Session);

    memcpy(&LastInputState, &InputState, sizeof(InputState));
    HasInputState      = (ovr_GetInputState(Session, ovrControllerType_Touch, &InputState ) == ovrSuccess);

    // TODO: This is a workaround for not being able to query multiple controllers at once. This
    //       should be pushed into CAPI if it's really desired functionality.
    ovrInputState sidInput = {};
    HasInputState = (ovr_GetInputState(Session, ovrControllerType_Remote, &sidInput) == ovrSuccess) || HasInputState;
    InputState.Buttons |= sidInput.Buttons;

    ConnectedControllerTypes = ovr_GetConnectedControllerTypes(Session);

    HasInputState = (ovr_GetInputState(Session, ovrControllerType_XBox, &GamepadInputState) == ovrSuccess) || HasInputState;

    ovr_GetInputState(Session, ovrControllerType_Active, &ActiveControllerState);

    if (sessionStatus.HasInputFocus) {
      UpdateActiveControllerState();
    } else {
      HasInputState = false;
    }

    HandPoses[0] = trackState.HandPoses[0].ThePose;
    HandPoses[1] = trackState.HandPoses[1].ThePose;
    HandStatus[0] = trackState.HandStatusFlags[0];
    HandStatus[1] = trackState.HandStatusFlags[1];

    // Update 3rd touch pose
    ovrTrackedDeviceType deviceTypes[] = { ovrTrackedDevice_Object0 };
    ovrPoseStatef devicePoses[_countof(deviceTypes)];
    if (ovr_GetDevicePoses(Session, deviceTypes, _countof(deviceTypes), HmdFrameTiming, devicePoses) == ovrSuccess)
		TrackedObjectPose = devicePoses[0].ThePose;

    if ( HasInputState )
    {
        HandleHaptics();
        HandleBoundaryControls();
    }

    ActiveGammaCurve = SrgbRequested ? SrgbGammaCurve : 1.0f;

    if (SceneBrightnessGrayscale)
    {
        SceneBrightness.y = SceneBrightness.x;
        SceneBrightness.z = SceneBrightness.x;
    }

    {
        // Minimum brightness of 5% so you can't lose the menu, and then put it into linear space if needed
        Vector3f sceneBrightness;
        sceneBrightness.x = Alg::Max(powf(SceneBrightness.x / 255.0f, ActiveGammaCurve), 0.02f);
        sceneBrightness.y = Alg::Max(powf(SceneBrightness.y / 255.0f, ActiveGammaCurve), 0.02f);
        sceneBrightness.z = Alg::Max(powf(SceneBrightness.z / 255.0f, ActiveGammaCurve), 0.02f);


        Menu_SetColorGammaCurveAndBrightness(ActiveGammaCurve, sceneBrightness);
    }

    // Report HMD vision tracking
    bool hadHMDVisionTracking = HaveHMDVisionTracking;
    HaveHMDVisionTracking = (trackState.StatusFlags & ovrStatus_PositionTracked) != 0;
    if (HaveHMDVisionTracking && !hadHMDVisionTracking) {
        Menu.SetPopupMessage("Vision Tracking Acquired");
        if (EnableTrackingStateLog) // Log HMD tracking state changes
            WriteLog("[OculusWorldDemoApp] HMD Vision Tracking Acquired at time\n");
    }
    if (!HaveHMDVisionTracking && hadHMDVisionTracking) {
        Menu.SetPopupMessage("Lost Vision Tracking");
        if (EnableTrackingStateLog) // Log HMD tracking state changes
            WriteLog("[OculusWorldDemoApp] HMD Lost Vision Tracking\n");
    }

    // Report position tracker
    bool hadPositionTracker = HavePositionTracker;
    HavePositionTracker = (trackerPose.TrackerFlags & ovrTracker_Connected) != 0;
    if (HavePositionTracker && !hadPositionTracker)
        Menu.SetPopupMessage("Position Tracker Connected");
    if (!HavePositionTracker && hadPositionTracker)
        Menu.SetPopupMessage("Position Tracker Disconnected");

    // Report HMD connectivity
    bool hadHMDConnected = HaveHMDConnected;
    HaveHMDConnected = (sessionStatus.HmdPresent == ovrTrue);
    if (HaveHMDConnected && !hadHMDConnected)
        Menu.SetPopupMessage("HMD Connected");
    if (!HaveHMDConnected && hadHMDConnected)
        Menu.SetPopupMessage("HMD Disconnected");

    // Check if any new devices were connected.
    ProcessDeviceNotificationQueue();
    // FPS count and timing.
    UpdateFrameRateCounter(curtime);
    

    // Player handling.
    switch ( ComfortTurnMode )
    {
    case ComfortTurn_Off:
        ThePlayer.ComfortTurnSnap = -1.0f;
        break;

    case ComfortTurn_30Degrees:
        ThePlayer.ComfortTurnSnap = DegreeToRad ( 30.0f );
        break;

    case ComfortTurn_45Degrees:
        ThePlayer.ComfortTurnSnap = DegreeToRad ( 45.0f );
        break;

    default:
        OVR_ASSERT ( false );
        break;
    }

    // Update pose based on frame.
    ThePlayer.HeadPose = trackState.HeadPose.ThePose;

    // Movement/rotation with the gamepad.
    ThePlayer.BodyYaw -= ThePlayer.GamepadRotate.x * dt;

    if (SceneMode == Scene_World || SceneMode == Scene_Cubes)
        ThePlayer.HandleMovement(dt, &CollisionModels, &GroundCollisionModels, ShiftDown);
    else {
        // No collision checking when not in Tuscany
        std::vector<Ptr<CollisionModel> > emptyModel;
        ThePlayer.HandleMovement(dt, &emptyModel, &emptyModel, ShiftDown);
    }

    // Find the pose of the player's torso (rather than their head) in the world.
    // Literally, this is the pose of the middle eye if they were sitting still and upright, facing forwards.
    Posef PlayerTorso;
    PlayerTorso.Translation = ThePlayer.GetBodyPos(TrackingOriginType);
    PlayerTorso.Rotation = Quatf ( Axis_Y, ThePlayer.GetApparentBodyYaw().Get() );

    // Record after processing time.
    Profiler.RecordSample(RenderProfiler::Sample_AfterGameProcessing);

    // This scene is so simple, it really doesn't stress the GPU or CPU out like a real game would.
    // So to simulate a more complex scene, each eye buffer can get rendered lots and lots of times.

    int totalSceneRenderCount;
    switch ( SceneRenderCountType )
    {
    case SceneRenderCount_FixedLow:
        totalSceneRenderCount = SceneRenderCountLow;
        break;
    case SceneRenderCount_SineTenSec: {
        float phase = (float)( fmod ( curtime * 0.1, 1.0 ) );
        totalSceneRenderCount = (int)( 0.49f + SceneRenderCountLow + ( SceneRenderCountHigh - SceneRenderCountLow ) * 0.5f * ( 1.0f + sinf ( phase * 2.0f * MATH_FLOAT_PI ) ) );
                                        } break;
    case SceneRenderCount_SquareTenSec: {
        float phase = (float)( fmod ( curtime * 0.1, 1.0 ) );
        totalSceneRenderCount = ( phase > 0.5f ) ? SceneRenderCountLow : SceneRenderCountHigh;
                                        } break;
    case SceneRenderCount_Spikes: {
        static int notRandom = 634785346;
        notRandom *= 543585;
        notRandom += 782353;
        notRandom ^= notRandom >> 17;
        // 0x1311 has 5 bits set = one frame in 32 on average. Simlates texture loading or other real-world mess.
        totalSceneRenderCount = ( ( notRandom & 0x1311 ) == 0 ) ? SceneRenderCountHigh : SceneRenderCountLow;
                                    } break;
    default:
         OVR_ASSERT ( false );
         totalSceneRenderCount = SceneRenderCountLow;
         break;
    }

    // Determine if we are rendering this frame. Frame rendering may be
    // skipped based on FreezeEyeUpdate and Time-warp timing state.
    bool bupdateRenderedView = NeverRenderedIntoEyeTextures ||
        (FrameNeedsRendering(curtime) && (sessionStatus.IsVisible || (LoadingState == LoadingState_Frame0)));

    // Pick an appropriately "big enough" size for the HUD and menu and render them their textures texture.
    Sizei hudTargetSize = Sizei ( 2048, 2048 );
    EnsureRendertargetAtLeastThisBig(Rendertarget_Hud, hudTargetSize, false, -1, error);
    if (HandleOvrError(error))
        return;

    // Menu brought up by tab
    Menu.SetShortcutChangeMessageEnable ( ShortcutChangeMessageEnable );
    Sizei menuTargetSize = Sizei ( 2048, 2048 );
    EnsureRendertargetAtLeastThisBig(Rendertarget_Menu, menuTargetSize, false, -1, error);
    if (HandleOvrError(error))
        return;

    if (bupdateRenderedView)
    {
        float textHeight = MenuHudTextPixelHeight;

        HudRenderedSize = RenderTextInfoHud(textHeight);
        OVR_ASSERT ( HudRenderedSize.w <= hudTargetSize.w );        // Grow hudTargetSize if needed.
        OVR_ASSERT ( HudRenderedSize.h <= hudTargetSize.h );
        OVR_UNUSED ( hudTargetSize );

        MenuRenderedSize = RenderMenu(textHeight);
        OVR_ASSERT ( MenuRenderedSize.w <= menuTargetSize.w );        // Grow hudTargetSize if needed.
        OVR_ASSERT ( MenuRenderedSize.h <= menuTargetSize.h );
        OVR_UNUSED ( menuTargetSize );
    }

    // Run menu "simulation" step.
    // Note all of this math is done relative to the avatar's "torso", not in game-world space.
    Quatf playerHeadOrn = ThePlayer.HeadPose.Rotation;

    float yaw, pitch, roll;
    playerHeadOrn.GetYawPitchRoll(&yaw, &pitch, &roll);
    // if head pitch < MenuHudMaxPitchToOrientToHeadRoll, then we ignore head roll on menu orientation
    if (fabs(pitch) < DegreeToRad(MenuHudMaxPitchToOrientToHeadRoll))
    {
        playerHeadOrn = Quatf(Vector3f(0, 1, 0), yaw) * Quatf(Vector3f(1, 0, 0), pitch);
    }
    OVR_MATH_ASSERT(playerHeadOrn.IsNormalized());

    switch ( MenuHudMovementMode )
    {
    case MenuHudMove_FixedToFace:
        // It's just nailed to your nose.
        MenuPose.Rotation = playerHeadOrn;
        break;
    case MenuHudMove_DragAtEdge:
    case MenuHudMove_RecenterAtEdge:
        {
            Quatf difference = MenuPose.Rotation.Inverse() * playerHeadOrn;
            float angleDifference = difference.Angle ( Quatf::Identity() );
            float angleTooFar = angleDifference - DegreeToRad ( MenuHudMovementRadius );
            if ( angleTooFar > 0.0f )
            {
                // Hit the edge - start recentering.
                MenuIsRotating = true;
            }
            if ( MenuIsRotating )
            {
                // Drag the menu closer to the center.
                Vector3f rotationVector = difference.ToRotationVector();
                // That's the right axis, but we want to rotate by a fixed speed.
                float rotationLeftToDo = angleDifference;
                if ( MenuHudMovementMode == MenuHudMove_DragAtEdge )
                {
                    // Stop when it hits the edge, not at the center.
                    rotationLeftToDo = Alg::Max ( 0.0f, rotationLeftToDo - DegreeToRad ( MenuHudMovementRadius ) );
                }
                float rotationAmount = DegreeToRad ( MenuHudMovementRotationSpeed ) * dt;
                if ( rotationAmount > rotationLeftToDo )
                {
                    // Reached where it needed to be.
                    rotationAmount = rotationLeftToDo;
                    MenuIsRotating = false;
                }
                rotationVector *= ( rotationAmount / rotationVector.Length() );
                Quatf rotation = Quatf::FromRotationVector ( rotationVector );
                MenuPose.Rotation *= rotation;
            }
        }

        {
            Vector3f hmdOffsetPos = trackState.HeadPose.ThePose.Position;
            float translationDiff = hmdOffsetPos.Distance(MenuTranslationOffset);
            float translationDiffOverThreshold = translationDiff - MenuHudMovementDistance;
            if (translationDiffOverThreshold > 0.0f)
            {
                MenuIsTranslating = true;
            }
            if (MenuIsTranslating)
            {
                // Drag the menu closer to the center.
                float translationLeftToDo = translationDiff;
                if (MenuHudMovementMode == MenuHudMove_DragAtEdge)
                {
                    // Stop when it hits the edge, not at the center.
                    translationLeftToDo = Alg::Max(0.0f, translationDiffOverThreshold);
                }
                float translationAmount = MenuHudMovementTranslationSpeed * dt;
                if (translationAmount > translationLeftToDo)
                {
                    // Reached where it needed to be.
                    translationAmount = translationLeftToDo;
                    MenuIsTranslating = false;
                }

                // Create final translation vector
                Vector3f translation = (hmdOffsetPos - MenuTranslationOffset).Normalized() * translationAmount;
                MenuTranslationOffset += translation;
            }
        }
        break;
    default: OVR_ASSERT ( false ); break;
    }
    // Move the menu forwards of the player's torso.
    MenuPose.Translation = MenuTranslationOffset + MenuPose.Rotation.Rotate(Vector3f(0.0f, 0.0f, -MenuHudDistance));

	// Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyePose) may change at runtime.
    EyeRenderDesc[0] = ovr_GetRenderDesc(Session, ovrEye_Left, g_EyeFov[0]);
    EyeRenderDesc[1] = ovr_GetRenderDesc(Session, ovrEye_Right, g_EyeFov[1]);

    // Pass in the mideye-to-real-eye vectors we get from the user's profile.
    ovrPosef HmdToEyePose[2] = { EyeRenderDesc[0].HmdToEyePose, EyeRenderDesc[1].HmdToEyePose};


    float localPositionTrackingScale = PositionTrackingScale;


    if (bupdateRenderedView)
    {
        // If render texture size is changing, apply dynamic changes to viewport.
        ApplyDynamicResolutionScaling();

        {
            Vector3f sceneBrightness;
            if (SceneBlack)
            {
                sceneBrightness = Vector3f(0.0f, 0.0f, 0.0f);
            }
            else
            {
                sceneBrightness.x = SceneBrightness.x / 255.0f;
                sceneBrightness.y = SceneBrightness.y / 255.0f;
                sceneBrightness.z = SceneBrightness.z / 255.0f;


                // put it into linear space if needed
                sceneBrightness.x = powf(sceneBrightness.x, ActiveGammaCurve);
                sceneBrightness.y = powf(sceneBrightness.y, ActiveGammaCurve);
                sceneBrightness.z = powf(sceneBrightness.z, ActiveGammaCurve);
            }
            pRender->SetGlobalTint(Vector4f(sceneBrightness.x, sceneBrightness.y, sceneBrightness.z, 1.0));
        }

        pRender->BeginScene();

        // In most cases eyeRenderHmdToEyePose should be the same values sent into ovrViewScaleDesc below
        // except when we want to force mono rendering even if we might end up doing stereo 3D reprojection
        // during positional timewarp when the depth buffer is available
        ovrPosef eyeRenderHmdToEyePose[2] = {HmdToEyePose[0], HmdToEyePose[1] };

        const ovrQuatf quatIdentity = {0, 0, 0, 1.0f};

        // Monoscopic rendering can do two things:
        //  1) Sets eye HmdToEyePose values to 0.0 (effective IPD == 0), but retains head-tracking at its original scale.
        //  2) Sets the player scale to zero, which effectively sets both IPD and head-tracking to 0.
        switch ( MonoscopicRenderMode )
        {
        case Mono_Off:
            break;
        case Mono_ZeroIpd:
            {
                // Make sure both eyes use the same "centered eye" offset
                Vector3f centerEyeOffset = ((Vector3f)EyeRenderDesc[0].HmdToEyePose.Position +
                                            (Vector3f)EyeRenderDesc[1].HmdToEyePose.Position) * 0.5f;
                eyeRenderHmdToEyePose[0].Position = centerEyeOffset;
                eyeRenderHmdToEyePose[1].Position = centerEyeOffset;
                eyeRenderHmdToEyePose[0].Orientation = quatIdentity;
                eyeRenderHmdToEyePose[1].Orientation = quatIdentity;
            }
            break;
        case Mono_ZeroPlayerScale:
            localPositionTrackingScale = 0.0f;
            eyeRenderHmdToEyePose[0].Orientation = quatIdentity;
            eyeRenderHmdToEyePose[1].Orientation = quatIdentity;
            break;
        default: OVR_ASSERT ( false ); break;
        }

        WasteCpuTime(SceneRenderWasteCpuTimePreTrackingQueryMs);

        if (!PredictionEnabled)
        {
          ovrTrackingState eyeTrackingState = ovr_GetTrackingState(Session, 0.0, ovrFalse);
          ovr_CalcEyePoses2(eyeTrackingState.HeadPose.ThePose, eyeRenderHmdToEyePose, EyeRenderPose);
          SensorSampleTimestamp = ovr_GetTimeInSeconds();
        }
        else
        {
          // These are in real-world physical meters. It's where the player *actually is*, not affected by any virtual world scaling.
          ovr_GetEyePoses(Session, frameIndex, ovrTrue, eyeRenderHmdToEyePose, EyeRenderPose, &SensorSampleTimestamp);
        }

        if (VisualizeSeatLevel && Sitting)
        {
            EyeRenderPose[0].Position.y += ExtraSittingAltitude;
            EyeRenderPose[1].Position.y += ExtraSittingAltitude;
        }

        OVR_MATH_ASSERT(Quatf(EyeRenderPose[0].Orientation).IsNormalized());
        OVR_MATH_ASSERT(Quatf(EyeRenderPose[1].Orientation).IsNormalized());

        UpdateDiscreteRotation();

        UpdateSubPixelJitter();

        // local to avoid modifying the EyeRenderPose[2] received from the SDK
        // which will be needed for ovrPositionTimewarpDesc
        ovrPosef localEyeRenderPose[2] = {EyeRenderPose[0], EyeRenderPose[1]};

        // Scale by player's virtual head size (usually 1.0, but for special effects we can make the player larger or smaller).
        localEyeRenderPose[0].Position = ((Vector3f)EyeRenderPose[0].Position) * localPositionTrackingScale;
        localEyeRenderPose[1].Position = ((Vector3f)EyeRenderPose[1].Position) * localPositionTrackingScale;

        EyeFromWorld[0] = CalculateViewFromPose(localEyeRenderPose[0]);
        EyeFromWorld[1] = CalculateViewFromPose(localEyeRenderPose[1]);

        // Now we have the eyes, find the cameras from them.
        for ( int camNum = 0; camNum < CamRenderPoseCount; camNum++ )
        {
            int eyeNum = camNum & 1;
            CamRenderPose[camNum] = Posef(localEyeRenderPose[eyeNum]) * CamFromEye[camNum];
            CamFromWorld[camNum] = CalculateViewFromPose(CamRenderPose[camNum]);
        }

        int currDrawFlushCount = 0;
        int numSwapChainsUsed = 1;
        CamLayerCount = 1;
        bool depthSwapChainUsed = true; // true in most cases

        WasteCpuTime(SceneRenderWasteCpuTimePreRenderMs);

        for ( int curSceneRenderCount = 0; curSceneRenderCount < totalSceneRenderCount; curSceneRenderCount++ )
        {

            if (GridDisplayMode == GridDisplay_GridDirect)
            {
                numSwapChainsUsed = 1;
                CamLayerCount = 1;
                depthSwapChainUsed = false;

                OVR_ASSERT(CamTexture[0] == RenderTargets[Rendertarget_BothEyes].pColorTex);
                OVR_ASSERT(CamTexture[1] == RenderTargets[Rendertarget_BothEyes].pColorTex);

                // Just going to draw a grid to one "eye" which will be displayed without distortion.
                pRender->SetRenderTarget(DrawEyeTargets[Rendertarget_BothEyes]->pColorTex, DrawEyeTargets[Rendertarget_BothEyes]->pDepthTex);
                pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
                RenderEyeView(CamRenderPose_Left, ovrEye_Left);
                FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
            }
            else if (MonoscopicRenderMode != Mono_Off)
            {
                // Zero IPD eye rendering: draw into left eye only, re-use texture for right eye.

                // Note that DrawEyeTargets[] may be different from EyeTexture[] in the case of MSAA.
                numSwapChainsUsed = 1;
                CamLayerCount = 1;
                OVR_ASSERT ( CamTexture[0] == RenderTargets[Rendertarget_Left].pColorTex );
                OVR_ASSERT ( CamTexture[1] == RenderTargets[Rendertarget_Left].pColorTex );

                OVR_ASSERT ( CamRenderPoseCount == 1 );
                pRender->SetRenderTarget(DrawEyeTargets[Rendertarget_Left]->pColorTex, DrawEyeTargets[Rendertarget_Left]->pDepthTex);
                pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));

                RenderEyeView(CamRenderPose_Left, ovrEye_Left);
                // Note: Second eye gets texture is (initialized to same value above).
                FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
            }
            else if (RendertargetIsSharedByBothEyes)
            {
                // Shared render target eye rendering; set up RT once for both eyes.
                OVR_ASSERT ( CamRenderPoseCount == 2 );

                // Note that DrawEyeTargets[] may be different from EyeTexture[] in the case of MSAA.
                numSwapChainsUsed = 1;
                CamLayerCount = 1;
                OVR_ASSERT ( CamTexture[0] == RenderTargets[Rendertarget_BothEyes].pColorTex );
                OVR_ASSERT ( CamTexture[1] == RenderTargets[Rendertarget_BothEyes].pColorTex );

                pRender->SetRenderTarget(DrawEyeTargets[Rendertarget_BothEyes]->pColorTex, DrawEyeTargets[Rendertarget_BothEyes]->pDepthTex);

                {
                    pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));
                }

                for (int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
                {
                    RenderEyeView((CamRenderPoseEnum)eyeIndex, (ovrEyeType)eyeIndex);
                    FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
                }
            }
            else
            {
                // Separate eye rendering - each eye gets its own render target.
                numSwapChainsUsed = CamRenderPoseCount;

                if (LayerCubemap == Cubemap_Render)
                {
                    if (!CubeRendered)
                    {
                        RenderCubemap();
                    }
                }
                else
                {
                    CubeRendered = false;
                }


                OVR_ASSERT ( ( CamRenderPoseCount >= 2 ) && ( CamRenderPoseCount <= 8 ) );
                CamLayerCount = CamRenderPoseCount / 2;
                static int rtNums[8] = { Rendertarget_Left,
                                         Rendertarget_Right,
                                         Rendertarget_Left1,
                                         Rendertarget_Right1,
                                         Rendertarget_Left2,
                                         Rendertarget_Right2,
                                         Rendertarget_Left3,
                                         Rendertarget_Right3 };


                for (int camIndex = 0; camIndex < CamRenderPoseCount; camIndex++)
                {
                    int rtNum = rtNums[camIndex];
                    int eyeNum = camIndex & 1;

                    // Note that DrawEyeTargets[] may be different from EyeTexture[] in the case of MSAA.
                    OVR_ASSERT ( CamTexture[camIndex] == RenderTargets[rtNum].pColorTex );

                    pRender->SetRenderTarget(
                        DrawEyeTargets[rtNum]->pColorTex,
                        DrawEyeTargets[rtNum]->pDepthTex);
                    {
                        pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));
                    }


                    RenderEyeView((CamRenderPoseEnum)camIndex, (ovrEyeType)eyeNum);
                    FlushIfApplicable(DrawFlush_AfterEachEyeRender, currDrawFlushCount);
                }
            }

            WasteCpuTime(SceneRenderWasteCpuTimeEachRenderMs);
            FlushIfApplicable(DrawFlush_AfterEyePairRender, currDrawFlushCount);
        }

        pRender->SetDefaultRenderTarget();
        pRender->FinishScene();
        pRender->SetGlobalTint ( Vector4f ( 1.0f, 1.0f, 1.0f, 1.0 ) );


        // Commit the swap chains
        // If Layer0GenMips was enabled, then Commit() will internally generate mips before actually committing texture
        for (int camIndex = 0; camIndex < numSwapChainsUsed; camIndex++)
        {
            CamTexture[camIndex]->Commit();
            if (Layer0Depth && depthSwapChainUsed)
            {
                CamDepthTexture[camIndex]->Commit();
            }
        }

        // Now that we have committed the cam textures, clear the NeverRendered flag.
        NeverRenderedIntoEyeTextures = false;
    }

    Profiler.RecordSample(RenderProfiler::Sample_AfterEyeRender);


    // Some texture sets are used for multiple layers. For those,
    // it's important that we only increment once per frame, not multiple times.
    // But we also want to increment just before using them. So this stores
    // the list of STSs that have already been advanced this frame.
    // This is used as an argument to IncrementSwapTextureSetIndexUnlessItHasAlreadyBeenDone()
    HashSet<ovrTextureSwapChain> textureSetsThatWereAdvanced;

    // TODO: These happen inside ovr_EndFrame; need to hook into it.
    //Profiler.RecordSample(RenderProfiler::Sample_BeforeDistortion);

    LayerList[LayerNum_MainEye] = nullptr;
    LayerList[LayerNum_MainEye1] = nullptr;
    LayerList[LayerNum_MainEye2] = nullptr;
    LayerList[LayerNum_MainEye3] = nullptr;


    if (Layer0Depth)
    {
        EyeLayer[0].Header.Type = ovrLayerType_EyeFovDepth;
    }
    else
    {
        EyeLayer[0].Header.Type = ovrLayerType_EyeFov;
    }

    EyeLayer[0].Header.Flags =
      (Layer0HighQuality ? ovrLayerFlag_HighQuality : 0) |
      (TextureOriginAtBottomLeft ? ovrLayerFlag_TextureOriginAtBottomLeft : 0);

    if (!EnableTimewarpOnMainLayer)
    {
        EyeLayer[0].Header.Flags |= ovrLayerFlag_HeadLocked;
    }


    if (GridDisplayMode == GridDisplay_GridDirect)
    {
    }
    else
    {
        for (int layerNum = 0; layerNum < CamLayerCount; layerNum++)
        {
            EyeLayer[layerNum] = EyeLayer[0];
            for (int eye = 0; eye < 2; eye++)
            {
                // For mono, both eyes share the data, so mark it as such
                int camNum = layerNum * 2 + (MonoscopicRenderMode == Mono_Off ? eye : 0);
                ovrRecti vp = CamRenderViewports[camNum];

                // Calc where the eye was when we asked the SDK earlier.
                // SDK wants the real HMD pose, not the scaled version,
                // so we scale back from render scale to tracking scale.
                Posef eyeTrackingPose = CamRenderPose[camNum];
                if (localPositionTrackingScale != 0.0f)
                {
                  eyeTrackingPose.Translation = CamRenderPose[camNum].Translation / localPositionTrackingScale;
                }

                EyeLayer[layerNum].EyeFov.ColorTexture[eye]   = CamTexture[camNum]->Get_ovrTextureSet(); // OVR_ASSERT(EyeLayer.EyeFov.ColorTexture[eye]); Can we assert this as valid for both eyes?
                EyeLayer[layerNum].EyeFov.Fov[eye]            = CamFovPort[camNum];
                EyeLayer[layerNum].EyeFov.RenderPose[eye]     = (EnableTimewarpOnMainLayer) ? 
                                                                eyeTrackingPose :
                                                                HmdToEyePose[eye];
                EyeLayer[layerNum].EyeFov.Viewport[eye]       = vp;
                EyeLayer[layerNum].EyeFov.SensorSampleTime    = SensorSampleTimestamp;

                if (TextureOriginAtBottomLeft)
                {
                    // The usual OpenGL viewports-don't-match-UVs oddness.
                    EyeLayer[layerNum].EyeFov.Viewport[eye].Pos.y = CamTexture[camNum]->GetHeight() - (vp.Pos.y + vp.Size.h);
                }

                if (Layer0Depth)
                {
                    OVR_ASSERT ( CamDepthTexture[camNum] != nullptr );
                    EyeLayer[layerNum].EyeFovDepth.DepthTexture[eye] = CamDepthTexture[camNum]->Get_ovrTextureSet();
                    EyeLayer[layerNum].EyeFovDepth.ProjectionDesc = PosTimewarpProjectionDesc;
                }


                // Do NOT advance TextureSet currentIndex - that has already been done above just before rendering.
            }
            // Normal eye layer.
            LayerList[LayerNum_MainEye + layerNum] = &EyeLayer[layerNum].Header;
        }
    }



    int numLayers = 0;
    if ( LayersEnabled )
    {
        // Super simple animation hack :-)
        static float timeHack = 0.0f;
        timeHack += 0.01f;


        if ( ( LayerCockpitEnabled != 0 ) && ( CockpitPanelTexture != NULL ) )
        {
            for ( size_t cockpitLayer = 0; cockpitLayer < OVR_ARRAY_COUNT(CockpitLayer); cockpitLayer++ )
            {
                // Pos+orn in the world.
                // Now move them relative to the torso.
                Posef pose = PlayerTorso.Inverted() *CockpitPanelPose[cockpitLayer];
                // Then scale from virtual-world size to real-world physical player size.
                pose.Translation /= PositionTrackingScale;

                ovrLayerQuad& cl = CockpitLayer[cockpitLayer];

                cl.Header.Type = ovrLayerType_Quad;
                cl.Header.Flags = LayerCockpitHighQuality ? ovrLayerFlag_HighQuality : 0;
                cl.QuadPoseCenter = pose;
                cl.QuadSize = CockpitPanelSize[cockpitLayer] / PositionTrackingScale;
                cl.ColorTexture = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(cl.ColorTexture);
                cl.Viewport = CockpitClipRect[cockpitLayer];

                // Always enable, then maybe disable if flag is not set.
                LayerList[cockpitLayer + LayerNum_CockpitFirst] = &cl.Header;

                if (( LayerCockpitEnabled & (1 << cockpitLayer) ) == 0)
                {
                    LayerList[cockpitLayer + LayerNum_CockpitFirst] = 0;
                }
            }
        }
        else
        {
            for ( int i = LayerNum_CockpitFirst; i <= LayerNum_CockpitLast; i++ )
            {
                LayerList[i] = nullptr;
            }

            // Layer 1 is fixed in the world.
            LayerList[LayerNum_Layer1] = nullptr;
            if ( Layer1Enabled && ( TextureOculusCube != NULL ) )
            {
                // Pos+orn in the world.
                Posef pose;
                pose.Rotation    = Quatf ( Axis_Y, timeHack );
                pose.Translation = Vector3f ( 5.25f, 1.5f, -0.75f );        // Sitting on top of the curly end bit of the bannister.
                // Now move them relative to the torso.
                pose = PlayerTorso.Inverted() * pose;
                // Physical size of the quad in meters.
                Vector2f quadSize(0.2f, 0.3f);

                // Scale from virtual-world size to real-world physical player size.
                quadSize /= PositionTrackingScale;
                pose.Translation /= PositionTrackingScale;

                Layer1.Header.Type      = ovrLayerType_Quad;
                Layer1.Header.Flags     = Layer1HighQuality ? ovrLayerFlag_HighQuality : 0;
                Layer1.QuadPoseCenter   = pose;
                Layer1.QuadSize         = quadSize;
                Layer1.ColorTexture     = TextureOculusCube->Get_ovrTextureSet(); OVR_ASSERT(Layer1.ColorTexture);
                Layer1.Viewport         = Recti(Sizei(TextureOculusCube->GetWidth(), TextureOculusCube->GetHeight()));

                LayerList[LayerNum_Layer1] = &Layer1.Header;
            }


            // Layer 2 is fixed in torso space.
            LayerList[LayerNum_Layer2] = nullptr;
            if ( Layer2Enabled && ( CockpitPanelTexture != NULL ) )
            {
                ovrRecti clipRect;
                // These numbers are just pixel positions in the texture.
                clipRect.Pos.x  = 391;
                clipRect.Pos.y  = 217;
                clipRect.Size.w = 494 - clipRect.Pos.x;
                clipRect.Size.h = 320 - clipRect.Pos.y;
                ovrPosef tempPose;
                // Pos+orn are relative to the "torso" of the player.
                // Deliberately NOT scaled by PositionTrackingScale, so it is always half a meter down & away in "meat space", not game space.
                tempPose.Orientation = Quatf ( Axis_X, -3.1416f/2.0f );
                tempPose.Position = Vector3f(0.0f, ThePlayer.GetHeadDistanceFromTrackingOrigin(TrackingOriginType) - 0.5f, -0.5f);

                // Assign Layer2 data.
                Layer2.Header.Type      = ovrLayerType_Quad;
                Layer2.Header.Flags     = Layer2HighQuality ? ovrLayerFlag_HighQuality : 0;
                Layer2.QuadPoseCenter   = tempPose;
                Layer2.QuadSize         = Vector2f (0.25f, 0.25f) * Layer234Size;
                Layer2.ColorTexture     = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(Layer2.ColorTexture);
                Layer2.Viewport         = clipRect;

                LayerList[LayerNum_Layer2] = &Layer2.Header;
            }

            // Layer 3 is fixed on your face.
            LayerList[LayerNum_Layer3] = nullptr;
            if ( Layer3Enabled && ( CockpitPanelTexture != NULL))
            {
              ovrRecti clipRect;
              // These numbers are just pixel positions in the texture.
                clipRect.Pos.x = 264;
                clipRect.Pos.y = 222;
                clipRect.Size.w = 371 - clipRect.Pos.x;
                clipRect.Size.h = 329 - clipRect.Pos.y;

                ovrPosef tempPose;
                // Pos+orn are relative to the current HMD.
                // Deliberately NOT scaled by PositionTrackingScale, so it is always a meter away in "meat space", not game space.
                float rotation       = 0.5f * sinf ( timeHack );
                tempPose.Orientation = Quatf ( Axis_Z, rotation );
                tempPose.Position    = Vector3f ( 0.0f, 0.0f, -1.0f );

                // Assign Layer3 data.
                Layer3.Header.Type      = ovrLayerType_Quad;
                Layer3.Header.Flags     = ( Layer3HighQuality ? ovrLayerFlag_HighQuality : 0 ) | ovrLayerFlag_HeadLocked;
                Layer3.QuadPoseCenter   = tempPose;
                Layer3.ColorTexture     = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(Layer3.ColorTexture);
                Layer3.QuadSize         = Vector2f (0.3f, 0.3f) * Layer234Size;
                Layer3.Viewport         = clipRect;

                LayerList[LayerNum_Layer3] = &Layer3.Header;
            }
            
            // Layer 4 is also fixed on your face, but tests the "matrix" style.
            LayerList[LayerNum_Layer4] = nullptr;
            if ( (Layer4Enabled && ( CockpitPanelTexture != NULL ))
              )
            {
                ovrRecti clipRect;
                // These numbers are just pixel positions in the texture.
                clipRect.Pos.x = 264;
                clipRect.Pos.y = 222;
                clipRect.Size.w = 371 - clipRect.Pos.x;
                clipRect.Size.h = 329 - clipRect.Pos.y;

                Layer4.Header.Type      = ovrLayerType_EyeMatrix;
                Layer4.Header.Flags     = ( Layer4HighQuality ? ovrLayerFlag_HighQuality : 0 ) | ovrLayerFlag_HeadLocked;
#if 0
                ovrPosef tempPose;
                // Pos+orn are relative to the current HMD.
                // Deliberately NOT scaled by PositionTrackingScale, so it is always a meter away in "meat space", not game space.
                float rotation       = 0.5f * sinf ( timeHack );
                tempPose.Orientation = Quatf ( Axis_Z, rotation );
                tempPose.Position    = Vector3f ( 0.0f, 0.0f, -1.0f );
                Layer4.QuadPoseCenter   = tempPose;
                Layer4.QuadSize         = Vector2f (0.3f, 0.3f) * Layer234Size;
#else
                ovrPosef tempPose;
                // Pos+orn are relative to the current HMD.
                // Deliberately NOT scaled by PositionTrackingScale, so it is always a meter away in "meat space", not game space.
                float rotation       = 0.5f * sinf ( timeHack );
                tempPose.Orientation = Quatf ( Axis_Z, rotation );
                tempPose.Position    = Vector3f ( 0.0f, 0.0f, -1.0f );
                Layer4.RenderPose[0] = tempPose;
                Layer4.RenderPose[1] = tempPose;

                Vector2f quadSize = Vector2f (0.3f, 0.3f) * Layer234Size;
                Vector2f textureSizeRecip = Vector2f ( 1.0f / (float)(CockpitPanelTexture->GetWidth()),
                                                       1.0f / (float)(CockpitPanelTexture->GetHeight()) );
                Vector2f middleInUV = textureSizeRecip * Vector2f ( (float)( clipRect.Pos.x + clipRect.Size.w / 2 ),
                                                                    (float)( clipRect.Pos.y + clipRect.Size.h / 2 ) );
                ovrMatrix4f matrix;
                matrix.M[0][0] = 1.0f/quadSize.x;
                matrix.M[0][1] = 0.0f;
                matrix.M[0][2] = middleInUV.x;
                matrix.M[0][3] = 0.0f;
                matrix.M[1][0] = 0.0f;
                matrix.M[1][1] = 1.0f/quadSize.y;
                matrix.M[1][2] = middleInUV.y;
                matrix.M[1][3] = 0.0f;
                matrix.M[2][0] = 0.0f;
                matrix.M[2][1] = 0.0f;
                matrix.M[2][2] = 1.0f;
                matrix.M[2][3] = 0.0f;
                matrix.M[3][0] = 0.0f;
                matrix.M[3][1] = 0.0f;
                matrix.M[3][2] = 0.0f;
                matrix.M[3][3] = 1.0f;
                Layer4.Matrix[0] = matrix;
                Layer4.Matrix[1] = matrix;
#endif

                Layer4.ColorTexture[0]  = CockpitPanelTexture->Get_ovrTextureSet(); OVR_ASSERT(Layer4.ColorTexture[0]);
                Layer4.ColorTexture[1]  = Layer4.ColorTexture[0];
                Layer4.Viewport[0]      = clipRect;
                Layer4.Viewport[1]      = clipRect;


                LayerList[LayerNum_Layer4] = &Layer4.Header;
            }

            // Layer HDCP is similar to Layer 3, but uses the protected flag.
            LayerList[LayerNum_LayerHdcp] = nullptr;
            if ( LayerHdcpEnabled && ( HdcpTexture != NULL))
            {
                // These numbers are just pixel positions in the texture.
                ovrRecti clipRect;
                clipRect.Pos.x = 274;
                clipRect.Pos.y = 104;
                clipRect.Size.w = 228;
                clipRect.Size.h = 103;

                ovrPosef tempPose;
                // Pos+orn are relative to the current HMD.
                // Deliberately NOT scaled by PositionTrackingScale, so it is always a meter away in "meat space", not game space.
                float rotation       = 0.5f * sinf ( timeHack );
                tempPose.Orientation = Quatf ( Axis_Z, rotation );
                tempPose.Position    = Vector3f ( 0.0f, 0.0f, -1.0f );

                // Assign LayerHdcp data.
                LayerHdcp.Header.Type      = ovrLayerType_Quad;
                LayerHdcp.Header.Flags     = ovrLayerFlag_HighQuality | ovrLayerFlag_HeadLocked;
                LayerHdcp.QuadPoseCenter   = tempPose;

                LayerHdcp.ColorTexture     = HdcpTexture->Get_ovrTextureSet(); OVR_ASSERT(LayerHdcp.ColorTexture);
                LayerHdcp.QuadSize         = Vector2f (0.5f, 0.23f) * Layer234Size;
                LayerHdcp.Viewport         = clipRect;

                LayerList[LayerNum_LayerHdcp] = &LayerHdcp.Header;
            }
        }


        // Cylinder is fixed in world space.
        LayerList[LayerNum_Cylinder] = nullptr;

        Ptr<Texture> cylinderTexture = TextureOculusCube;
        if ( LayerCylinderEnabled && ( cylinderTexture != NULL ) )
        {
            ovrRecti clipRect;
            clipRect = Recti( 0, 0, 715,   715 );

            Posef pose;
            pose.Translation = Vector3f(0.0f, 0.0f, 0.0f);

            // Scale from virtual-world size to real-world physical player size.
            pose.Translation /= PositionTrackingScale;

            CylinderLayer.Header.Type      = ovrLayerType_Cylinder;
            CylinderLayer.Header.Flags     = (LayerCylinderHighQuality ? ovrLayerFlag_HighQuality : 0);
            CylinderLayer.Header.Flags     |= (LayerCylinderHeadlocked ? ovrLayerFlag_HeadLocked : 0);
            CylinderLayer.ColorTexture     = cylinderTexture->Get_ovrTextureSet(); OVR_ASSERT(CylinderLayer.ColorTexture);
            CylinderLayer.Viewport         = clipRect;

            CylinderLayer.CylinderPoseCenter  = pose;
            CylinderLayer.CylinderAspectRatio = LayerCylinderAspectRatio;
            CylinderLayer.CylinderRadius      = LayerCylinderRadius / PositionTrackingScale;
            CylinderLayer.CylinderAngle       = LayerCylinderAngle;

            LayerList[LayerNum_Cylinder] = &CylinderLayer.Header;
        }

        LayerList[LayerNum_Cubemap] = nullptr;
        Ptr<Texture> cubemapTexture;
        if (LayerCubemap == Cubemap_Render )
        {
            cubemapTexture = CubemapRenderTexture;
        }
        else if ( LayerCubemap == Cubemap_Load )
        {
            cubemapTexture = CubemapLoadTexture;
        }

        if ( ( LayerCubemap != Cubemap_Off ) && cubemapTexture != NULL )
		    {
            CubemapLayer.Header.Type = ovrLayerType_Cube;
            CubemapLayer.Header.Flags = LayerCubemapHighQuality ? ovrLayerFlag_HighQuality : 0;
            CubemapLayer.Orientation = Quatf::Identity();
            CubemapLayer.CubeMapTexture = cubemapTexture->Get_ovrTextureSet();

            LayerList[LayerNum_Cubemap] = &CubemapLayer.Header;
        }

        // Hack variable - Tuned to match existing rendering path size, and the default of 22 pixels.
        float magicOrthoScale       = HmdDesc.Type == ovrHmd_DK2 ? 0.00146f : 0.001013f;
        float metersPerPixelMenuHud = ( MenuHudSize * MenuHudDistance ) * magicOrthoScale * (22.0f / MenuHudTextPixelHeight);

        ovrPosef menuPose = MenuPose;
        bool menuIsHeadLocked = false;
        switch ( MenuHudMovementMode )
        {
        case MenuHudMove_FixedToFace:
            // Pos+orn are relative to the current HMD.
            menuIsHeadLocked = true;
            menuPose.Orientation = Quatf ();
            menuPose.Position = Vector3f ( 0.0f, 0.0f, -MenuHudDistance );
            break;
        case MenuHudMove_RecenterAtEdge:
        case MenuHudMove_DragAtEdge:
            // The menu is an in-world "simulated" thing, done above.
            menuIsHeadLocked = false;
            menuPose = MenuPose;
            break;
        default: OVR_ASSERT ( false ); break;
        }


        // Hud and menu are layers 20 and 21.
        // HUD is fixed to your face.
        if ( HudRenderedSize.w > 0 )
        {
            OVR_ASSERT( HudRenderedSize.h > 0 );


            // Assign HudLayer data.
            // Note the use of TextureOriginAtBottomLeft because these are rendertargets.
            HudLayer.Header.Type      = ovrLayerType_Quad;
            HudLayer.Header.Flags     = (LayerHudMenuHighQuality ? ovrLayerFlag_HighQuality : 0) |
                                        (TextureOriginAtBottomLeft ? ovrLayerFlag_TextureOriginAtBottomLeft : 0) |
                                        (menuIsHeadLocked ? ovrLayerFlag_HeadLocked : 0);
            HudLayer.QuadPoseCenter   = menuPose;
            HudLayer.QuadSize         = Vector2f((float)HudRenderedSize.w, (float)HudRenderedSize.h) * metersPerPixelMenuHud;
            HudLayer.ColorTexture     = DrawEyeTargets[Rendertarget_Hud]->pColorTex->Get_ovrTextureSet();
            HudLayer.Viewport         = (ovrRecti)HudRenderedSize;

            // Grow the cliprect slightly to get a nicely-filtered edge.
            HudLayer.Viewport.Pos.x -= 1;
            HudLayer.Viewport.Pos.y -= 1;
            HudLayer.Viewport.Size.w += 2;
            HudLayer.Viewport.Size.h += 2;

            // IncrementSwapTextureSetIndex ( HudLayer.ColorTexture ) was done above when it was rendered.
            LayerList[LayerNum_Hud] = &HudLayer.Header;
        }
        else
        {
            OVR_ASSERT(HudRenderedSize.h == 0);
            LayerList[LayerNum_Hud] = nullptr;
        }

        // Menu is fixed to your face.
        if ( ( MenuRenderedSize.w > 0 ) && LayerHudMenuEnabled )
        {
            OVR_ASSERT ( MenuRenderedSize.h > 0 );

            ovrPosef tempPose;
            // Pos+orn are relative to the current HMD.
            tempPose.Orientation = Quatf ();
            tempPose.Position    = Vector3f ( 0.0f, 0.0f, -MenuHudDistance );

            // Assign MenuLayer data.
            // Note the use of TextureOriginAtBottomLeft because these are rendertargets.
            MenuLayer.Header.Type      = ovrLayerType_Quad;
            MenuLayer.Header.Flags     = (LayerHudMenuHighQuality ? ovrLayerFlag_HighQuality : 0) |
                                         (TextureOriginAtBottomLeft ? ovrLayerFlag_TextureOriginAtBottomLeft : 0) |
                                         (menuIsHeadLocked ? ovrLayerFlag_HeadLocked : 0);
            MenuLayer.QuadPoseCenter   = menuPose;
            MenuLayer.QuadSize         = Vector2f((float)MenuRenderedSize.w, (float)MenuRenderedSize.h) * metersPerPixelMenuHud;
            MenuLayer.ColorTexture     = DrawEyeTargets[Rendertarget_Menu]->pColorTex->Get_ovrTextureSet();
            MenuLayer.Viewport         = (ovrRecti)MenuRenderedSize;

            // Grow the cliprect slightly to get a nicely-filtered edge.
            MenuLayer.Viewport.Pos.x -= 1;
            MenuLayer.Viewport.Pos.y -= 1;
            MenuLayer.Viewport.Size.w += 2;
            MenuLayer.Viewport.Size.h += 2;

            // IncrementSwapTextureSetIndex ( MenuLayer.ColorTexture ) was done above when it was rendered.
            LayerList[LayerNum_Menu] = &MenuLayer.Header;
        }
        else
        {
            LayerList[LayerNum_Menu] = nullptr;
        }

        // Load-screen screen shot image if loading
        if ( ( LoadingState == LoadingState_DoLoad ) && ( LoadingTexture != NULL ) )
        {
            Posef tempPose = ThePlayer.HeadPose;
            tempPose.Translation = tempPose.Transform(Vector3f(0.0f, 0.0f, -1.0f));
            float currentYaw, currentPitch, currentRoll;
            tempPose.Rotation.GetYawPitchRoll(&currentYaw, &currentPitch, &currentRoll);
            // Ignore the roll, so that the image is not tited if the player's headset was titled initially
            tempPose.Rotation = Quatf(Vector3f(0, 1, 0), currentYaw) * Quatf(Vector3f(1, 0, 0), currentPitch);

            // Assign LoadingLayer data.
            LoadingLayer.Header.Type = ovrLayerType_Quad;
            LoadingLayer.Header.Flags = (LayerLoadingHighQuality ? ovrLayerFlag_HighQuality : 0);
            LoadingLayer.QuadPoseCenter = tempPose;
            LoadingLayer.QuadSize = Vector2f(1.0f, 1.0f) * Layer234Size;
            LoadingLayer.ColorTexture = LoadingTexture->Get_ovrTextureSet();
            LoadingLayer.Viewport = Recti(Sizei(LoadingTexture->GetWidth(), LoadingTexture->GetHeight()));;

            LayerList[LayerNum_Loading] = &LoadingLayer.Header;
        }
        else
        {
            LayerList[LayerNum_Loading] = nullptr;
        }

        numLayers = LayerNum_TotalLayers;
    }
    else
    {
        // Layers disabled, so the only layer is the eye buffer.
        numLayers = 1;
    }

    WasteCpuTime(SceneRenderWasteCpuTimePreSubmitMs);


    // Set up positional data.
    ovrViewScaleDesc viewScaleDesc;
    viewScaleDesc.HmdSpaceToWorldScaleInMeters = localPositionTrackingScale;
    viewScaleDesc.HmdToEyePose[0] = HmdToEyePose[0];
    viewScaleDesc.HmdToEyePose[1] = HmdToEyePose[1];

    // MRC API function demo use


    // Draw the mirror window contents using the previous frame's HMD mirror buffer
    // We do this before calling ovr_SubmitFrame() to avoid the GPU from doing tiny
    // amounts of render work before starting to process the next frame. This will
    // in turn allow the GPU timing values to be more accurate as it won't be affected
    // as much by queue ahead logic.
    {
        // Reset the back buffer format in case the sRGB flag was flipped
        if (RenderParams.SrgbBackBuffer != SrgbRequested)
        {
            RenderParams.SrgbBackBuffer = SrgbRequested;
            pRender->SetParams(RenderParams);
        }

        // Update Window with mirror texture
        if (MirrorToWindow && ((!MirrorTexture) ||
                               (MirrorTexture->GetWidth() != WindowSize.w) ||
                               (MirrorTexture->GetHeight() != WindowSize.h) ||
                               (MirrorIsSrgb != SrgbRequested) ||
                               (MirrorOptionsChanged)))
        {
            MirrorTexture.Clear();  // first make sure we destroy the old mirror texture if one existed

            // Create one
            uint64_t format = Texture_RGBA | Texture_Mirror;
            if (SrgbRequested)
            {
                // The mirror texture must match the backbuffer - the blit will not convert.
                format |= Texture_SRGB;
            }

            format |= CurrentMirrorOptionsFormat;
            MirrorTexture = *pRender->CreateTexture(format, WindowSize.w, WindowSize.h, nullptr, 1, &error);
            if (HandleOvrError(error))
                return;

            MirrorIsSrgb = SrgbRequested;    // now they match
            MirrorOptionsChanged = false;
        }
        if (MirrorTexture && !MirrorToWindow)
        {
            // Destroy it
            MirrorTexture.Clear();
        }

        // Start to render to the main window on the monitor
        pRender->SetDefaultRenderTarget();
        pRender->Clear(0.0f, 1.0f, 0.0f, 0.0f);	// green screen for mixed reality capture mode

        if (MirrorTexture)
        {
            if (IsMixedRealityCaptureMode || IsMRCDemoMode)
            {
                 pRender->Blt(MirrorTexture, 0, 0, WindowSize.w, WindowSize.h);
            }
            else
            {
                pRender->Blt(MirrorTexture);
            }
        }

        if (IsMixedRealityCaptureMode)
        {
            RenderCamNearFarHandsView();
        }

        // MRC API function demo use

        if (MenuHudAlwaysOnMirrorWindow && (MenuRenderedSize.w > 0))
        {
            pRender->BeginRendering();

            Matrix4f ortho = Matrix4f::Ortho2D((float)WindowSize.w, (float)WindowSize.h);
            pRender->SetProjection(ortho);
            pRender->SetViewport(0, 0, WindowSize.w, WindowSize.h);
            pRender->SetDepthMode(false, false);
            float menuW = 1.0f / (float)(DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetWidth());
            float menuH = 1.0f / (float)(DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetHeight());

            pRender->FillTexturedRect(0.0f, 0.0f, (float)MenuRenderedSize.w, (float)MenuRenderedSize.h,
                (float)MenuRenderedSize.x * menuW, (float)MenuRenderedSize.y * menuH,
                (float)(MenuRenderedSize.x + MenuRenderedSize.w) * menuW, (float)(MenuRenderedSize.y + MenuRenderedSize.h) * menuH,
                Color(255, 255, 255, 128),
                DrawEyeTargets[Rendertarget_Menu]->pColorTex,
                nullptr, true);
        }

        // Cubemap must be at the bottom of the layer list to replace the main scene
        // Must switch MainEye and Cubemap layer positions when cubemap is enabled
        // This avoids causing an error when the LayerList (cubemap) shows up as null
        if (LayerCubemap != Cubemap_Off)
        {
            ovrLayerHeader* maineye = LayerList[LayerNum_MainEye];
            ovrLayerHeader* cubemap = LayerList[LayerNum_Cubemap];
            memcpy(maineye, LayerList[LayerNum_MainEye], sizeof(ovrLayerHeader*));
            memcpy(cubemap, LayerList[LayerNum_Cubemap], sizeof(ovrLayerHeader*));
            LayerList[LayerNum_MainEye] = cubemap;
            LayerList[LayerNum_Cubemap] = maineye;
        }


#if USE_WAITFRAME
        error = ovr_EndFrame(Session, frameIndex, &viewScaleDesc, LayerList, numLayers);
#else
        error = ovr_SubmitFrame(Session, frameIndex, &viewScaleDesc, LayerList, numLayers);
#endif


        if (HandleOvrError(error))
            return;

        // Result could have been ovrSuccess_NotVisible.
        if (error == ovrSuccess_NotVisible)
        {
            // Don't pound on the CPU if not visible.
            Sleep(100);
        }

        pRender->Present(false);
    }

    Profiler.RecordSample(RenderProfiler::Sample_AfterPresent);
}

bool OculusWorldDemoApp::HandleOvrError(ovrResult error)
{
    if (error < ovrSuccess)
    {
        if (error == ovrError_DisplayLost)
        {
            DestroyRendering();
            LoadingState = LoadingState_DoLoad; // this will trigger everything to reload

            GetPlatformCore()->SetNotificationOverlay(0, 28, 8,
                "HMD disconnected - Please reconnect Rift to continue");
        }
        else
        {
            OVR_FAIL_M("Unhandled texture creation error");
        }

        return true;    // halt execution
    }

    return false;
}


void OculusWorldDemoApp::UpdateDiscreteRotation()
{
  if (DiscreteHmdRotationEnable)
  {
    // Hold onto actual eye poses as we will need them to calculate scissors below
    DiscreteActualEyePoses[ovrEye_Left] = EyeRenderPose[ovrEye_Left];
    DiscreteActualEyePoses[ovrEye_Right] = EyeRenderPose[ovrEye_Right];

    float dotDirs = DiscreteEyePoses[0].Rotation.Dot(EyeRenderPose[0].Orientation);
    float threshDot = cosf(DegreeToRad(DiscreteRotationThresholdAngle * 0.5f)); // quat dot needs half diff

    // override rotation if it's not rotated large enough yet
    if (DiscreteHmdRotationNeedsInit || dotDirs < threshDot)
    {
      DiscreteHmdRotationNeedsInit = false;
      DiscreteEyePoses[0] = EyeRenderPose[0];
      DiscreteEyePoses[1] = EyeRenderPose[1];
    }
    else
    {
      EyeRenderPose[0].Orientation = DiscreteEyePoses[0].Rotation;
      EyeRenderPose[1].Orientation = DiscreteEyePoses[1].Rotation;
    }

    // Calculate a sub-viewport for the area the HMD will actually show
    if (DiscreteRotationScissorTech != DRScissorTechnique_None)
    {
      if (CamRenderPoseCount != 2)
      {
        OVR_FAIL_M("Unhandled cam render count. Only 2 used for Discrete Rotation.");
        // disable scissor calculation as a fallback
        DiscreteRotationScissorTech = DRScissorTechnique_None;
      }
      else
      {
        for (int eye = 0; eye < ovrEye_Count; eye++)
        {
          // Discrete rotation eye poses are stored in DiscreteEyePoses[]
          // The actual eye poses expected to be visible in the HMD are stored in DiscreteActualEyePoses[]
          // Calc the rotation offset from our Discrete eye pose to the actual eye pose
          Quatf scissorRot = DiscreteEyePoses[eye].Rotation.Inverted() * DiscreteActualEyePoses[eye].Rotation;
          
          // Apply our debug tweak var to scale the scissor edges
          FovPort scaledFov = FovPort::ScaleFovPort(DiscreteRotationSubFovPort[eye],
            Vector2f(DiscreteRotationScissorMult, DiscreteRotationScissorMult));

          FovPort scissorFov;
          
          // Rotate scissor FovPort that is centered around the discrete FovPort
          switch (DiscreteRotationScissorTech)
          {
          case OculusWorldDemoApp::DRScissorTechnique_ProjectCorners:
            {
              // This is the standard behavior for the helper function, so just use that
              scissorFov = FovPort::Uncant(scaledFov, scissorRot);
            }
            break;
          case OculusWorldDemoApp::DRScissorTechnique_ProjectEdges:
            {
              const FovPort& cantedFov = scaledFov;
              Quatf canting = scissorRot;
                  
              // make 3D vectors from the FovPorts projected to z=1 plane
              Vector3f leftVec  = Vector3f(   cantedFov.LeftTan,               0.0f, 1.0f);
              Vector3f rightVec = Vector3f( -cantedFov.RightTan,               0.0f, 1.0f);
              Vector3f upVec    = Vector3f(                0.0f,   -cantedFov.UpTan, 1.0f);
              Vector3f downVec  = Vector3f(                0.0f,  cantedFov.DownTan, 1.0f);

              // rotate these vectors using the canting specified
              leftVec  = canting.Rotate(leftVec);
              rightVec = canting.Rotate(rightVec);
              upVec    = canting.Rotate(upVec);
              downVec  = canting.Rotate(downVec);

              // If the z coordinates of any of the corners end up being really small or negative, then
              // projection will generate extremely large or inverted frustums and we don't really want that
              const float kMinValidZ = 0.01f;

              // re-project back to z=1 plane while making sure we don't generate gigantic values (hence max)
              leftVec  /= OVRMath_Max( leftVec.z, kMinValidZ);
              rightVec /= OVRMath_Max(rightVec.z, kMinValidZ);
              upVec    /= OVRMath_Max(   upVec.z, kMinValidZ);
              downVec  /= OVRMath_Max( downVec.z, kMinValidZ);

              scissorFov.LeftTan  =   leftVec.x;
              scissorFov.RightTan = -rightVec.x;
              scissorFov.UpTan    =    -upVec.y;
              scissorFov.DownTan  =   downVec.y;
            }
            break;

          case OculusWorldDemoApp::DRScissorTechnique_None:
          default:
            OVR_FAIL();
            break;
          }          

          // Discrete rotation FovPort is stored in g_EyeFov
          // Smaller FovPort that will actually be visible in the HMD is stored in scissorFov
          // Find where the edge lands on the larger FovPort stored in g_EyeFov[]
          const float discreteFovWidth = (g_EyeFov[eye].LeftTan + g_EyeFov[eye].RightTan);
          const float discreteFovHeight = (g_EyeFov[eye].UpTan + g_EyeFov[eye].DownTan);

          // Calculate ratios which will be scaled to full width and height below
          float widthPerc = (scissorFov.LeftTan + scissorFov.RightTan) / discreteFovWidth;
          float leftPerc = (g_EyeFov[eye].LeftTan - scissorFov.LeftTan) / discreteFovWidth;

          float heightPerc = (scissorFov.UpTan + scissorFov.DownTan) / discreteFovHeight;
          float upPerc = (g_EyeFov[eye].UpTan - scissorFov.UpTan) / discreteFovHeight;

          auto ClampFloat = [](float value, float min, float max) -> float {
            return OVRMath_Min(OVRMath_Max(value, min), max);
          };

          {
            // Make sure the values make sense
            widthPerc   = ClampFloat(widthPerc, 0.0f, 1.0f);
            leftPerc    = ClampFloat(leftPerc, 0.0f, 1.0f);
            heightPerc  = ClampFloat(heightPerc, 0.0f, 1.0f);
            upPerc      = ClampFloat(upPerc, 0.0f, 1.0f);
            if (widthPerc  + leftPerc > 1.0f) { widthPerc  = 1.0f - leftPerc; }
            if (heightPerc + upPerc   > 1.0f) { heightPerc = 1.0f - upPerc; }
          }

          DiscreteRotationScissor[eye].x = (int)(CamRenderSize[eye].w * leftPerc);
          DiscreteRotationScissor[eye].y = (int)(CamRenderSize[eye].h * upPerc);
          DiscreteRotationScissor[eye].w = (int)(CamRenderSize[eye].w * widthPerc);
          DiscreteRotationScissor[eye].h = (int)(CamRenderSize[eye].h * heightPerc);
                
          OVR_ASSERT(
            (DiscreteRotationScissor[eye].x >= 0) &&
            (DiscreteRotationScissor[eye].y >= 0) &&
            (DiscreteRotationScissor[eye].w >= 0) &&
            (DiscreteRotationScissor[eye].h >= 0) &&
            (DiscreteRotationScissor[eye].w + DiscreteRotationScissor[eye].x <= CamRenderSize[eye].w) &&
            (DiscreteRotationScissor[eye].h + DiscreteRotationScissor[eye].y <= CamRenderSize[eye].h));

          // Calculate some debug menu values for sanity checks
          DiscreteRotationScissorMenuViewSavings[eye] = (1.0f - (widthPerc * heightPerc)) * 100.0f;

          // Calculate how many extra pixels we're rendering as a % even when using scissor
          DiscreteRotationScissorMenuViewPerfHit[eye] =
            (float)(DiscreteRotationScissor[eye].w * DiscreteRotationScissor[eye].h) /
            (float)(DiscreteRotationScissorIdealRenderSize[eye].w * DiscreteRotationScissorIdealRenderSize[eye].h);
          DiscreteRotationScissorMenuViewPerfHit[eye] = 100.0f * (DiscreteRotationScissorMenuViewPerfHit[eye] - 1.0f);
        }
      }
    }
  }
  else
  {
      DiscreteEyePoses[0] = EyeRenderPose[0];
      DiscreteEyePoses[1] = EyeRenderPose[1];
  }
}

void OculusWorldDemoApp::UpdateSubPixelJitter()
{
  if (SubpixelJitterEnable)
  {
    SubpixelJitterCurrentIndex++;

    const float kJitterRasterShrink = 0.333f; // The pixel coverage shrink amount
    float radialStep = MATH_FLOAT_TWOPI / SubpixelJitterSampleCount;
    const float polarDistance = kJitterRasterShrink * SubpixelJitterSamplePolarDistMult;
    const float polarAngle = radialStep * SubpixelJitterCurrentIndex + SubpixelJitterSamplePolarOffset;
    Vector2f jitter = Vector2f(sinf(polarAngle) * polarDistance,
                               cosf(polarAngle) * polarDistance);

    unsigned int projectionModifier = createProjectionModifier();

    for (int camNum = 0; camNum < CamRenderPose_Count; camNum++)
    {
      const FovPort& baseFov = SubpixelJitterBaseCamFovPort[camNum];

      Vector2f pixelTanAngle = Vector2f((float)CamRenderViewports[camNum].w,
                                        (float)CamRenderViewports[camNum].h);
      pixelTanAngle.x = (baseFov.LeftTan + baseFov.RightTan) / pixelTanAngle.x;
      pixelTanAngle.y = (baseFov.UpTan   + baseFov.DownTan) / pixelTanAngle.y;
      Vector2f curJit = Vector2f(jitter.x * pixelTanAngle.x,
                                 jitter.y * pixelTanAngle.y);

      // Take base FovPort, and offset TanAngles by sub-pixel amounts in a circle
      FovPort& jitFov = CamFovPort[camNum];
      jitFov.LeftTan  = baseFov.LeftTan  - curJit.x;
      jitFov.RightTan = baseFov.RightTan + curJit.x;
      jitFov.DownTan  = baseFov.DownTan  - curJit.y;
      jitFov.UpTan    = baseFov.UpTan    + curJit.y;

      CamProjection[camNum] = ovrMatrix4f_Projection(CamFovPort[camNum], NearClip, FarClip, projectionModifier);
    }
  }
}

void OculusWorldDemoApp::UpdateTrackingStateForSittingHeight(ovrTrackingState* ts)
{
    // FIXME: use the calibrated floor position.


    // Detect based on altitude
    if (SittingAutoSwitch)
    {
        double curtime = ovr_GetTimeInSeconds();

        if ((ts->HeadPose.ThePose.Position.y < -0.32))
        {
            TransitionToStandTime = 0.0;
            if (!Sitting)
            {
                if (TransitionToSitTime == 0.0)
                {
                    TransitionToSitTime = curtime + 1.2;
                }
                else if (curtime > TransitionToSitTime)
                {
                    Sitting = true;
                    TransitionToSitTime = 0.0;
                }
            }
        }
        else if (ts->HeadPose.ThePose.Position.y > -0.15)
        {
            TransitionToSitTime = 0.0;
            if (Sitting)
            {
                if (TransitionToStandTime == 0.0)
                {
                    TransitionToStandTime = curtime + 1.2;
                }
                else if (curtime > TransitionToStandTime)
                {
                    Sitting = false;
                    TransitionToStandTime = 0.0;
                }
            }
        }
    }

    if (VisualizeSeatLevel && Sitting)
        ts->HeadPose.ThePose.Position.y += ExtraSittingAltitude;

}

// Determine whether this frame needs rendering based on time-warp timing and flags.
bool OculusWorldDemoApp::FrameNeedsRendering(double curtime)
{
    static double   lastUpdate          = 0.0;
    double          timeSinceLast       = curtime - lastUpdate;
    bool            updateRenderedView  = true;
    double          renderInterval      = TimewarpRenderIntervalInSeconds;
    if ( !TimewarpRenderIntervalEnabled )
    {
        renderInterval = 0.0;
    }

    if (FreezeEyeUpdate)
    {
        updateRenderedView = false;
    }
    else
    {
        if ( (timeSinceLast < 0.0) || ((float)timeSinceLast > renderInterval) )
        {
            // This allows us to do "fractional" speeds, e.g. 45fps rendering on a 60fps display.
            lastUpdate += renderInterval;
            if ( timeSinceLast > 5.0 )
            {
                // renderInterval is probably tiny (i.e. "as fast as possible")
                lastUpdate = curtime;
            }

            updateRenderedView = true;
        }
        else
        {
            updateRenderedView = false;
        }
    }

    return updateRenderedView;
}


void OculusWorldDemoApp::ApplyDynamicResolutionScaling()
{
    if (ResolutionScalingMode == ResolutionScalingMode_Off)
    {
        // Restore viewport rectangle in case dynamic res scaling was enabled before.
        for (int i = 0; i < CamRenderPoseCount; i++)
        {
            CamRenderViewports[i].SetSize ( CamRenderSize[i] );
        }
    }
    else if (ResolutionScalingMode == ResolutionScalingMode_Dynamic)
    {
        // Demonstrate dynamic-resolution rendering.
        // This demo is too simple to actually have a framerate that varies that much, so we'll
        // just pretend this is trying to cope with highly dynamic rendering load.
        float dynamicRezScale = 1.0f;

        {
            // Hacky stuff to make up a scaling...
            // This produces value oscillating as follows: 0 -> 1 -> 0.
            double curtime = ovr_GetTimeInSeconds();
            static double dynamicRezStartTime   = curtime;
            float         dynamicRezPhase       = float ( curtime - dynamicRezStartTime );
            const float   dynamicRezTimeScale   = 4.0f;

            dynamicRezPhase /= dynamicRezTimeScale;
            if ( dynamicRezPhase < 1.0f )
            {
                dynamicRezScale = dynamicRezPhase;
            }
            else if ( dynamicRezPhase < 2.0f )
            {
                dynamicRezScale = 2.0f - dynamicRezPhase;
            }
            else
            {
                // Reset it to prevent creep.
                dynamicRezStartTime = curtime;
                dynamicRezScale     = 0.0f;
            }

            // Map oscillation: 0.5 -> 1.0 -> 0.5
            dynamicRezScale = dynamicRezScale * 0.5f + 0.5f;
        }

        // This viewport is used for rendering and passed into ovr_EndEyeRender.
        for (int i = 0; i < CamRenderPoseCount; i++)
        {
            CamRenderViewports[i].SetSize ( Sizei(int(CamRenderSize[i].w  * dynamicRezScale),
                                                  int(CamRenderSize[i].h  * dynamicRezScale)) );
        }
    }
    else if (ResolutionScalingMode == ResolutionScalingMode_Adaptive)
    {
        // Restore most viewport rectangles
        for (int i = 0; i < CamRenderPoseCount; i++)
        {
            CamRenderViewports[i].SetSize ( CamRenderSize[i] );
        }

        // Now scale the two main cameras.
        ovrPerfStats perfStats = {};
        ovr_GetPerfStats(Session, &perfStats);

        AdaptivePixelDensity *= sqrtf(perfStats.AdaptiveGpuPerformanceScale);
        AdaptivePixelDensity = Alg::Clamp(AdaptivePixelDensity, 0.1f, DesiredPixelDensity);

        Sizei recommendedTex0Size = ovr_GetFovTextureSize(Session, ovrEye_Left, g_EyeFov[0], AdaptivePixelDensity);
        Sizei recommendedTex1Size = ovr_GetFovTextureSize(Session, ovrEye_Right, g_EyeFov[1], AdaptivePixelDensity);

        CamRenderViewports[0].SetSize(Sizei::Min(recommendedTex0Size, CamRenderSize[0]));
        CamRenderViewports[1].SetSize(Sizei::Min(recommendedTex1Size, CamRenderSize[1]));
    }
    else if (ResolutionScalingMode == ResolutionScalingMode_AlternatingEyes)
    {
      // Restore most viewport rectangles
      for (int i = 0; i < CamRenderPoseCount; i++)
      {
        CamRenderViewports[i].SetSize (CamRenderSize[i]);
      }

      OVR_ASSERT(ovrEye_Left == 0 && ovrEye_Right == 1);
      static int scaleEyeIndex = 0;
      scaleEyeIndex = !scaleEyeIndex;

      // Now scale the two main cameras.
      Sizei recommendedTex0Size = ovr_GetFovTextureSize(Session, ovrEye_Left, g_EyeFov[0], (scaleEyeIndex == ovrEye_Left) ? AlternatingEyeResScale : 1.0f);
      Sizei recommendedTex1Size = ovr_GetFovTextureSize(Session, ovrEye_Right, g_EyeFov[1], (scaleEyeIndex == ovrEye_Right) ? AlternatingEyeResScale: 1.0f);

      CamRenderViewports[0].SetSize(Sizei::Min(recommendedTex0Size, CamRenderSize[0]));
      CamRenderViewports[1].SetSize(Sizei::Min(recommendedTex1Size, CamRenderSize[1]));
    }
}


void OculusWorldDemoApp::UpdateFrameRateCounter(double curtime)
{
    FrameCounter++;
    TotalFrameCounter++;
    float secondsSinceLastMeasurement = (float)( curtime - LastFpsUpdate );

    if ( (secondsSinceLastMeasurement >= SecondsOfFpsMeasurement) && ( FrameCounter >= 10 ) )
    {
        SecondsPerFrame = (float)( curtime - LastFpsUpdate ) / (float)FrameCounter;
        FPS             = 1.0f / SecondsPerFrame;
        LastFpsUpdate   = curtime;
        FrameCounter    = 0;
#if 0
        char temp[1000];
        sprintf ( temp, "SPF: %f, FPS: %f\n", SecondsPerFrame, FPS );
        OutputDebugStringA ( temp );
#endif
    }
}

void OculusWorldDemoApp::RenderEyeView(CamRenderPoseEnum camNum, ovrEyeType eyeNum, const Matrix4f* optionalMatrix, bool onlyRenderWorld)
{
    Recti renderViewport = CamRenderViewports[camNum];

    // *** 3D - Configures Viewport/Projection and Render
    if (!onlyRenderWorld && (LayerCubemap != Cubemap_Off))
    {
        pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));
    }

    // Prepare projection matrix for FOV Stencil mesh
    pRender->SetViewport(0, 0, renderViewport.w, renderViewport.h);
    Matrix4f fovStencilOrtho = Matrix4f::Ortho2D(1.0f, 1.0f);

    if (FovStencilMeshes[eyeNum][FovStencilMeshMode_WriteDepth])
    {
      // we want the mesh depth to make it through the vertex shader without being modified
      fovStencilOrtho.M[2][2] = 1.0f;
      fovStencilOrtho.M[3][3] = 1.0f;

      pRender->SetProjection(fovStencilOrtho);
      pRender->SetDepthMode(true, true, RenderDevice::Compare_Always);
      pRender->SetCullMode(RenderDevice::Cull_Off);

      FovStencilMeshes[eyeNum][FovStencilMeshMode_WriteDepth]->Render(Matrix4f::Identity(), pRender);
    }
    else if (FovStencilLineCount > 0)
    {
      pRender->SetProjection(fovStencilOrtho);
      pRender->SetDepthMode(true, true, RenderDevice::Compare_Always);

      Color lineColor = 0;
      switch (FovStencilColor)
      {
      case FovStencilColor_Black: lineColor = 0xFF000000; break;
      case FovStencilColor_Green: lineColor = 0xFF00FF00; break;
      case FovStencilColor_Red:   lineColor = 0xFFFF0000; break;
      case FovStencilColor_White: lineColor = 0xFFFFFFFF; break;
      default: OVR_FAIL(); break;
      }

      pRender->RenderLines(
        FovStencilLineCount,
        lineColor,
        FovStencilLinesX[eyeNum],
        FovStencilLinesY[eyeNum]);
    }

    {
      if (DiscreteHmdRotationEnable && (DiscreteRotationScissorTech != DRScissorTechnique_None))
      {
        pRender->SetScissors(1, &DiscreteRotationScissor[eyeNum]);
        pRender->EnableScissor(true);
      }

      if (!onlyRenderWorld)
      {
        pRender->ApplyStereoParams(renderViewport, CamProjection[camNum]);
      }
    }

    pRender->SetCullMode(RenderDevice::Cull_Back);
    pRender->SetDepthMode(true, true, (DepthModifier == NearLessThanFar ?
                                       RenderDevice::Compare_Less :
                                       RenderDevice::Compare_Greater));

    Matrix4f baseTranslate = Matrix4f::Translation(ThePlayer.GetBodyPos(TrackingOriginType));
    Matrix4f baseYaw       = Matrix4f::RotationY(ThePlayer.GetApparentBodyYaw().Get());

    if ( (GridDisplayMode != GridDisplay_GridOnly) && (GridDisplayMode != GridDisplay_GridDirect) )
    {
        if (SceneMode == Scene_World || SceneMode == Scene_Cubes)
        {
            if (optionalMatrix)
            {
                MainScene.Render(pRender, *optionalMatrix);
            }
            else if (LayerCubemap == Cubemap_Off)
            {
                MainScene.Render(pRender, CamFromWorld[camNum]);
            }

            if (!onlyRenderWorld)
            {
                RenderCockpitPanels(camNum);
                RenderAnimatedBlocks(camNum, GetSceneAnimationTime());
            }

            // Boundary cubes
            if (TextScreen == Text_BoundaryInfo)
            {
                Matrix4f view = CamFromWorld[camNum] * baseTranslate * baseYaw;
                if (!onlyRenderWorld)
                {
                  RenderBoundaryScene(view);
                }
            }


			      // This renders the "floor level" under the player, useful to visualize sit floor
			      // level in standing games.
			      int visualIndex = DonutFloorVisual ? 1 : 0;
			      if (VisualizeSeatLevel && pRoundFloorModel[visualIndex])
			      {
				      pRender->SetDepthMode(false, true);
				      pRender->SetCullMode(RenderDevice::Cull_Off);
				      Vector3f pos = ThePlayer.GetHeadPosition(TrackingOriginType);
				      Matrix4f posTranslate = Matrix4f::Translation(pos);

				      // Store the neck model
				      float neckeye[2] = { OVR_DEFAULT_NECK_TO_EYE_HORIZONTAL, OVR_DEFAULT_NECK_TO_EYE_VERTICAL };
				      ovr_GetFloatArray(Session, OVR_KEY_NECK_TO_EYE_DISTANCE, neckeye, 2);
				      float	 neckToEyeVertical = neckeye[1];
				      float	 neckToEyeHorizontal = neckeye[0];
				      Vector3f neckModel(0.0, neckToEyeVertical, -neckToEyeHorizontal);
				      Vector3f rotatedNeck = ThePlayer.GetOrientation().Rotate(neckModel);

				      pos.x -= rotatedNeck.x;
				      pos.z -= rotatedNeck.z;
				      // User Pos
				      pRoundFloorModel[visualIndex]->SetPosition(Vector3f(pos.x, Sitting ? ExtraSittingAltitude : 0.002f, pos.z));
				      OculusRoundFloor[visualIndex].Render(pRender, CamFromWorld[camNum]);
				      pRender->SetCullMode(RenderDevice::Cull_Back);
				      pRender->SetDepthMode(true, true);
			      }

                  //Draw sensors
                  if (VisualizeTracker)
                  {
                    int numTrackers = ovr_GetTrackerCount(Session);
                    for (int i = 0; i < numTrackers; i++) {
                      PositionalTrackers[i].Draw(
                          Session,
                          pRender,
                          ThePlayer,
                          TrackingOriginType,
                          Sitting,
                          ExtraSittingAltitude,
                          CamFromWorld,
                          eyeNum,
                          EyeRenderPose);
                    }
                  }
    		}
        if(!onlyRenderWorld)
        {
            if (SceneMode == Scene_Cubes)
            {
                // Draw scene cubes overlay. Green if position tracked, red otherwise.
                if (HaveHMDVisionTracking)
                {
                    if (TrackingTrackerCount == ConnectedTrackerCount)
                    {
                        GreenCubesScene.Render(pRender, CamFromWorld[camNum] * baseTranslate * baseYaw);
                    }
                    else
                    {
                        YellowCubesScene.Render(pRender, CamFromWorld[camNum] * baseTranslate * baseYaw);
                    }
                }
                else
                {
                    // To consider: Check if ovrStatus_PositionValid is true and if so then draw
                    // in pink instead of red, to indicate that tracking is currently estimated.
                    // We need a central color define table. Pink = Vector4f(1, 0.75, 0.80, 1).
                    RedCubesScene.Render(pRender, CamFromWorld[camNum] * baseTranslate * baseYaw);
                }
            }

            else if (SceneMode == Scene_OculusCubes)
            {
                float heightOffset = ThePlayer.GetHeadDistanceFromTrackingOrigin(TrackingOriginType);
                Matrix4f heightTranslate = Matrix4f::Translation(0.0f, heightOffset, 0.0f);
                OculusCubesScene.Render(pRender, CamFromWorld[camNum] * baseTranslate * heightTranslate * baseYaw);
            }
            RenderControllers(camNum, eyeNum);
        }
        if (ShowCalibratedOrigin)
        {
            Matrix4f cubeScale = Matrix4f::Scaling(0.05f / BlockModelSize); // cubes have an original scale of 4 mm, and we want 5 cm.
            OVR_ASSERT ( !"fix me" );
            SmallGreenCube.Render(pRender, Matrix4f(Posef(EyeRenderPose[eyeNum]).Inverted() * Posef(CalibratedTrackingOrigin)) * cubeScale);
        }
    }

    if (GridDisplayMode != GridDisplay_None)
    {
        switch ( GridDisplayMode )
        {
        case GridDisplay_GridOnly:
            if (LayerCubemap != Cubemap_Off)
            {
                pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));
            }
        case GridDisplay_GridAndScene:
            RenderGrid(camNum, CamRenderViewports[camNum]);
            break;
        case GridDisplay_GridDirect:
            if (LayerCubemap != Cubemap_Off)
            {
                pRender->Clear(0.0f, 0.0f, 0.0f, 1.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f));
            }
            RenderGrid(camNum, Recti(HmdDesc.Resolution));
            break;
        default: OVR_ASSERT ( false ); break;
        }
    }



    // *** 2D Text - Configure Orthographic rendering.
    float    orthoDistance = MenuHudDistance;
    Vector2f orthoScale = Vector2f(1.0f) / Vector2f(EyeRenderDesc[eyeNum].PixelsPerTanAngleAtCenter);
    CamOrthoProjection[camNum] = ovrMatrix4f_OrthoSubProjection(CamProjection[camNum], orthoScale,
                                                        orthoDistance + EyeRenderDesc[eyeNum].HmdToEyePose.Position.z,
                                                        EyeRenderDesc[eyeNum].HmdToEyePose.Position.x);

    // Render UI in 2D orthographic coordinate system that maps [-1,1] range
    // to a readable FOV area centered at your eye and properly adjusted.
    pRender->ApplyStereoParams(renderViewport, CamOrthoProjection[camNum]);
    pRender->SetDepthMode(false, false);

    // Display Loading screen-shot in frame 0.
    if (LoadingState != LoadingState_Finished)
    {
        LoadingState = LoadingState_DoLoad;
    }

    if ( !LayersEnabled )
    {
        // Layers are off, so we need to draw the HUD and menu as conventional quads.

        // Default text size is 22 pixels, default distance is 0.8m
        float menuHudScale = ( MenuHudSize * 0.8f ) * ( 22.0f / MenuHudTextPixelHeight );

        // HUD is fixed to your face.
        if ( HudRenderedSize.w > 0 )
        {
            OVR_ASSERT ( HudRenderedSize.h > 0 );
            Sizei texSize = Sizei(DrawEyeTargets[Rendertarget_Hud]->pColorTex->GetWidth(), DrawEyeTargets[Rendertarget_Hud]->pColorTex->GetHeight());
            ovrRecti viewportClipRect = (ovrRecti)HudRenderedSize;
            ovrVector2f quadSize;
            quadSize.x = menuHudScale * (float)HudRenderedSize.w;
            quadSize.y = menuHudScale * (float)HudRenderedSize.h;
            Vector2f uvTL = Vector2f ( (float)viewportClipRect.Pos.x, (float)viewportClipRect.Pos.y );
            Vector2f uvBR = Vector2f ( (float)viewportClipRect.Pos.x + (float)viewportClipRect.Size.w, (float)viewportClipRect.Pos.y + (float)viewportClipRect.Size.h );
            uvTL.x *= 1.0f / (float)texSize.w;
            uvTL.y *= 1.0f / (float)texSize.h;
            uvBR.x *= 1.0f / (float)texSize.w;
            uvBR.y *= 1.0f / (float)texSize.h;
            if ( TextureOriginAtBottomLeft )
            {
                uvTL.y = 1.0f - uvTL.y;
                uvBR.y = 1.0f - uvBR.y;
            }
            Color c = Color ( 255, 255, 255, 255 );

            pRender->FillTexturedRect ( -quadSize.x * 0.5f, -quadSize.y * 0.5f, quadSize.x * 0.5f, quadSize.y * 0.5f,
                                        uvTL.x, uvTL.y, uvBR.x, uvBR.y, c, DrawEyeTargets[Rendertarget_Hud]->pColorTex, NULL, true );
        }

        // Menu is fixed to your face.
        if ( ( MenuRenderedSize.w > 0 ) && LayerHudMenuEnabled )
        {
            OVR_ASSERT ( MenuRenderedSize.h > 0 );
            Sizei texSize = Sizei(DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetWidth(), DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetHeight());
            ovrRecti viewportClipRect = (ovrRecti)MenuRenderedSize;
            ovrVector2f quadSize;
            quadSize.x = menuHudScale * (float)MenuRenderedSize.w;
            quadSize.y = menuHudScale * (float)MenuRenderedSize.h;
            Vector2f uvTL = Vector2f ( (float)viewportClipRect.Pos.x, (float)viewportClipRect.Pos.y );
            Vector2f uvBR = Vector2f ( (float)viewportClipRect.Pos.x + (float)viewportClipRect.Size.w, (float)viewportClipRect.Pos.y + (float)viewportClipRect.Size.h );
            uvTL.x *= 1.0f / (float)texSize.w;
            uvTL.y *= 1.0f / (float)texSize.h;
            uvBR.x *= 1.0f / (float)texSize.w;
            uvBR.y *= 1.0f / (float)texSize.h;
            if ( TextureOriginAtBottomLeft )
            {
                uvTL.y = 1.0f - uvTL.y;
                uvBR.y = 1.0f - uvBR.y;
            }
            Color c = Color ( 255, 255, 255, 255 );

            pRender->FillTexturedRect ( -quadSize.x * 0.5f, -quadSize.y * 0.5f, quadSize.x * 0.5f, quadSize.y * 0.5f,
                                        uvTL.x, uvTL.y, uvBR.x, uvBR.y, c, DrawEyeTargets[Rendertarget_Menu]->pColorTex, NULL, true );
        }

        // Load-screen screen shot image if loading
        if ( ( LoadingState == LoadingState_DoLoad ) && ( LoadingTexture != NULL ) )
        {
            Sizei texSize = Sizei(LoadingTexture->GetWidth(), LoadingTexture->GetHeight());
            ovrRecti viewportClipRect = Recti(texSize);
            ovrVector2f quadSize;
            // TODO : A better way to select the quadSize
            quadSize.x = 2.5f * (float)texSize.w;
            quadSize.y = 2.5f * (float)texSize.h;
            Vector2f uvTL = Vector2f((float)viewportClipRect.Pos.x, (float)viewportClipRect.Pos.y);
            Vector2f uvBR = Vector2f((float)viewportClipRect.Pos.x + (float)viewportClipRect.Size.w, (float)viewportClipRect.Pos.y + (float)viewportClipRect.Size.h);
            uvTL.x *= 1.0f / (float)texSize.w;
            uvTL.y *= 1.0f / (float)texSize.h;
            uvBR.x *= 1.0f / (float)texSize.w;
            uvBR.y *= 1.0f / (float)texSize.h;
            if (TextureOriginAtBottomLeft)
            {
                uvTL.y = 1.0f - uvTL.y;
                uvBR.y = 1.0f - uvBR.y;
            }
            Color c = Color(255, 255, 255, 255);

            pRender->FillTexturedRect( -quadSize.x * 0.5f, -quadSize.y * 0.5f, quadSize.x * 0.5f, quadSize.y * 0.5f,
                                       uvTL.x, uvTL.y, uvBR.x, uvBR.y, c, LoadingTexture, NULL, true);
        }
    }

    if (DiscreteHmdRotationEnable && (DiscreteRotationScissorTech != DRScissorTechnique_None))
    {
      pRender->EnableScissor(false);
    }

#if 0
    // Disabled as it's not needed since the compositor deals with this
    // Clear out the depth cull region back to the far plane to make sure PTW plays nice
    if (FovStencilMeshes[eyeNum][FovStencilMeshMode_UndoDepth])
    {
      pRender->SetProjection(fovStencilOrtho);
      pRender->SetDepthMode(true, true, RenderDevice::Compare_Always);
      pRender->SetCullMode(RenderDevice::Cull_Off);

      FovStencilMeshes[eyeNum][FovStencilMeshMode_UndoDepth]->Render(Matrix4f::Identity(), pRender);
    }
#endif
}


bool OculusWorldDemoApp::SetupMixedRealityCapture()
{
    NumExternalCameraProperties = OVR_MAX_EXTERNAL_CAMERA_COUNT;
    ovrResult error = ovr_GetExternalCameras(Session, &ExternalCameras[0], &NumExternalCameraProperties);   // Here the 2nd parameter is the actual size of camera array
    if (OVR_FAILURE(error) && error != ovrError_NoExternalCameraInfo) {
        DisplayLastErrorMessageBox("ovr_GetExternalCameras failure.");
        return false;
    }
    if (NumExternalCameraProperties == 0) {
        DisplayLastErrorMessageBox(
          "Please send external camera calibration info to service via CameraTool.exe.");
        return false;
    }
    // TO DO : need to set this by the user's camera name string
    CurrentCameraID = 0;

    WindowSize.w = ExternalCameras[CurrentCameraID].Intrinsics.ImageSensorPixelResolution.w;
    WindowSize.h = ExternalCameras[CurrentCameraID].Intrinsics.ImageSensorPixelResolution.h;
    // We have a 2X2 window for the Mixed Reality Capture mode. The top left quadrant is the mirror view;
    // the top right quadrant is the foreground view; the bottom left quadrant is the hands/touches view;
    // the bottom right quadrant is the background view.

    NearRenderViewport = Recti(Vector2i(WindowSize.w, 0), WindowSize);
    FarRenderViewport = Recti(Vector2i(WindowSize.w, WindowSize.h), WindowSize);
    HandsRenderViewport = Recti(Vector2i(0, WindowSize.h), WindowSize);

    Sizei tempWindowSize;
    tempWindowSize.w = 2 * WindowSize.w;
    tempWindowSize.h = 2 * WindowSize.h;

    RenderParams.Resolution = tempWindowSize;
    if (pRender != nullptr)
    {
        pRender->SetWindowSize(tempWindowSize.w, tempWindowSize.h);
        pRender->SetParams(RenderParams);
    }
    pPlatform->SetWindowSize(tempWindowSize.w, tempWindowSize.h);   // resize the window
    return true;
}

void OculusWorldDemoApp::CleanupMixedRealityCapture()
{
    NumExternalCameraProperties = 0;
    CurrentCameraID = -1;
    WindowSize.w = DefaultWindowWidth;
    WindowSize.h = DefaultWindowHeight;
    RenderParams.Resolution = WindowSize;
    if (pRender != nullptr)
    {
        pRender->SetWindowSize(WindowSize.w, WindowSize.h);
        pRender->SetParams(RenderParams);
    }
    pPlatform->SetWindowSize(WindowSize.w, WindowSize.h);   // resize the window
}


void OculusWorldDemoApp::RenderCamNearFarHandsView()
{
    Posef tempPose;
    tempPose.SetIdentity();     // fixed camera pose
    if (ExternalCameras[CurrentCameraID].Extrinsics.AttachedToDevice ==  ovrTrackedDevice_LTouch)
        tempPose = HandPoses[0];
    else if (ExternalCameras[CurrentCameraID].Extrinsics.AttachedToDevice == ovrTrackedDevice_LTouch)
        tempPose = HandPoses[1];
    else if (ExternalCameras[CurrentCameraID].Extrinsics.AttachedToDevice == ovrTrackedDevice_Object0)
        tempPose = TrackedObjectPose;

    Posef CamPose = tempPose * Posef(ExternalCameras[CurrentCameraID].Extrinsics.RelativePose);

    Posef CamPosePlayer = ThePlayer.VirtualWorldTransformfromRealPose(CamPose, TrackingOriginType);
    Vector3f up = CamPosePlayer.Rotation.Rotate(UpVector);
    Vector3f forward = CamPosePlayer.Rotation.Rotate(ForwardVector);
    Vector3f dif = ThePlayer.GetHeadPosition(TrackingOriginType) - CamPosePlayer.Translation;
    DistanceToCamInMeters = forward.Dot(dif);
    EyeRenderPose[CamRenderPose_ExternalCamera] = ovrPosef(CamPose);  // external camera pose

    unsigned int projectionModifier = createProjectionModifier();

    CamFromWorld[CamRenderPose_ExternalCamera] = Matrix4f::LookAtRH(CamPosePlayer.Translation, CamPosePlayer.Translation + forward, up);

    // near view
    ExternalCamProjection = ovrMatrix4f_Projection(ExternalCameras[CurrentCameraID].Intrinsics.FOVPort,
        ExternalCameras[CurrentCameraID].Intrinsics.VirtualNearPlaneDistanceMeters,
        DistanceToCamInMeters + NearFarOverlapInMeters,    // extend foreground viewing volume with 0.1 meter to avoid the seam between foreground and background images
        projectionModifier);
    pRender->ApplyStereoParams(NearRenderViewport, ExternalCamProjection);
    pRender->SetDepthMode(true, true, (DepthModifier == NearLessThanFar ?
        RenderDevice::Compare_Less :
        RenderDevice::Compare_Greater));

    if ((GridDisplayMode != GridDisplay_GridOnly) && (GridDisplayMode != GridDisplay_GridDirect))
    {
        if (SceneMode == Scene_World || SceneMode == Scene_Cubes)
        {
            MainScene.Render(pRender, CamFromWorld[CamRenderPose_ExternalCamera]);
        }
    }

    // far view
    ExternalCamProjection = ovrMatrix4f_Projection(ExternalCameras[CurrentCameraID].Intrinsics.FOVPort,
        DistanceToCamInMeters,
        ExternalCameras[CurrentCameraID].Intrinsics.VirtualFarPlaneDistanceMeters,
        projectionModifier);
    pRender->ApplyStereoParams(FarRenderViewport, ExternalCamProjection);
    pRender->SetDepthMode(true, true, (DepthModifier == NearLessThanFar ?
        RenderDevice::Compare_Less :
        RenderDevice::Compare_Greater));

    if ((GridDisplayMode != GridDisplay_GridOnly) && (GridDisplayMode != GridDisplay_GridDirect))
    {
        if (SceneMode == Scene_World || SceneMode != Scene_Cubes)
        {
            MainScene.Render(pRender, CamFromWorld[CamRenderPose_ExternalCamera]);
        }
    }

    // hands view
    ExternalCamProjection = ovrMatrix4f_Projection(ExternalCameras[CurrentCameraID].Intrinsics.FOVPort,
        ExternalCameras[CurrentCameraID].Intrinsics.VirtualNearPlaneDistanceMeters,
        ExternalCameras[CurrentCameraID].Intrinsics.VirtualFarPlaneDistanceMeters,
        projectionModifier);
    pRender->ApplyStereoParams(HandsRenderViewport, ExternalCamProjection);
    pRender->SetDepthMode(true, true, (DepthModifier == NearLessThanFar ?
        RenderDevice::Compare_Less :
        RenderDevice::Compare_Greater));

    if ((GridDisplayMode != GridDisplay_GridOnly) && (GridDisplayMode != GridDisplay_GridDirect))
    {
        if (SceneMode == Scene_World || SceneMode == Scene_Cubes)
        {
            RenderControllers(CamRenderPose_ExternalCamera, ovrEye_Count);   // from the external camera
        }
    }
}

void OculusWorldDemoApp::RenderCubemap()
{
    int numFaces = 6;
    int cubeMapSize = 1024;
    Matrix4f currCubemapCameraView; // This will replace ViewFromWorld[eye] in RenderEyeView

    uint64_t format = GetRenderDeviceTextureFormatForEyeTextureFormat(EyeTextureFormat);
    Texture* color = pRender->CreateTexture(
        format | Texture_RenderTarget | Texture_Cubemap | Texture_SwapTextureSetStatic | Texture_GenMipmapsBySdk,
        cubeMapSize, cubeMapSize, NULL, 1, nullptr);
    Texture* depth = pRender->GetDepthBuffer(cubeMapSize, cubeMapSize, 1, Texture_Depth32f);

    // Set new projection matrix with FOV 45 degrees on all sides
    FovPort newFOV = FovPort(1.0f, 1.0f, 1.0f, 1.0f);
    unsigned int projectionModifier = createProjectionModifier();
    Matrix4f projection = ovrMatrix4f_Projection(newFOV, NearClip, FarClip, projectionModifier);

    Recti renderViewport = Recti(0, 0, cubeMapSize, cubeMapSize);

    ovrPosef localEyeRenderPose[2] = { EyeRenderPose[0], EyeRenderPose[1] };
    float localPositionTrackingScale = PositionTrackingScale;
    localEyeRenderPose[0].Position = ((Vector3f)EyeRenderPose[0].Position) * localPositionTrackingScale;
    localEyeRenderPose[0].Orientation = Quatf::Identity();

    Matrix4f worldPose = (Matrix4f)ThePlayer.VirtualWorldTransformfromRealPose(localEyeRenderPose[0], TrackingOriginType);
    Vector3f transl = worldPose.GetTranslation();
    worldPose.SetIdentity();
    worldPose.SetTranslation(transl);

    for (int faceIndex = 0; faceIndex < numFaces; ++faceIndex)
    {
        switch (faceIndex)
        {
        case 0:
            currCubemapCameraView = Matrix4f::RotationY(-MATH_FLOAT_PIOVER2) * worldPose; break; // Positive X
        case 1:
            currCubemapCameraView = Matrix4f::RotationY(MATH_FLOAT_PIOVER2) * worldPose; break; // Negative X
        case 2:
            currCubemapCameraView = Matrix4f::RotationZ(MATH_FLOAT_PI) * Matrix4f::RotationX(-MATH_FLOAT_PIOVER2) * worldPose; break; // Positive Y
        case 3:
            currCubemapCameraView = Matrix4f::RotationZ(MATH_FLOAT_PI) * Matrix4f::RotationX(MATH_FLOAT_PIOVER2) * worldPose; break; // Negative Y
        case 4:
            currCubemapCameraView = Matrix4f::RotationY(MATH_FLOAT_PI) * worldPose; break; // Positive Z
        case 5:
            currCubemapCameraView = worldPose; break; // Negative Z
        }
        currCubemapCameraView.SetTranslation(-(currCubemapCameraView.GetTranslation()));

        // Give the current face index to set the current render target view
        pRender->SetRenderTarget(color, depth, NULL, faceIndex);
        pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f, (DepthModifier == NearLessThanFar ? 1.0f : 0.0f), true, true, faceIndex);

        // Set viewport and projection outside of RenderEyeView
        pRender->ApplyStereoParams(renderViewport, projection);
        RenderEyeView(CamRenderPose_Left, ovrEye_Left, &currCubemapCameraView, true); // Give an optional matrix and boolean indicating not to render certain layers
    }
    // Flip each face of cubemap if OpenGL
    if (TextureOriginAtBottomLeft)
    {
        Ptr<Texture> tempTex = pRender->CreateTexture(format, cubeMapSize, cubeMapSize, NULL, 1, nullptr);
        pRender->BltFlipCubemap(color, tempTex);
    }
    // Save cubemap texture to DDS file if Direct3D
    else if(LayerCubemapSaveOnRender)
    {
        std::string error;

        // strip off the filename - does the same thing as GetFilePath, but also removes the last /
        const char* filename = 0;
        ScanFilePath(MainFilePath.c_str(), &filename, 0);
        std::string saveFileName = std::string(MainFilePath.c_str(), filename ? (filename - MainFilePath.c_str() - 1) : MainFilePath.size());

        bool result = pRender->SaveCubemapTexture(color, transl, saveFileName, &error);
        if (!result)
        {
            std::string message = "Cubemap texture render save failure: " + error;
            OVR_FAIL_M(message.c_str());
        }
    }
    color->Commit();
    CubemapRenderTexture = color;
    CubeRendered = true;
}

void OculusWorldDemoApp::RenderCockpitPanels(CamRenderPoseEnum camNum)
{
    if ( LayersEnabled )
    {
        // Using actual layers, not rendering quads.
        return;
    }

    if ( LayerCockpitEnabled != 0 )
    {
        for ( int cockpitLayer = 0; cockpitLayer < 5; cockpitLayer++ )
        {
            if ( ( LayerCockpitEnabled & ( 1<<cockpitLayer ) ) != 0 )
            {
                Sizei texSize = Sizei(CockpitPanelTexture->GetWidth(), CockpitPanelTexture->GetHeight());
                ovrRecti viewportClipRect = CockpitClipRect[cockpitLayer];
                Vector2f sizeInMeters = CockpitPanelSize[cockpitLayer];
                Vector2f uvTL = Vector2f ( (float)viewportClipRect.Pos.x, (float)viewportClipRect.Pos.y );
                Vector2f uvBR = Vector2f ( (float)viewportClipRect.Pos.x + (float)viewportClipRect.Size.w, (float)viewportClipRect.Pos.y + (float)viewportClipRect.Size.h );
                uvTL.x *= 1.0f / (float)texSize.w;
                uvTL.y *= 1.0f / (float)texSize.h;
                uvBR.x *= 1.0f / (float)texSize.w;
                uvBR.y *= 1.0f / (float)texSize.h;
                Color c = Color ( 255, 255, 255, 255 );

                // Pos+orn in the world.
                Posef pose = CockpitPanelPose[cockpitLayer];
                Matrix4f matrix = CamFromWorld[camNum] * Matrix4f ( pose );

                pRender->FillTexturedRect ( -sizeInMeters.x * 0.5f, sizeInMeters.y * 0.5f, sizeInMeters.x * 0.5f, -sizeInMeters.y * 0.5f,
                                            uvTL.x, uvTL.y, uvBR.x, uvBR.y, c, CockpitPanelTexture, &matrix, true );
            }
        }
    }
}




// Returns rendered bounds.
Recti OculusWorldDemoApp::RenderMenu(float textHeight)
{
    OVR_UNUSED ( textHeight );

    pRender->SetRenderTarget ( DrawEyeTargets[Rendertarget_Menu]->pColorTex, NULL);
    pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f);
    pRender->SetDepthMode ( false, false );
    Recti vp;
    vp.x = 0;
    vp.y = 0;
    vp.w = DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetWidth();
    vp.h = DrawEyeTargets[Rendertarget_Menu]->pColorTex->GetHeight();
    pRender->SetViewport(vp);

    float centerX = 0.5f * (float)vp.w;
    float centerY = 0.5f * (float)vp.h;

    // This sets up a coordinate system with origin at top-left and units of a pixel.
    Matrix4f ortho;
    ortho.SetIdentity();
    ortho.M[0][0] = 2.0f / (vp.w);       // X scale
    ortho.M[0][3] = -1.0f;               // X offset
    ortho.M[1][1] = -2.0f / (vp.h);      // Y scale (for Y=down)
    ortho.M[1][3] = 1.0f;                // Y offset (Y=down)
    ortho.M[2][2] = 0;
    pRender->SetProjection(ortho);

    Recti rect = Menu.Render(pRender, "", textHeight, centerX, centerY);

    if (rect.w > 0 && rect.h > 0) {
    // Commit changes to the menu swap chain
    DrawEyeTargets[Rendertarget_Menu]->pColorTex->Commit();
    }

    return rect;
}



// NOTE - try to keep these in sync with the PDF docs!
static const char* HelpText1 =
    "Spacebar 	            \t500 Toggle debug info overlay\n"
    "Tab                    \t500 Toggle options menu\n"
    "W, S            	    \t500 Move forward, back\n"
    "A, D 		    	    \t500 Strafe left, right\n"
    "Mouse move 	        \t500 Look left, right\n"
    "Left gamepad stick     \t500 Move\n"
    "Right gamepad stick    \t500 Turn\n"
    "T			            \t500 Reset player position";

static const char* HelpText2 =
    "R              \t250 Reset sensor orientation\n"
    "G 			    \t250 Cycle grid overlay mode\n"
    "-, +           \t250 Adjust eye height\n"
    "Esc            \t250 Cancel full-screen\n"
    "F4			    \t250 Multisampling toggle\n"
    "F9             \t250 Hardware full-screen (low latency)\n"
    "F11            \t250 Cycle Performance HUD modes\n"
    "Ctrl+Q		    \t250 Quit";


void FormatLatencyReading(char* buff, size_t size, float val)
{
    snprintf(buff, size, "%4.2fms", val * 1000.0f);
}


// Returns rendered bounds.
Recti OculusWorldDemoApp::RenderTextInfoHud(float textHeight)
{
    Recti bounds;
    float hmdYaw, hmdPitch, hmdRoll;

    pRender->SetRenderTarget ( DrawEyeTargets[Rendertarget_Hud]->pColorTex, NULL);
    pRender->Clear(0.0f, 0.0f, 0.0f, 0.0f);
    pRender->SetDepthMode ( false, false );
    Recti vp;
    vp.x = 0;
    vp.y = 0;
    vp.w = DrawEyeTargets[Rendertarget_Hud]->pColorTex->GetWidth();
    vp.h = DrawEyeTargets[Rendertarget_Hud]->pColorTex->GetHeight();
    pRender->SetViewport(vp);

    float centerX = 0.5f * (float)vp.w;
    float centerY = 0.5f * (float)vp.h;

    // This sets up a coordinate system with origin at top-left and units of a pixel.
    Matrix4f ortho;
    ortho.SetIdentity();
    ortho.M[0][0] = 2.0f / (vp.w);       // X scale
    ortho.M[0][3] = -1.0f;               // X offset
    ortho.M[1][1] = -2.0f / (vp.h);      // Y scale (for Y=down)
    ortho.M[1][3] = 1.0f;                // Y offset (Y=down)
    ortho.M[2][2] = 0;
    pRender->SetProjection(ortho);

    switch(TextScreen)
    {
    case Text_Info:
    {
        char buf[2048];

        char fovText[1024*(CamRenderPose_Count+1)];
        char fovTextLine[1024];
        fovText[0] = '\0';

        int totalPixels = 0;
        for (int camNum = 0; camNum < CamRenderPoseCount; camNum++ )
        {
            snprintf (fovTextLine, sizeof(fovTextLine),
                " %i: FOV %3.1fx%3.1f, Resolution: %ix%i\n",
                camNum,
                CamFovPort[camNum].GetHorizontalFovDegrees(),
                CamFovPort[camNum].GetVerticalFovDegrees(),
                CamRenderSize[camNum].w,
                CamRenderSize[camNum].h );
            strncat ( fovText, fovTextLine, sizeof (fovText) );
            totalPixels += CamRenderSize[camNum].w * CamRenderSize[camNum].h;
        }


        FovPort leftFov  = g_EyeFov[0];
        FovPort rightFov = g_EyeFov[1];
        int sqrtPixels = (int)floorf(0.5f+sqrtf((float)totalPixels));

        snprintf (fovTextLine, sizeof(fovTextLine),
            " Total: FOV %3.1fx%3.1f, %.1fMpixels = %ix%i",
            (leftFov.GetHorizontalFovDegrees() + rightFov.GetHorizontalFovDegrees()) * 0.5f,
            (leftFov.GetVerticalFovDegrees()   + rightFov.GetVerticalFovDegrees())   * 0.5f,
            (1.0f/1000000.0f) * (float)totalPixels,
            sqrtPixels,
            sqrtPixels );
        strncat ( fovText, fovTextLine, sizeof (fovText) );


        // No DK2, no message.
        char latency2Text[128] = "";
        {
            static const int NUM_LATENCIES = 5;
            float latencies[NUM_LATENCIES] = {};
            if (ovr_GetFloatArray(Session, "DK2Latency", latencies, NUM_LATENCIES) == NUM_LATENCIES)
            {
                bool nonZero = false;
                char text[5][32];
                for (int i = 0; i < NUM_LATENCIES; ++i)
                {
                    FormatLatencyReading(text[i], sizeof(text[i]), latencies[i]);
                    nonZero |= (latencies[i] != 0.f);
                }

                if (nonZero)
                {
                    // MTP: Motion-to-Photon
                    snprintf(latency2Text, sizeof(latency2Text),
                        " M2P Latency  App: %s  TWrp: %s\n"
                        " PostPresent: %s  Err: %s %s",
                        text[0], text[1], text[2], text[3], text[4]);
                }
                else
                {
                    snprintf(latency2Text, sizeof(latency2Text),
                        " M2P Latency  (Readings unavailable.)");
                }
            }
        }

        Vector3f bodyPosFromOrigin = ThePlayer.GetBodyPos(TrackingOriginType);

        float ipd = EyeRenderDesc[1].HmdToEyePose.Position.x - EyeRenderDesc[0].HmdToEyePose.Position.x;

        ThePlayer.HeadPose.Rotation.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&hmdYaw, &hmdPitch, &hmdRoll);
        snprintf(buf, sizeof(buf),
                    " HMD Pos: %4.4f  %4.4f  %4.4f\n"
                    " HMD YPR: %4.2f  %4.2f  %4.2f\n"
                    " Player Pos: %3.2f  %3.2f  %3.2f  Player Yaw:%4.0f\n"
                    " FPS: %.1f  ms/frame: %.1f  Frame: %03d %d\n\n"
                    " HMD: %s\n"
                    " RecSpec res: %s  Audio mirror: %s\n"
                    " Shutter type: %s, IAD: %.1fmm\n"
                    " EyeHeight: %3.2f, IPD: %.1fmm\n"
                    "%s\n\n"
                    "%s",
                    ThePlayer.HeadPose.Translation.x, ThePlayer.HeadPose.Translation.y, ThePlayer.HeadPose.Translation.z,
                    RadToDegree(hmdYaw), RadToDegree(hmdPitch), RadToDegree(hmdRoll),
                    bodyPosFromOrigin.x, bodyPosFromOrigin.y, bodyPosFromOrigin.z,
                    RadToDegree(ThePlayer.BodyYaw.Get()),       // deliberately not GetApparentBodyYaw()
                    FPS, SecondsPerFrame * 1000.0f, FrameCounter, TotalFrameCounter % 2,
                    HmdDesc.ProductName,
                    ovr_GetBool(Session, "server:UseRecSpecResolution", false) ? "true" : "false", ovr_GetBool(Session, "AudioMirroringEnabled", false) ? "true" : "false",
                    ShutterType.c_str(),
                    InterAxialDistance * 1000.0f,   // convert to millimeters
                    ThePlayer.UserStandingEyeHeight,
                    ipd * 1000.0f, // convert to millimeters
                    fovText,
                    latency2Text
                    );

#if 0   // Enable if interested in texture memory usage stats
        size_t texMemInMB = pRender->GetTotalTextureMemoryUsage() / (1024 * 1024); // 1 MB
        if (texMemInMB)
        {
            char gpustat[256];
            snprintf(gpustat, sizeof(gpustat), "\nGPU Tex: %u MB", texMemInMB);
            OVR_strcat(buf, sizeof(buf), gpustat);
        }
#endif

        bounds = DrawTextBox(pRender, centerX, centerY, textHeight, buf, DrawText_Center);
    }
    break;

    case Text_Timing:
        bounds = Profiler.DrawOverlay(pRender, centerX, centerY, textHeight);
        break;

    case Text_PerfStats:
        {
            ovrPerfStats perfStats = {};
            ovr_GetPerfStats(Session, &perfStats);

            // Since AdaptiveGpuPerformanceScale only becomes non-1.0 once every couple of seconds
            // for visualization purposes of the value, we need to hold onto the values when it is not 1.0
            // So we end up doing this weird looking bit of code
            static float non10GpuPerfScale = perfStats.AdaptiveGpuPerformanceScale * 100.0f;
            static int numFramesGpuPerfScaleWasntUpdated = 0;
            const int MaxNumFramesGpuPerfScaleIsFrozen = 200;   // roughly > 2 seconds, but doesn't really matter
            if (perfStats.AdaptiveGpuPerformanceScale != 1.0 ||
                numFramesGpuPerfScaleWasntUpdated > MaxNumFramesGpuPerfScaleIsFrozen)
            {
                non10GpuPerfScale = perfStats.AdaptiveGpuPerformanceScale * 100.0f;
                numFramesGpuPerfScaleWasntUpdated = 0;
            }

            numFramesGpuPerfScaleWasntUpdated++;

            char buf[2048];
            if (perfStats.FrameStatsCount >= 1)
            {
                // Get the first entry which is the most recent
                ovrPerfStatsPerCompositorFrame& newestFramePerfStats = perfStats.FrameStats[0];

                snprintf(buf, sizeof(buf),
                    " SDK Performance Stats\n\n"
                    " HMD Frame #:\t400 %d\n"
                    " App Frame #:\t400 %d\t650 App Dropped Count:\t1200 %d\t1650\n"
                    " Comp Frame #:\t400 %d\t650 Comp Dropped Count:\t1200 %d\t1650\n\n"
                    " App Latency (ms):\t580 %4.2f\t770 Comp Latency (ms):\t1450 %4.2f\t1650\n"
                    " App GPU Time (ms):\t580 %4.2f\t770 App CPU Time (ms):\t1450 %4.2f\t1650\n"
                    " App Q-Ahead (ms):\t580 %4.2f\n"
                    " Comp GPU Time (ms):\t580 %4.2f\t770 Comp CPU Time (ms):\t1450 %4.2f\n"
                    " Comp CPU-to-GPU (ms):\t580 %4.2f\t770 Comp Present Buffer (ms):\t1450 %4.2f\n\n"
                    " ASW Status:\t330 %s\t770 ASW Toggle Count:\t1250 %d\n"
                    " ASW Present Count:\t550 %d\t770 ASW Fail Count:\t1250 %d\n\n"
                    " Perf Frame Count: %d / %d\t770 Dropped Stats: %s\n\n"
                    " Current GPU Perf: %4.2f%% \t770 Latched GPU Perf: %4.2f%%\n\n"
                    " < Press KeyPad \'/\' to reset stats >",
                    newestFramePerfStats.HmdVsyncIndex,
                    newestFramePerfStats.AppFrameIndex, newestFramePerfStats.AppDroppedFrameCount,
                    newestFramePerfStats.CompositorFrameIndex, newestFramePerfStats.CompositorDroppedFrameCount,
                    newestFramePerfStats.AppMotionToPhotonLatency * 1000.0f, newestFramePerfStats.CompositorLatency * 1000.0f,
                    newestFramePerfStats.AppGpuElapsedTime * 1000.0f, newestFramePerfStats.AppCpuElapsedTime * 1000.0f,
                    newestFramePerfStats.AppQueueAheadTime * 1000.0f,
                    newestFramePerfStats.CompositorGpuElapsedTime * 1000.0f, newestFramePerfStats.CompositorCpuElapsedTime * 1000.0f,
                    newestFramePerfStats.CompositorCpuStartToGpuEndElapsedTime * 1000.0f, newestFramePerfStats.CompositorGpuEndToVsyncElapsedTime * 1000.0f,
                    perfStats.AswIsAvailable ? (newestFramePerfStats.AswIsActive ? "Available - On" : "Available - Off") : "Not Available", newestFramePerfStats.AswActivatedToggleCount,
                    newestFramePerfStats.AswPresentedFrameCount, newestFramePerfStats.AswFailedFrameCount,
                    perfStats.FrameStatsCount, ovrMaxProvidedFrameStats, perfStats.AnyFrameStatsDropped ? "Yes" : "No",
                    perfStats.AdaptiveGpuPerformanceScale * 100.0f, non10GpuPerfScale);
            }
            else
            {
                snprintf(buf, sizeof(buf),
                    " SDK Performance Stats\n\n"
                    " HMD Frame #:\t400 N/A\n"
                    " App Frame #:\t400 N/A\t650 App Dropped Count:\t1200 N/A\t1650\n"
                    " Comp Frame #:\t400 N/A\t650 Comp Dropped Count:\t1200 N/A\t1650\n\n"
                    " App Latency (ms):\t550 N/A\t770 Comp Latency (ms):\t1310 N/A\t1650\n"
                    " App GPU Time (ms):\t550 N/A\t770 App CPU Time (ms):\t1310 N/A\t1650\n"
                    " App Q-Ahead (ms):\t580 N/A\n"
					" Comp GPU Time (ms):\t550 N/A\t770 Comp CPU Time (ms):\t1310 N/A\n"
                    " Comp CPU-to-GPU Time (ms):\t750 N/A\n\n"
                    " ASW Status:\t330 %s - N/A\t770 ASW Toggles: N/A\n"
                    " ASW Present Count:\t550 N/A\t770 ASW Fail Count: N/A\n\n"
                    " Perf Frame Count: %d / %d\t770 Dropped Stats: N/A\n\n"
                    " Current GPU Perf: %4.2f%% \t770 Latched GPU Perf: %4.2f%%\n\n"
                    " < Press KeyPad \'/\' to reset stats >",
                    perfStats.AswIsAvailable ? "Available" : "Not Available",
                    perfStats.FrameStatsCount, ovrMaxProvidedFrameStats, perfStats.AdaptiveGpuPerformanceScale * 100.0f, non10GpuPerfScale);
            }
            bounds = DrawTextBox(pRender, centerX, centerY, textHeight, buf, DrawText_Center);
        }
        break;

    case Text_TouchState:
        bounds = Recti(0, 0, 0, 0);
        bounds = RenderControllerStateHud(centerX, centerY, textHeight, InputState,
                                          ovrControllerType_Remote | ovrControllerType_Touch);
        break;

    case Text_GamepadState:
        bounds = Recti(0, 0, 0, 0);
        bounds = RenderControllerStateHud(centerX, centerY, textHeight, GamepadInputState, ovrControllerType_XBox);
        break;

    case Text_ActiveControllerState:
        bounds = Recti(0, 0, 0, 0);
        bounds = RenderControllerStateHud(centerX, centerY, textHeight, ActiveControllerState, ActiveControllerState.ControllerType);
        break;

    case Text_Help1:
        bounds = DrawTextBox(pRender, centerX, centerY, textHeight, HelpText1, DrawText_Center);
        break;
    case Text_Help2:
        bounds = DrawTextBox(pRender, centerX, centerY, textHeight, HelpText2, DrawText_Center);
        break;
    case Text_BoundaryInfo:
    {
        ovrBoundaryTestResult result_1 = {};
        ovrBoundaryTestResult result_2 = {};
        ovrBoundaryTestResult result_3 = {};
        ovrBool isVisible = false;

        ovr_TestBoundary(Session, ovrTrackedDevice_HMD, ovrBoundary_Outer, &result_1);
        ovr_TestBoundary(Session, ovrTrackedDevice_LTouch, ovrBoundary_Outer, &result_2);
        ovr_TestBoundary(Session, ovrTrackedDevice_RTouch, ovrBoundary_Outer, &result_3);
        ovr_GetBoundaryVisible(Session, &isVisible);
        char buf[1024];
        snprintf(buf, sizeof(buf),
            " Boundary System Information \n\n"
            "HMD \n"
            "    Triggering:     %s\n"
            "    Distance:       %.3f\n"
            "    Point:          (%.3f, %.3f, %.3f)\n"
            "    Normal:         (%.3f, %.3f, %.3f)\n\n"
            "Left Touch \n"
            "    Triggering:     %s\n"
            "    Distance:       %.3f\n"
            "    Point:          (%.3f, %.3f, %.3f)\n"
            "    Normal:         (%.3f, %.3f, %.3f)\n\n"
            "Right Touch \n"
            "    Triggering:     %s\n"
            "    Distance:       %.3f\n"
            "    Point:          (%.3f, %.3f, %.3f)\n"
            "    Normal:         (%.3f, %.3f, %.3f)\n\n"
            "Is Boundary Visible? %s \n\n"
            "These button presses will only work if this menu is visible: \n"
            "Press 'A' (lower) on right touch to force boundary to visible \n"
            "Press 'X' (lower) on left touch to reset color\n"
            , (result_1.IsTriggering) ? "YES" : "NO", result_1.ClosestDistance, result_1.ClosestPoint.x, result_1.ClosestPoint.y, result_1.ClosestPoint.z, result_1.ClosestPointNormal.x, result_1.ClosestPointNormal.y, result_1.ClosestPointNormal.z,
            (result_2.IsTriggering) ? "YES" : "NO", result_2.ClosestDistance, result_2.ClosestPoint.x, result_2.ClosestPoint.y, result_2.ClosestPoint.z, result_2.ClosestPointNormal.x, result_2.ClosestPointNormal.y, result_2.ClosestPointNormal.z,
            (result_3.IsTriggering) ? "YES" : "NO", result_3.ClosestDistance, result_3.ClosestPoint.x, result_3.ClosestPoint.y, result_3.ClosestPoint.z, result_3.ClosestPointNormal.x, result_3.ClosestPointNormal.y, result_3.ClosestPointNormal.z,
            isVisible ? "YES" : "NO"
        );
        bounds = DrawTextBox(pRender, centerX, centerY, textHeight, buf, DrawText_Center);
        break;
    }
    case Text_None:
        bounds = Recti (  0, 0, 0, 0 );
        break;

    default:
        OVR_ASSERT ( !"Missing text screen" );
        break;
    }

    if (TextScreen != Text_None)
    {
        // Commit changes to the HUD swap chain
        DrawEyeTargets[Rendertarget_Hud]->pColorTex->Commit();
    }

    return bounds;
}


Recti OculusWorldDemoApp::RenderControllerStateHud(float cx, float cy, float textHeight,
                                                   const ovrInputState& is, unsigned controllerType)
{
    Recti bounds;

    // Button press state
    std::string  s;

    switch (TextScreen)
    {
    case Text_TouchState:               s = "Touch State\n";                break;
    case Text_GamepadState:             s = "Gamepad State\n";              break;
    case Text_ActiveControllerState:    s = "Active Controller State\n";    break;
    default:    OVR_FAIL(); // shouldn't be here if TextScreen type is not of expected mode
    }

    if (!HasInputState)
    {
        s += "\nNo input detected";
    }
    else
    {
        if (TextScreen == Text_ActiveControllerState)
        {
            s += "Controller Type: [ ";
            if (controllerType & ovrControllerType_XBox)
                s += "XBox ";
            if (controllerType & ovrControllerType_Remote)
                s += "Remote ";
            if (controllerType & ovrControllerType_LTouch)
                s += "LTouch ";
            if (controllerType & ovrControllerType_RTouch)
                s += "RTouch ";
            s += "] \n";
        }
        s += "Buttons: [";
    if (is.Buttons & ovrButton_A)           s += "A ";
    if (is.Buttons & ovrButton_B)           s += "B ";
    if (is.Buttons & ovrButton_LThumb)      s += "LThumb ";
    if (is.Buttons & ovrButton_LShoulder)   s += "LShoulder ";
    if (is.Buttons & ovrButton_X)           s += "X ";
    if (is.Buttons & ovrButton_Y)           s += "Y ";
    if (is.Buttons & ovrButton_RThumb)      s += "RThumb ";
    if (is.Buttons & ovrButton_RShoulder)   s += "RShoulder ";
    if (is.Buttons & ovrButton_Enter)       s += "Enter ";
    s += "] \n";

    if (controllerType & ovrControllerType_Touch)
    {
        // Touch state
        s += "Touching [";
        if (is.Touches & ovrTouch_A)         s += "A ";
        if (is.Touches & ovrTouch_B)         s += "B ";
        if (is.Touches & ovrTouch_LThumb)    s += "LThumb ";
        if (is.Touches & ovrTouch_LIndexTrigger)  s += "LIndexTrigger ";
        if (is.Touches & ovrTouch_LThumbRest)     s += "LThumbRest ";
        if (is.Touches & ovrTouch_X)         s += "X ";
        if (is.Touches & ovrTouch_Y)         s += "Y ";
        if (is.Touches & ovrTouch_RThumb)    s += "RThumb ";
        if (is.Touches & ovrTouch_RIndexTrigger)  s += "RIndexTrigger ";
        if (is.Touches & ovrTouch_RThumbRest)     s += "RThumbRest ";
        s += "] \n";
        // Do gestures on a separate line for readability
        s += "Gesture [";
        if (is.Touches & ovrTouch_LThumbUp)        s += "LThumbUp ";
        if (is.Touches & ovrTouch_RThumbUp)        s += "RThumbUp ";
        if (is.Touches & ovrTouch_LIndexPointing)  s += "LIndexPoint ";
        if (is.Touches & ovrTouch_RIndexPointing)  s += "RIndexPoint ";
        s += "] \n";
        s += "Trigger Values [\n";
        s = s + "    Left Index " + std::to_string(is.IndexTrigger[ovrHandType::ovrHand_Left]) + "\n";
        s = s + "    Left Grip " + std::to_string(is.HandTrigger[ovrHandType::ovrHand_Left]) + "\n";
        s = s + "    Right Index " + std::to_string(is.IndexTrigger[ovrHandType::ovrHand_Right]) + "\n";
        s = s + "    Right Grip " + std::to_string(is.HandTrigger[ovrHandType::ovrHand_Right]) + "\n";
        s += "] \n";
    }

    const char* deviceName = "XBox/Remote ";
    unsigned    controllerMask = ovrControllerType_XBox | ovrControllerType_Remote;

    if (controllerType & controllerMask)
    {
        if (ConnectedControllerTypes & controllerMask)
        {
            // Button press state
            s += std::string(deviceName) + "Buttons: [";

            if (is.Buttons & ovrButton_Up)      s += "Up ";
            if (is.Buttons & ovrButton_Down)    s += "Down ";
            if (is.Buttons & ovrButton_Left)    s += "Left ";
            if (is.Buttons & ovrButton_Right)   s += "Right ";
            if (is.Buttons & ovrButton_Back)    s += "Back ";
            if (is.Buttons & ovrButton_VolUp)   s += "VolUp ";
            if (is.Buttons & ovrButton_VolDown) s += "VolDown ";

            s += "] \n";
        }
        else
        {
            s += std::string(deviceName) + "[Not Connected]\n";
        }
    }


    if ((controllerType & ovrControllerType_Touch) &&
        (ConnectedControllerTypes & ovrControllerType_Touch))
    {
        s += "Tracking: ";

        // We make the assumption below that if tracking is true then valid is also true,
        // otherwise we'd have to handle the odd case of tracking = true but valid = false.
        if (HandStatus[ovrHand_Left] & ovrStatus_PositionTracked)        s += "L_Pos ";
        else if (HandStatus[ovrHand_Left] & ovrStatus_PositionValid)     s += "L_Pos_Est ";

        if (HandStatus[ovrHand_Left] & ovrStatus_OrientationTracked)     s += "L_IMU ";
        else if (HandStatus[ovrHand_Left] & ovrStatus_OrientationValid)  s += "L_IMU_Est ";

        if (HandStatus[ovrHand_Right] & ovrStatus_PositionTracked)       s += "R_Pos ";
        else if (HandStatus[ovrHand_Right] & ovrStatus_PositionValid)    s += "R_Pos_Est ";

        if (HandStatus[ovrHand_Right] & ovrStatus_OrientationTracked)    s += "R_IMU ";
        else if (HandStatus[ovrHand_Right] & ovrStatus_OrientationValid) s += "R_IMU_Est ";
        s += "\n";
    }


    char strBuffer[256];
    if ((controllerType & ovrControllerType_XBox) &&
        (ConnectedControllerTypes & ovrControllerType_XBox))
    {
        snprintf(strBuffer, sizeof(strBuffer), "Gamepad:  LT:%0.3f RT:%0.3f \n"
                                                  "          LX:%0.3f,LY:%0.3f  RX:%0.3f,RY:%0.3f \n",
            is.IndexTrigger[ovrHand_Left], is.IndexTrigger[ovrHand_Right],
            is.Thumbstick[ovrHand_Left].x, is.Thumbstick[ovrHand_Left].y,
            is.Thumbstick[ovrHand_Right].x, is.Thumbstick[ovrHand_Right].y);
        s += strBuffer;

        snprintf(strBuffer, sizeof(strBuffer), "GamepadRaw:  LT:%0.3f RT:%0.3f \n"
                                                   "            LX:%0.3f,LY:%0.3f  RX:%0.3f,RY:%0.3f \n",
            is.IndexTriggerNoDeadzone[ovrHand_Left], is.IndexTriggerNoDeadzone[ovrHand_Right],
            is.ThumbstickNoDeadzone[ovrHand_Left].x, is.ThumbstickNoDeadzone[ovrHand_Left].y,
            is.ThumbstickNoDeadzone[ovrHand_Right].x, is.ThumbstickNoDeadzone[ovrHand_Right].y);
        s += strBuffer;
    }

    if (controllerType & ovrControllerType_Touch)
    {
        // If controllers are connected, display their axis/trigger status.
        if (ConnectedControllerTypes & ovrControllerType_LTouch)
        {
            snprintf(strBuffer, sizeof(strBuffer), "Left Touch:  T:%0.3f M:%0.3f   X:%0.3f,Y:%0.3f\n",
                is.IndexTrigger[ovrHand_Left], is.HandTrigger[ovrHand_Left],
                is.Thumbstick[ovrHand_Left].x, is.Thumbstick[ovrHand_Left].y);
            s += strBuffer;

            snprintf(strBuffer, sizeof(strBuffer), "Left Touch Raw:  T:%0.3f M:%0.3f   X:%0.3f,Y:%0.3f\n",
                is.IndexTriggerRaw[ovrHand_Left], is.HandTriggerRaw[ovrHand_Left],
                is.ThumbstickRaw[ovrHand_Left].x, is.ThumbstickRaw[ovrHand_Left].y);
            s += strBuffer;

            snprintf(strBuffer, sizeof(strBuffer), "Left Touch Position:  X:%0.3f Y:%0.3f Z:%0.3f\n",
                HandPoses[ovrHand_Left].Position.x, HandPoses[ovrHand_Left].Position.y, HandPoses[ovrHand_Left].Position.z );
            s += strBuffer;

            snprintf(strBuffer, sizeof(strBuffer), "Left Touch Orientation:  X:%0.3f Y:%0.3f Z:%0.3f W:%.03f\n",
                HandPoses[ovrHand_Left].Orientation.x, HandPoses[ovrHand_Left].Orientation.y, HandPoses[ovrHand_Left].Orientation.z, HandPoses[ovrHand_Left].Orientation.w);
            s += strBuffer;
        }
        else
        {
            s += "Left Touch [Not Connected]\n";
        }

        if (ConnectedControllerTypes & ovrControllerType_RTouch)
        {
            snprintf(strBuffer, sizeof(strBuffer), "Right Touch: T:%0.3f M:%0.3f   X:%0.3f,Y:%0.3f\n",
                is.IndexTrigger[ovrHand_Right], is.HandTrigger[ovrHand_Right],
                is.Thumbstick[ovrHand_Right].x, is.Thumbstick[ovrHand_Right].y);
            s += strBuffer;

            snprintf(strBuffer, sizeof(strBuffer), "Right Touch Raw: T:%0.3f M:%0.3f   X:%0.3f,Y:%0.3f\n",
                is.IndexTriggerRaw[ovrHand_Right], is.HandTriggerRaw[ovrHand_Right],
                is.ThumbstickRaw[ovrHand_Right].x, is.ThumbstickRaw[ovrHand_Right].y);
            s += strBuffer;

            snprintf(strBuffer, sizeof(strBuffer), "Right Touch Position:  X:%0.3f Y:%0.3f Z:%0.3f\n",
                HandPoses[ovrHand_Right].Position.x, HandPoses[ovrHand_Right].Position.y, HandPoses[ovrHand_Right].Position.z);
            s += strBuffer;

            snprintf(strBuffer, sizeof(strBuffer), "Right Touch Orientation:  X:%0.3f Y:%0.3f Z:%0.3f W:%.03f\n",
                HandPoses[ovrHand_Right].Orientation.x, HandPoses[ovrHand_Right].Orientation.y, HandPoses[ovrHand_Right].Orientation.z, HandPoses[ovrHand_Right].Orientation.w);
            s += strBuffer;
        }
        else
        {
            s += "Right Touch [Not Connected]";
        }
    }

    // Haptics
    snprintf(strBuffer, sizeof(strBuffer), "\n\nHaptics Mode: %d/%d (Press X change)\n%s",
        HapticsMode, kMaxHapticsModes-1, HapticsModeDesc[HapticsMode]);
    s += strBuffer;

    }
    bounds = DrawTextBox(pRender, cx, cy, textHeight, s.c_str(), DrawText_Center);

    return bounds;
}


//-----------------------------------------------------------------------------
// ***** Callbacks For Menu changes

// Non-trivial callback go here.

void OculusWorldDemoApp::HmdSettingChangeFreeRTs(OptionVar*)
{
    HmdSettingsChanged = true;
    // Cause the RTs to be recreated with the new mode.
    for (int rtNum = 0; rtNum < Rendertarget_LAST; rtNum++)
    {
        RenderTargets[rtNum].pColorTex = NULL;
        RenderTargets[rtNum].pDepthTex = NULL;
    }
}


void OculusWorldDemoApp::RendertargetResolutionModeChange(OptionVar*)
{
    if (ResolutionScalingMode == ResolutionScalingMode_Adaptive)
    {
        // crank desired PD as this will determine how high we can go
        // during adaptive resolution selection
        DesiredPixelDensity = 2.0;
    }
    else
    {
        // Prevent desired PD from being "stuck" in 2.0 as the user goes between various resolution modes
        DesiredPixelDensity = 1.0;
    }
    HmdSettingChangeFreeRTs();
}

void OculusWorldDemoApp::RendertargetFormatChange(OptionVar*)
{
    bool newSrgbState = ShouldLoadedTexturesBeSrgb(EyeTextureFormat);
    if (newSrgbState != SrgbRequested)
    {
        // Reload assets if we're switching sRGB state
        LoadingState = LoadingState_DoLoad;
    }

    SrgbRequested = newSrgbState;
    HmdSettingChangeFreeRTs();
}

void OculusWorldDemoApp::TriggerHdcpLoad(OptionVar*)
{
  if(LayerHdcpEnabled && !HdcpTexture)
    LoadHdcpTexture();
}

void OculusWorldDemoApp::ForceAssetReloading(OptionVar* opt)
{
    // Reload scene with new srgb flag applied to textures
    LoadingState = LoadingState_DoLoad;

    HmdSettingChangeFreeRTs(opt);
}

uint64_t OculusWorldDemoApp::ConvertMirrorOptionsToMirrorFormatFlags()
{
    uint64_t format = 0;

    if (CurrentMirrorOptionStage == MirrorOptionStage_PostDistortion)
    {
        format |= Texture_MirrorPostDistortion;
    }
    switch (CurrentMirrorOptionEyes)
    {
    case OculusWorldDemoApp::MirrorOptionEyes_Both:         break;
    case OculusWorldDemoApp::MirrorOptionEyes_LeftOnly:     format |= Texture_MirrorLeftEyeOnly;    break;
    case OculusWorldDemoApp::MirrorOptionEyes_RightOnly:    format |= Texture_MirrorRightEyeOnly;   break;
    default: OVR_FAIL();    break;
    }

    if (CurrentMirrorOptionForceSymmetricFov)
    {
      format |= Texture_MirrorForceSymmetricFov;
    }

    if (CurrentMirrorOptionCaptureGuardian)
    {
        format |= Texture_MirrorIncludeGuardian;
    }
    if (CurrentMirrorOptionCaptureNotifications)
    {
        format |= Texture_MirrorIncludeNotifications;
    }
    if (CurrentMirrorOptionCaptureSystemGui)
    {
        format |= Texture_MirrorIncludeSystemGui;
    }

    return format;
}

void OculusWorldDemoApp::MirrorSettingChange(OptionVar*)
{
    CurrentMirrorOptionsFormat = ConvertMirrorOptionsToMirrorFormatFlags();
    MirrorOptionsChanged = true;
    HmdSettingsChanged = true;
    NotificationTimeout = ovr_GetTimeInSeconds() + 10.0f;
}


void OculusWorldDemoApp::DebugHudSettingQuadPropChange(OptionVar*)
{
    switch ( DebugHudStereoPresetMode )
    {
    case DebugHudStereoPreset_Free:
        // Don't change the values.
        break;
    case DebugHudStereoPreset_Off:
        DebugHudStereoMode = ovrDebugHudStereo_Off;
        break;
    case DebugHudStereoPreset_OneMeterTwoMetersAway:
        DebugHudStereoMode = ovrDebugHudStereo_QuadWithCrosshair;
        DebugHudStereoGuideSize.x = 1.0f;
        DebugHudStereoGuideSize.y = 1.0f;
        DebugHudStereoGuidePosition.x = 0.0f;
        DebugHudStereoGuidePosition.y = 0.0f;
        DebugHudStereoGuidePosition.z = -2.0f;
        DebugHudStereoGuideYawPitchRollDeg.x = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.y = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.z = 0.0f;
        DebugHudStereoGuideColor.x = 1.0f;
        DebugHudStereoGuideColor.y = 0.5f;
        DebugHudStereoGuideColor.z = 0.1f;
        DebugHudStereoGuideColor.w = 0.8f;
        break;
    case DebugHudStereoPreset_HugeAndBright:
        DebugHudStereoMode = ovrDebugHudStereo_Quad;
        DebugHudStereoGuideSize.x = 10.0f;
        DebugHudStereoGuideSize.y = 10.0f;
        DebugHudStereoGuidePosition.x = 0.0f;
        DebugHudStereoGuidePosition.y = 0.0f;
        DebugHudStereoGuidePosition.z = -1.0f;
        DebugHudStereoGuideYawPitchRollDeg.x = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.y = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.z = 0.0f;
        DebugHudStereoGuideColor.x = 1.0f;
        DebugHudStereoGuideColor.y = 1.0f;
        DebugHudStereoGuideColor.z = 1.0f;
        DebugHudStereoGuideColor.w = 1.0f;
        break;
    case DebugHudStereoPreset_HugeAndDim:
        DebugHudStereoMode = ovrDebugHudStereo_Quad;
        DebugHudStereoGuideSize.x = 10.0f;
        DebugHudStereoGuideSize.y = 10.0f;
        DebugHudStereoGuidePosition.x = 0.0f;
        DebugHudStereoGuidePosition.y = 0.0f;
        DebugHudStereoGuidePosition.z = -1.0f;
        DebugHudStereoGuideYawPitchRollDeg.x = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.y = 0.0f;
        DebugHudStereoGuideYawPitchRollDeg.z = 0.0f;
        DebugHudStereoGuideColor.x = 0.1f;
        DebugHudStereoGuideColor.y = 0.1f;
        DebugHudStereoGuideColor.z = 0.1f;
        DebugHudStereoGuideColor.w = 1.0f;
        break;
    default: OVR_ASSERT ( false ); break;
    }

    DebugHudSettingModeChange();

    ovr_SetBool      (Session, OVR_DEBUG_HUD_STEREO_GUIDE_INFO_ENABLE, DebugHudStereoGuideInfoEnable);
    ovr_SetFloatArray(Session, OVR_DEBUG_HUD_STEREO_GUIDE_SIZE, &DebugHudStereoGuideSize[0], DebugHudStereoGuideSize.ElementCount);
    ovr_SetFloatArray(Session, OVR_DEBUG_HUD_STEREO_GUIDE_POSITION, &DebugHudStereoGuidePosition[0], DebugHudStereoGuidePosition.ElementCount);
    DebugHudStereoGuideYawPitchRollRad.x = DegreeToRad(DebugHudStereoGuideYawPitchRollDeg.x);
    DebugHudStereoGuideYawPitchRollRad.y = DegreeToRad(DebugHudStereoGuideYawPitchRollDeg.y);
    DebugHudStereoGuideYawPitchRollRad.z = DegreeToRad(DebugHudStereoGuideYawPitchRollDeg.z);
    ovr_SetFloatArray(Session, OVR_DEBUG_HUD_STEREO_GUIDE_YAWPITCHROLL, &DebugHudStereoGuideYawPitchRollRad[0], DebugHudStereoGuideYawPitchRollRad.ElementCount);
    ovr_SetFloatArray(Session, OVR_DEBUG_HUD_STEREO_GUIDE_COLOR, &DebugHudStereoGuideColor[0], DebugHudStereoGuideColor.ElementCount);
}

double OculusWorldDemoApp::GetSceneAnimationTime() const
{
    return SceneAnimationTime;
}

void OculusWorldDemoApp::CenterPupilDepthChange(OptionVar*)
{
    ovr_SetFloat(Session, "CenterPupilDepth", CenterPupilDepthMeters);
}


void OculusWorldDemoApp::DistortionClearColorChange(OptionVar*)
{
    float clearColor[2][4] = { { 0.0f, 0.0f, 0.0f, 0.0f },
                               { 0.0f, 0.5f, 1.0f, 0.0f } };
    ovr_SetFloatArray(Session, "DistortionClearColor",
                         clearColor[(int)DistortionClearBlue], 4);
}

void OculusWorldDemoApp::WindowSizeChange(OptionVar*)
{
    pPlatform->SetWindowSize(WindowSize.w, WindowSize.h);

    RenderParams.Resolution = WindowSize;
    if ( pRender != nullptr )
    {
        pRender->SetParams(RenderParams);
    }
}

void OculusWorldDemoApp::WindowSizeToNativeResChange(OptionVar*)
{
    pPlatform->SetWindowSize( (int)(HmdDesc.Resolution.w * MirrorNativeResScale),
                              (int)(HmdDesc.Resolution.h * MirrorNativeResScale));

    RenderParams.Resolution = WindowSize;
    pRender->SetParams(RenderParams);
}

void OculusWorldDemoApp::ResetHmdPose(OptionVar* /* = 0 */)
{
    ovr_RecenterTrackingOrigin(Session);
    Menu.SetPopupMessage("Sensor Fusion Recenter Tracking Origin");

    if (IsMixedRealityCaptureMode)
    {
        // Handle the special use case : reset tracking center for static external camera
        if (ExternalCameras[CurrentCameraID].Extrinsics.AttachedToDevice == ovrTrackedDevice_None)
        {
            NumExternalCameraProperties = OVR_MAX_EXTERNAL_CAMERA_COUNT;
            ovrResult error = ovr_GetExternalCameras(Session, &ExternalCameras[0], &NumExternalCameraProperties);   // Here the 2nd parameter is the actual size of camera array
            if (OVR_FAILURE(error) && error != ovrError_NoExternalCameraInfo)
            {
                DisplayLastErrorMessageBox("ovr_GetExternalCameras failure.");
            }
        }
    }
}

void OculusWorldDemoApp::SpecifyTrackingOrigin(OptionVar* /* = 0 */)
{
    Posef originPose;
    originPose.Translation.x = ProvidedTrackingOriginTranslationYaw.x;
    originPose.Translation.y = ProvidedTrackingOriginTranslationYaw.y;
    originPose.Translation.z = ProvidedTrackingOriginTranslationYaw.z;
    originPose.Rotation = Quatf(Vector3f(0.0f, 1.0f, 0.0f), DegreeToRad(ProvidedTrackingOriginTranslationYaw.w));

    ovr_SpecifyTrackingOrigin(Session, originPose);
}


void OculusWorldDemoApp::UpdatedTrackingOriginType(OptionVar*)
{
    ovr_SetTrackingOriginType(Session, TrackingOriginType);
}

void OculusWorldDemoApp::TestUpdateFirmware(OptionVar*)
{
    ovr_SetBool(nullptr, "TestFirmwareUpdate", ovrTrue);
    Menu.SetPopupMessage("Testing Firmware Update");
}

unsigned int OculusWorldDemoApp::createProjectionModifier()
{
    bool flipZ = DepthModifier != NearLessThanFar;
    bool farAtInfinity = DepthModifier == FarLessThanNearAndInfiniteFarClip;

    unsigned int projectionModifier = ovrProjection_None;
    projectionModifier |= (RenderParams.RenderAPI == RenderAPI_OpenGL) ? ovrProjection_ClipRangeOpenGL : 0;
    projectionModifier |= flipZ ? ovrProjection_FarLessThanNear : 0;
    projectionModifier |= farAtInfinity ? ovrProjection_FarClipAtInfinity : 0;

    return projectionModifier;
}

//-----------------------------------------------------------------------------

void OculusWorldDemoApp::ProcessDeviceNotificationQueue()
{
    // TBD: Process device plug & Unplug
}


void OculusWorldDemoApp::DisplayLastErrorMessageBox(const char* pMessage)
{
    ovrErrorInfo errorInfo;
    ovr_GetLastErrorInfo(&errorInfo);

    if(InteractiveMode)
    {
        OVR::Util::DisplayMessageBoxF("OculusWorldDemo", "%s %d -- %s", pMessage, errorInfo.Result, errorInfo.ErrorString);
    }

    WriteLog("[OculusWorldDemoApp] %s %d -- %s", pMessage, errorInfo.Result, errorInfo.ErrorString);
}


void OculusWorldDemoApp::UpdateActiveControllerState()
{
    if (LastControllerType != ActiveControllerState.ControllerType)
    {
        // Controller type changed; make sure deltas aren't computed from the last type.
        LastControllerType = ActiveControllerState.ControllerType;
        LastControllerState = ActiveControllerState;
        return;
    }

    Vector2f leftStick = ActiveControllerState.Thumbstick[ovrHand_Left];
    Vector2f rightStick = ActiveControllerState.Thumbstick[ovrHand_Right];


    // TODO: Remove this workaround in the future. Apply dead zones to Touch controllers.
    if (ActiveControllerState.ControllerType == ovrControllerType_Touch)
    {
        const float radialDeadZone = 0.25f * 0.25f;
        if (leftStick.LengthSq() < radialDeadZone)
        {
            leftStick = Vector2f(0, 0);
        }

        if (rightStick.LengthSq() < radialDeadZone)
        {
            rightStick = Vector2f(0, 0);
        }
    }

    ThePlayer.GamepadMove = Vector3f(leftStick.x * leftStick.x * (leftStick.x > 0 ? 1 : -1),
        0,
        leftStick.y * leftStick.y * (leftStick.y > 0 ? -1 : 1));
    ThePlayer.GamepadRotate = Vector3f(2 * rightStick.x, -2 * rightStick.y, 0);

    uint32_t gamepadDeltas = (ActiveControllerState.Buttons ^ LastControllerState.Buttons) & ActiveControllerState.Buttons;

    if (gamepadDeltas)
    {
        bool handled = Menu.OnGamepad(gamepadDeltas);
        if (!handled)
        {
            if (ComfortTurnMode != ComfortTurn_Off)
            {
                float howMuch = 0.0f;
                switch (ComfortTurnMode)
                {
                case ComfortTurn_30Degrees:
                    howMuch = DegreeToRad(30.0f);
                    break;

                case ComfortTurn_45Degrees:
                    howMuch = DegreeToRad(45.0f);
                    break;

                default:
                    OVR_ASSERT(false);
                    break;
                }

                if ((gamepadDeltas & ovrButton_LShoulder) != 0)
                {
                    ThePlayer.BodyYaw += howMuch;
                }

                if ((gamepadDeltas & ovrButton_RShoulder) != 0)
                {
                    ThePlayer.BodyYaw -= howMuch;
                }
            }
        }
    }

    LastControllerState = ActiveControllerState;
}




//-------------------------------------------------------------------------------------

OVR_PLATFORM_APP(OculusWorldDemoApp);


