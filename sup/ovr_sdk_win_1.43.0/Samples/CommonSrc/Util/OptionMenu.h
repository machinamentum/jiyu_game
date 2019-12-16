/************************************************************************************

Filename    :   OptionMenu.h
Content     :   Option selection and editing for OculusWorldDemo
Created     :   March 7, 2014
Authors     :   Michael Antonov, Caleb Leak

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

#ifndef OVR_OptionMenu_h
#define OVR_OptionMenu_h

#include "Kernel/OVR_Types.h"
#include "Kernel/OVR_Allocator.h"
#include "Kernel/OVR_RefCount.h"
#include "Kernel/OVR_System.h"
#include "Kernel/OVR_Nullptr.h"
#include "Kernel/OVR_Timer.h"
#include "Extras/OVR_Math.h"

#include "../Platform/Platform_Default.h"
#include "../Render/Render_Device.h"
#include "../Platform/Gamepad.h"

#include <vector>
#include <string>

using namespace OVR;
using namespace OVR::OvrPlatform;
using namespace OVR::Render;


//-------------------------------------------------------------------------------------
struct FunctionNotifyBase : public NewOverrideBase
{
	virtual ~FunctionNotifyBase(){}
    virtual void CallNotify(void*) { }
    virtual void CallNotify() { }
};

// Simple member pointer wrapper to support calling class members
template<class C, class X>
struct FunctionNotifyContext : public FunctionNotifyBase
{
    typedef void (C::*FnPtr)(X*);

    FunctionNotifyContext(C* p, FnPtr fn) : pContext(NULL), pClass(p), pFn(fn) { }
    FunctionNotifyContext(C* p, FnPtr fn, X* pContext) : pContext(pContext), pClass(p), pFn(fn) { }
    virtual void CallNotify(void* var)  { (void)(pClass->*pFn)(static_cast<X*>(var)); }
    virtual void CallNotify()  { (void)(pClass->*pFn)(pContext); }
private:

    X*      pContext;
    C*      pClass;
    FnPtr   pFn;
};

template<class C>
struct FunctionNotifySimple : public FunctionNotifyBase
{
    typedef void (C::*FnPtr)(void);

    FunctionNotifySimple(C* p, FnPtr fn) : pClass(p), pFn(fn) { }
    virtual void CallNotify(void*)  { CallNotify(); }
    virtual void CallNotify()  { (void)(pClass->*pFn)(); }
private:

    C*      pClass;
    FnPtr   pFn;
};


//-------------------------------------------------------------------------------------
// Describes a shortcut key 
struct ShortcutKey
{
    enum ShiftUsageType
    {
        Shift_Ignore,       // The shift key state has no effect on the shortcut, which triggers either way.
        Shift_Modify,       // The shift key being on results in the notification function being told it was on.
        Shift_RequireOn,    // The shift key must be on to trigger the shortcut.
        Shift_RequireOff    // The shift key must be off to trigger the shortcut.
    };

    ShortcutKey(OVR::KeyCode key = Key_None, ShiftUsageType shiftUsage = Shift_RequireOff)
        : Key(key), ShiftUsage(shiftUsage) { }

    OVR::KeyCode        Key;
    ShiftUsageType ShiftUsage;
};


//-------------------------------------------------------------------------------------
struct OptionShortcut
{
    std::vector<ShortcutKey>  Keys;
    std::vector<uint32_t>     GamepadButtons;
    FunctionNotifyBase* pNotify;

    OptionShortcut() : pNotify(NULL) {}
    OptionShortcut(FunctionNotifyBase* pNotify) : pNotify(pNotify) {}
    ~OptionShortcut()  { delete pNotify; }

    void AddShortcut(ShortcutKey key) { Keys.push_back(key); }
    void AddShortcut(uint32_t gamepadButton) { GamepadButtons.push_back(gamepadButton); }

    bool MatchKey(OVR::KeyCode key, bool shift) const;
    bool MatchGamepadButton(uint32_t gamepadButtonMask) const;
};


//-------------------------------------------------------------------------------------

// Base class for a menu item. Internally, this can also be OptionSelectionMenu itself
// to support nested menus. Users shouldn't need to use this class.
class OptionMenuItem : public NewOverrideBase
{
public:
    virtual ~OptionMenuItem() { }

    virtual void         Select() { }
        
    virtual bool         SetValue(const std::string& newVal) { OVR_UNUSED1(newVal); return false; }
    virtual void         NextValue(bool* pFastStep = NULL) { OVR_UNUSED1(pFastStep); }
    virtual void         PrevValue(bool* pFastStep = NULL) { OVR_UNUSED1(pFastStep); }

    virtual std::string  GetLabel() { return Label; }
    virtual std::string  GetValue() { return ""; }

    // Returns empty string if shortcut not handled
    virtual std::string  ProcessShortcutKey(OVR::KeyCode key, bool shift) = 0;
    virtual std::string  ProcessShortcutButton(uint32_t buttonMask) = 0;

    virtual bool         IsMenu() const { return false; }

//protected:
    std::string          Label;
    std::string          PopNamespaceFrom(OptionMenuItem* menuItem);
};


//-------------------------------------------------------------------------------------

// OptionVar implements a basic menu item, which binds to an external variable,
// displaying and editing its state.
class OptionVar : public OptionMenuItem
{
public:

    enum VarType
    {
        Type_Enum,
        Type_Int,
        Type_Float,
        Type_Bool,
        Type_Trigger,
    };

    struct EnumEntry
    {
        // Human readable name for enum.
        std::string Name;
        int32_t Value;
    };    

    struct VarSpecification
    {
        // Identifies which of the specifications below is relevant.
        VarType Type;

        // Name, which applies to any var.
        std::string Name;

        // Shortcut keystroke, or Key_None key if unused.
        ShortcutKey ShortcutKeyUp;
        ShortcutKey ShortcutKeyDown;

        struct EnumSpecification
        {
            std::vector<EnumEntry> EnumEntryArray;
        } EnumSpecification;

        struct IntSpecification
        {
            int32_t Min;
            int32_t Max;
            int32_t StepSize;
            std::string FormatString;
        } IntSpecification;

        struct FloatSpecification
        {
            float Min;
            float Max;
            float StepSize;
            std::string FormatString;
        } FloatSpecification;

        // Nothing needed for Type_Bool
        // Nothing needed for Trigger
    };
    
    typedef std::vector<VarSpecification> VarSpecificationArray;

    typedef std::string (*FormatFunction)(OptionVar*);
    typedef void (*UpdateFunction)(OptionVar*);
    
    static std::string FormatEnum(OptionVar* var);
    static std::string FormatInt(OptionVar* var);
    static std::string FormatFloat(OptionVar* var);
    static std::string FormatTan(OptionVar* var);
    static std::string FormatBool(OptionVar* var);
    static std::string FormatTrigger(OptionVar* var);

    OptionVar(const char* name, void* pVar, VarType type,
              FormatFunction formatFunction,
              UpdateFunction updateFunction = NULL);

    // Integer with range and step size.
    OptionVar(const char* name, int32_t* pVar,
              int32_t min, int32_t max, int32_t stepSize=1,
              const char* formatString = "%d",
              FormatFunction formatFunction = 0, // Default int formatting.
              UpdateFunction updateFunction = NULL);

    // Float with range and step size.
    OptionVar(const char* name, float* pvar, 
              float minf, float maxf, float stepSize = 1.0f,
              const char* formatString = "%.3f", float formatScale = 1.0f,
              FormatFunction formatFunction = 0, // Default float formatting.
              UpdateFunction updateFunction = 0 );

    virtual ~OptionVar();

    int32_t* AsInt()             { return reinterpret_cast<int32_t*>(pVar); }
    bool*    AsBool()            { return reinterpret_cast<bool*>(pVar); }
    float*   AsFloat()           { return reinterpret_cast<float*>(pVar); }
    VarType  GetType()           { return Type; }


    //virtual void Select()   { if(Type == Type_Trigger) SignalUpdate(); }

    // Step through values (wrap for enums).
    virtual void NextValue(bool* pFastStep);
    virtual void PrevValue(bool* pFastStep);
    // Set value from a string. Returns true on success.
    virtual bool SetValue(const std::string& newVal) override;


    // Executes shortcut message and returns notification string.
    // Returns empty string for no action.
    std::string         HandleShortcutUpdate();
    virtual std::string ProcessShortcutKey(OVR::KeyCode key, bool shift);
    virtual std::string ProcessShortcutButton(uint32_t buttonMask);

    OptionVar& AddEnumValue(const char* displayName, int32_t value);

    template<class C>
    OptionVar& SetNotify(C* p, void (C::*fn)(OptionVar*))
    {
        OVR_ASSERT(pNotify == 0); // Can't set notifier twice.
        pNotify = new FunctionNotifyContext<C, OptionVar>(p, fn, this);
        return *this;
    }   


    //String Format();
    virtual std::string GetValue();

    // Shortcut Up Keys are keys that cause stepped values to go up by one step.
    OptionVar& AddShortcutUpKey(const ShortcutKey& shortcut)
    { ShortcutUp.AddShortcut(shortcut); return *this; }

    OptionVar& AddShortcutUpKey(OVR::KeyCode key,
                                ShortcutKey::ShiftUsageType shiftUsage = ShortcutKey::Shift_Modify)
    { ShortcutUp.AddShortcut(ShortcutKey(key, shiftUsage)); return *this; }

    OptionVar& AddShortcutUpButton(uint32_t gamepadButton)
    { ShortcutUp.AddShortcut(gamepadButton); return *this; }

    // Shortcut Down Keys are keys that cause stepped values to go down by one step.
    OptionVar& AddShortcutDownKey(const ShortcutKey& shortcut)
    { ShortcutDown.AddShortcut(shortcut); return *this; }

    OptionVar& AddShortcutDownKey(OVR::KeyCode key,
                                  ShortcutKey::ShiftUsageType shiftUsage = ShortcutKey::Shift_Modify)
    { ShortcutDown.AddShortcut(ShortcutKey(key, shiftUsage)); return *this; }

    OptionVar& AddShortcutDownButton(uint32_t gamepadButton)
    { ShortcutDown.AddShortcut(gamepadButton); return *this; }
    
    // Shortcut keys (neither down nor up) call the only notification, but are internally mapped
    // to shortcup up keys in order to re-use that member data.
    OptionVar& AddShortcutKey(const ShortcutKey& shortcut)
    { return AddShortcutUpKey(shortcut); }

    OptionVar& AddShortcutKey(OVR::KeyCode key,
                               ShortcutKey::ShiftUsageType shiftUsage = ShortcutKey::Shift_RequireOff)
    { return AddShortcutUpKey(key, shiftUsage); }

    OptionVar& AddShortcutButton(uint32_t gamepadButton)
    { return AddShortcutUpButton(gamepadButton); }

    // Enumerates VarSpecifications by appending to the array. Since this class (OptionVar) is a 
    // leaf node in the menu tree, this call will append a single element to varSpecificationArray.
    virtual void Enumerate(OptionVar::VarSpecificationArray& varSpecificationArray, 
                        const char* parentBranchPath = "") const;

private:

    void SignalUpdate()
    {
        if (fUpdate) fUpdate(this);
        if (pNotify) pNotify->CallNotify(this);
    }

    // Array of possible enum values.
    std::vector<EnumEntry>    EnumValues;
    // Gets the index of the current enum value.
    uint32_t                  GetEnumIndex();

    FormatFunction            fFormat;
    UpdateFunction            fUpdate;
    FunctionNotifyBase*       pNotify; 

    VarType         Type;
    void*           pVar;
    const char*     FormatString;
    
    OptionShortcut  ShortcutUp;
    OptionShortcut  ShortcutDown;

    float       MinFloat;
    float       MaxFloat;
    float       StepFloat;
    float       FormatScale; // Multiply float by this before rendering

    int32_t     MinInt;
    int32_t     MaxInt;
    int32_t     StepInt;

    int         SelectedIndex;
};


//-------------------------------------------------------------------------------------
// ***** OptionSelectionMenu

// Implements an overlay option menu, brought up by the 'Tab' key.
// Items are added to the menu with AddBool, AddEnum, AddFloat on startup,
// and are editable by using arrow keys (underlying variable is modified).
// 
// Call Render() to render the menu every frame.
//
// Menu also support displaying popup messages with a timeout, displayed
// when menu body isn't up.

class OptionSelectionMenu : public OptionMenuItem
{
public:
    OptionSelectionMenu(OptionSelectionMenu* parentMenu = NULL);

    bool OnKey(OVR::KeyCode key, int chr, bool down, int modifiers);
    bool OnGamepad(uint32_t buttonMask);

    // Returns rendered bounds.
    Recti Render(RenderDevice* prender, std::string title, float textHeight, float centerX, float centerY);

    // Clear all the menu items
    void Clear();

    void AddItem(OptionMenuItem* menuItem);

    // Adds a boolean toggle. Returns added item to allow customization.
    OptionVar& AddBool(const char* name, bool* pvar,
                       OptionVar::UpdateFunction updateFunction = 0,
                       OptionVar::FormatFunction formatFunction = OptionVar::FormatBool)
    {
        OptionVar* p = new OptionVar(name, pvar, OptionVar::Type_Bool,
                                     formatFunction, updateFunction);
        AddItem(p);
        return *p;
    }

    // Adds a boolean toggle. Returns added item to allow customization.
    OptionVar& AddEnum(const char* name, void* pvar,
                       OptionVar::UpdateFunction updateFunction = 0)
    {
        OptionVar* p = new OptionVar(name, pvar, OptionVar::Type_Enum,
                                     OptionVar::FormatEnum, updateFunction);
        AddItem(p);
        return *p;
    }


    // Adds a Float variable. Returns added item to allow customization.
    OptionVar& AddFloat( const char* name, float* pvar, 
                         float minf, float maxf, float stepSize = 1.0f,
                         const char* formatString = "%.3f", float formatScale = 1.0f,
                         OptionVar::FormatFunction formatFunction = 0, // Default float formatting.
                         OptionVar::UpdateFunction updateFunction = 0 )
    {
        OptionVar* p = new OptionVar(name, pvar, minf, maxf, stepSize,
                                     formatString, formatScale,
                                     formatFunction, updateFunction);
        AddItem(p);
        return *p;
    }
    
        // Adds an Int variable. Returns added item to allow customization.
    OptionVar& AddInt(   const char* name, int32_t* pvar, 
                         int32_t min, int32_t max, int32_t stepSize = 1,
                         const char* formatString = "%d",
                         OptionVar::FormatFunction formatFunction = 0, // Default formatting.
                         OptionVar::UpdateFunction updateFunction = 0 )
    {
        OptionVar* p = new OptionVar(name, pvar, min, max, stepSize,
                                     formatString,
                                     formatFunction, updateFunction);
        AddItem(p);
        return *p;
    }

    OptionVar& AddTrigger( const char* name, OptionVar::UpdateFunction updateFunction = 0 )
    {
        OptionVar* p = new OptionVar(name, NULL, OptionVar::Type_Trigger,
                                     NULL, updateFunction);
        AddItem(p);
        return *p;
    }

    virtual void Hide() {
      DisplayState = DisplayStateType::Display_None;
    }

    virtual void    Select();
    virtual std::string  GetLabel() { return Label + " >"; }

    virtual std::string  ProcessShortcutKey(OVR::KeyCode key, bool shift);
    virtual std::string  ProcessShortcutButton(uint32_t buttonMask);
    
    // Sets a message to display with a time-out. Default time-out is 4 seconds.
    // This uses the same overlay approach as used for shortcut notifications.
    void            SetPopupMessage(const char* format, ...);
    // Overrides current timeout, in seconds (not the future default value);
    // intended to be called right after SetPopupMessage.
    void            SetPopupTimeout(double timeoutSeconds, bool border = false);

    // If the menu is not visible, it still shows a message when an option changes.
    // You can disable that with this.
    void            SetShortcutChangeMessageEnable ( bool enabled );

    virtual bool    IsMenu() const { return true; }
    
    // Recursively collects all OptionVars into varSpecificationArray. Only leaf nodes (OptionVars) 
    // are enumerated. Branches with no leaves (which may exist in this system) are not enumerated.
    // The VarSpecification::Name member will hold the menu path to the leaf node and not just 
    // name of the leaf node. Each path in this system is separated by a dot (.) and not a slash (/).
    virtual void    Enumerate(OptionVar::VarSpecificationArray& varSpecificationArray, 
                        const char* parentBranchPath = "") const;

    // Writes all OptionVars recursively as a string, one per line, into varDescription.
    // Returns number of items, which is the same as the number of lines. The last line is 
    // terminated by a newline (\n). 
    size_t          Enumerate(std::string& optionVarLines) const;

protected:

    Recti renderShortcutChangeMessage(RenderDevice* prender, float textSize, float centerX, float centerY);

public:
    OptionSelectionMenu* GetSubmenu();
    OptionSelectionMenu* GetOrCreateSubmenu(const std::string& submenuName);

    // Find a submenu of this menu. Returns nullptr if not found.
    OptionMenuItem  *FindMenuItem(const std::string& menuItemLabel);


    enum DisplayStateType
    {
        Display_None,
        Display_Menu,
        Display_SingleItem
    };

    DisplayStateType             DisplayState;
    OptionSelectionMenu*         ParentMenu;
   
    std::vector<std::unique_ptr<OptionMenuItem>> Items;
    int                          SelectedIndex;
    bool                         SelectionActive;

    std::string                  PopupMessage;
    double                       PopupMessageTimeout;
    bool                         PopupMessageBorder;

    bool                         RenderShortcutChangeMessages;

    // Possible menu navigation actions.
    enum NavigationActions
    {
        Nav_Up,
        Nav_Down,
        Nav_Left,
        Nav_Right,
        Nav_Select,
        Nav_Back,
        Nav_LAST
    };

    // Handlers for navigation actions.
    void HandleUp(bool* pFast);
    void HandleDown(bool* pFast);
    void HandleLeft();
    void HandleRight();
    void HandleSelect();
    void HandleBack();

    void HandleMenuToggle();
    void HandleSingleItemToggle();

    OptionShortcut NavShortcuts[Nav_LAST];
    OptionShortcut ToggleShortcut;
    OptionShortcut ToggleSingleItemShortcut;
};


//-------------------------------------------------------------------------------------
// Text Rendering Utility
enum DrawTextCenterType
{
    DrawText_NoCenter= 0,
    DrawText_VCenter = 0x01,
    DrawText_HCenter = 0x02,
    DrawText_Center  = DrawText_VCenter | DrawText_HCenter,
    DrawText_Border  = 0x10,
};

// Returns rendered bounds.
Recti    DrawTextBox(RenderDevice* prender, float x, float y,
                    float textSize, const char* text, unsigned centerType);

Sizef   DrawTextMeasure(RenderDevice* prender, float textSize, const char* text);

void    CleanupDrawTextFont();

// This is a separate function because we have functions like DrawTextBox() that do not have any class state
// so we need to push the gamma curve value and shade as a static variable for the options menu
void    Menu_SetColorGammaCurveAndBrightness(float colorGammaCurve, Vector3f brightness);

#endif // OVR_OptionMenu_h
