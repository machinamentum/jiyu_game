
#include "../Header/libextra.h"

//---------------------------------------------------------
void Debug::Error(char * errorMessage, ...)
{
    static char string_text[1000];
    va_list args; va_start(args, errorMessage);
    vsprintf_s(string_text, errorMessage, args);
    va_end(args);
    Output("%s\n", string_text);
    MessageBoxA(NULL, string_text, "OculusRoomTiny", MB_ICONERROR | MB_OK);
    exit(-1);
}

//---------------------------------------------------------
void Debug::Assert(bool test, char * errorMessage, ...) 
{
    va_list args; va_start(args, errorMessage);
    if (!test) Error(errorMessage,args);
    va_end(args);
}

//------------------------------------------------------
void Debug::Output(const char * fnt, ...)
{
    static char string_text[1000];
    va_list args; va_start(args, fnt);
    vsprintf_s(string_text, fnt, args);
    va_end(args);
    OutputDebugStringA(string_text);
}
