/*****************************************************************************
Filename    :   main.cpp
Content     :   Simple minimal VR demo for Vulkan
Created     :   02/09/2017
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
*****************************************************************************/

// Requirements:
// Vulkan SDK 1.0.54.0 with VULKAN_SDK set in the environment
// NVIDIA: driver 382.83 from https://developer.nvidia.com/vulkan-driver
// AMD: driver 17.4.4 from http://support.amd.com/en-us/download
// Note: Debug layers are not working for AMD, use the Release build.

#define ORT_NAME "OculusRoomTiny (Vk)"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include "../../OculusRoomTiny_Advanced/Common/Win32_VulkanAppUtil.h"

// Include the Oculus SDK
// Note we include vulkan.h before OVR_CAPI_Vk.h to get real type definitions instead of placeholders
#include "OVR_CAPI_Vk.h"

// Log Oculus SDK errors
static void LogOculusError(std::string msg)
{
    ovrErrorInfo ei{};
    ovr_GetLastErrorInfo(&ei);
    Debug.Log(msg + " failed: " + ei.ErrorString);
}

// VkImage + framebuffer wrapper
class RenderTexture: public VulkanObject
{
public:
    VkImage         image;
    VkImageView     colorView;
    VkImageView     depthView;
    Framebuffer     fb;

    RenderTexture() :
        image(VK_NULL_HANDLE),
        colorView(VK_NULL_HANDLE),
        depthView(VK_NULL_HANDLE),
        fb()
    {
    }

    bool Create(VkImage anImage, VkImage depth, VkFormat format, VkExtent2D size, RenderPass& renderPass)
    {
        image = anImage;

        // Create color image view
        VkImageViewCreateInfo colorViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        colorViewInfo.image = image;
        colorViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        colorViewInfo.format = format;
        colorViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        colorViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        colorViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        colorViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        colorViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorViewInfo.subresourceRange.baseMipLevel = 0;
        colorViewInfo.subresourceRange.levelCount = 1;
        colorViewInfo.subresourceRange.baseArrayLayer = 0;
        colorViewInfo.subresourceRange.layerCount = 1;
        CHECKVK(vkCreateImageView(Platform.device, &colorViewInfo, nullptr, &colorView));

        // Create depth image view
        VkImageViewCreateInfo depthViewInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        depthViewInfo.image = depth;
        depthViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        depthViewInfo.format = VK_FORMAT_D32_SFLOAT;
        depthViewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
        depthViewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
        depthViewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
        depthViewInfo.components.a = VK_COMPONENT_SWIZZLE_A;
        depthViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthViewInfo.subresourceRange.baseMipLevel = 0;
        depthViewInfo.subresourceRange.levelCount = 1;
        depthViewInfo.subresourceRange.baseArrayLayer = 0;
        depthViewInfo.subresourceRange.layerCount = 1;
        depthViewInfo.flags = 0;
        CHECKVK(vkCreateImageView(Platform.device, &depthViewInfo, nullptr, &depthView));
		
        CHECK(fb.Create(size, renderPass, colorView, depthView));

        return true;
    }

    void Release()
    {
        fb.Release();
        if (colorView)
        {
            vkDestroyImageView(Platform.device, colorView, nullptr);
        }

        if (depthView)
        {
            vkDestroyImageView(Platform.device, depthView, nullptr);
        }

        // Note we don't own image, it will get destroyed when ovr_DestroyTextureSwapChain is called
        image = VK_NULL_HANDLE;
        colorView = VK_NULL_HANDLE;
        depthView = VK_NULL_HANDLE;
    }
};

// ovrSwapTextureSet wrapper class for Vulkan rendering
class TextureSwapChain: public VulkanObject
{
public:
    ovrSession                  session;
    VkExtent2D                  size;
    ovrTextureSwapChain         textureChain;
    ovrTextureSwapChain         depthChain;
    std::vector<RenderTexture>  texElements;

    TextureSwapChain() :
        session(nullptr),
        size{},
        textureChain(nullptr),
        depthChain(nullptr),
        texElements()
    {
    }

    bool Create(ovrSession aSession, VkExtent2D aSize, RenderPass& renderPass )
    {
        session = aSession;
        size = aSize;

        // depth
        ovrTextureSwapChainDesc depthDesc = {};
        depthDesc.Type = ovrTexture_2D;
        depthDesc.ArraySize = 1;
        depthDesc.Format = OVR_FORMAT_D32_FLOAT;
        depthDesc.Width = (int)size.width;
        depthDesc.Height = (int)size.height;
        depthDesc.MipLevels = 1;
        depthDesc.SampleCount = 1;
        depthDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
        depthDesc.BindFlags = ovrTextureBind_DX_DepthStencil;
        depthDesc.StaticImage = ovrFalse;
        CHECKOVR(ovr_CreateTextureSwapChainVk(session, Platform.device, &depthDesc, &depthChain));

        // color
        ovrTextureSwapChainDesc colorDesc = {};
        colorDesc.Type = ovrTexture_2D;
        colorDesc.ArraySize = 1;
        colorDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
        colorDesc.Width = (int)size.width;
        colorDesc.Height = (int)size.height;
        colorDesc.MipLevels = 1;
        colorDesc.SampleCount = 1;
        colorDesc.MiscFlags = ovrTextureMisc_DX_Typeless;
        colorDesc.BindFlags = ovrTextureBind_DX_RenderTarget;
        colorDesc.StaticImage = ovrFalse;
        CHECKOVR(ovr_CreateTextureSwapChainVk(session, Platform.device, &colorDesc, &textureChain));
        
        int textureCount = 0;
        CHECKOVR(ovr_GetTextureSwapChainLength(session, textureChain, &textureCount));

        int depthCount = 0;
        CHECKOVR(ovr_GetTextureSwapChainLength(session, textureChain, &depthCount));

        CHECK(textureCount == depthCount);

        texElements.reserve(textureCount);
        for (int i = 0; i < textureCount; ++i)
        {
            VkImage colorImage;
            CHECKOVR(ovr_GetTextureSwapChainBufferVk(session, textureChain, i, &colorImage));

            VkImage depthImage;
            CHECKOVR(ovr_GetTextureSwapChainBufferVk(session, depthChain, i, &depthImage));

            texElements.emplace_back(RenderTexture());
            CHECK(texElements.back().Create(colorImage, depthImage, VK_FORMAT_B8G8R8A8_SRGB, size, renderPass));
        }

        return true;
    }

    const Framebuffer& GetFramebuffer()
    {
        int index = 0;
        ovr_GetTextureSwapChainCurrentIndex(session, textureChain, &index);
        return texElements[index].fb;
    }

    Recti GetViewport()
    {
        return Recti(0, 0, size.width, size.height);
    }

    // Commit changes
    void Commit()
    {
        ovr_CommitTextureSwapChain(session, textureChain);
        ovr_CommitTextureSwapChain(session, depthChain);
	}

    void Release()
    {
        if (Platform.device)
        {
            for (auto& te: texElements)
            {
                te.Release();
            }
        }
        if (session)
        {
            if (textureChain)
            {
                ovr_DestroyTextureSwapChain(session, textureChain);
                textureChain = nullptr;
            }

            if (depthChain)
            {
                ovr_DestroyTextureSwapChain(session, depthChain);
                depthChain = nullptr;
            }
        }

        texElements.clear();
        session = nullptr;
    }
};

// ovrMirrorTexture wrapper for rendering the mirror window
class MirrorTexture: public VulkanObject
{
public:
    ovrSession                  session;
    ovrMirrorTexture            mirrorTexture;
    VkImage                     image;

    MirrorTexture() :
        session(nullptr),
        mirrorTexture(nullptr),
        image(VK_NULL_HANDLE)
    {
    }

    bool Create(ovrSession aSession, ovrSizei windowSize)
    {
        session = aSession;

        ovrMirrorTextureDesc mirrorDesc = {};
        // This must match Platform.sc.format
        mirrorDesc.Format = OVR_FORMAT_B8G8R8A8_UNORM_SRGB;
        mirrorDesc.Width = windowSize.w;
        mirrorDesc.Height = windowSize.h;
        CHECKOVR(ovr_CreateMirrorTextureWithOptionsVk(session, Platform.device, &mirrorDesc, &mirrorTexture));

        CHECKOVR(ovr_GetMirrorTextureBufferVk(session, mirrorTexture, &image));

        // Switch the mirror buffer from UNDEFINED -> TRANSFER_SRC_OPTIMAL
        VkImageMemoryBarrier imgBarrier{ VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        imgBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imgBarrier.srcAccessMask = 0;
        imgBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imgBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imgBarrier.image = image;
        imgBarrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };
        vkCmdPipelineBarrier(Platform.CurrentDrawCmd().buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &imgBarrier);

        return true;
    }

    void Release()
    {
        if (mirrorTexture) ovr_DestroyMirrorTexture(session, mirrorTexture);
        // Note we don't own image, it will get destroyed when ovr_DestroyTextureSwapChain is called
        image = VK_NULL_HANDLE;
        mirrorTexture = nullptr;
        session = nullptr;
    }
};

// return true to retry later (e.g. after display lost)
static bool MainLoop(bool retryCreate)
{
    #define Abort(err) \
    do { \
        retryCreate = false; \
        result = (err); \
        goto Done; \
    } while (0)

    // Per-eye render state
    class EyeState: public VulkanObject
    {
    public:
        ovrSizei                size;
        RenderPass              rp;
        Pipeline                pipe;
        TextureSwapChain        tex;

        EyeState() :
            size(),
            rp(),
            pipe(),
            tex()
        {
        }

        bool Create(ovrSession session, ovrSizei eyeSize, const PipelineLayout& layout, const ShaderProgram& sp, const VertexBuffer<Vertex>& vb)
        {
            size = eyeSize;
            VkExtent2D vkSize = { (uint32_t)size.w, (uint32_t)size.h };
            // Note: Format is hard-coded to VK_FORMAT_B8G8R8A8_SRGB since it is directly presentable on both Nvidia and AMD
            CHECK(rp.Create(VK_FORMAT_B8G8R8A8_SRGB, VK_FORMAT_D32_SFLOAT));
            CHECK(pipe.Create(vkSize, layout, rp, sp, vb));
            CHECK(tex.Create(session, vkSize, rp));
            return true;
        }

        void Release()
        {
            tex.Release();
            pipe.Release();
            rp.Release();
        }
    };
    
    EyeState                    perEye[ovrEye_Count];

    MirrorTexture               mirrorTexture;

    Scene                       roomScene; 
    bool                        isVisible = true;
    long long                   frameIndex = 0;

    ovrSession                  session;
    ovrGraphicsLuid             luid;

    PipelineLayout              layout;
    ShaderProgram               sp;
    VertexBuffer<Vertex>        vb;

    ovrResult result = ovr_Create(&session, &luid);
    if (!OVR_SUCCESS(result))
        return retryCreate;

    ovrHmdDesc hmdDesc = ovr_GetHmdDesc(session);

    // Setup Window and Graphics
    // Note: the mirror window can be any size, for this sample we use 1/2 the HMD resolution
    ovrSizei windowSize = { hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2 };

    struct Vulkan::VrApi vrApi =
    {
        // GetInstanceExtensionsVk
        [luid](char* extensionNames, uint32_t* extensionNamesSize)->bool
        {
            auto ret = ovr_GetInstanceExtensionsVk(luid, extensionNames, extensionNamesSize);
            if (!OVR_SUCCESS(ret))
            {
                LogOculusError("ovr_GetInstanceExtensionsVk");
                return false;
            }
            return true;
        },

        // GetSessionPhysicalDeviceVk
        [session, luid](VkInstance instance, VkPhysicalDevice* physicalDevice)->bool
        {
            auto ret = ovr_GetSessionPhysicalDeviceVk(session, luid, instance, physicalDevice);
            if (!OVR_SUCCESS(ret))
            {
                LogOculusError("ovr_GetSessionPhysicalDeviceVk");
                return false;
            }
            return true;
        },

        // GetDeviceExtensionsVk
        [luid](char* extensionNames, uint32_t* extensionNamesSize)->bool
        {
            auto ret = ovr_GetDeviceExtensionsVk(luid, extensionNames, extensionNamesSize);
            if (!OVR_SUCCESS(ret))
            {
                LogOculusError("ovr_GetDeviceExtensionsVk");
                return false;
            }
            return true;
        }
    };

    // Initialize the Vulkan renderer
    if (!Platform.InitDevice("OculusRoomTiny (Vk)", windowSize.w, windowSize.h, vrApi))
    {
        Abort(ovrError_DeviceUnavailable);
    }

    // Begin the initialization command buffer, note that various initialization steps need a valid command buffer to operate correctly
    if (!Platform.CurrentDrawCmd().Begin())
    {
        Debug.Log("Begin command buffer failed");
        Abort(ovrError_InvalidOperation);
    }

    // Create mirror texture
    if (!mirrorTexture.Create(session, windowSize))
    {
        if (retryCreate) goto Done;
        VALIDATE(false, "Failed to create mirror texture.");
    }

    // FloorLevel will give tracking poses where the floor height is 0
    ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);

    if (!sp.Create("vert", "frag"))
    {
        Debug.Log("Failed to create shader program");
        Abort(ovrError_InvalidOperation);
    }

    vb.Attr({ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 0 })
      .Attr({ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, 16 })
      .Attr({ 2, 0, VK_FORMAT_R32G32_SFLOAT, 32 });

    if (!layout.Create())
    {
        Debug.Log("Failed to create pipeline layout");
        Abort(ovrError_InvalidOperation);
    }

    // Make a scene
    roomScene.Create(layout, vb);

    // Make per-eye rendering state
    for (auto eye: { ovrEye_Left, ovrEye_Right })
    {
        if (!perEye[eye].Create(session, ovr_GetFovTextureSize(session, eye, hmdDesc.DefaultEyeFov[eye], 1), layout, sp, vb))
        {
            Debug.Log("Failed to create render state for eye " + std::to_string(eye));
            Abort(ovrError_InvalidOperation);
        }
    }

    // Get swapchain images ready for blitting (use drawCmd instead of xferCmd to keep things simple)
    Platform.sc.Prepare(Platform.CurrentDrawCmd().buf);

    // Perform all init-time commands
    if (!(Platform.CurrentDrawCmd().End() && Platform.CurrentDrawCmd().Exec(Platform.drawQueue) && Platform.CurrentDrawCmd().Wait()))
    {
        Debug.Log("Executing initial command buffer failed");
        Abort(ovrError_InvalidOperation);
    }
    Platform.CurrentDrawCmd().Reset();

    // Let the compositor know which queue to synchronize with
    ovr_SetSynchronizationQueueVk(session, Platform.drawQueue);

    Debug.Log("Main loop...");

    // Main loop
    while (Platform.HandleMessages())
    {
        ovrSessionStatus sessionStatus;
        ovr_GetSessionStatus(session, &sessionStatus);

        // Keyboard inputs to adjust player orientation
        static float yaw(3.141592f); // I like pie
        if (Platform.Key[VK_LEFT])  yaw += 0.02f;
        if (Platform.Key[VK_RIGHT]) yaw -= 0.02f;

        // Keyboard inputs to adjust player position
        static Vector3f playerPos(0.0f, 0.0f, -5.0f);
        if (Platform.Key['W'] || Platform.Key[VK_UP])   playerPos += Matrix4f::RotationY(yaw).Transform(Vector3f( 0.00f, 0, -0.05f));
        if (Platform.Key['S'] || Platform.Key[VK_DOWN]) playerPos += Matrix4f::RotationY(yaw).Transform(Vector3f( 0.00f, 0, +0.05f));
        if (Platform.Key['D'])                          playerPos += Matrix4f::RotationY(yaw).Transform(Vector3f(+0.05f, 0,  0.00f));
        if (Platform.Key['A'])                          playerPos += Matrix4f::RotationY(yaw).Transform(Vector3f(-0.05f, 0,  0.00f));

        Matrix4f rollPitchYaw = Matrix4f::RotationY(yaw);
        
        // Animate the cube
        static float cubeClock = 0;
        // Pause the application unless we have input focus.
        if (sessionStatus.HasInputFocus)
        {
            roomScene.models[0].pos = Vector3f(9 * sinf(cubeClock), 3, 9 * cosf(cubeClock += 0.015f));
            if (cubeClock >= 2.0f * float(M_PI))
                cubeClock -= 2.0f * float(M_PI);
        }

        // Call ovr_GetRenderDesc each frame to get the ovrEyeRenderDesc, as the returned values (e.g. hmdToEyePose) may change at runtime.
        ovrEyeRenderDesc eyeRenderDesc[ovrEye_Count];
        for (auto eye: { ovrEye_Left, ovrEye_Right })
            eyeRenderDesc[eye]  = ovr_GetRenderDesc(session, eye,  hmdDesc.DefaultEyeFov[eye]);

        // Get eye poses, feeding in correct IPD offset
        ovrPosef HmdToEyePose[ovrEye_Count] = { eyeRenderDesc[ovrEye_Left].HmdToEyePose,
                                                eyeRenderDesc[ovrEye_Right].HmdToEyePose};
        ovrPosef eyeRenderPose[ovrEye_Count];
        double sensorSampleTime; // sensorSampleTime is fed into ovr_SubmitFrame later
        ovr_GetEyePoses(session, frameIndex, ovrTrue, HmdToEyePose, eyeRenderPose, &sensorSampleTime);

        ovrTimewarpProjectionDesc posTimewarpProjectionDesc = {};

	if (isVisible)
        {
            Platform.NextDrawCmd();
            auto& cmd = Platform.CurrentDrawCmd();
            cmd.Reset();
            cmd.Begin();

            for (auto eye: { ovrEye_Left, ovrEye_Right })
            {
                // Switch to eye render target
                static std::array<VkClearValue, 2> clearValues;
                clearValues[0].color.float32[0] = 0.0f;
                clearValues[0].color.float32[1] = 0.0f;
                clearValues[0].color.float32[2] = 0.0f;
                clearValues[0].color.float32[3] = 1.0f;
                clearValues[1].depthStencil.depth = 1.0f;
                clearValues[1].depthStencil.stencil = 0;
                VkRenderPassBeginInfo rpBegin = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
                rpBegin.renderPass = perEye[eye].rp.pass;
                rpBegin.framebuffer = perEye[eye].tex.GetFramebuffer().fb;
                rpBegin.renderArea = { { 0, 0 }, perEye[eye].tex.size };
                rpBegin.clearValueCount = (uint32_t)clearValues.size();
                rpBegin.pClearValues = clearValues.data();

                vkCmdBeginRenderPass(cmd.buf, &rpBegin, VK_SUBPASS_CONTENTS_INLINE);

                vkCmdBindPipeline(cmd.buf, VK_PIPELINE_BIND_POINT_GRAPHICS, perEye[eye].pipe.pipe);

                // Get view and projection matrices
                Matrix4f finalRollPitchYaw = rollPitchYaw * Matrix4f(eyeRenderPose[eye].Orientation);
                Vector3f finalUp = finalRollPitchYaw.Transform(Vector3f(0, 1, 0));
                Vector3f finalForward = finalRollPitchYaw.Transform(Vector3f(0, 0, -1));
                Vector3f shiftedEyePos = playerPos + rollPitchYaw.Transform(eyeRenderPose[eye].Position);

                Matrix4f view = Matrix4f::LookAtRH(shiftedEyePos, shiftedEyePos + finalForward, finalUp);
                Matrix4f proj = Matrix4f::Scaling(1.0f, -1.0f, 1.0f) * ovrMatrix4f_Projection(hmdDesc.DefaultEyeFov[eye], 0.2f, 1000.0f, ovrProjection_None);
                posTimewarpProjectionDesc = ovrTimewarpProjectionDesc_FromProjection(proj, ovrProjection_None);

                // Render world
                roomScene.Render(view, proj, layout, vb);

                vkCmdEndRenderPass(cmd.buf);
            }

            cmd.End();
            cmd.Exec(Platform.drawQueue);

            // Commit changes to the textures so they get picked up by the compositor
            for (auto eye: { ovrEye_Left, ovrEye_Right })
            {
                perEye[eye].tex.Commit();
	    }
        }
	else // Sleep to avoid spinning on mirror updates while HMD is doffed
	{
            ::Sleep(10);
	}

        // Submit rendered eyes as an EyeFovDepth layer
        ovrLayerEyeFovDepth ld = {};
        ld.Header.Type  = ovrLayerType_EyeFovDepth;
        ld.Header.Flags = 0;
        ld.ProjectionDesc = posTimewarpProjectionDesc;
        ld.SensorSampleTime = sensorSampleTime;

        for (auto eye: { ovrEye_Left, ovrEye_Right })
        {
            ld.ColorTexture[eye] = perEye[eye].tex.textureChain;
            ld.DepthTexture[eye] = perEye[eye].tex.depthChain;
            ld.Viewport[eye]     = perEye[eye].tex.GetViewport();
            ld.Fov[eye]          = hmdDesc.DefaultEyeFov[eye];
            ld.RenderPose[eye]   = eyeRenderPose[eye];
        }

        ovrLayerHeader* layers = &ld.Header;
        ovrResult result = ovr_SubmitFrame(session, frameIndex, nullptr, &layers, 1);
        // Exit the rendering loop if submit returns an error, will retry on ovrError_DisplayLost
        if (!OVR_SUCCESS(result))
            goto Done;

        isVisible = (result == ovrSuccess);

        if (sessionStatus.ShouldQuit)
            goto Done;
        if (sessionStatus.ShouldRecenter)
            ovr_RecenterTrackingOrigin(session);

        // Blit mirror texture to the swapchain's back buffer
        // For now block until we have an output to render into
        // The swapchain uses VK_PRESENT_MODE_IMMEDIATE_KHR or VK_PRESENT_MODE_MAILBOX_KHR to avoid blocking eye rendering
        Platform.sc.Aquire();

        Platform.xferCmd.Reset();
        Platform.xferCmd.Begin();

        auto presentImage = Platform.sc.image[Platform.sc.renderImageIdx];

        // PRESENT_SRC_KHR -> TRANSFER_DST_OPTIMAL
        VkImageMemoryBarrier presentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        presentBarrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        presentBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; 
        presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.image = presentImage;
        presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presentBarrier.subresourceRange.baseMipLevel = 0;
        presentBarrier.subresourceRange.levelCount = 1;
        presentBarrier.subresourceRange.baseArrayLayer = 0;
        presentBarrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(Platform.xferCmd.buf, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &presentBarrier);

        //#define USE_MIRROR_BLIT
        #if defined(USE_MIRROR_BLIT)
            // Blit
            // Note this needs VK_QUEUE_GRAPHICS_BIT on xferCmd, and AMD only has one graphics queue
            // which we're already using to draw with, though in theory we could use a single queue
            // since the two command streams are single-threaded.
            VkImageBlit region = {};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.mipLevel = 0;
            region.srcSubresource.baseArrayLayer = 0;
            region.srcSubresource.layerCount = 1;
            region.srcOffsets[0] = { 0, 0, 0 };
            region.srcOffsets[1] = { windowSize.w, windowSize.h, 1 };
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.mipLevel = 0;
            region.dstSubresource.baseArrayLayer = 0;
            region.dstSubresource.layerCount = 1;
            region.dstOffsets[0] = { 0, 0, 0 };
            region.dstOffsets[1] = { windowSize.w, windowSize.h, 1 };
            vkCmdBlitImage(Platform.xferCmd.buf, mirrorTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                presentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region, VK_FILTER_LINEAR);
        #else
            // Copy using xferCmd which has VK_QUEUE_TRANSFER_BIT set and can operate asynchronously to drawCmd
            VkImageCopy region = {};
            region.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.srcSubresource.mipLevel = 0;
            region.srcSubresource.baseArrayLayer = 0;
            region.srcSubresource.layerCount = 1;
            region.srcOffset = { 0, 0, 0 };
            region.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.dstSubresource.mipLevel = 0;
            region.dstSubresource.baseArrayLayer = 0;
            region.dstSubresource.layerCount = 1;
            region.dstOffset = { 0, 0, 0 };
            region.extent = { (uint32_t)windowSize.w, (uint32_t)windowSize.h, 1 };
            vkCmdCopyImage(Platform.xferCmd.buf,
                mirrorTexture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                presentImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &region);
        #endif

        // TRANSFER_DST_OPTIMAL -> PRESENT_SRC_KHR
        presentBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
        presentBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        presentBarrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        presentBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        presentBarrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        presentBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED; 
        presentBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        presentBarrier.image = presentImage;
        presentBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        presentBarrier.subresourceRange.baseMipLevel = 0;
        presentBarrier.subresourceRange.levelCount = 1;
        presentBarrier.subresourceRange.baseArrayLayer = 0;
        presentBarrier.subresourceRange.layerCount = 1;
        vkCmdPipelineBarrier(Platform.xferCmd.buf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &presentBarrier);

        Platform.xferCmd.End();

        Platform.xferCmd.Exec(Platform.xferQueue);
        Platform.xferCmd.Wait();

        // For now just block on Aquire's fence, could use a semaphore with e.g.:
        // Platform.sc.Present(Platform.xferQueue, Platform.xferDone);
        Platform.sc.Present(Platform.xferQueue, VK_NULL_HANDLE);

        ++frameIndex;
    }

Done:
    Debug.Log("Exiting main loop...");

    roomScene.Release();

    vb.Release();
    sp.Release();
    layout.Release();

    mirrorTexture.Release();

    for (auto eye: { ovrEye_Left, ovrEye_Right })
    {
        perEye[eye].Release();
    }

    Platform.ReleaseDevice();
    ovr_Destroy(session);

    // Retry on ovrError_DisplayLost
    return retryCreate || OVR_SUCCESS(result) || (result == ovrError_DisplayLost);
}

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
    // Initializes LibOVR, and the Rift
    ovrInitParams initParams = { ovrInit_RequestVersion | ovrInit_FocusAware, OVR_MINOR_VERSION, NULL, 0, 0 };
    ovrResult result = ovr_Initialize(&initParams);
    VALIDATE(OVR_SUCCESS(result), "Failed to initialize libOVR.");

    VALIDATE(Platform.InitWindow(hinst, L"Oculus Room Tiny (Vk)"), "Failed to open window.");

    Platform.Run(MainLoop);

    ovr_Shutdown();

    return(0);
}
