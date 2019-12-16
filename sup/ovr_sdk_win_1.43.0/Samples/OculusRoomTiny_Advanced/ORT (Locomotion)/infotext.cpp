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

TextModel * debugInfoModel;
Camera * textCam;

//------------------------------
void INFOTEXT_Init()
{
    Font * arialFont = new Font("arial72.bmp", "arial72data.txt", false);
    debugInfoModel = new TextModel(arialFont, 2000, 0);
    textCam = new Camera(&Vector3(0.0f, 0.0f, 0.69f), &Quat4()); // 69cm away
}

//----------------------------------------------
void INFOTEXT_StartAddingText()
{
    debugInfoModel->Reset();
    debugInfoModel->Scale(0.03f, 0.03f);
}

//---------------------------------
TextModel * INFOTEXT_GetModelToAdd()
{
    return(debugInfoModel);
}

//----------------------------------
void INFOTEXT_RenderWithOwnCameraAndReset(Matrix44 projMatrix, ovrPosef pose)
{
    debugInfoModel->UpdateModel(); // OK, might do this twice for each eye - no matter.
    Matrix44 textViewMatrix = VRCONVERT_GetViewMatrixWithEyePose(textCam, pose);
    debugInfoModel->Render(&(textViewMatrix * projMatrix), 1, 1, 1, 1);
}
