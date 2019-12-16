
#include "../Header/libextra.h"

// This is the only place we include this, for maths types
#include "DirectXMath.h"    
using namespace DirectX;    

// THESE ARE ALL LOCAL CONVERTERS
Matrix44 LOCAL_GetMatrix44(XMFLOAT4X4 * xmf){ Matrix44 temp;    { for (int i = 0; i < 4; i++)    for (int j = 0; j < 4; j++)    temp.m[i][j] = xmf->m[i][j]; }    return(temp);}
XMMATRIX LOCAL_GetXMMATRIX(Matrix44 M)      { return(XMMatrixSet(M.m[0][0], M.m[0][1], M.m[0][2], M.m[0][3], M.m[1][0], M.m[1][1], M.m[1][2], M.m[1][3], M.m[2][0], M.m[2][1], M.m[2][2], M.m[2][3], M.m[3][0], M.m[3][1], M.m[3][2], M.m[3][3]));}
Matrix44 LOCAL_GetMatrix44(XMMATRIX * m)    { XMFLOAT4X4 temp;  XMStoreFloat4x4(&temp, *m); return(LOCAL_GetMatrix44(&temp)); }
Quat4    LOCAL_GetQuat4(XMVECTOR v)         { XMFLOAT4 temp;    XMStoreFloat4(&temp, v); return(Quat4(temp.x, temp.y, temp.z, temp.w)); };
XMVECTOR LOCAL_GetXMVECTOR(Quat4 q)          { return(XMVectorSet(q.x, q.y, q.z, q.w)); }
Vector3  LOCAL_GetVector3(XMVECTOR v)        { XMFLOAT3 temp;    XMStoreFloat3(&temp, v); return(Vector3(temp.x, temp.y, temp.z)); };
XMVECTOR LOCAL_GetXMVECTOR(Vector3 v)        { return(XMVectorSet(v.x, v.y, v.z, 0)); }

// Matrix44 class members that need the converters
Matrix44 Matrix44::operator * (Matrix44 a)            { return(LOCAL_GetMatrix44(&XMMatrixMultiply(LOCAL_GetXMMATRIX(*this), LOCAL_GetXMMATRIX(a)))); }

// Quat4 class members that need the converters
Quat4::Quat4(float pitch, float yaw, float roll)    { *this = LOCAL_GetQuat4(XMQuaternionRotationRollPitchYaw(pitch, yaw, roll)); }
Quat4 Quat4::operator * (Quat4 a)                    { return(LOCAL_GetQuat4(XMQuaternionMultiply(LOCAL_GetXMVECTOR(*this), LOCAL_GetXMVECTOR(a)))); }
Matrix44 Quat4::ToRotationMatrix44()                { return(LOCAL_GetMatrix44(&XMMatrixRotationQuaternion(LOCAL_GetXMVECTOR(*this)))); }

// Vector3 class members that need the converters
Vector3 Vector3::Rotate(Quat4 * rot)                { return(LOCAL_GetVector3(XMVector3Rotate(LOCAL_GetXMVECTOR(*this), LOCAL_GetXMVECTOR(*rot)))); }
Matrix44 Vector3::ToTranslationMatrix44()           { return(LOCAL_GetMatrix44(&XMMatrixTranslationFromVector(LOCAL_GetXMVECTOR(*this)))); }
Matrix44 Vector3::ToScalingMatrix44()               { return(LOCAL_GetMatrix44(&XMMatrixScalingFromVector(LOCAL_GetXMVECTOR(*this)))); }
Matrix44 Vector3::GetLookAtRH(Vector3 pos, Vector3 forw, Vector3 up)    { return(LOCAL_GetMatrix44(&XMMatrixLookAtRH(LOCAL_GetXMVECTOR(pos), LOCAL_GetXMVECTOR(forw), LOCAL_GetXMVECTOR(up)))); };











