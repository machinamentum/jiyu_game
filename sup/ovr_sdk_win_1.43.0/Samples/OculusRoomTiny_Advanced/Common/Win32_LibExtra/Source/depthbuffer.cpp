
#include "../Header/libextra.h"

//--------------------------------------------------
DepthBuffer::DepthBuffer(int sizeW, int sizeH, int sampleCount)
{
    DXGI_FORMAT format = DXGI_FORMAT_D32_FLOAT;
    D3D11_TEXTURE2D_DESC dsDesc;
    dsDesc.Width = sizeW;
    dsDesc.Height = sizeH;
    dsDesc.MipLevels = 1;
    dsDesc.ArraySize = 1;
    dsDesc.Format = format;
    dsDesc.SampleDesc.Count = sampleCount;
    dsDesc.SampleDesc.Quality = 0;
    dsDesc.Usage = D3D11_USAGE_DEFAULT;
    dsDesc.CPUAccessFlags = 0;
    dsDesc.MiscFlags = 0;
    dsDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    ID3D11Texture2D * Tex;
    DIRECTX.Device->CreateTexture2D(&dsDesc, NULL, &Tex);
    DIRECTX.Device->CreateDepthStencilView(Tex, NULL, &TexDsv);
}

//--------------------------------------------------
DepthBuffer::~DepthBuffer()
{
    Release(TexDsv);
}

