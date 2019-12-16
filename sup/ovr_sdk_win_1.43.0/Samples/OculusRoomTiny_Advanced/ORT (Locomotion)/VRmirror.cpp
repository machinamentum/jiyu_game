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

// Distorted both
ovrMirrorTexture mirrorTexture;

// Undistorted left
Model *renderEyeTexture[10]; // Can't be more than 10 different textures in the set
VRLayer * layerForUndistortedLeft;

//----------------------------------------------
void VRMIRROR_Init(ovrSession session, VRLayer * argLayerForUndistortedLeft)
{
    // Distorted both
    ovrMirrorTextureDesc desc = {};
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.Width = DIRECTX.WinSizeW;
    desc.Height = DIRECTX.WinSizeH;
    ovr_CreateMirrorTextureDX(session, DIRECTX.Device, &desc, &mirrorTexture);

    // Undistorted left
    layerForUndistortedLeft = argLayerForUndistortedLeft;
    int textureCount;
    ovr_GetTextureSwapChainLength(session, layerForUndistortedLeft->ld.ColorTexture[0], &textureCount);
    Debug::Assert(textureCount < 10, "Too many textures");

    for (int index = 0; index < textureCount; index++)
    {
        Texture * mirrorEyeBufferTexture = new Texture();
        ID3D11Texture2D* texture = nullptr;
        ovr_GetTextureSwapChainBufferDX(session, layerForUndistortedLeft->ld.ColorTexture[0], index, IID_PPV_ARGS(&texture));
        D3D11_SHADER_RESOURCE_VIEW_DESC srvd{};
        srvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        srvd.Texture2D.MipLevels = 1;
        srvd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
        DIRECTX.Device->CreateShaderResourceView(texture, &srvd, &mirrorEyeBufferTexture->TexSv);
        texture->Release();
        renderEyeTexture[index] = new Model(new Material(mirrorEyeBufferTexture, new Shader()), -1, -1, 1, 1);
    }
}

//----------------------------------------------
void VRMIRROR_Render(ovrSession session, int distortedboth0_undistortedleft1_independent2, void(*independentRenderFunc)(void))
{
    switch (distortedboth0_undistortedleft1_independent2)
    {
    case(0) :   // Distorted both
        ID3D11Texture2D* tex;
        ovr_GetMirrorTextureBufferDX(session, mirrorTexture, IID_PPV_ARGS(&tex));
        DIRECTX.Context->CopyResource(DIRECTX.BackBuffer, tex);
        tex->Release();
        break;

    case(1) :   // Undistorted left
        DIRECTX.SetAndClearRenderTarget(DIRECTX.BackBufferRT, DIRECTX.MainDepthBuffer);
        DIRECTX.SetViewport(0, 0, float(DIRECTX.WinSizeW), float(DIRECTX.WinSizeH));
        int currentIndex;
        ovr_GetTextureSwapChainCurrentIndex(session, layerForUndistortedLeft->ld.ColorTexture[0], &currentIndex);
        renderEyeTexture[currentIndex]->Render(&Matrix44(), 1, 1, 1, 1, true);
        break;

    case(2) :   // Direct independent render
        DIRECTX.SetAndClearRenderTarget(DIRECTX.BackBufferRT, DIRECTX.MainDepthBuffer, 0, 0, 0);
        DIRECTX.SetViewport(0, 0, (float)DIRECTX.WinSizeW, (float)DIRECTX.WinSizeH);
        if (independentRenderFunc) independentRenderFunc();
        break;
    }
    
    // Finally, after we've been through case statements, lets resize world(Copied out of DirectX)
    // Its really only the single window that warrants it
    RECT size = { 0, 0, DIRECTX.WinSizeW, DIRECTX.WinSizeH };
    if (distortedboth0_undistortedleft1_independent2 == 1) size.right = DIRECTX.WinSizeH; ///makes it square, a little crude
    AdjustWindowRect(&size, WS_OVERLAPPEDWINDOW, false);
    DIRECTX.LiveMoveOrStretchWindow(0, 0, size.right - size.left, size.bottom - size.top);
}
