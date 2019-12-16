
#include "../Header/libextra.h"

//-----------------------------------------------------------------------------
Texture::Texture() : Tex(nullptr), TexSv(nullptr), TexRtv(nullptr) {};

//----------------------------------------------------------------------------
void Texture::Init(int sizeW, int sizeH, bool rendertarget, int mipLevels, int sampleCount)
{
    SizeW = sizeW;
    SizeH = sizeH;

    D3D11_TEXTURE2D_DESC dsDesc;
    dsDesc.Width = SizeW;
    dsDesc.Height = SizeH;
    dsDesc.MipLevels = mipLevels;
    dsDesc.ArraySize = 1;
    dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    dsDesc.SampleDesc.Count = sampleCount;
    dsDesc.SampleDesc.Quality = 0;
    dsDesc.Usage = D3D11_USAGE_DEFAULT;
    dsDesc.CPUAccessFlags = 0;
    dsDesc.MiscFlags = 0;
    dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (rendertarget) dsDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;

    DIRECTX.Device->CreateTexture2D(&dsDesc, NULL, &Tex);
    DIRECTX.Device->CreateShaderResourceView(Tex, NULL, &TexSv);
    TexRtv = nullptr;
    if (rendertarget) DIRECTX.Device->CreateRenderTargetView(Tex, NULL, &TexRtv);
}

//-------------------------------------------------------------------------------------
Texture::Texture(bool rendertarget, int sizeW, int sizeH, int autoFillData, int sampleCount, int mipLevels)
{
    Init(sizeW, sizeH, rendertarget, mipLevels, sampleCount);
    if (autoFillData) AutoFillTexture(autoFillData,mipLevels);
}

// Use a function to dictate data-----------------------------------------------------
Texture::Texture(DWORD(*colourFunc)(int, int, uint8_t *, int),  int sizeW, int sizeH, uint8_t * optionalData, int mipLevels) 
{
    Init(sizeW, sizeH, false, mipLevels,1);

    DWORD * pix = new DWORD[sizeW * sizeH];
    for (int j = 0; j < sizeH; j++)
        for (int i = 0; i < sizeW; i++)
        {
            pix[j * sizeW + i] = colourFunc(i, j, optionalData, sizeW);
        }
    FillTexture(pix, mipLevels);
    delete [] pix;
}

//--------------------------------------------------------------------------
Texture::~Texture()
{
    Release(TexRtv);
    Release(TexSv);
    Release(Tex);
}

//------------------------------------------------------------------------------
void Texture::FillTexture(DWORD * pix, int mipLevels)
{
    // Make local ones, because will be reducing them
    int sizeW = SizeW; 
    int sizeH = SizeH;
    for (int level = 0; level < mipLevels; level++)
    {
        DIRECTX.Context->UpdateSubresource(Tex, level, NULL, (unsigned char *)pix, sizeW * 4, sizeH * 4);

        for (int j = 0; j < (sizeH & ~1); j += 2)
        {
            uint8_t * psrc = (uint8_t *)pix + (sizeW * j * 4);
            uint8_t * pdest = (uint8_t *)pix + (sizeW * j);
            for (int i = 0; i < sizeW >> 1; i++, psrc += 8, pdest += 4)
            {
                pdest[0] = (((int)psrc[0]) + psrc[4] + psrc[sizeW * 4 + 0] + psrc[sizeW * 4 + 4]) >> 2;
                pdest[1] = (((int)psrc[1]) + psrc[5] + psrc[sizeW * 4 + 1] + psrc[sizeW * 4 + 5]) >> 2;
                pdest[2] = (((int)psrc[2]) + psrc[6] + psrc[sizeW * 4 + 2] + psrc[sizeW * 4 + 6]) >> 2;
                pdest[3] = (((int)psrc[3]) + psrc[7] + psrc[sizeW * 4 + 3] + psrc[sizeW * 4 + 7]) >> 2;
            }
        }
        sizeW >>= 1;  sizeH >>= 1;
    }
}

//-----------------------------------------------------------------------------------------
void Texture::AutoFillTexture(int autoFillData, int mipLevels)
{
    DWORD * pix = (DWORD *)malloc(sizeof(DWORD) *  SizeW * SizeH);
    for (int j = 0; j < SizeH; j++)
    for (int i = 0; i < SizeW; i++)
    {
        DWORD * curr = &pix[j*SizeW + i];
        switch (autoFillData)
        {
        case(AUTO_WALL) : *curr = (((j / 4 & 15) == 0) || (((i / 4 & 15) == 0) && ((((i / 4 & 31) == 0) ^ ((j / 4 >> 4) & 1)) == 0))) ?
            0xff3c3c3c : 0xffb4b4b4; break;
        case(AUTO_FLOOR) : *curr = (((i >> 7) ^ (j >> 7)) & 1) ? 0xffb4b4b4 : 0xff505050; break;
        case(AUTO_CEILING) : *curr = (i / 4 == 0 || j / 4 == 0) ? 0xff505050 : 0xffb4b4b4; break;
        case(AUTO_WHITE) : *curr = 0xffffffff;              break;
        case(AUTO_GRADE_256) : *curr = 0xff000000 + i*0x010101;              break;
        case(AUTO_GRID) : *curr = (i<4) || (i>(SizeW - 5)) || (j<4) || (j>(SizeH - 5)) ? 0xffffffff : 0xff000000; break;
        case(AUTO_GRID_CLEAR) : *curr = (i<4) || (i>(SizeW - 5)) || (j<4) || (j>(SizeH - 5)) ? 0xffffffff : 0x00000000; break;
        case(AUTO_DOUBLEGRID_CLEAR) : *curr = (i<8) || (i>(SizeW - 9)) || (j<8) || (j>(SizeH - 9)) ? 0xffffffff : 0x00000000; break;
        case(AUTO_GRID_GREENY) : *curr = (i<4) || (i>(SizeW - 5)) || (j<4) || (j>(SizeH - 5)) ? 0xff00ff00 : 0x80005500; break;
        case(AUTO_CLEAR) : *curr = 0x00000000; break;
        case(AUTO_GREENY) : *curr = 0x80005500; break;
        default: *curr = 0xffffffff;              break;
        }
    }
    FillTexture(pix,mipLevels);
}

// Functions for generating textures-----------------------------------------------------------------
DWORD MakeWall(int i, int j, uint8_t * unused1, int unused2)
{
    DWORD result = ((((j / 4 & 15) == 0) || (((i / 4 & 15) == 0) && ((((i / 4 & 31) == 0) ^ ((j / 4 >> 4) & 1)) == 0))) ? 0xff3c3c3c : 0xffb4b4b4);
    return(result);
}

DWORD FullWhite(int i, int j, uint8_t * unused1, int unused2)
{
    DWORD result = 0xffffffff;
    return(result);
}

DWORD MakeWhiteToBlack256(int i, int j, uint8_t * unused1, int unused2)
{
    DWORD result = 0xff000000 + (j * 0x010101);
    return(result);
}

DWORD MakeBMPFullAlpha(int i, int j, uint8_t * data, int sizeW)
{
    int first = 3 * (sizeW*j + i);
    uint8_t alpha = 0xff;
    DWORD result = ((alpha << 24) + (data[first + 0] << 16) + (data[first + 1] << 8) + (data[first + 2])); // ABGR
    return(result);
}

DWORD MakeBMPRedAlpha(int i, int j, uint8_t * data, int sizeW)
{
    int first = 3 * (sizeW*j + i);
    uint8_t alpha = data[first + 2];
    return((alpha << 24) + (data[first + 0] << 16) + (data[first + 1] << 8) + (data[first + 2])); // ABGR
}

DWORD MakeBMPBlocky(int i, int j, uint8_t * data, int sizeW)
{
    int first = 3 * (sizeW*j + i);
    uint8_t alpha = data[first + 2];
    if (alpha < 192) return(0);
    else       return(0xffffffff);
}

// Load from a file - make sure power of 2, and can take 8 mip levels-----------------------------------------------------------------
Texture *LoadBMP(char * filename, DWORD(*colFunc)(int, int, uint8_t *, int), int mipLevels)
{
    // Open the file
    FILE * f;
    fopen_s(&f, File.Path(filename,true), "rb");
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    auto buff = new uint8_t[size];
    if (fread(buff, 1, size, f) != size)
    {
        Debug::Output("Short read of %s\n", filename);
        return nullptr;
    }

    fclose(f);

    // Get the first 6 bytes, to find length
    BITMAPFILEHEADER *header = reinterpret_cast<BITMAPFILEHEADER*>(buff);
    int lengthOfFile = header->bfSize;
    Debug::Output("Length = %d\n", lengthOfFile);
    BITMAPINFO *info = reinterpret_cast<BITMAPINFO*>(&header[1]);

    // Now get the rest of the file, to get data
    int sizeW = info->bmiHeader.biWidth;
    int sizeH = info->bmiHeader.biHeight;

    Debug::Assert(((sizeW % 4) == 0), "Textures sizes need to be divisible by 4");
    Debug::Assert(((sizeH % 4) == 0), "Textures sizes need to be divisible by 4");

    int startOfData = header->bfOffBits;
    Debug::Output("Width = %d.  Height = %d\n Start of data = %d\n", sizeW, sizeH, startOfData);

    // Make the texture itself
    auto ret = new Texture(colFunc, sizeW, sizeH, &buff[startOfData],mipLevels);

    // Done with the data
    free(buff);

    return ret;
}
    


