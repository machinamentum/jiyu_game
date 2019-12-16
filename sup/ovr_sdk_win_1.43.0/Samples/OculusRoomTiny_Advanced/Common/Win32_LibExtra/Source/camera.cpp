
#include "../Header/libextra.h"

//-----------------------------------------------------------
Camera::Camera()
{
    Pos = Vector3(0, 0, 0);
    Rot = Quat4(0, 0, 0);
    OptionalYaw = 0;
    idealPos = Pos;
    smoothedPos = Vector3(0, 0, 0);
};
//-----------------------------------------------------------
Camera::Camera(Vector3 * pos, Quat4 * rot) : Pos(*pos), Rot(*rot)
{
    OptionalYaw = 0;
    idealPos = Pos;
    smoothedPos = Pos;
};

//-----------------------------------------------------------
Matrix44 Camera::GetViewMatrix()
{
    Vector3 forward = Vector3(0, 0, -1).Rotate(&Rot);
    return(Vector3::GetLookAtRH(Pos, Pos + forward, Vector3(0, 1, 0).Rotate(&Rot)));
}

//------------------------------------------------------------
Matrix44 Camera::GetViewProjMatrix(Matrix44 * projMat)
{
    return (GetViewMatrix() * (*projMat));
}

//--------------------------------------------
Matrix44 Camera::GetViewMatrixWithEyePose(Vector3 eyePos, Quat4 eyeQuat)
{
    Camera worldCamWithPose(&(Pos + eyePos.Rotate(&Rot)),
        &(eyeQuat * Rot));
    return(worldCamWithPose.GetViewMatrix());
}


