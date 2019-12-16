
#include "../Header/libextra.h"

//--------------------------------------------------
DataBuffer::DataBuffer(D3D11_BIND_FLAG use, const void* buffer, size_t size) : Size(size)
{
    D3D11_BUFFER_DESC desc;   memset(&desc, 0, sizeof(desc));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.BindFlags = use;
    desc.ByteWidth = (unsigned)size;
    D3D11_SUBRESOURCE_DATA sr;
    sr.pSysMem = buffer;
    sr.SysMemPitch = sr.SysMemSlicePitch = 0;
    DIRECTX.Device->CreateBuffer(&desc, buffer ? &sr : NULL, &D3DBuffer);
}

//--------------------------------------------------
DataBuffer::~DataBuffer()
{
    Release(D3DBuffer);
}
