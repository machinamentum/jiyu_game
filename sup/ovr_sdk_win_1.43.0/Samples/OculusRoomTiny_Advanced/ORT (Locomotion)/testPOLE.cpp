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

namespace TESTPOLE
{
    /// Variables / Demo controls
    /// =========================
    /// This demo doesn't show much, just a starting project
    /// '1' recentres
    /// '2' hold to toggle other holodeck/ORT model
    /// Touch sticks rotate, and its engineered to pivot around point of contact
    /// if ground is grasped. 
    bool lastTouchHasMomentum;  // Should be last controller
    float smoothFactor = 0.95f;
    bool smoothedMomentum = true;
    float directionOfTwistRotate; // +1 for rotating world under you (RECOMMENDED), -1 for steering player
    bool allowTouchTwistRotate; /// THIS IS STILL CAUSING NAUSEA
    float maxLength = 8.0f;   // How long the poles are and still able to grasp

    // Local
    Model * touchModel[2] = { 0, 0 };
    Model * floorImpactModel[2] = { 0, 0 };
}
using namespace TESTPOLE;

//-------------------------------
int(*TESTPOLE_Init(bool argLastTouchHasMomentum, bool argAllowTouchTwistRotate, float argDirectionOfTwistRotate))(void)
{
    lastTouchHasMomentum = argLastTouchHasMomentum;
    allowTouchTwistRotate = argAllowTouchTwistRotate;
    directionOfTwistRotate = argDirectionOfTwistRotate;

    // Make the VR layer, with all its ingredients 
    Layer0 = standardLayer0;

    if (!touchModel[0]) touchModel[0] = SCENE_CreateTouchModel(maxLength);
    if (!touchModel[1]) touchModel[1] = SCENE_CreateTouchModel(maxLength);
    if (!floorImpactModel[0]) floorImpactModel[0] = SCENE_CreateTouchModel(10);
    if (!floorImpactModel[1]) floorImpactModel[1] = SCENE_CreateTouchModel(10);

    *mainCam = Camera(&Vector3(0.0f, 2.0f, 0.0f), &Quat4());

    return(TESTPOLE_Process_And_Render);
}

//----------------------------------
int TESTPOLE_Process_And_Render()
{
    APPCAMERA_DefaultMove(mainCam, 0.05f, 0.02f);

    // Generate new text
    if (frameIndex < 100)
    {
        INFOTEXT_GetModelToAdd()->Add(1, "USING TOUCH AS SKIPOLE MOVEMENT")->CR();
        INFOTEXT_GetModelToAdd()->Add(1, "Press '1' if this isn't in front of you");
    }

    // Get the current pose, for standard eye offset, for general use.
    double sensorSampleTime = 0;
    ovrPosef currPose[2];

#ifdef BUILD_FOR_OLD_SDK
    ovrVector3f HmdToEyeOffset[2] = { { -0.032f, 0, 0 }, { +0.032f, 0, 0 } };
    ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyeOffset, currPose, &sensorSampleTime);
#else
    ovrPosef HmdToEyePose[2];
    HmdToEyePose[0].Position = VRCONVERT_ovrVector3f(Vector3(-0.032f, 0, 0));
    HmdToEyePose[1].Position = VRCONVERT_ovrVector3f(Vector3(+0.032f, 0, 0));
    HmdToEyePose[0].Orientation = VRCONVERT_ovrQuatf(Quat4());
    HmdToEyePose[1].Orientation = VRCONVERT_ovrQuatf(Quat4());
    ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, currPose, &sensorSampleTime);
#endif

    // Get Touch positions, and write them in. 
    double ftiming = ovr_GetPredictedDisplayTime(session, 0);
    ovrTrackingState trackingState = ovr_GetTrackingState(session, ftiming, ovrTrue);
    Vector3 touchPos[2];
    touchPos[0] = mainCam->Pos + VRCONVERT_Vector3(trackingState.HandPoses[ovrHand_Left].ThePose.Position).Rotate(&mainCam->Rot);
    touchPos[1] = mainCam->Pos + VRCONVERT_Vector3(trackingState.HandPoses[ovrHand_Right].ThePose.Position).Rotate(&mainCam->Rot);
    Quat4   touchRot[2];
    Quat4   pureTouchRot[2];
    touchRot[0] = VRCONVERT_Quat4(trackingState.HandPoses[ovrHand_Left].ThePose.Orientation) * mainCam->Rot;
    touchRot[1] = VRCONVERT_Quat4(trackingState.HandPoses[ovrHand_Right].ThePose.Orientation) * mainCam->Rot;
    pureTouchRot[0] = VRCONVERT_Quat4(trackingState.HandPoses[ovrHand_Left].ThePose.Orientation);
    pureTouchRot[1] = VRCONVERT_Quat4(trackingState.HandPoses[ovrHand_Right].ThePose.Orientation);

    // Read button presses
    bool triggerHeld[2] = { false, false };
    static bool pureTriggerHeld[2] = { false, false };
    static int timeWhenLastHeld[2] = { 0, 0 };
    static int lastTouchThatTriggerHeld;
    ovrInputState inputState;
    ovr_GetInputState(session, ovrControllerType_Touch, &inputState);
    // ...and do Yaw
    float yawOfTouch[2];
    static float savedYawOfTouch[2] = { 0, 0 };
    static float savedCameraYaw[2] = { 0, 0 };
    static bool beyondMaxLength[2] = { false, false };

    // Lets deal with identifying triggers held first, so we can put 
    // subsequent logic dependent on it. 

    for (int t = 0; t < 2; t++)
    {
        if (((t == 0) && (inputState.Buttons & ovrTouch_Y))
            || ((t == 1) && (inputState.Buttons & ovrTouch_B))
            || (inputState.HandTrigger[t] > 0.5f)
            || (inputState.IndexTrigger[t] > 0.5f))
        {
            // If newly held, set time
            if (pureTriggerHeld[t] == false)
                timeWhenLastHeld[t] = frameIndex;

            pureTriggerHeld[t] = true;
        }
        else
        {
            pureTriggerHeld[t] = false;
        }
        triggerHeld[t] = pureTriggerHeld[t];
    }

    // If they are both held, then turn off least-recent one
    // Only if they are not both held, do we update the time of when last pressed
    bool makeBlackCosConflict[2] = { false, false };
    if ((triggerHeld[0]) && (triggerHeld[1]))
    {
        if (timeWhenLastHeld[0] > timeWhenLastHeld[1]) // So 0 is most recent
        {
            triggerHeld[1] = false;
            makeBlackCosConflict[1] = true;
        }
        else
        {
            triggerHeld[0] = false;
            makeBlackCosConflict[0] = true;
        }
    }

    for (int t = 0; t < 2; t++)
    {
        // Now lets try and find impact with floor (PART 1 - need early to see if not touching, for rotation)
        Vector3 origin = touchModel[t]->Pos;
        Vector3 direction = Vector3(0, -1, 0).Rotate(&touchModel[t]->Rot);
        float m = -origin.y / direction.y;
        if ((m > maxLength) || (m<0)/*stop upside down*/) beyondMaxLength[t] = true;
        else                                              beyondMaxLength[t] = false;

        // Grabbing and touch stuff
        yawOfTouch[t] = Maths::GetEulerFromQuat(pureTouchRot[t]).z;

        if (beyondMaxLength[t] == true)
        {
            // Take away all response if beyond
            triggerHeld[t] = false;
        }
        else
        {
            if (triggerHeld[t] == true)
            {
                // Lets also rotate from Touch
                if (allowTouchTwistRotate)
                {
                    mainCam->OptionalYaw = savedCameraYaw[t] + directionOfTwistRotate*(savedYawOfTouch[t] - yawOfTouch[t]);
                    mainCam->Rot = Quat4(0, mainCam->OptionalYaw, 0);
                }
            }
            else
            {
                // Needed for rotation from Touch
                savedYawOfTouch[t] = yawOfTouch[t];
                savedCameraYaw[t] = mainCam->OptionalYaw;
            }
        }

        // Initial placement
        touchModel[t]->Pos = touchPos[t];
        touchModel[t]->Rot = touchRot[t];

        // Now lets try and find impact with floor (PART 2)
        Vector3 floorPos = origin + direction * m;
        floorImpactModel[t]->Pos = floorPos;

        // Move main camera by drag
        static Vector3 lastUnheldCamPos[2] = { Vector3(0, 0, 0), Vector3(0, 0, 0) };
        static Vector3 lastUnheldFloorPosOffset[2] = { Vector3(0, 0, 0), Vector3(0, 0, 0) };
        static Vector3 lastCamVel[2] = { Vector3(0, 0, 0), Vector3(0, 0, 0) };
        static Vector3 lastCamPos[2] = { Vector3(0, 0, 0), Vector3(0, 0, 0) };
        static Vector3 smoothedLastCamVel[2] = { Vector3(0, 0, 0), Vector3(0, 0, 0) }; 
        if (!triggerHeld[t])
        {
            lastUnheldCamPos[t] = mainCam->Pos;
            lastUnheldFloorPosOffset[t] = floorPos - mainCam->Pos;
            if ((t == lastTouchThatTriggerHeld) && (lastTouchHasMomentum))
            {
                if (smoothedMomentum) mainCam->Pos += smoothedLastCamVel[t];
                else                  mainCam->Pos += lastCamVel[t];
            }
        }
        else
        {
            mainCam->Pos = lastUnheldCamPos[t] - ((floorPos - mainCam->Pos) - lastUnheldFloorPosOffset[t]);
            lastCamVel[t] = mainCam->Pos - lastCamPos[t];
            smoothedLastCamVel[t] = (smoothedLastCamVel[t] * smoothFactor) + (lastCamVel[t] * (1.0f - smoothFactor));
        }

        lastCamPos[t] = mainCam->Pos;

        // Temporary touch up touch!!!!
        // No that mainCam pos has changed
        touchPos[t] = mainCam->Pos + VRCONVERT_Vector3(trackingState.HandPoses[t==0? ovrHand_Left : ovrHand_Right].ThePose.Position).Rotate(&mainCam->Rot);
        touchModel[t]->Pos = touchPos[t];

        // And temp retouch up floor thing
        origin = touchModel[t]->Pos;
        direction = Vector3(0, -1, 0).Rotate(&touchModel[t]->Rot);
        m = -origin.y / direction.y;
        floorPos = origin + direction * m;
        floorImpactModel[t]->Pos = floorPos;
    }
    
    // Render Scene to Eye Buffers
    for (int eye = 0; eye < 2; ++eye)
    {
        // Layer0
        Layer0->PrepareToRender(session, eye, currPose[eye], sensorSampleTime, 0, 0, 0, 0, true);
        Matrix44 projMatrix = VRCONVERT_Matrix44(ovrMatrix4f_Projection(Layer0->ld.Fov[eye], 0.1f, 1000.0f, ovrProjection_None));
        Matrix44 viewMatrix = VRCONVERT_GetViewMatrixWithEyePose(mainCam, Layer0->ld.RenderPose[eye]);

        roomModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        corridorModel->Render(&(viewMatrix * projMatrix), 1, 1, 1, 1, true);
        if ((beyondMaxLength[0]) || (makeBlackCosConflict[0]))
            touchModel[0]->Render(&(viewMatrix * projMatrix), 1, 0, 0, 1, true);
        else
            touchModel[0]->Render(&(viewMatrix * projMatrix), 1, 1, triggerHeld[0] ? 0.0f : 1.0f, 1, true);
        if ((beyondMaxLength[1]) || (makeBlackCosConflict[1]))
            touchModel[1]->Render(&(viewMatrix * projMatrix), 1, 0, 0, 1, true);
        else
            touchModel[1]->Render(&(viewMatrix * projMatrix), 1, 1, triggerHeld[1] ? 0.0f : 1.0f, 1, true);
        if (triggerHeld[0]) floorImpactModel[0]->Render(&(viewMatrix * projMatrix), 1, 1, 0, 1, true);
        if (triggerHeld[1]) floorImpactModel[1]->Render(&(viewMatrix * projMatrix), 1, 1, 0, 1, true);

        // Output text
        INFOTEXT_RenderWithOwnCameraAndReset(projMatrix, Layer0->ld.RenderPose[eye]);
        Layer0->FinishRendering(session, eye);
    }

    // Submit all the layers to the compositor
    return(1);
}

