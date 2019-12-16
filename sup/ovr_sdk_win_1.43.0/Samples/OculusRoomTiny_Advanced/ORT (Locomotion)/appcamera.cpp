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

#include "general.h"

//-------------------------------------
void APPCAMERA_DefaultMove(Camera * cam, float linearScale, float rotateScale) 
{
    static ovrInputState inputState; // For some reason, in later LibOVRs, this doesn't sit happily on the stack
    ovr_GetInputState(session, ovrControllerType_Touch, &inputState);
    Vector3 forward = Vector3(0, 0, -linearScale).Rotate(&cam->Rot);
    Vector3 right = Vector3(linearScale, 0, 0).Rotate(&cam->Rot);
    if (DIRECTX.Key['W'] || DIRECTX.Key[VK_UP])      cam->Pos += forward;
    if (DIRECTX.Key['S'] || DIRECTX.Key[VK_DOWN]) cam->Pos -= forward;
    if (DIRECTX.Key['D'])                         cam->Pos += right;
    if (DIRECTX.Key['A'])                         cam->Pos -= right;
    if ((DIRECTX.Key[VK_LEFT])
     || (DIRECTX.Key['Q'])
     || (inputState.Thumbstick[0].x < -0.5f)
     || (inputState.Thumbstick[1].x < -0.5f))
     cam->Rot = Quat4(0, cam->OptionalYaw += rotateScale, 0);
    if ((DIRECTX.Key[VK_RIGHT])
     || (DIRECTX.Key['E'])
     || (inputState.Thumbstick[0].x > 0.5f)
     || (inputState.Thumbstick[1].x > 0.5f))
        cam->Rot = Quat4(0, cam->OptionalYaw -= rotateScale, 0);
}

//-------------------------------------------
void APPCAMERA_ApplyLurchAfterCameraMove(Camera * cam, float scalePitch, float scaleRoll, float frequency)
{
    cam->Rot = Quat4(scalePitch*sin(0.03f*frameIndex), cam->OptionalYaw, scaleRoll*sin(frequency*frameIndex));
}

//-------------------------------------
void APPCAMERA_SmoothedMove(Camera * cam, float linearScale, float rotateScale) 
{
    // Nb it moves ideal pos, so can't just use shared function above
    Vector3 forward = Vector3(0, 0, -linearScale).Rotate(&cam->Rot);
    Vector3 right = Vector3(linearScale, 0, 0).Rotate(&cam->Rot);
    if (DIRECTX.Key['W'] || DIRECTX.Key[VK_UP])      cam->idealPos += forward;
    if (DIRECTX.Key['S'] || DIRECTX.Key[VK_DOWN]) cam->idealPos -= forward;
    if (DIRECTX.Key['D'])                         cam->idealPos += right;
    if (DIRECTX.Key['A'])                         cam->idealPos -= right;
    if ((DIRECTX.Key[VK_LEFT]) || (DIRECTX.Key['Q'])) cam->Rot = Quat4(0, cam->OptionalYaw += rotateScale, 0);
    if ((DIRECTX.Key[VK_RIGHT]) || (DIRECTX.Key['E']))cam->Rot = Quat4(0, cam->OptionalYaw -= rotateScale, 0);

    cam->smoothedPos = cam->smoothedPos*0.985f + cam->idealPos*0.015f;
    cam->Pos = cam->smoothedPos;
}

//-------------------------------------
void APPCAMERA_VerySmoothedMoveForTilt(Camera * cam, float linearScale, float rotateScale)  
{
    // Nb it moves ideal pos, so can't just use shared function above
    Vector3 forward = Vector3(0, 0, -linearScale).Rotate(&cam->Rot);
    Vector3 right = Vector3(linearScale, 0, 0).Rotate(&cam->Rot);

    static Vector3 vel = Vector3(0, 0, 0);

    if (DIRECTX.Key['W'] || DIRECTX.Key[VK_UP])      vel += forward;
    if (DIRECTX.Key['S'] || DIRECTX.Key[VK_DOWN]) vel -= forward;
    if (DIRECTX.Key['D'])                         vel += right;
    if (DIRECTX.Key['A'])                         vel -= right;
    if ((DIRECTX.Key[VK_LEFT]) || (DIRECTX.Key['Q'])) cam->Rot = Quat4(0, cam->OptionalYaw += rotateScale, 0);
    if ((DIRECTX.Key[VK_RIGHT]) || (DIRECTX.Key['E']))cam->Rot = Quat4(0, cam->OptionalYaw -= rotateScale, 0);

    vel *= 0.95f;  // fade it off
    cam->idealPos += vel;

    cam->smoothedPos = cam->smoothedPos*0.985f + cam->idealPos*0.015f;
    cam->Pos = cam->smoothedPos;
}


//-------------------------------------
// This was a special one-off one
void APPCAMERA_SmoothedMoveAndCounterMove(Camera * cam, Camera * counterCam, bool doWeSync)
{
    static bool firstTime = true;
    static Vector3 idealPos = Vector3(0, 0, 0);
    static Vector3 smoothedPos = Vector3(0, 0, 0);
    
    if (firstTime)
    {
        idealPos = cam->Pos;
        firstTime = false;
    }

    if (doWeSync)
    {
        counterCam->OptionalYaw = cam->OptionalYaw;
        counterCam->Pos = cam->Pos;
        counterCam->Rot = cam->Rot;
    }

    Vector3 forward = Vector3(0, 0, -0.1f).Rotate(&cam->Rot);
    Vector3 right = Vector3(0.1f, 0, 0).Rotate(&cam->Rot);
    if (DIRECTX.Key['W'] || DIRECTX.Key[VK_UP])      idealPos += forward;
    if (DIRECTX.Key['S'] || DIRECTX.Key[VK_DOWN]) idealPos -= forward;
    if (DIRECTX.Key['D'])                         idealPos += right;
    if (DIRECTX.Key['A'])                         idealPos -= right;
    if (DIRECTX.Key[VK_LEFT])  cam->Rot = Quat4(0, cam->OptionalYaw += 0.02f, 0);
    if (DIRECTX.Key[VK_RIGHT]) cam->Rot = Quat4(0, cam->OptionalYaw -= 0.02f, 0);
    if (DIRECTX.Key[VK_LEFT])  counterCam->Rot = Quat4(0, counterCam->OptionalYaw -= 0.02f, 0);
    if (DIRECTX.Key[VK_RIGHT]) counterCam->Rot = Quat4(0, counterCam->OptionalYaw += 0.02f, 0);

    smoothedPos = smoothedPos*0.985f + idealPos*0.015f;

    // Find the difference so we can negate it from counter cam
    Vector3 diff = smoothedPos - cam->Pos;
    counterCam->Pos -= diff;

    cam->Pos = smoothedPos;
}

