
#include "../Header/libextra.h"

#define COMPILE_SHADERS 0 // Switch this to 1, to recompile shaders
#if COMPILE_SHADERS 
#include "d3dcompiler.h"    // For shaders
#pragma comment(lib, "d3dcompiler.lib")

//---------------------------------------------------------
void LOCAL_TestShader(char * text, int vert0pix1)
{
    ID3DBlob * blobData;
    ID3DBlob* errors;
    HRESULT hr;
    if (vert0pix1 == 0) hr = D3DCompile(text, strlen(text), 0, 0, 0, "main", "vs_4_0", 0, 0, &blobData, &errors);
    else                hr = D3DCompile(text, strlen(text), 0, 0, 0, "main", "ps_4_0", 0, 0, &blobData, &errors);
    if (FAILED(hr))
 		Debug::Error((char *)errors->GetBufferPointer());
    blobData->Release();
}
#endif

//--------------------------------------------------------------
void LOCAL_SaveBlobToFile(char * fileName, char * suffix, char * bufferToSave, size_t size)
{
    char fullFileName[100];    sprintf_s(fullFileName,sizeof(fullFileName),"%s%s", fileName, suffix);
    FILE * file; fopen_s(&file, File.Path(fullFileName, false), "wb+");
    fwrite(&size, sizeof(size_t), 1, file);
    size_t numberWritten = fwrite(bufferToSave, 1, size, file);
    fclose(file);
}

//--------------------------------------------------------------
size_t LOCAL_LoadBlobFromFile(char * fileName, char * suffix, void * loadedBuffer)
{
    char fullFileName[100];    sprintf_s(fullFileName, sizeof(fullFileName), "%s%s", fileName, suffix);
    size_t loadedSize;
    FILE * file; fopen_s(&file, File.Path(fullFileName, true), "rb");
    fread(&loadedSize, sizeof(size_t), 1, file);
    size_t numberRead = fread(loadedBuffer, 1, loadedSize, file);
    fclose(file);
    return(loadedSize);
}

//---------------------------------------------------------------------------------
Shader::Shader(char * fileName, D3D11_INPUT_ELEMENT_DESC * vertexDesc, int numVertexDesc, char* vertexShader, char* pixelShader, int vertexSize)
{
    VertexSize = vertexSize;

    D3D11_INPUT_ELEMENT_DESC defaultVertexDesc[] = {
        { "Position", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "Color", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TexCoord", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 }, };

    // Use defaults if no shaders specified
    char* defaultVertexShaderSrc =
        "float4x4 ProjView;  float4 MasterCol;"
        "void main(in  float4 Position  : POSITION,    in  float4 Color : COLOR0, in  float2 TexCoord  : TEXCOORD0,"
        "          out float4 oPosition : SV_Position, out float4 oColor: COLOR0, out float2 oTexCoord : TEXCOORD0)"
        "{   oPosition = mul(ProjView, Position); oTexCoord = TexCoord; "
        "    oColor = MasterCol * Color; }";
    char* defaultPixelShaderSrc =
        "Texture2D Texture   : register(t0); SamplerState Linear : register(s0); "
        "float4 main(in float4 Position : SV_Position, in float4 Color: COLOR0, in float2 TexCoord : TEXCOORD0) : SV_Target"
        "{   float4 TexCol = Texture.Sample(Linear, TexCoord); "
        "    if (TexCol.a==0) clip(-1); " // If alpha = 0, don't draw
        "    return(Color * TexCol); }";

    if (!vertexDesc)   vertexDesc = defaultVertexDesc;
    if (!vertexShader) vertexShader = defaultVertexShaderSrc;
    if (!pixelShader)  pixelShader = defaultPixelShaderSrc;

    // Create vertex shader
    static char loadedBuffer[5000];
#if COMPILE_SHADERS
    ID3DBlob * blobData;
    LOCAL_TestShader(vertexShader, 0);
    D3DCompile(vertexShader, strlen(vertexShader), 0, 0, 0, "main", "vs_4_0", 0, 0, &blobData, 0);
    LOCAL_SaveBlobToFile(fileName, "(vertex)", (char *)blobData->GetBufferPointer(), blobData->GetBufferSize());
    blobData->Release();
#endif
    size_t size = LOCAL_LoadBlobFromFile(fileName, "(vertex)", loadedBuffer);
    DIRECTX.Device->CreateVertexShader(loadedBuffer, size, NULL, &D3DVert);
    DIRECTX.Device->CreateInputLayout(vertexDesc, numVertexDesc,loadedBuffer, size, &InputLayout);

    // Create pixel shader
#if COMPILE_SHADERS
    LOCAL_TestShader(pixelShader, 1);
    D3DCompile(pixelShader, strlen(pixelShader), 0, 0, 0, "main", "ps_4_0", 0, 0, &blobData, 0);
    LOCAL_SaveBlobToFile(fileName, "(pixel)", (char *)blobData->GetBufferPointer(), blobData->GetBufferSize());
    blobData->Release();
#endif
    size = LOCAL_LoadBlobFromFile(fileName,"(pixel)", loadedBuffer);
    DIRECTX.Device->CreatePixelShader(loadedBuffer, size, NULL, &D3DPix);
}

//-------------------------------------------------------------------------------
Shader::~Shader()
{
    Release(D3DVert);
    Release(D3DPix);
    Release(InputLayout);
}
