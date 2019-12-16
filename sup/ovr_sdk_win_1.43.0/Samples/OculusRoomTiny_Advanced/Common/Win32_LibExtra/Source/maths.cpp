
#include "../Header/libextra.h"

//----------------------------------------------------------
float Maths::Pi()
{ 
    return(3.14159265f);
}

//------------------------------------------------------------
float Maths::RTOD()
{
    return(180.0f / Pi());
}

//------------------------------------------------------------
float Maths::Rand01()
{
    return(((float)(rand() % 32768)) / 32768.0f);
}

//------------------------------------------------------------
void Maths::Limit(float * value, float min, float max)
{
    if (*value < min) *value = min;
    if (*value > max) *value = max;
}

//------------------------------------------------------------
Vector3 Maths::Lerp(Vector3 a, Vector3 b, float propOfB)
{
    return(Vector3(
        a.x*(1.0f - propOfB) + b.x*propOfB,
        a.y*(1.0f - propOfB) + b.y*propOfB,
        a.z*(1.0f - propOfB) + b.z*propOfB));
}

//------------------------------------------------------------
float Maths::Lerp(float a, float b, float propOfB)
{
    return(a*(1.0f - propOfB) + b*propOfB);
}

//----------------------------------------------------
DWORD Maths::ColourLerp(DWORD one, DWORD two, float propOfOne)
{
    float a1 = (float)((one >> 24) & 0xff);
    float r1 = (float)((one >> 16) & 0xff);
    float g1 = (float)((one >> 8) & 0xff);
    float b1 = (float)((one >> 0) & 0xff);
    float a2 = (float)((two >> 24) & 0xff);
    float r2 = (float)((two >> 16) & 0xff);
    float g2 = (float)((two >> 8) & 0xff);
    float b2 = (float)((two >> 0) & 0xff);

    float a = Maths::Lerp(a1, a2, propOfOne);
    float r = Maths::Lerp(r1, r2, propOfOne);
    float g = Maths::Lerp(g1, g2, propOfOne);
    float b = Maths::Lerp(b1, b2, propOfOne);

    DWORD result = (((DWORD)a) << 24) + (((DWORD)r) << 16) + (((DWORD)g) << 8) + (((DWORD)b) << 0);
    return(result);
};

//-----------------------------------------------------------
Vector3 Maths::GetEulerFromQuat(float x, float y, float z, float w)
{
    Vector3 pitchYawRoll;
    pitchYawRoll.z = atan2(2 * y*w - 2 * x*z, 1 - 2 * y*y - 2 * z*z);
    pitchYawRoll.x = atan2(2 * x*w - 2 * y*z, 1 - 2 * x*x - 2 * z*z);
    pitchYawRoll.y = asin(2 * x*y + 2 * z*w);
    return(pitchYawRoll);
}

//-----------------------------------------------------------
Vector3 Maths::GetEulerFromQuat(Quat4 q)
{
    return(GetEulerFromQuat(q.x, q.y, q.z, q.w));
}


