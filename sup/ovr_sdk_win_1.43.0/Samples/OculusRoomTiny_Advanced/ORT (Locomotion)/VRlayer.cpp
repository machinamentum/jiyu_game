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

//---------------------------------------------------------------------
void VRLayer::CommonInit(ovrSession session, int eye, ovrSizei idealSize, bool argUseMSAA)
{
    // For general (done twice for both eyes, slightly wasteful
    ld.Header.Flags = 0;
    useMSAA = false;

    // That gives us a viewport
    ld.Viewport[eye].Pos.x = 0;
    ld.Viewport[eye].Pos.y = 0;
    ld.Viewport[eye].Size = idealSize;

    // Make texture chains
    ovrTextureSwapChainDesc desc = {};
    desc.Type = ovrTexture_2D;
    desc.ArraySize = 1;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.Width = idealSize.w;
    desc.Height = idealSize.h;
    desc.MipLevels = 1;
    desc.SampleCount = 1;
    desc.MiscFlags = ovrTextureMisc_DX_Typeless;
    desc.BindFlags = ovrTextureBind_DX_RenderTarget;
    desc.StaticImage = ovrFalse;
    ovr_CreateTextureSwapChainDX(session, DIRECTX.Device, &desc, &ld.ColorTexture[eye]);

    // Now make the rtvs
    int textureCount;
    ovr_GetTextureSwapChainLength(session, ld.ColorTexture[eye], &textureCount);
    for (int i = 0; i < textureCount; ++i)
    {
        ID3D11Texture2D* tex;
        ovr_GetTextureSwapChainBufferDX(session, ld.ColorTexture[eye], i, IID_PPV_ARGS(&tex));
        D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
        rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        rtvd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
        DIRECTX.Device->CreateRenderTargetView(tex, &rtvd, &TexRtv[eye][i]);
        tex->Release();
    }

    // And a depth buffer    
    pEyeDepthBuffer[eye] = new DepthBuffer(idealSize.w, idealSize.h);
}

//---------------------------------------------------------------------
VRLayer::VRLayer(ovrSession session, ovrFovPort leftFOV, ovrFovPort rightFOV, float pixelsPerDisplayPixel, bool argUseMSAA)
{
    ld.Header.Type = ovrLayerType_EyeFov;
    for (int eye = 0; eye < 2; ++eye)
    {
        // Find the size from FOV
        if (eye == 0) ld.Fov[eye] = leftFOV;
        else          ld.Fov[eye] = rightFOV;
        ovrSizei idealSize = ovr_GetFovTextureSize(session, (ovrEyeType)eye, ld.Fov[eye], pixelsPerDisplayPixel);

        CommonInit(session, eye, idealSize,argUseMSAA);

        // Finally, lets have MSAA stuff ready, even if don't use it
        static const int sampleCount = 4;
        MSAATexture[eye]     = new Texture(true, idealSize.w, idealSize.h, 0, sampleCount,1);
        MSAADepthBuffer[eye] = new DepthBuffer(idealSize.w, idealSize.h, sampleCount);
    }
}

//--------------------------------------------
void VRLayer::SetUseMSAA(bool isItEnabled)
{
    useMSAA = isItEnabled;
}

//---------------------------------------------
void VRLayer::PrepareToRender(ovrSession session, int eye, ovrPosef poseWeUsed, double sensorSampleTime, float R, float G, float B, float A, bool doWeClear)
{
    int index;
    ovr_GetTextureSwapChainCurrentIndex(session, ld.ColorTexture[eye], &index);

    // MSAA
    if (useMSAA)    DIRECTX.SetAndClearRenderTarget(MSAATexture[eye]->TexRtv, MSAADepthBuffer[eye], R, G, B, A, doWeClear);
    else            DIRECTX.SetAndClearRenderTarget(TexRtv[eye][index], pEyeDepthBuffer[eye], R, G, B, A, doWeClear);

    DIRECTX.SetViewport((float)ld.Viewport[eye].Pos.x, (float)ld.Viewport[eye].Pos.y,
        (float)ld.Viewport[eye].Size.w, (float)ld.Viewport[eye].Size.h);

    // Only relevent for EyeFov
    ld.RenderPose[eye] = poseWeUsed; // We do this now, as might be relying on it in to get cameras in a moment
    ld.SensorSampleTime = sensorSampleTime;
}

//---------------------------------------------
void VRLayer::SetShuntedViewport(int eye, int xShunt, int yShunt)
{
    DIRECTX.SetViewport((float)ld.Viewport[eye].Pos.x + xShunt, (float)ld.Viewport[eye].Pos.y + yShunt,
        (float)ld.Viewport[eye].Size.w, (float)ld.Viewport[eye].Size.h);
}

//------------------------------------------------
void VRLayer::ClearDepth(ovrSession session, int eye)
{
    int index;
    ovr_GetTextureSwapChainCurrentIndex(session, ld.ColorTexture[eye], &index);

    if (useMSAA) DIRECTX.Context->ClearDepthStencilView(MSAADepthBuffer[eye]->TexDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
    else         DIRECTX.Context->ClearDepthStencilView(pEyeDepthBuffer[eye]->TexDsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1, 0);
}

//-------------------------------------------------------
void VRLayer::FinishRendering(ovrSession session, int eye)
{
    // Resolve down into smaller buffer if MSAA
    if (useMSAA)
    {
        int destIndex = 0;
        ovr_GetTextureSwapChainCurrentIndex(session, ld.ColorTexture[eye], &destIndex);
        ID3D11Resource* dstTex = nullptr;
        ovr_GetTextureSwapChainBufferDX(session, ld.ColorTexture[eye], destIndex, IID_PPV_ARGS(&dstTex));
        DIRECTX.Context->ResolveSubresource(dstTex, 0, MSAATexture[eye]->Tex, 0, DXGI_FORMAT_R8G8B8A8_UNORM);
        dstTex->Release();
    }

    // Commit rendering to the swap chain
    ovr_CommitTextureSwapChain(session, ld.ColorTexture[eye]);
}





































#if 0 //old

//---------------------------------------------------------------
VRLayer::VRLayer(ovrSession argHMD, const ovrFovPort * fov, float pixelsPerDisplayPixel, bool doFullPanelRes)
{
    HMD = argHMD;
    MakeEyeBuffers(pixelsPerDisplayPixel, doFullPanelRes);
    ConfigureRendering(fov);
    frameIndex = 0;
}

//---------------------------------------------------------------
VRLayer::~VRLayer()
{
    for (int eye = 0; eye < 2; ++eye)
    {
        delete pEyeRenderTexture[eye];
        pEyeRenderTexture[eye] = nullptr;
        delete pEyeDepthBuffer[eye];
        pEyeDepthBuffer[eye] = nullptr;
    }
}

//-----------------------------------------------------------------------
void VRLayer::MakeEyeBuffers(float pixelsPerDisplayPixel, bool doFullPanelRes)
{
    for (int eye = 0; eye < 2; ++eye)
    {
        ovrSizei idealSize = ovr_GetFovTextureSize(HMD, (ovrEyeType)eye, ovr_GetHmdDesc(HMD).DefaultEyeFov[eye], pixelsPerDisplayPixel);
        pEyeRenderTexture[eye] = new VRTexture();

        if (doFullPanelRes)
        {
            idealSize = ovr_GetHmdDesc(HMD).Resolution;
            idealSize.w /= 2;  // Just want one panel
        }



        if (!pEyeRenderTexture[eye]->Init(HMD, idealSize.w, idealSize.h))
            return;
        pEyeDepthBuffer[eye] = new DepthBuffer(idealSize.w, idealSize.h);
        EyeRenderViewport[eye].Pos.x = 0;
        EyeRenderViewport[eye].Pos.y = 0;
        EyeRenderViewport[eye].Size = idealSize;
    }
}

//--------------------------------------------------------
void VRLayer::ConfigureRendering(const ovrFovPort * fov)
{
    // If any values are passed as NULL, then we use the default basic case
    if (!fov) fov = ovr_GetHmdDesc(HMD).DefaultEyeFov;
    EyeRenderDesc[0] = ovr_GetRenderDesc(HMD, ovrEye_Left, fov[0]);
    EyeRenderDesc[1] = ovr_GetRenderDesc(HMD, ovrEye_Right, fov[1]);
}

//------------------------------------------------------------
ovrTrackingState VRLayer::GetEyePoses(ovrPosef * useEyeRenderPose, float * scaleIPD, float * newIPD)
{
    // Get both eye poses simultaneously, with IPD offset already included. 
    ovrVector3f useHmdToEyeViewOffset[2] = { EyeRenderDesc[0].HmdToEyeOffset,
        EyeRenderDesc[1].HmdToEyeOffset };

    // If any values are passed as NULL, then we use the default basic case
    if (!useEyeRenderPose) useEyeRenderPose = EyeRenderPose;
    if (scaleIPD)
    {
        useHmdToEyeViewOffset[0].x *= *scaleIPD;
        useHmdToEyeViewOffset[1].x *= *scaleIPD;
    }
    if (newIPD)
    {
        useHmdToEyeViewOffset[0].x = -(*newIPD * 0.5f);
        useHmdToEyeViewOffset[1].x = +(*newIPD * 0.5f);
    }

//    double ftiming = ovr_GetPredictedDisplayTime(HMD, 0);
//    ovrTrackingState trackingState = ovr_GetTrackingState(HMD, ftiming, ovrTrue);
//    ovr_CalcEyePoses(trackingState.HeadPose.ThePose, useHmdToEyeViewOffset, useEyeRenderPose);

    ovrTrackingState trackingState;
    ovr_GetPredictedDisplayTime(HMD, 0);
    // Keeping sensorSampleTime as close to ovr_GetTrackingState as possible - fed into the layer
    sensorSampleTime = ovr_GetTimeInSeconds();
    ovr_GetEyePoses(HMD, frameIndex, ovrTrue, useHmdToEyeViewOffset, useEyeRenderPose, &trackingState);




    return(trackingState);
}

//-----------------------------------------------------------
XMMATRIX VRLayer::RenderSetupToEyeBuffer(Camera * player, int eye, ID3D11RenderTargetView * rtv,
    ovrPosef * eyeRenderPose, int /*timesToRenderRoom*/,
    float alpha, float red, float green, float blue, float nearZ, float farZ,
    bool doWeSetupRender, DepthBuffer * depthBuffer,
    float backRed, float backGre, float backBlu)
{
    // If any values are passed as NULL, then we use the default basic case
    if (!depthBuffer)
        depthBuffer = pEyeDepthBuffer[eye];
    if (!eyeRenderPose)
        eyeRenderPose = &EyeRenderPose[eye];


    if (doWeSetupRender)
    {
        // If none specified, then using special, and default, Oculus eye buffer render target
        if (rtv)
            DIRECTX.SetAndClearRenderTarget(rtv, depthBuffer, backRed, backGre, backBlu);
        else
        {
            // We increment which texture we are using, to the next one, just before writing
            DIRECTX.SetAndClearRenderTarget(pEyeRenderTexture[eye]->GetRTV(), depthBuffer, backRed, backGre, backBlu);
        }

        DIRECTX.SetViewport((float)EyeRenderViewport[eye].Pos.x, (float)EyeRenderViewport[eye].Pos.y,
            (float)EyeRenderViewport[eye].Size.w, (float)EyeRenderViewport[eye].Size.h);
    }

    // Get view and projection matrices for the Rift camera
    XMVECTOR CombinedPos = XMVectorAdd(player->Pos, XMVector3Rotate(ConvertToXM(eyeRenderPose->Position), player->Rot));
    Camera finalCam(&CombinedPos, &(XMQuaternionMultiply(ConvertToXM(eyeRenderPose->Orientation), player->Rot)));
    XMMATRIX view = finalCam.GetViewMatrix();
    ovrMatrix4f p = ovrMatrix4f_Projection(EyeRenderDesc[eye].Fov, nearZ, farZ, ovrProjection_None);
    XMMATRIX prod = XMMatrixMultiply(view, ConvertToXM(p));

    return(prod);
}

//------------------------------------------------------------
void VRLayer::PrepareLayerHeader(VRTexture * leftEyeTexture, ovrPosef * leftEyePose, XMVECTOR * extraQuat)
{
    // Use defaults where none specified
    VRTexture *   useEyeTexture[2] = { pEyeRenderTexture[0], pEyeRenderTexture[1] };
    ovrPosef    useEyeRenderPose[2] = { EyeRenderPose[0], EyeRenderPose[1] };
    if (leftEyeTexture) useEyeTexture[0] = leftEyeTexture;
    if (leftEyePose)    useEyeRenderPose[0] = *leftEyePose;

    // If we need to fold in extra rotations to the timewarp, per eye
    // We make the changes to the temporary copy, rather than 
    // the global one.
    if (extraQuat)
    {
        for (int i = 0; i < 2; i++)
        {
            XMVECTOR localPoseQuat = XMVectorSet(useEyeRenderPose[i].Orientation.x, useEyeRenderPose[i].Orientation.y,
                useEyeRenderPose[i].Orientation.z, useEyeRenderPose[i].Orientation.w);
            XMVECTOR temp = XMQuaternionMultiply(localPoseQuat, XMQuaternionInverse(extraQuat[i]));
            useEyeRenderPose[i].Orientation.w = XMVectorGetW(temp);
            useEyeRenderPose[i].Orientation.x = XMVectorGetX(temp);
            useEyeRenderPose[i].Orientation.y = XMVectorGetY(temp);
            useEyeRenderPose[i].Orientation.z = XMVectorGetZ(temp);
        }
    }

    ovrLayer.Header.Type = ovrLayerType_EyeFov;
    ovrLayer.Header.Flags = 0;
    ovrLayer.SensorSampleTime = sensorSampleTime;

    for (int i = 0; i < 2; i++)
    {
        ovrLayer.ColorTexture[i] = useEyeTexture[i]->TextureSet;
        ovrLayer.RenderPose[i] = useEyeRenderPose[i];
        ovrLayer.Fov[i] = EyeRenderDesc[i].Fov;
        ovrLayer.Viewport[i] = EyeRenderViewport[i];
    }
}


#endif
