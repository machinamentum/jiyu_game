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

//-------------------------------------------------------
Matrix44 VRCONVERT_Matrix44(ovrMatrix4f ovrm)
{
    Matrix44 temp;
    for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
        temp.m[i][j] = ovrm.M[j][i];
    return(temp);
}

//----------------------------------------------------
Quat4 VRCONVERT_Quat4(ovrQuatf v)
{
    return(Quat4(v.x, v.y, v.z, v.w));
}

//--------------------------------------------------
Vector3 VRCONVERT_Vector3(ovrVector3f v)
{
    return(Vector3(v.x, v.y, v.z));
}

//--------------------------------------------------
ovrVector3f VRCONVERT_ovrVector3f(Vector3 v)
{
    ovrVector3f ret;
    ret.x = v.x;
    ret.y = v.y;
    ret.z = v.z;
    return(ret);
}
//--------------------------------------------------
ovrQuatf VRCONVERT_ovrQuatf(Quat4 v)
{
    ovrQuatf ret;
    ret.w = v.w;
    ret.x = v.x;
    ret.y = v.y;
    ret.z = v.z;
    return(ret);
}

//-------------------------------------------
Matrix44 VRCONVERT_GetViewMatrixWithEyePose(Camera * cam, ovrPosef pose)
{
    return(cam->GetViewMatrixWithEyePose(VRCONVERT_Vector3(pose.Position), VRCONVERT_Quat4(pose.Orientation)));
}





















