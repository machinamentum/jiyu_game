/************************************************************************************

Filename    :   OculusWorldDemo.h
Content     :   First-person view test application for Oculus Rift - Header file
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

#ifndef OVR_OculusWorldDemo_h
#define OVR_OculusWorldDemo_h

#include "Kernel/OVR_Types.h"
#include "Kernel/OVR_Allocator.h"
#include "Kernel/OVR_RefCount.h"
#include "Kernel/OVR_System.h"
#include "Kernel/OVR_Nullptr.h"
#include "Kernel/OVR_Timer.h"
#include "Kernel/OVR_DebugHelp.h"
#include "Extras/OVR_Math.h"
#include "../CommonSrc/Platform/Platform_Default.h"
#include "../CommonSrc/Render/Render_Device.h"
#include "../CommonSrc/Render/Render_XmlSceneLoader.h"
#include "../CommonSrc/Platform/Gamepad.h"
#include "../CommonSrc/Util/OptionMenu.h"
#include "../CommonSrc/Util/RenderProfiler.h"
#include "../CommonSrc/Util/StringHelper.h"
#include "../CommonSrc/Util/Logger.h"

#include "Player.h"
#include "Tracker.h"
#include "Script.h"

#include <vector>
#include <string>
#include <array>
// Filename to be loaded by default, searching specified paths.
#define WORLDDEMO_ASSET_FILE  "Tuscany.xml"


#define WORLDDEMO_ASSET_PATH "Assets/Tuscany/"

using namespace OVR;
using namespace OVR::OvrPlatform;
using namespace OVR::Render;




//-------------------------------------------------------------------------------------
// ***** OculusWorldDemo Description

// This app renders a loaded scene allowing the user to move along the
// floor and look around with an HMD, mouse and keyboard. The following keys work:
//
//  'W', 'S', 'A', 'D' and Arrow Keys - Move forward, back; strafe left/right.
//
//  Space - Bring up status and help display.
//  Tab   - Bring up/hide menu with editable options.
//  F4    - Toggle MSAA.
//  F9    - Cycle through fullscreen and windowed modes.
//          Necessary for previewing content with Rift.
//
// Important Oculus-specific logic can be found at following locations:
//
//  OculusWorldDemoApp::OnStartup - This function will initialize the SDK, creating the Hmd
//									and delegating to CalculateHmdValues to initialize it.
//
//  OculusWorldDemoApp::OnIdle    - Here we poll SensorFusion for orientation, apply it
//									to the scene and handle movement.
//									Stereo rendering is also done here, by delegating to
//									to the RenderEyeView() function for each eye.
//

//-------------------------------------------------------------------------------------
// ***** OculusWorldDemo Application class

// An instance of this class is created on application startup (main/WinMain).
// It then works as follows:
//  - Graphics and HMD setup is done OculusWorldDemoApp::OnStartup(). Much of
//    HMD configuration here is moved to CalculateHmdValues.
//    OnStartup also creates the room model from Slab declarations.
//
//  - Per-frame processing is done in OnIdle(). This function processes
//    sensor and movement input and then renders the frame.
//
//  - Additional input processing is done in OnMouse, OnKey.

class OculusWorldDemoApp : public Application
{
protected:
    enum CamRenderPoseEnum;

public:
    OculusWorldDemoApp();
    ~OculusWorldDemoApp();

    virtual int  OnStartup(int argc, const char** argv);
    virtual void OnIdle();
    virtual void OnShutdown();

    virtual void OnMouseMove(int x, int y, int modifiers);
    virtual void OnKey(OVR::KeyCode key, int chr, bool down, int modifiers);
    virtual void OnResize(int width, int height);
    void         Exit(int exitCode = 0);
    bool         SetupWindowAndRendering(bool firstTime, int argc, const char** argv, ovrGraphicsLuid luid);
    void         SetCommandLineMenuOption(int argc, const char** argv);     // Set menu options using inputs from the command line

    int          InitializeRendering(bool firstTime);
    void         DestroyFovStencil();
    void         DestroyRendering();

    bool         HandleOvrError(ovrResult error);   // returns TRUE for error out, FALSE otherwise

    void		 UpdateTrackingStateForSittingHeight(ovrTrackingState* ts);

    // Adds room model to scene.
    void         InitMainFilePath();
    void         PopulateScene(const char* fileName);
    void         PopulatePreloadScene();
    void         LoadHdcpTexture();
    void         ClearScene();
    void         PopulateOptionMenu();
    bool         SetMenuValue ( std::string menuFullName, std::string newValue );
    bool         SetupMixedRealityCapture();
    void         CleanupMixedRealityCapture();
    double       GetSceneAnimationTime() const;

    // Computes all of the Hmd values and configures render targets.
    ovrResult    CalculateHmdValues();
    // Returns the actual size present.
    Sizei        EnsureRendertargetAtLeastThisBig (int rtNum, Sizei requestedSize, bool genMips, int mipCount, ovrResult& error);

    ovrResult    UpdateFovStencilMeshes();

    // Renders HUD/menu overlays; 2D viewport must be set before call.
    Recti        RenderTextInfoHud(float textHeight);
    Recti        RenderMenu(float textHeight);
    Recti        RenderControllerStateHud(float cx, float xy, float textHeight,
                                          const ovrInputState& is, unsigned controllerType);

    // Renders full stereo scene for one eye.
    void         RenderEyeView(CamRenderPoseEnum camNum, ovrEyeType eyeType, const Matrix4f* optionalMatrix = nullptr, bool onlyRenderWorld = false);
    void         RenderAnimatedBlocks(CamRenderPoseEnum camNum, double appTime);
    void         RenderGrid(CamRenderPoseEnum camNum, Recti viewport);
    void         RenderControllers(CamRenderPoseEnum camNum, ovrEyeType eyeNum);
    void         RenderCockpitPanels(CamRenderPoseEnum camNum);
    void         RenderCamNearFarHandsView();

    void         RenderCubemap();
    bool         CubeRendered = false; // Render cubemap once

    // helper function to just draw a controller
    void         RenderController(CamRenderPoseEnum camNum, const ovrPosef pose, bool rightSide);


    Matrix4f     CalculateViewFromPose(const Posef& pose);

    // Determine whether this frame needs rendering based on timewarp timing and flags.
    bool        FrameNeedsRendering(double curtime);
    void        ApplyDynamicResolutionScaling();
    void        UpdateFrameRateCounter(double curtime);

    // Model creation and misc functions.
    Model*      CreateModel(Vector3f pos, struct SlabModel* sm);
    Model*      CreateBoundingModel(CollisionModel &cm);
    void        UpdateActiveControllerState();

    // Processes DeviceNotificationStatus queue to handles plug/unplug.
    void         ProcessDeviceNotificationQueue();

    void         DisplayLastErrorMessageBox(const char* pMessage);

    // ***** Callbacks for Menu option changes

    // These contain extra actions to be taken in addition to switching the state.
    void HmdSettingChange(OptionVar* = 0)   { HmdSettingsChanged = true; }
    void MirrorSettingChange(OptionVar* = 0);

    void PerfHudSettingChange(OptionVar* = 0) { ovr_SetInt(Session, OVR_PERF_HUD_MODE, (int)PerfHudMode); }

    void DebugHudSettingModeChange(OptionVar* = 0) { ovr_SetInt(Session, OVR_DEBUG_HUD_STEREO_MODE, (int)DebugHudStereoMode); }
    void DebugHudSettingQuadPropChange(OptionVar* = 0);

    void BlockShowChange(OptionVar* = 0)    { BlocksCenter = ThePlayer.GetBodyPos(ovrTrackingOrigin_EyeLevel); }
    void EyeHeightChange(OptionVar* = 0)
    {
        float newHeightScale = ScaleAffectsEyeHeight ? PositionTrackingScale : 1.0f;
        ThePlayer.SetUserStandingEyeHeight(MenuUserEyeHeight, newHeightScale);
    }

    void HmdSettingChangeFreeRTs(OptionVar* = 0);
    void RendertargetFormatChange(OptionVar* = 0);
    void RendertargetResolutionModeChange(OptionVar* = 0);
    void TriggerHdcpLoad(OptionVar* = 0);
    void ForceAssetReloading(OptionVar* = 0);
    void CenterPupilDepthChange(OptionVar* = 0);
    void DistortionClearColorChange(OptionVar* = 0);
    void WindowSizeChange(OptionVar* = 0);
    void WindowSizeToNativeResChange(OptionVar* = 0);

    void ResetHmdPose(OptionVar* = 0);
    void SpecifyTrackingOrigin(OptionVar* = 0);


    void MultiresTexLayoutChanged(OptionVar* = 0);
    void MultiresEmulatedChanged(OptionVar* = 0);
    void MultiresEarlyZAndAluChanged(OptionVar* = 0);
    void MultiresParamsChanged(OptionVar* = 0);

    void UpdatedTrackingOriginType(OptionVar* = 0);

    void TestUpdateFirmware(OptionVar* = 0);

    void TraceAllMenuOptions(OptionVar* = 0);


    unsigned int createProjectionModifier();

protected:
    friend class OWDScript;

    ExceptionHandler     OVR_ExceptionHandler;
    GUIExceptionListener OVR_GUIExceptionListener;
    OWDScript            OwdScript;

    int                 Argc;
    const char**        Argv;

    RenderDevice*       pRender;
    RendererParams      RenderParams;
    Sizei               WindowSize;
    bool                HmdDisplayAcquired;
    int                 FirstScreenInCycle;
    bool                SupportsSrgbSwitching;
    bool                SupportsMultisampling;
    bool                SupportsDepthSubmit;
    float               ActiveGammaCurve;
    float               SrgbGammaCurve;
    bool                MirrorIsSrgb;
    ovrTrackingOrigin   TrackingOriginType;
    float               MenuUserEyeHeight;

    struct RenderTarget
    {
        Ptr<Texture>    pColorTex;
        Ptr<Texture>    pDepthTex;
    };
    enum RendertargetsEnum
    {
        Rendertarget_Left,
        Rendertarget_Right,
        Rendertarget_BothEyes,    // Used when both eyes are rendered to the same target.
        Rendertarget_Left1,       // For multicam rendering.
        Rendertarget_Right1,
        Rendertarget_Left2,       // For multicam rendering.
        Rendertarget_Right2,
        Rendertarget_Left3,       // For multicam rendering.
        Rendertarget_Right3,
        Rendertarget_Hud,
        Rendertarget_Menu,
        Rendertarget_LAST
    };

    enum CamRenderPoseEnum
    {
        CamRenderPose_Left = 0,
        CamRenderPose_Right,
        CamRenderPose_Left1, // For multicam rendering.
        CamRenderPose_Right1,
        CamRenderPose_Left2, // For multicam rendering.
        CamRenderPose_Right2,
        CamRenderPose_Left3, // For multicam rendering.
        CamRenderPose_Right3,
        CamRenderPose_ExternalCamera,
        CamRenderPose_Count
    };

    enum TrackingCameraType {
      TrackingCamera_0,
      TrackingCamera_1,
      TrackingCamera_2,
      TrackingCamera_3,
      TrackingCamera_Count
    };

    // Last render target for eye FOV buffers.
    static const int RenderTarget_EyeLast = Rendertarget_BothEyes + 1;

    RenderTarget        RenderTargets[Rendertarget_LAST];
    RenderTarget*       DrawEyeTargets[Rendertarget_LAST]; // the buffers we'll actually render to
    static const bool   AllowMsaaTargets[Rendertarget_LAST]; // whether or not we allow the layer to use MSAA
    static const bool   UseDepth[Rendertarget_LAST]; // whether or not we need depth buffers for this layer
    int                 GenMipCount[Rendertarget_LAST];   // number of mips currently used on render target

    // ***** Oculus OVR Session Variables

    ovrSession          Session;
    ovrHmdDesc          HmdDesc;
    Ptr<Texture>        MirrorTexture;
    ovrTimewarpProjectionDesc PosTimewarpProjectionDesc;
    bool                UsingDebugHmd;
    ovrPosef            EyeRenderPose[3];       // Poses we used for rendering. (left eye, right eye, external camera)
    ovrEyeRenderDesc    EyeRenderDesc[2];

    Posef               CamRenderPose[CamRenderPose_Count];
    FovPort             CamFovPort[CamRenderPose_Count];
    Matrix4f            CamProjection[CamRenderPose_Count];       // Projection matrix.
    Matrix4f            CamOrthoProjection[CamRenderPose_Count];  // Projection for 2D.
    Ptr<Texture>        CamTexture[CamRenderPose_Count];
    Ptr<Texture>        CamDepthTexture[CamRenderPose_Count];
    Recti               CamRenderViewports[CamRenderPose_Count];
    Sizei               CamRenderSize[CamRenderPose_Count];       // Saved render eye sizes; base for dynamic sizing.
    Posef               CamFromEye[CamRenderPose_Count];          // Offset pose from the respective eye to the camera.
    Matrix4f            CamFromWorld[CamRenderPose_Count];
    int                 CamRenderPoseCount;                       // How many of the above eyes we are actually rendering.
    int                 CamLayerCount;                            // How many layers we use for cameras.

    // Discrete Render Rotation
    bool                DiscreteHmdRotationEnable;
    bool                DiscreteHmdRotationNeedsInit; // pulls new HMD poses regardless of threshold
    Posef               DiscreteEyePoses[ovrEye_Count];
    Posef               DiscreteActualEyePoses[ovrEye_Count];
    float               DiscreteRotationThresholdAngle;
    float               DiscreteRotationFovMult; // this value is just for illustration purposes

    enum DRScissorTechnique
    {
        DRScissorTechnique_None,
        DRScissorTechnique_ProjectCorners,
        DRScissorTechnique_ProjectEdges,
    };
    DRScissorTechnique  DiscreteRotationScissorTech;
    FovPort             DiscreteRotationSubFovPort[ovrEye_Count];
    Recti               DiscreteRotationScissor[ovrEye_Count];
    float               DiscreteRotationScissorMult;
    float               DiscreteRotationScissorMenuViewSavings[ovrEye_Count]; // calculated pixel processing savings
    float               DiscreteRotationScissorMenuViewPerfHit[ovrEye_Count]; // calculated pixel perf hit after culling
    Sizei               DiscreteRotationScissorIdealRenderSize[ovrEye_Count]; // used for perf hit calc

    // Sub-pixel jitter
    bool                SubpixelJitterEnable;
    int                 SubpixelJitterSampleCount;
    float               SubpixelJitterSamplePolarOffset;
    float               SubpixelJitterSamplePolarDistMult;
    FovPort             SubpixelJitterBaseCamFovPort[CamRenderPose_Count];
    int                 SubpixelJitterCurrentIndex;

    // Frame timing logic.
    float               SecondsOfFpsMeasurement;
    int                 FrameCounter;
    int				          TotalFrameCounter;
    float               SecondsPerFrame;
    float               FPS;
    double              LastFpsUpdate;

    // Times a single frame.
    double              LastUpdate;

    // Touch Haptics
    ovrTouchHapticsDesc TouchHapticsDesc[2];
    ovrHapticsClip      TouchHapticsClip;
    int                 TouchHapticsPlayIndex;

    // Loaded data.
    std::string	                          MainFilePath;
    std::vector<Ptr<CollisionModel> >     CollisionModels;
    std::vector<Ptr<CollisionModel> >     GroundCollisionModels;

    // Loading process displays screenshot in first frame
    // and then proceeds to load until finished.
    enum LoadingStateType
    {
        LoadingState_Frame0,
        LoadingState_DoLoad,
        LoadingState_Finished
    } LoadingState;

    // Current status flags so that edges can be reported
    bool                InteractiveMode;        // If true then we assume a user is present, else this app is assumed to be running unattended.
    bool                HaveHMDVisionTracking;
    bool                HaveBothControllersVisionTracking;
    bool                HavePositionTracker;
    bool                HaveHMDConnected;
    bool                HaveSync;
    bool                Replaying;
    bool                Recording;

    double              LastSyncTime;
    unsigned int        LastCameraFrame;

    ovrInputState       LastControllerState;
    ovrControllerType   LastControllerType;

    Player				      ThePlayer;
    Matrix4f            EyeFromWorld[3];   // One per eye; external camera view matrix

    enum RenderControllerType
    {
        RenderController_None = 0,
        RenderController_AvatarHands,
        RenderController_Touches,
        RenderController_Both,
        RenderController_Count
    };

    // External camera for Mixed Reality Capture
    unsigned int        NumExternalCameraProperties = OVR_MAX_EXTERNAL_CAMERA_COUNT;    // number of calibrated cameras sent by ovr_SetExternalCameraProperties().
    int                 CurrentCameraID;
    std::array<ovrExternalCamera, OVR_MAX_EXTERNAL_CAMERA_COUNT> ExternalCameras;
    float               DistanceToCamInMeters = 0.0f;      // distance from the player to the external camera
    float               NearFarOverlapInMeters = 0.1f;     // to avoid the seam between the foreground and background images
    RenderControllerType RenderControllerFlag = RenderController_Both;
    Matrix4f            ExternalCamProjection;			// External camera projection matrix
    Recti               NearRenderViewport;
    Recti               FarRenderViewport;
    Recti               HandsRenderViewport;
    std::array<bool, OVR_MAX_EXTERNAL_CAMERA_COUNT>  ExternalCameraNameOpenBits;    // bool flags
    std::array<bool, OVR_MAX_EXTERNAL_CAMERA_COUNT>  OldExternalCameraNameOpenBits; // from last menu click


    Scene               MainScene;
    Scene               SmallGreenCube;
    Scene               SmallOculusCube;
    Scene               SmallOculusGreenCube;
    Scene               SmallOculusRedCube;

    Scene				OculusCubesScene;
    Scene               ControllerScene;
    Scene               GreenCubesScene;
    Scene               RedCubesScene;
    Scene               YellowCubesScene;

    //Boundary information related
    Scene               BoundaryScene;
    void                PopulateBoundaryScene(Scene* scene);
    void                RenderBoundaryScene(Matrix4f& view);
    void                HandleBoundaryControls();
    void                HandleHaptics();


    // Small floor models used to visualize seated floor level under player.
    // We allow switching between two modes: [0] = solid, [1] = donut shaped.
    Scene               OculusRoundFloor[2];
    Ptr<Model>          pRoundFloorModel[2];

    std::array<Tracker, TrackingCamera_Count> PositionalTrackers;

    Ptr<Texture>        TextureOculusCube;

    Ptr<Texture>        HdcpTexture;
    Ptr<Texture>        CockpitPanelTexture;
    Ptr<Texture>        LoadingTexture;

    Ptr<Texture>        CubemapLoadTexture;
    Ptr<Texture>        CubemapRenderTexture;

    // Last frame asn sensor data reported by BeginFrame().
    double              HmdFrameTiming;
    unsigned            HmdStatus;
    bool                EnableTrackingStateLog;
    int                 TrackingTrackerCount = 0;
    int                 ConnectedTrackerCount = 0;

    // Overlay notifications time out in
    double              NotificationTimeout;

    // ***** Modifiable Menu Options

    // This flag is set when HMD settings change, causing HMD to be re-initialized.
    bool                HmdSettingsChanged;

    // Render Target - affecting state.
    bool                RendertargetIsSharedByBothEyes;

    int                 FovStencilType;  // -1 is off, rest are tied to ovrFovStencilType
    enum FovStencilMeshModes
    {
        FovStencilMeshMode_WriteDepth,
        FovStencilMeshMode_UndoDepth,
        FovStencilMeshMode_Count,
    };
    Ptr<Model>          FovStencilMeshes[ovrEye_Count][FovStencilMeshMode_Count];
    float*              FovStencilLinesX[ovrEye_Count];
    float*              FovStencilLinesY[ovrEye_Count];
    int                 FovStencilLineCount;
    enum FovStencilColorType
    {
      FovStencilColor_Black,
      FovStencilColor_Green,
      FovStencilColor_Red,
      FovStencilColor_White,
    } FovStencilColor;


    enum ResolutionScalingModeEnum
    {
        ResolutionScalingMode_Off,
        ResolutionScalingMode_Dynamic,
        ResolutionScalingMode_Adaptive,
        ResolutionScalingMode_AlternatingEyes,
    };
    ResolutionScalingModeEnum ResolutionScalingMode;
    float AlternatingEyeResScale;






    std::string         ShutterType;

    // Recorded tracking and input state, for rendering and reporting the state.
    bool                HasInputState;
    unsigned int        ConnectedControllerTypes;
    ovrInputState       LastInputState;
    ovrInputState       InputState;
    ovrInputState       GamepadInputState;
    ovrInputState       ActiveControllerState;
    ovrPosef            HandPoses[2];
    unsigned int        HandStatus[2];
    ovrPosef            TrackedObjectPose;

    // The size of the rendered HUD in pixels. If size==0, there's no HUD at the moment.
    Recti               HudRenderedSize;
    Recti               MenuRenderedSize;

    // Read from the device, not sent to it.
    float               InterAxialDistance;
      
    enum EyeTextureFormatOpt
    {
        //EyeTextureFormat_UNKNOWN,
        //EyeTextureFormat_B5G6R5,  - Not currently supported on PC. Would require a DirectX 11.1 device.
        //EyeTextureFormat_BGR5A1,  - Not currently supported on PC. Would require a DirectX 11.1 device.
        //EyeTextureFormat_BGRA4,   - Not currently supported on PC. Would require a DirectX 11.1 device.
        EyeTextureFormat_RGBA8,
        //EyeTextureFormat_BGRA8,       - sRGB format is not supported in OpenGL mode, so don't use this either
        //EyeTextureFormat_BGRX8,       - Not supported in OpenGL mode
        EyeTextureFormat_RGBA16F,
        EyeTextureFormat_R11G11B10F,
        // Srgb formats below here to minimize srgb-to-linear switching and
        // avoid unnecessary texture asset reloading with each switch
        EyeTextureFormat_RGBA8_SRGB,
        //EyeTextureFormat_BGRA8_SRGB,  - Not supported in OpenGL mode
        //EyeTextureFormat_BGRX8_SRGB,  - Not supported in OpenGL mode
    };
    EyeTextureFormatOpt EyeTextureFormat;
    static bool ShouldLoadedTexturesBeSrgb(EyeTextureFormatOpt eyeTextureFormat);
    static int GetRenderDeviceTextureFormatForEyeTextureFormat(EyeTextureFormatOpt eyeTextureFormat);

    enum MonoscopicMode
    {
        Mono_Off,               // Disabled.
        Mono_ZeroIpd,           // Set the player's IPD to zero (but still hve head-tracking - WARNING - UNPLEASANT FOR SOME)
        Mono_ZeroPlayerScale,   // Set the player's scale to zero (removes head-tracking)

        Mono_Count
    };
    MonoscopicMode      MonoscopicRenderMode;
    float               PositionTrackingScale;
    bool                ScaleAffectsEyeHeight;
    float               DesiredPixelDensity;
    float               AdaptivePixelDensity;
    float               FovScaling;
    bool                FovPortOverrideEnable;
    bool                FovPortOverrideResetToDefault;
    FovPort             FovPortOverride;


    float               NearClip;
    float               FarClip;
    enum DepthMod
    {
        NearLessThanFar,
        FarLessThanNear,
        FarLessThanNearAndInfiniteFarClip,
    };
    DepthMod            DepthModifier;

    enum DepthFormatOpt
    {
        DepthFormatOption_D32f,
        DepthFormatOption_D24_S8,
        DepthFormatOption_D16,
        DepthFormatOption_D32f_S8,
    };
    DepthFormatOpt DepthFormatOption;

    enum SceneRenderCountEnum
    {
        SceneRenderCount_FixedLow,
        SceneRenderCount_SineTenSec,
        SceneRenderCount_SquareTenSec,
        SceneRenderCount_Spikes,

        SceneRenderCount_LAST
    } SceneRenderCountType;
    int32_t             SceneRenderCountLow;
    int32_t             SceneRenderCountHigh;
    float               SceneRenderWasteCpuTimePreTrackingQueryMs;
    float               SceneRenderWasteCpuTimePreRenderMs;
    float               SceneRenderWasteCpuTimeEachRenderMs;
    float               SceneRenderWasteCpuTimePreSubmitMs;

    enum DrawFlushModeEnum
    {
        DrawFlush_Off,
        DrawFlush_AfterEachEyeRender,
        DrawFlush_AfterEyePairRender,
    };
    DrawFlushModeEnum   DrawFlushMode;
    int32_t             DrawFlushCount;

    void FlushIfApplicable(DrawFlushModeEnum flushMode, int& currDrawFlushCount);

    enum MenuHudMovementModeEnum
    {
        MenuHudMove_FixedToFace,
        MenuHudMove_DragAtEdge,
        MenuHudMove_RecenterAtEdge,

        MenuHudMove_LAST,
    } MenuHudMovementMode;
    float               MenuHudMovementRadius;
    float               MenuHudMovementDistance;
    float               MenuHudMovementRotationSpeed;
    float               MenuHudMovementTranslationSpeed;
    float               MenuHudTextPixelHeight;
    float               MenuHudDistance;
    float               MenuHudSize;
    // if head pitch < MenuHudMaxPitchToOrientToHeadRoll, then we ignore head roll on menu orientation
    float               MenuHudMaxPitchToOrientToHeadRoll;
    bool				MenuHudAlwaysOnMirrorWindow;

    bool                TimewarpRenderIntervalEnabled;
    float               TimewarpRenderIntervalInSeconds;
    bool                NeverRenderedIntoEyeTextures;
    bool                EnableTimewarpOnMainLayer;
    bool                PredictionEnabled;

    bool                FreezeEyeUpdate;
    bool                LayersEnabled;              // Using layers, or just rendering quads into the eye buffers?
    bool                Layer0HighQuality;
    int                 Layer0GenMipCount;
    bool                Layer0Depth;
    bool                Layer1Enabled;
    bool                Layer1HighQuality;
    bool                Layer2Enabled;
    bool                Layer2HighQuality;
    bool                Layer3Enabled;
    bool                Layer3HighQuality;
    bool                LayerHdcpEnabled;
    bool                Layer4Enabled;
    bool                Layer4HighQuality;
    float               Layer234Size;
    bool                LayerDirectEnabled;
    int                 LayerCockpitEnabled;        // A bitfield - one enable bit per layer.
    bool                LayerCockpitHighQuality;
    bool                LayerHudMenuEnabled;        // So you can hide the menu with Shift+Tab while toggling visual things.
    bool                LayerHudMenuHighQuality;
    bool                LayerLoadingHighQuality;

    bool                LayerCylinderEnabled;
    bool                LayerCylinderHighQuality;
    float               LayerCylinderRadius;
    float               LayerCylinderAspectRatio;
    float               LayerCylinderAngle;
    bool                LayerCylinderHeadlocked;

    bool                LayerCubemapSaveOnRender;
	  bool                LayerCubemapHighQuality;

    // Other global settings.
    float               CenterPupilDepthMeters;
    bool                ForceZeroHeadMovement;
    bool                MultisampleRequested;       // What the menu option is set to.
    bool                MultisampleEnabled;         // Did we actually get it?
    bool                SrgbRequested;
    bool                AnisotropicSample;
    bool                SceneBrightnessGrayscale;
    Vector3f            SceneBrightness;
    bool                SceneBlack;
    bool                TextureOriginAtBottomLeft;
#if defined(OVR_OS_LINUX)
    bool                LinuxFullscreenOnDevice;
#endif
    // DK2 only:
    bool                PositionTrackingEnabled;
    bool				PixelLuminanceOverdrive;
    bool                MirrorToWindow;
    float               MirrorNativeResScale;

    // Support toggling background color for distortion so that we can see
    // the effect on the periphery.
    int                 DistortionClearBlue;


    // Stereo settings adjustment state.
    bool                ShiftDown;
    bool                CtrlDown;

    // Logging
    bool                IsLogging;

    ovrPerfHudMode      PerfHudMode;

    enum DebugHudStereoPresetEnum
    {
        DebugHudStereoPreset_Free,
        DebugHudStereoPreset_Off,
        DebugHudStereoPreset_OneMeterTwoMetersAway,
        DebugHudStereoPreset_HugeAndBright,
        DebugHudStereoPreset_HugeAndDim,

        DebugHudStereoPreset_Count,
    }                       DebugHudStereoPresetMode;
    ovrDebugHudStereoMode   DebugHudStereoMode;
    bool                    DebugHudStereoGuideInfoEnable;
    Vector3f                DebugHudStereoGuidePosition;
    Vector2f                DebugHudStereoGuideSize;
    Vector3f                DebugHudStereoGuideYawPitchRollDeg;
    Vector3f                DebugHudStereoGuideYawPitchRollRad;
    Vector4f                DebugHudStereoGuideColor;

    // ***** Scene Rendering Modes

    enum SceneRenderMode
    {
        Scene_World,
        Scene_Cubes,
        Scene_OculusCubes,
        Scene_MAX
    };
    SceneRenderMode    SceneMode;

    enum GridDispayModeType
    {
        GridDisplay_None,
        GridDisplay_GridOnly,
        GridDisplay_GridDirect,
        GridDisplay_GridAndScene
    };
    GridDispayModeType  GridDisplayMode;

    // What type of grid to display.
    enum GridModeType
    {
        Grid_Rendertarget4,
        Grid_Rendertarget16,
        Grid_Lens,
        Grid_Last
    };
    GridModeType       GridMode;

    enum GridColorType {
      GridColor_Default,
      GridColor_White,
      GridColor_Red,
      GridColor_Green,
      GridColor_Blue,
      GridColor_Yellow,
      GridColor_Magenta,
      GridColor_Cyan,
    };
    GridColorType GridColor;

    // What help screen we display, brought up by 'Spacebar'.
    enum TextScreenType
    {
        Text_None,
        Text_Info,
        Text_Timing,
        Text_PerfStats,
        Text_BoundaryInfo,
        Text_TouchState,
        Text_GamepadState,
        Text_ActiveControllerState,
        Text_Help1,
        Text_Help2,
        Text_Count
    };
    TextScreenType      TextScreen;

    enum ComfortTurnModeEnum
    {
        ComfortTurn_Off,
        ComfortTurn_30Degrees,
        ComfortTurn_45Degrees,

        ComfortTurn_LAST
    }                   ComfortTurnMode;

    enum MirrorOptionStage
    {
        MirrorOptionStage_Rectilinear = 0,
        MirrorOptionStage_PostDistortion = 1,
    } CurrentMirrorOptionStage;

    enum MirrorOptionEyes
    {
        MirrorOptionEyes_Both = 0,
        MirrorOptionEyes_LeftOnly = 1,
        MirrorOptionEyes_RightOnly = 2,
    } CurrentMirrorOptionEyes;

    bool CurrentMirrorOptionCaptureGuardian;
    bool CurrentMirrorOptionCaptureNotifications;
    bool CurrentMirrorOptionCaptureSystemGui;
    bool CurrentMirrorOptionForceSymmetricFov;
    uint64_t CurrentMirrorOptionsFormat;
    bool MirrorOptionsChanged;

    uint64_t ConvertMirrorOptionsToMirrorFormatFlags();

    bool                SceneAnimationEnabled;  // If false then animations are frozen.
    double              SceneAnimationTime;     // Specifies the time we use for animation progression.

    // Whether we are displaying animated blocks and what type.
    int                 BlocksShowType;
    int                 BlocksShowMeshType;
    float               BlocksSpeed;
    Vector3f            BlocksCenter;
    Vector3f            BlockScale;
    int                 BlocksHowMany;
    float               BlocksMovementRadius;
    int                 BlocksMovementType;
    float               BlocksMovementScale;
    float               BlockModelSize;

    // Sitting/Standing Mode visualization
    // When Sitting state is set, extra altitude is added to height
    bool                VisualizeSeatLevel;
	  bool                VisualizeTracker;
    bool                Sitting;
    bool                SittingAutoSwitch;
    float               ExtraSittingAltitude;
    double              TransitionToSitTime;
    double              TransitionToStandTime;
    // Use "donut shape" circle with a hole instead of a filled circle
    bool                DonutFloorVisual;

    bool                ShowCalibratedOrigin;
    Posef               CalibratedTrackingOrigin;
    Vector4f            ProvidedTrackingOriginTranslationYaw;

    // User configurable options, brought up by 'Tab' key.
    // Also handles shortcuts and pop-up overlay messages.
    OptionSelectionMenu Menu;
    bool                ShortcutChangeMessageEnable;
    bool                MenuPopulated;

    // Profiler for rendering - displays timing stats.
    RenderProfiler      Profiler;

    // true if logging tracking data to file
    bool                IsVisionLogging;

    // Will contain the time when the HMD sensor was sampled and passed into the EyeFov layer
    double              SensorSampleTimestamp;


    // **** Rendering Layer Setup

    enum LayerNumbers
    {
        LayerNum_MainEye = 0,
        LayerNum_MainEye1 = 1,
        LayerNum_MainEye2 = 2,
        LayerNum_MainEye3 = 3,
        LayerNum_Cubemap = 4, // Switch Cubemap and MainEye layers when cubemap layer is enabled
        LayerNum_Layer1 = 5,
        LayerNum_Layer2 = 6,
        LayerNum_Layer3 = 7,
        LayerNum_Layer4 = 8,
        LayerNum_LayerHdcp = 9,
        LayerNum_CockpitFirst = 5,      // Cockpit shares with Layers1-4 - we're out of layers.
        LayerNum_CockpitLast = 5 + 4,
        LayerNum_Loading = 10,
        LayerNum_Cylinder = 11,
        LayerNum_Hud = ovrMaxLayerCount - 2,
        LayerNum_Menu = ovrMaxLayerCount - 1,
        // Total # of layers.
        LayerNum_TotalLayers = ovrMaxLayerCount
    };

    // Complete layer list. Some entries may be null.
    ovrLayerHeader*     LayerList[LayerNum_TotalLayers];


    // Individual layer objects.
    // EyeLayer can be either regular or with depth, so use a union type.
    ovrLayer_Union      EyeLayer[4]; // Up to 4 muticam layers.

    ovrLayerQuad        Layer1, Layer2, Layer3, LayerHdcp;
    ovrLayerEyeMatrix   Layer4;
    ovrLayerQuad        CockpitLayer[LayerNum_CockpitLast - LayerNum_CockpitFirst + 1];
    ovrLayerQuad        HudLayer;
    ovrLayerQuad        MenuLayer;
    ovrLayerQuad        LoadingLayer;
    ovrLayerCylinder    CylinderLayer;
    ovrLayerCube        CubemapLayer;

    // Menu position & state info.
    Posef               MenuPose;
    bool                MenuIsRotating;
    bool                MenuIsTranslating;
    Vector3f            MenuTranslationOffset;

    enum CubemapMode
    {
        Cubemap_Off,
        Cubemap_Render,
        Cubemap_Load,
    } LayerCubemap;

    void UpdateDiscreteRotation();

    void UpdateSubPixelJitter();


};

#endif // OVR_OculusWorldDemo_h
