
#include "../Header/libextra.h"

// This is the only place this input code is included
#define DIRECTINPUT_VERSION 0x0800  // For control
#include <dinput.h>                    // For control

LPDIRECTINPUT8       g_pDI              = NULL;         
LPDIRECTINPUTDEVICE8 g_pJoystick        = NULL;  

//-----------------------------------------------------------------------------
BOOL CALLBACK EnumJoysticksCallback( const DIDEVICEINSTANCE* pdidInstance, VOID* pContext )
{
    if( FAILED( g_pDI->CreateDevice( pdidInstance->guidInstance, &g_pJoystick, NULL ))) return DIENUM_CONTINUE;
    return DIENUM_STOP;
}

//-----------------------------------------------------------------------------
BOOL CALLBACK EnumObjectsCallback( const DIDEVICEOBJECTINSTANCE* pdidoi, VOID* pContext )
    {
    if( pdidoi->dwType & DIDFT_AXIS )
    {
        DIPROPRANGE diprg; 
        diprg.diph.dwSize       = sizeof(DIPROPRANGE); 
        diprg.diph.dwHeaderSize = sizeof(DIPROPHEADER); 
        diprg.diph.dwHow        = DIPH_BYID; 
        diprg.diph.dwObj        = pdidoi->dwType; // Specify the enumerated axis
        diprg.lMin              = -1000; 
        diprg.lMax              = +1000; 
        if( FAILED( g_pJoystick->SetProperty( DIPROP_RANGE, &diprg.diph ) ) ) return DIENUM_STOP;
          }
    return DIENUM_CONTINUE;
    }

//----------------------------------------------------------------------------------------
Control::Control() // Only make one 
{
    g_pJoystick = NULL;

    // Zero last keys
    for (int i=0;i<256;i++)
        LastKey[i] = false;

    // Xbox controller
    if ((SUCCEEDED( DirectInput8Create( GetModuleHandle(NULL), DIRECTINPUT_VERSION, IID_IDirectInput8, (VOID**)&g_pDI, NULL ) ) )
        && (SUCCEEDED( g_pDI->EnumDevices( DI8DEVCLASS_GAMECTRL, EnumJoysticksCallback,NULL, DIEDFL_ATTACHEDONLY ) ) )
        && ( g_pJoystick != NULL)
        && (SUCCEEDED( g_pJoystick->SetDataFormat( &c_dfDIJoystick2 ) ) )
        && (SUCCEEDED( g_pJoystick->SetCooperativeLevel( DIRECTX.Window, DISCL_EXCLUSIVE | DISCL_BACKGROUND) ) )) 
        g_pJoystick->EnumObjects( EnumObjectsCallback, (VOID*)DIRECTX.Window, DIDFT_ALL );
}

//-------------------------------------------------------------------------------------
bool Control::AnyKeyDown(bool includeXBOXPads)
{
    for (int i=0;i<(includeXBOXPads ? 256 : XBOX_DPAD_UP) ;i++)
    {
        if (DIRECTX.Key[i]) return(true);
    }
    return(false);
}

//-----------------------------------------------------------------------------
void Control::Process(void)
{
    // Lets update various control and logic
    SaveLastKeys();

    // Update joystate
    if( g_pJoystick )
    {
        DIJOYSTATE2 joystate;
        HRESULT hr = g_pJoystick->Poll(); 
        if( FAILED(g_pJoystick->Poll()) )  
            {
            hr = g_pJoystick->Acquire();
            while( hr == DIERR_INPUTLOST ) 
                hr = g_pJoystick->Acquire();
            return; 
            }
        else g_pJoystick->GetDeviceState( sizeof(DIJOYSTATE2), &joystate );

        // Set response to keypresses
        DIRECTX.Key[XBOX_A] = (joystate.rgbButtons[0]) ? true : false;
        DIRECTX.Key[XBOX_B] = (joystate.rgbButtons[1]) ? true : false;
        DIRECTX.Key[XBOX_X] = (joystate.rgbButtons[2]) ? true : false;
        DIRECTX.Key[XBOX_Y] = (joystate.rgbButtons[3]) ? true : false;
        DIRECTX.Key[XBOX_DPAD_LEFT]  = (joystate.lX < -800) ? true : false;
        DIRECTX.Key[XBOX_DPAD_RIGHT] = (joystate.lX > +800) ? true : false;
        DIRECTX.Key[XBOX_DPAD_UP]    = (joystate.lY < -800) ? true : false;
        DIRECTX.Key[XBOX_DPAD_DOWN]  = (joystate.lY > +800) ? true : false;
    }
}

//----------------------------------------------------------------------
void Control::SaveLastKeys()
{
    for (int i=0;i<256;i++)
        LastKey[i] = DIRECTX.Key[i];
}

//----------------------------------------------------------------------
bool Control::KeyDown(int c) 
{
    return(DIRECTX.Key[c]);
}

//----------------------------------------------------------------------
bool Control::KeyPress(int c)
{ 
    return (  (DIRECTX.Key[c] && !LastKey[c]) ? true : false);
}

