
#include "../Header/libextra.h"

//---------------------------------------------------------------------------------
Material::Material(Texture * t, Shader * s, DWORD flags) : Tex(t), TheShader(s), SamplerState(0), Rasterizer(0), DepthState(0), BlendState(0)
{
    CreateNewSamplerState(flags);
    CreateNewRasterizer(flags);
    CreateNewDepthState(flags);
    CreateNewBlendState(flags);
}

//--------------------------------------------------------------------------------
void Material::CreateNewSamplerState(DWORD flags)
{
    //if (SamplerState) (Not allowed to I don't think, but leaves a possible issue of not releasing.)
    //    delete SamplerState;
    D3D11_SAMPLER_DESC ss; memset(&ss, 0, sizeof(ss));
    ss.AddressU = ss.AddressV = ss.AddressW = flags & MAT_WRAP ? D3D11_TEXTURE_ADDRESS_WRAP : D3D11_TEXTURE_ADDRESS_BORDER;
    ss.Filter = flags & MAT_NOBLUR ? D3D11_FILTER_MIN_MAG_MIP_POINT : D3D11_FILTER_ANISOTROPIC;
    ss.MaxAnisotropy = 8;
    ss.MaxLOD = 15;
    DIRECTX.Device->CreateSamplerState(&ss, &SamplerState);
}

//--------------------------------------------------------------------------------
void Material::CreateNewRasterizer(DWORD flags)
{
    if (Rasterizer) delete Rasterizer;
    D3D11_RASTERIZER_DESC rs; memset(&rs, 0, sizeof(rs));
    rs.DepthClipEnable = true;
    rs.AntialiasedLineEnable = true;
    rs.CullMode = flags & MAT_NOCULL ? D3D11_CULL_NONE : D3D11_CULL_BACK;
    rs.FillMode = flags & MAT_WIRE ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DIRECTX.Device->CreateRasterizerState(&rs, &Rasterizer);
}

//--------------------------------------------------------------------------------
void Material::CreateNewDepthState(DWORD flags)
{
    if (DepthState) delete DepthState;
    D3D11_DEPTH_STENCIL_DESC dss;
    memset(&dss, 0, sizeof(dss));
    dss.DepthEnable = true;
    dss.DepthFunc = flags & MAT_ZALWAYS ? D3D11_COMPARISON_ALWAYS : D3D11_COMPARISON_LESS;
    dss.DepthWriteMask = flags & MAT_ZNOWRITE ? D3D11_DEPTH_WRITE_MASK_ZERO : D3D11_DEPTH_WRITE_MASK_ALL;
    DIRECTX.Device->CreateDepthStencilState(&dss, &DepthState);
}

//--------------------------------------------------------------------------------
void Material::CreateNewBlendState(DWORD flags)
{
    if (BlendState) delete BlendState;
    D3D11_BLEND_DESC bm;
    memset(&bm, 0, sizeof(bm));
    bm.RenderTarget[0].BlendEnable = flags & MAT_TRANS ? true : false;
    bm.RenderTarget[0].BlendOp = bm.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    bm.RenderTarget[0].SrcBlend = bm.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
    bm.RenderTarget[0].DestBlend = bm.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    bm.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    DIRECTX.Device->CreateBlendState(&bm, &BlendState);
}

//-------------------------------------------------------------------------------
Material::~Material()
{
    delete TheShader;
    delete Tex; Tex = nullptr;
    Release(SamplerState);
    Release(Rasterizer);
    Release(DepthState);
    Release(BlendState);
}
