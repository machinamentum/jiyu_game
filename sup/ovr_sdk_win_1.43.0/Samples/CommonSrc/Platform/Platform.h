/************************************************************************************

Filename    :   Platform.h
Content     :   Platform-independent app and rendering framework for Oculus samples
Created     :   September 6, 2012
Authors     :   Andrew Reisse

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

************************************************************************************/

#ifndef OVR_Platform_h
#define OVR_Platform_h

#include "Kernel/OVR_Types.h"
#include "Kernel/OVR_Allocator.h"
#include "Kernel/OVR_RefCount.h"
#include "Kernel/OVR_System.h"
#include "Kernel/OVR_Nullptr.h"
#include "Kernel/OVR_Timer.h"
#include "Extras/OVR_Math.h"

#include "Kernel/OVR_KeyCodes.h"

#include <vector>
#include <string>

namespace OVR { namespace Render {
    class RenderDevice;
    struct RendererParams;
}}

namespace OVR { namespace OvrPlatform {

using Render::RenderDevice;

class PlatformCore;
class Application;
class GamepadManager;

// MouseMode configures mouse input behavior of the app. Three states are
// currently supported:
//   Normal          - Reports absolute coordinates with cursor shown.
//   Relative        - Reports relative delta coordinates with cursor hidden
//                     until 'Esc' key is pressed or window loses focus.
//   RelativeEscaped - Relative input is desired, but has been escaped until
//                     mouse is clicked in the window, which will return the state
//                     to relative. Absolute coordinates are reported.

enum MouseMode
{
    Mouse_Normal,
    Mouse_Relative,        // Cursor hidden, mouse grab, OnMouseMove reports relative deltas.
    Mouse_RelativeEscaped, // Clicking in window will return to Relative state.
};


enum Modifiers
{
    Mod_Shift       = 0x001,
    Mod_Control     = 0x002,
    Mod_Meta        = 0x004,
    Mod_Alt         = 0x008,

    // Set for input Mouse_Relative mode, indicating that x,y are relative deltas.
    Mod_MouseRelative = 0x100,
};

//-------------------------------------------------------------------------------------
// ***** SetupGraphicsDeviceSet

typedef RenderDevice* (*RenderDeviceCreateFunc)(ovrSession session, const Render::RendererParams&, void*, ovrGraphicsLuid luid);

// SetupGraphicsDeviceSet is a PlatformCore::SetupGraphics initialization helper class,
// used to build up a list of RenderDevices that can be used for rendering.
// Specifying a smaller set allows application to avoid linking unused graphics devices.
struct SetupGraphicsDeviceSet
{    
    SetupGraphicsDeviceSet(const char* typeArg, RenderDeviceCreateFunc createFunc)
        : pTypeArg(typeArg), pCreateDevice(createFunc), pNext(0) { }
    SetupGraphicsDeviceSet(const char* typeArg, RenderDeviceCreateFunc createFunc,
                           const SetupGraphicsDeviceSet& next)
        : pTypeArg(typeArg), pCreateDevice(createFunc), pNext(&next) { }

    // Selects graphics object based on type string; returns 'this' if not found.
    const SetupGraphicsDeviceSet* PickSetupDevice(const char* typeArg) const;

    const char*               pTypeArg;
    RenderDeviceCreateFunc    pCreateDevice;        

private:
    const SetupGraphicsDeviceSet*  pNext;
};

//-------------------------------------------------------------------------------------
// ***** PlatformCore

// PlatformCore defines abstract system window/viewport setup functionality and
// maintains a renderer. This class is separated from Application because it can have
// derived platform-specific implementations.
// Specific implementation classes are hidden within platform-specific versions
// such as Win32::PlatformCore.

class PlatformCore : public NewOverrideBase
{
protected:
    Application*        pApp;
    Ptr<RenderDevice>   pRender;
    Ptr<GamepadManager> pGamepadManager;
    double              StartupSeconds; 

public:
    PlatformCore(Application *app);
    virtual ~PlatformCore() { }
    Application*		GetApp() { return pApp; }
    RenderDevice*		GetRenderer() const { return pRender; }
    GamepadManager*		GetGamepadManager() const { return pGamepadManager; }

    virtual void*		SetupWindow(int w, int h) = 0;
    // Destroys window and also releases renderer.
    virtual void		DestroyWindow() = 0;
    virtual void		Exit(int exitcode) = 0;

    virtual void		ShowWindow(bool visible) = 0;
    
    // Search for a matching graphics renderer based on type argument and initializes it.    
    virtual RenderDevice* SetupGraphics(ovrSession session, const SetupGraphicsDeviceSet& setupGraphicsDesc,
                                        const char* gtype,
                                        const Render::RendererParams& rp,
                                        ovrGraphicsLuid luid) = 0;

    void DestroyGraphics() { pRender.Clear(); }

    virtual void		SetMouseMode(MouseMode mm) { OVR_UNUSED(mm); }

    virtual void		GetWindowSize(int* w, int* h) const = 0;
    virtual void        SetWindowSize(int w, int h) = 0;

    virtual void		SetWindowTitle(const char*title) = 0;
	virtual void		PlayMusicFile(const char *fileName) { OVR_UNUSED(fileName); }
    
    // Get time since start of application in seconds.
    double			    GetAppTime() const; 
    
    virtual std::string GetContentDirectory() const { return "."; }

    // Creates notification overlay text box over the top of OS window. Multiple
    // messages can be created with different 'index' values. Pass null string
    // to remove the overlay.
    // Intended to be used with Oculus display driver only; may be unsupported on some platforms.
    virtual void         SetNotificationOverlay(int index, int fontHeightPixels,
                                               int yoffset, const char* text)
    { OVR_UNUSED4(index, fontHeightPixels, yoffset, text); }
};

//-------------------------------------------------------------------------------------
// PlatformApp is a base application class from which end-user application
// classes derive.

class Application : public NewOverrideBase
{
protected:
    class PlatformCore* pPlatform;

public:
    virtual ~Application() { }

    virtual int  OnStartup(int argc, const char** argv) = 0;
    virtual void OnQuitRequest(int exitCode = 0) { pPlatform->Exit(exitCode); }
    virtual void OnShutdown() = 0; // Matches OnStartup. Clean up everything initialized there.

    virtual void OnIdle() {}

    virtual void OnKey(KeyCode key, int chr, bool down, int modifiers)
    { OVR_UNUSED4(key, chr, down, modifiers); }
    virtual void OnMouseMove(int x, int y, int modifiers)
    { OVR_UNUSED3(x, y, modifiers); }
    virtual void OnWindowProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
    { OVR_UNUSED4(hwnd, msg, wp, lp); }

    virtual void OnResize(int width, int height)
    { OVR_UNUSED2(width, height); }

    void         SetPlatformCore(PlatformCore* p) { pPlatform = p; }
    PlatformCore* GetPlatformCore() const         { return pPlatform; }


    // Static functions defined by OVR_PLATFORM_APP and used to initialize and
    // shut down the application class.
    static Application* CreateApplication();
    static void         DestroyApplication(Application* app);
};


}}


// OVR_PLATFORM_APP_ARGS specifies the Application class to use for startup,
// providing it with startup arguments.
#if !defined(OVR_PLATFORM_APP_ARGS)
    #define OVR_PLATFORM_APP_ARGS(AppClass, args)                                                   \
        OVR::OvrPlatform::Application* OVR::OvrPlatform::Application::CreateApplication()           \
        {                                                                                           \
            OVR::System::Init();                                                                    \
            return new AppClass args;                                                               \
        }                                                                                           \
                                                                                                    \
        void OVR::OvrPlatform::Application::DestroyApplication(OVR::OvrPlatform::Application* app)  \
        {                                                                                           \
            OVR::OvrPlatform::PlatformCore* platform = app->pPlatform;                              \
            delete platform;                                                                        \
            app->pPlatform = nullptr;                                                               \
            delete app;                                                                             \
            OVR::System::Destroy();                                                                 \
        }
#endif

// OVR_PLATFORM_APP_ARGS specifies the Application startup class with no args.
#if !defined(OVR_PLATFORM_APP)
    #define OVR_PLATFORM_APP(AppClass) OVR_PLATFORM_APP_ARGS(AppClass, ())
#endif


#if !defined(OVR_PLATFORM_APP_ARGS_WITH_LOG)
    #define OVR_PLATFORM_APP_ARGS_WITH_LOG(AppClass, LogClass, args)                                \
        OVR::OvrPlatform::Application* OVR::OvrPlatform::Application::CreateApplication()           \
        {                                                                                           \
            static LogClass log;                                                                    \
            OVR::System::Init(&log);                                                                \
            return new AppClass args;                                                               \
        }                                                                                           \
                                                                                                    \
        void OVR::OvrPlatform::Application::DestroyApplication(OVR::OvrPlatform::Application* app)  \
        {                                                                                           \
            OVR::OvrPlatform::PlatformCore* platform = app->pPlatform;                              \
            delete platform;                                                                        \
            app->pPlatform = nullptr;                                                               \
            delete app;                                                                             \
            OVR::System::Destroy();                                                                 \
        }
#endif

#if !defined(OVR_PLATFORM_APP_ARGS_WITH_LOG)
    #define OVR_PLATFORM_APP_WITH_LOG(AppClass, LogClass) OVR_PLATFORM_APP_ARGS_WITH_LOG(AppClass, LogClass, ())
#endif


#endif
