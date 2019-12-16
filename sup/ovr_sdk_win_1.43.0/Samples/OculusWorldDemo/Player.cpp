/************************************************************************************

Filename    :   Player.cpp
Content     :   Player location and hit-testing logic source
Created     :   October 4, 2012

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

#include "Player.h"
#include "Kernel/OVR_Alg.h"


Player::Player() :
    ProfileStandingEyeHeight(1.76f - 0.15f),
    UserStandingEyeHeight(1.76f - 0.15f), // 1.76 meters height (ave US male, Wikipedia), less 15 centimeters (TomF's top-of-head-to-eye distance).
    UserFrozen(false),
    BodyYaw(YawInitial),
    HeadPose(),
    MoveForward(0),
    MoveBack(0),
    MoveLeft(0),
    MoveRight(0),
    GamepadMove(),
    GamepadRotate(),
    bMotionRelativeToBody(false),
    ComfortTurnSnap(-1.0f),
    BodyPos(7.7f, 1.76f - 0.15f, -1.0f),
    BodyPoseFloorLevel(7.7f, 0.0f, -1.0f),
    HeightScale(1.0f)
{
}

Player::~Player()
{
}

// Accounts for ComfortTurn setting.
Anglef Player::GetApparentBodyYaw()
{
    Anglef yaw = BodyYaw;
    if ( ComfortTurnSnap > 0.0f )
    {
        float yawR = yaw.Get();
        yawR *= 1.0f / ComfortTurnSnap;
        yawR = floorf ( yawR + 0.5f );
        yawR *= ComfortTurnSnap;
        yaw.Set ( yawR );
    }
    return yaw;
}

Vector3f Player::GetHeadPosition(ovrTrackingOrigin trackingOrigin)
{
    return GetBodyPos(trackingOrigin) + Quatf(Vector3f(0, 1, 0), GetApparentBodyYaw().Get()).Rotate(HeadPose.Translation);
}

Quatf Player::GetOrientation(bool baseOnly)
{
    Quatf baseQ = Quatf(Vector3f(0,1,0), GetApparentBodyYaw().Get());
    return baseOnly ? baseQ : baseQ * HeadPose.Rotation;
}

Posef Player::VirtualWorldTransformfromRealPose(const Posef &sensorHeadPose, ovrTrackingOrigin trackingOrigin)
{
    Quatf baseQ = Quatf(Vector3f(0,1,0), GetApparentBodyYaw().Get());

    Vector3f bodyPosInOrigin = trackingOrigin == ovrTrackingOrigin_EyeLevel ? BodyPos : BodyPoseFloorLevel;

    return Posef(baseQ * sensorHeadPose.Rotation,
                 bodyPosInOrigin + baseQ.Rotate(sensorHeadPose.Translation));
}

void Player::HandleMovement(double dt, std::vector<Ptr<CollisionModel> >* collisionModels,
	                        std::vector<Ptr<CollisionModel> >* groundCollisionModels, bool shiftDown)
{
    if(UserFrozen)
        return;

    // Handle keyboard movement.
    // This translates BasePos based on the orientation and keys pressed.
    // Note that Pitch and Roll do not affect movement (they only affect view).
    Vector3f controllerMove;
    if(MoveForward || MoveBack || MoveLeft || MoveRight)
    {
        if (MoveForward)
        {
            controllerMove += ForwardVector;
        }
        else if (MoveBack)
        {
            controllerMove -= ForwardVector;
        }

        if (MoveRight)
        {
            controllerMove += RightVector;
        }
        else if (MoveLeft)
        {
            controllerMove -= RightVector;
        }
    }
    else if (GamepadMove.LengthSq() > 0)
    {
        controllerMove = GamepadMove;
    }
    controllerMove = GetOrientation(bMotionRelativeToBody).Rotate(controllerMove);    
    controllerMove.y = 0; // Project to the horizontal plane
    if (controllerMove.LengthSq() > 0)
    {
        // Normalize vector so we don't move faster diagonally.
        controllerMove.Normalize();
        controllerMove *= OVR::Alg::Min<float>(MoveSpeed * (float)dt * (shiftDown ? 3.0f : 1.0f), 1.0f);
    }

    // Compute total move direction vector and move length
    Vector3f orientationVector = controllerMove;
    float moveLength = orientationVector.Length();
    if (moveLength > 0)
        orientationVector.Normalize();
        
    float   checkLengthForward = moveLength;
    Planef  collisionPlaneForward;
    bool    gotCollision = false;

    for(size_t i = 0; i < collisionModels->size(); ++i)
    {
        // Checks for collisions at model base level, which should prevent us from
		// slipping under walls
        if (collisionModels->at(i)->TestRay(BodyPos, orientationVector, checkLengthForward,
				                            &collisionPlaneForward))
        {
            gotCollision = true;
            break;
        }
    }

    if (gotCollision)
    {
        // Project orientationVector onto the plane
        Vector3f slideVector = orientationVector - collisionPlaneForward.N
			* (orientationVector.Dot(collisionPlaneForward.N));

        // Make sure we aren't in a corner
        for(size_t j = 0; j < collisionModels->size(); ++j)
        {
            if (collisionModels->at(j)->TestPoint(BodyPos - Vector3f(0.0f, RailHeight, 0.0f) +
					                                (slideVector * (moveLength))) )
            {
                moveLength = 0;
                break;
            }
        }
        if (moveLength != 0)
        {
            orientationVector = slideVector;
        }
    }
    // Checks for collisions at foot level, which allows us to follow terrain
    orientationVector *= moveLength;
    BodyPos += orientationVector;

    Planef collisionPlaneDown;
    float adjustedUserEyeHeight = GetFloorDistanceFromTrackingOrigin(ovrTrackingOrigin_EyeLevel);
    float finalDistanceDown = adjustedUserEyeHeight + 10.0f;

    // Only apply down if there is collision model (otherwise we get jitter).
    if (groundCollisionModels->size())
    {
        for(size_t i = 0; i < groundCollisionModels->size(); ++i)
        {
            float checkLengthDown = adjustedUserEyeHeight + 10;
            if (groundCollisionModels->at(i)->TestRay(BodyPos, Vector3f(0.0f, -1.0f, 0.0f),
                checkLengthDown, &collisionPlaneDown))
            {
                finalDistanceDown = Alg::Min(finalDistanceDown, checkLengthDown);
            }
        }

        // Maintain the minimum camera height
        if (adjustedUserEyeHeight - finalDistanceDown < 1.0f)
        {
            BodyPos.y += adjustedUserEyeHeight - finalDistanceDown;
        }
    }
    
    SetBodyPos(BodyPos, false);
}

void Player::AdjustBodyYaw(float degrees)
{
    if(!UserFrozen)
        BodyYaw += degrees;
}

// Handle directional movement. Returns 'true' if movement was processed.
bool Player::HandleMoveKey(OVR::KeyCode key, bool down)
{
    switch(key)
    {
        // Handle player movement keys.
        // We just update movement state here, while the actual translation is done in OnIdle()
        // based on time.
    case OVR::Key_W:     MoveForward = down ? (MoveForward | 1) : (MoveForward & ~1); return true;
    case OVR::Key_S:     MoveBack    = down ? (MoveBack    | 1) : (MoveBack    & ~1); return true;
    case OVR::Key_A:     MoveLeft    = down ? (MoveLeft    | 1) : (MoveLeft    & ~1); return true;
    case OVR::Key_D:     MoveRight   = down ? (MoveRight   | 1) : (MoveRight   & ~1); return true;
    case OVR::Key_MouseWheelAwayFromUser:
    case OVR::Key_Up:    MoveForward = down ? (MoveForward | 2) : (MoveForward & ~2); return true;
    case OVR::Key_MouseWheelTowardUser:
    case OVR::Key_Down:  MoveBack    = down ? (MoveBack    | 2) : (MoveBack    & ~2); return true;
    case OVR::Key_Left:  MoveLeft    = down ? (MoveLeft    | 2) : (MoveLeft    & ~2); return true;
    case OVR::Key_Right: MoveRight   = down ? (MoveRight   | 2) : (MoveRight   & ~2); return true;
    default: return false;
    }
}
