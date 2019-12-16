/************************************************************************************
Filename    :   Win32_RoomTiny_Main.cpp
Content     :   First-person view test application for Oculus Rift
Created     :   11th May 2015
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
/// This is an entry-level sample, showing a minimal VR sample, 
/// in a simple environment.  Use WASD keys to move around, and cursor keys.
/// Dismiss the health and safety warning by tapping the headset, 
/// or pressing any key. 
/// It runs with DirectX11.

// Include DirectX
#include "../../OculusRoomTiny_Advanced/Common/Win32_DirectXAppUtil.h"

// Include the Oculus SDK
#include "OVR_CAPI_D3D.h"


//------------------------------------------------------------
// ovrSwapTextureSet wrapper class that also maintains the render target views
// needed for D3D11 rendering.
struct OculusTexture
{
    ovrSession               Session;
    ovrTextureSwapChain      TextureChain;
    ovrTextureSwapChain      DepthTextureChain;
    std::vector<ID3D11RenderTargetView*> TexRtv;
    std::vector<ID3D11DepthStencilView*> TexDsv;

    OculusTexture() :
        Session(nullptr),
        TextureChain(nullptr),
        DepthTextureChain(nullptr)
    {
    }

    bool Init(ovrSession session, int sizeW, int sizeH, int sampleCount, bool createDepth)
    {
        Session = session;

        // create color texture swap chain first
        {
            ovrTextureSwapChainDesc desc = {};
            desc.Type = ovrTexture_2D;
            desc.ArraySize = 1;
            desc.Width = sizeW;
            desc.Height = sizeH;
            desc.MipLevels = 1;
            desc.SampleCount = sampleCount;
            desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
            desc.MiscFlags = ovrTextureMisc_DX_Typeless | ovrTextureMisc_AutoGenerateMips;
            desc.BindFlags = ovrTextureBind_DX_RenderTarget;
            desc.StaticImage = ovrFalse;

            ovrResult result = ovr_CreateTextureSwapChainDX(session, DIRECTX.Device, &desc, &TextureChain);
            if (!OVR_SUCCESS(result))
                return false;

            int textureCount = 0;
            ovr_GetTextureSwapChainLength(Session, TextureChain, &textureCount);
            for (int i = 0; i < textureCount; ++i)
            {
                ID3D11Texture2D* tex = nullptr;
                ovr_GetTextureSwapChainBufferDX(Session, TextureChain, i, IID_PPV_ARGS(&tex));

                D3D11_RENDER_TARGET_VIEW_DESC rtvd = {};
                rtvd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                rtvd.ViewDimension = (sampleCount > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS
                                                       : D3D11_RTV_DIMENSION_TEXTURE2D;
                ID3D11RenderTargetView* rtv;
                HRESULT hr = DIRECTX.Device->CreateRenderTargetView(tex, &rtvd, &rtv);
                VALIDATE((hr == ERROR_SUCCESS), "Error creating render target view");
                TexRtv.push_back(rtv);
                tex->Release();
            }
        }

        // if requested, then create depth swap chain
        if (createDepth)
        {
            ovrTextureSwapChainDesc desc = {};
            desc.Type = ovrTexture_2D;
            desc.ArraySize = 1;
            desc.Width = sizeW;
            desc.Height = sizeH;
            desc.MipLevels = 1;
            desc.SampleCount = sampleCount;
            desc.Format = OVR_FORMAT_D32_FLOAT;
            desc.MiscFlags = ovrTextureMisc_None;
            desc.BindFlags = ovrTextureBind_DX_DepthStencil;
            desc.StaticImage = ovrFalse;

            ovrResult result = ovr_CreateTextureSwapChainDX(session, DIRECTX.Device, &desc, &DepthTextureChain);
            if (!OVR_SUCCESS(result))
                return false;

            int textureCount = 0;
            ovr_GetTextureSwapChainLength(Session, DepthTextureChain, &textureCount);
            for (int i = 0; i < textureCount; ++i)
            {
                ID3D11Texture2D* tex = nullptr;
                ovr_GetTextureSwapChainBufferDX(Session, DepthTextureChain, i, IID_PPV_ARGS(&tex));

                D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
                dsvDesc.Format = DXGI_FORMAT_D32_FLOAT;
                dsvDesc.ViewDimension = (sampleCount > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS
                                                          : D3D11_DSV_DIMENSION_TEXTURE2D;
                dsvDesc.Texture2D.MipSlice = 0;

                ID3D11DepthStencilView* dsv;
                HRESULT hr = DIRECTX.Device->CreateDepthStencilView(tex, &dsvDesc, &dsv);
                VALIDATE((hr == ERROR_SUCCESS), "Error creating depth stencil view");
                TexDsv.push_back(dsv);
                tex->Release();
            }
        }

        return true;
    }

    ~OculusTexture()
    {
        for (int i = 0; i < (int)TexRtv.size(); ++i)
        {
            Release(TexRtv[i]);
        }
        for (int i = 0; i < (int)TexDsv.size(); ++i)
        {
          Release(TexDsv[i]);
        }
        if (TextureChain)
        {
            ovr_DestroyTextureSwapChain(Session, TextureChain);
        }
        if (DepthTextureChain)
        {
            ovr_DestroyTextureSwapChain(Session, DepthTextureChain);
        }
    }

    ID3D11RenderTargetView* GetRTV()
    {
        int index = 0;
        ovr_GetTextureSwapChainCurrentIndex(Session, TextureChain, &index);
        return TexRtv[index];
    }
    ID3D11DepthStencilView* GetDSV()
    {
      int index = 0;
      ovr_GetTextureSwapChainCurrentIndex(Session, DepthTextureChain, &index);
      return TexDsv[index];
    }

    // Commit changes
    void Commit()
    {
        ovr_CommitTextureSwapChain(Session, TextureChain);
        ovr_CommitTextureSwapChain(Session, DepthTextureChain);
    }
};

// return true to retry later (e.g. after display lost)
static bool MainLoop(bool retryCreate)
{
    // Initialize these to nullptr here to handle device lost failures cleanly
    ovrMirrorTexture mirrorTexture = nullptr;
    OculusTexture  * pEyeRenderTexture[2] = { nullptr, nullptr };
    Scene          * roomScene = nullptr; 
    Camera         * mainCam = nullptr;
    ovrMirrorTextureDesc mirrorDesc = {};
    long long frameIndex = 0;
    int msaaRate = 4;

    ovrSession session;
    ovrGraphicsLuid luid;
    ovrResult result = ovr_Create(&session, &luid);
    if (!OVR_SUCCESS(result))
        return retryCreate;

    ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

    // Setup Device and Graphics
    // Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
    if (!DIRECTX.InitDevice(hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2, reinterpret_cast<LUID*>(&luid)))
        goto Done;

    // Make the eye render buffers (caution if actual size < requested due to HW limits). 
    ovrRecti         eyeRenderViewport[2];

    for (int eye = 0; eye < 2; ++eye)
    {
        ovrSizei idealSize = ovr_GetFovTextureSize(session, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], 1.0f);
        pEyeRenderTexture[eye] = new OculusTexture();
        if (!pEyeRenderTexture[eye]->Init(session, idealSize.w, idealSize.h, msaaRate, true))
        {
            if (retryCreate) goto Done;
            FATALERROR("Failed to create eye texture.");
        }
        eyeRenderViewport[eye].Pos.x = 0;
        eyeRenderViewport[eye].Pos.y = 0;
        eyeRenderViewport[eye].Size = idealSize;
        if (!pEyeRenderTexture[eye]->TextureChain || !pEyeRenderTexture[eye]->DepthTextureChain)
        {
            if (retryCreate) goto Done;
            FATALERROR("Failed to create texture.");
        }
    }

    // Create a mirror to see on the monitor.
    mirrorDesc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    mirrorDesc.Width = DIRECTX.WinSizeW;
    mirrorDesc.Height = DIRECTX.WinSizeH;
    mirrorDesc.MirrorOptions = ovrMirrorOption_Default;
    result = ovr_CreateMirrorTextureWithOptionsDX(session, DIRECTX.Device, &mirrorDesc, &mirrorTexture);
    
    if (!OVR_SUCCESS(result))
    {
        if (retryCreate) goto Done;
        FATALERROR("Failed to create mirror texture.");
    }

    // Create the room model
    roomScene = new Scene(false);

    // Create camera
    mainCam = new Camera(XMVectorSet(0.0f, 0.0f, 5.0f, 0), XMQuaternionIdentity());

    // FloorLevel will give tracking poses where the floor height is 0
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
    
    // Main loop
    while (DIRECTX.HandleMessages())
    {
        ovrSessionStatus sessionStatus;
        result = ovr_GetSessionStatus(session, &sessionStatus);
        if(OVR_FAILURE(result))
          FATALERROR("Connection failed.");

        if (sessionStatus.ShouldQuit)
        {
            // Because the application is requested to quit, should not request retry
            retryCreate = false;
            break;
        }
        if (sessionStatus.ShouldRecenter)
            ovr_RecenterTrackingOrigin(session);

        if (sessionStatus.IsVisible)
        {
            XMVECTOR forward = XMVector3Rotate(XMVectorSet(0, 0, -0.05f, 0), mainCam->Rot);
            XMVECTOR right   = XMVector3Rotate(XMVectorSet(0.05f, 0, 0, 0),  mainCam->Rot);
            if (DIRECTX.Key['W'] || DIRECTX.Key[VK_UP])      mainCam->Pos = XMVectorAdd(mainCam->Pos, forward);
            if (DIRECTX.Key['S'] || DIRECTX.Key[VK_DOWN])    mainCam->Pos = XMVectorSubtract(mainCam->Pos, forward);
            if (DIRECTX.Key['D'])                            mainCam->Pos = XMVectorAdd(mainCam->Pos, right);
            if (DIRECTX.Key['A'])                            mainCam->Pos = XMVectorSubtract(mainCam->Pos, right);
            static float Yaw = 0;
            if (DIRECTX.Key[VK_LEFT])  mainCam->Rot = XMQuaternionRotationRollPitchYaw(0, Yaw += 0.02f, 0);
            if (DIRECTX.Key[VK_RIGHT]) mainCam->Rot = XMQuaternionRotationRollPitchYaw(0, Yaw -= 0.02f, 0);

            // Animate the cube
            static float cubePositionClock = 0;
            if (sessionStatus.HasInputFocus) // Pause the application if we are not supposed to have input.
                roomScene->Models[0]->Pos = XMFLOAT3(9 * sin(cubePositionClock), 3, 9 * cos(cubePositionClock += 0.015f));
            
            // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. HmdToEyePose) may change at runtime.
            ovrEyeRenderDesc eyeRenderDesc[2];
            eyeRenderDesc[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
            eyeRenderDesc[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

            // Get both eye poses simultaneously, with IPD offset already included. 
            ovrPosef EyeRenderPose[2];
            ovrPosef HmdToEyePose[2] = { eyeRenderDesc[0].HmdToEyePose,
                                         eyeRenderDesc[1].HmdToEyePose};

            double sensorSampleTime;    // sensorSampleTime is fed into the layer later
            ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, EyeRenderPose, &sensorSampleTime);

            ovrTimewarpProjectionDesc posTimewarpProjectionDesc = {};

            // Render Scene to Eye Buffers
            for (int eye = 0; eye < 2; ++eye)
            {
                // Clear and set up rendertarget
                DIRECTX.SetAndClearRenderTarget(pEyeRenderTexture[eye]->GetRTV(), pEyeRenderTexture[eye]->GetDSV());
                DIRECTX.SetViewport((float)eyeRenderViewport[eye].Pos.x, (float)eyeRenderViewport[eye].Pos.y,
                                    (float)eyeRenderViewport[eye].Size.w, (float)eyeRenderViewport[eye].Size.h);

                //Get the pose information in XM format
                XMVECTOR eyeQuat = XMVectorSet(EyeRenderPose[eye].Orientation.x, EyeRenderPose[eye].Orientation.y,
                                               EyeRenderPose[eye].Orientation.z, EyeRenderPose[eye].Orientation.w);
                XMVECTOR eyePos = XMVectorSet(EyeRenderPose[eye].Position.x, EyeRenderPose[eye].Position.y, EyeRenderPose[eye].Position.z, 0);

                // Get view and projection matrices for the Rift camera
                XMVECTOR CombinedPos = XMVectorAdd(mainCam->Pos, XMVector3Rotate(eyePos, mainCam->Rot));
                Camera finalCam(CombinedPos, XMQuaternionMultiply(eyeQuat,mainCam->Rot));
                XMMATRIX view = finalCam.GetViewMatrix();
                ovrMatrix4f p = ovrMatrix4f_Projection(eyeRenderDesc[eye].Fov, 0.2f, 1000.0f, ovrProjection_None);
                posTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(p, ovrProjection_None);
                XMMATRIX proj = XMMatrixSet(p.M[0][0], p.M[1][0], p.M[2][0], p.M[3][0],
                                            p.M[0][1], p.M[1][1], p.M[2][1], p.M[3][1],
                                            p.M[0][2], p.M[1][2], p.M[2][2], p.M[3][2],
                                            p.M[0][3], p.M[1][3], p.M[2][3], p.M[3][3]);
                XMMATRIX prod = XMMatrixMultiply(view, proj);
                roomScene->Render(&prod, 1, 1, 1, 1, true);

                // Commit rendering to the swap chain
                pEyeRenderTexture[eye]->Commit();
            }

            // Initialize our single full screen Fov layer.
            ovrLayerEyeFovDepth ld = {};
            ld.Header.Type = ovrLayerType_EyeFovDepth;
            ld.Header.Flags = 0;
            ld.ProjectionDesc = posTimewarpProjectionDesc;
            ld.SensorSampleTime = sensorSampleTime;

            for (int eye = 0; eye < 2; ++eye)
            {
                ld.ColorTexture[eye] = pEyeRenderTexture[eye]->TextureChain;
                ld.DepthTexture[eye] = pEyeRenderTexture[eye]->DepthTextureChain;
                ld.Viewport[eye] = eyeRenderViewport[eye];
                ld.Fov[eye] = hmdDesc.DefaultEyeFov[eye];
                ld.RenderPose[eye] = EyeRenderPose[eye];
            }
            
            ovrLayerHeader* layers = &ld.Header;
            result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
            // exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
            if (!OVR_SUCCESS(result))
                goto Done;

            frameIndex++;
        }

        // Render mirror
        ID3D11Texture2D* tex = nullptr;
        ovr_GetMirrorTextureBufferDX(session, mirrorTexture, IID_PPV_ARGS(&tex));

        DIRECTX.Context->CopyResource(DIRECTX.BackBuffer, tex);
        tex->Release();
        DIRECTX.SwapChain->Present(0, 0);

    }

    // Release resources
Done:
    delete mainCam;
    delete roomScene;
    if (mirrorTexture)
        ovr_DestroyMirrorTexture(session, mirrorTexture);
    for (int eye = 0; eye < 2; ++eye)
    {
        delete pEyeRenderTexture[eye];
    }
    DIRECTX.ReleaseDevice();
    ovr_Destroy(session);

    // Retry on ovrError_DisplayLost
    return retryCreate || (result == ovrError_DisplayLost);
}

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
    // Initializes LibOVR, and the Rift
	ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
	ovrResult result = ovr_Initialize(&initParams);
    VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

    VALIDATE(DIRECTX.InitWindow(hinst, L"Oculus Room Tiny (DX11)"), "Failed to open window.");

    DIRECTX.Run(MainLoop);

    ovr_Shutdown();
    return(0);
}
