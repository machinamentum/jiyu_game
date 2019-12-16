
#include "../Header/libextra.h"

//-------------------------------------------
void Model::Init(TriangleSet * t)
{
    NumIndices = t->numIndices;
    VertexBuffer = new DataBuffer(D3D11_BIND_VERTEX_BUFFER, &t->Vertices[0], t->numVertices * sizeof(Vertex));
    IndexBuffer = new DataBuffer(D3D11_BIND_INDEX_BUFFER, &t->Indices[0], t->numIndices * sizeof(short));
    SubModel = 0;
    MaxVerts = t->numVertices;
    MaxIndices = t->numIndices;
    Scale = Vector3(1, 1, 1);
}

//---------------------------------------------------
Model::Model(TriangleSet * t, Vector3 argPos, Quat4 argRot, Material * argMat) :
    Pos(argPos),
    Rot(argRot),
    Mat(argMat)
{
    Init(t);
}

// Its an empty container for things like live poly, and for genuinely empty
Model::Model(int maxVerts, int maxIndices, Vector3 argPos, Quat4 argRot, Material * argMat) 
{
    MaxVerts = maxVerts; 
    MaxIndices = maxIndices; 
    Pos = argPos;
    Rot = argRot;
    Mat = argMat;  
    Scale = Vector3(1, 1, 1);
    NumIndices = 0;
    SubModel = 0;
    VertexBuffer = new DataBuffer(D3D11_BIND_VERTEX_BUFFER, NULL, maxVerts * sizeof(Vertex));
    IndexBuffer = new DataBuffer(D3D11_BIND_INDEX_BUFFER, NULL, maxIndices * sizeof(short));
}

//--------------------------- For 2D scenes, for latency tester and full screen copies, etc
Model::Model(Material * mat, float minx, float miny, float maxx, float maxy, float zDepth) :
    Pos(Vector3(0, 0, 0)),
    Rot(Quat4(0, 0, 0, 1)),
    Mat(mat)
{
    TriangleSet quad;
    quad.AddQuad(Vertex(Vector3(minx, miny, zDepth), 0xffffffff, 0, 1.0f),
                 Vertex(Vector3(minx, maxy, zDepth), 0xffffffff, 0, 0.0f),
                 Vertex(Vector3(maxx, miny, zDepth), 0xffffffff, 1, 1.0f),
                 Vertex(Vector3(maxx, maxy, zDepth), 0xffffffff, 1, 0.0f));
    Init         (&quad);
}
//-----------------------------------------------------------------------------
Model::~Model()
{
    delete Mat; Mat = nullptr;
    delete VertexBuffer; VertexBuffer = nullptr;
    delete IndexBuffer; IndexBuffer = nullptr;
}

//-----------------------------------------------------------------------------
void Model::Render(Matrix44 * projView, float R, float G, float B, float A, bool standardUniforms)
{
    RenderElement(projView, R, G, B, A, standardUniforms);
    if (SubModel) SubModel->Render(projView, R, G, B, A, standardUniforms);
}
//-----------------------------------------------------------------------------
void Model::RenderElement(Matrix44 * projView, float R, float G, float B, float A, bool standardUniforms)
{
    if (NumIndices==0) return;// Don't bother - its just a dummy
    
    Matrix44 modelMat = Rot.ToRotationMatrix44() * Pos.ToTranslationMatrix44();
    Matrix44 scaleMat = Scale.ToScalingMatrix44();
    modelMat = scaleMat * modelMat;  

    Matrix44 mat = modelMat * (*projView);
    float col[] = { R, G, B, A };
    if (standardUniforms) memcpy(DIRECTX.UniformData + 0, &mat, 64); // ProjView
    if (standardUniforms) memcpy(DIRECTX.UniformData + 64, &col, 16); // MasterCol
    D3D11_MAPPED_SUBRESOURCE map;
    DIRECTX.Context->Map(DIRECTX.UniformBufferGen->D3DBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, &DIRECTX.UniformData, DIRECTX.UNIFORM_DATA_SIZE);
    DIRECTX.Context->Unmap(DIRECTX.UniformBufferGen->D3DBuffer, 0);
    DIRECTX.Context->IASetInputLayout(Mat->TheShader->InputLayout);
    DIRECTX.Context->IASetIndexBuffer(IndexBuffer->D3DBuffer, DXGI_FORMAT_R16_UINT, 0);
    UINT offset = 0;
    DIRECTX.Context->IASetVertexBuffers(0, 1, &VertexBuffer->D3DBuffer, &Mat->TheShader->VertexSize, &offset);
    DIRECTX.Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    DIRECTX.Context->VSSetShader(Mat->TheShader->D3DVert, NULL, 0);
    DIRECTX.Context->PSSetShader(Mat->TheShader->D3DPix, NULL, 0);
    DIRECTX.Context->PSSetSamplers(0, 1, &Mat->SamplerState);
    DIRECTX.Context->RSSetState(Mat->Rasterizer);
    DIRECTX.Context->OMSetDepthStencilState(Mat->DepthState, 0);
    DIRECTX.Context->OMSetBlendState(Mat->BlendState, NULL, 0xffffffff);
    DIRECTX.Context->PSSetShaderResources(0, 1, &Mat->Tex->TexSv);
    DIRECTX.Context->DrawIndexed((UINT)NumIndices, 0, 0);
}

// Must be smaller or equal----------------------------------------
void Model::ReplaceTriangleSet(TriangleSet * t) 
{
    Debug::Assert(t->numVertices <= MaxVerts, "Can't send more verts than the size of the containing model");
    Debug::Assert(t->numIndices  <= MaxIndices, "Can't send more indices than the size of the containing model");
    NumIndices = t->numIndices;

    // Verts
    D3D11_MAPPED_SUBRESOURCE map;
    DIRECTX.Context->Map(VertexBuffer->D3DBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, &t->Vertices[0], t->numVertices * sizeof(Vertex));
    DIRECTX.Context->Unmap(VertexBuffer->D3DBuffer, 0);

    // Indices
    DIRECTX.Context->Map(IndexBuffer->D3DBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &map);
    memcpy(map.pData, &t->Indices[0], t->numIndices * sizeof(short));
    DIRECTX.Context->Unmap(IndexBuffer->D3DBuffer, 0);
}

//---------------------------------------------------------------
// Effectively it puts itself as the submodel, but preserves the chain that was there.
void Model::AddSubModel(Model * extraModel)
{
    Model * currentSubModel = SubModel; // Could be zero
    // Add it to the tail of the new model
    extraModel->SubModel = currentSubModel;
    // Now we can add this new one into position
    SubModel = extraModel;
}

//--------------------------------------------
void Model::AddPosHierarchy(Vector3 newPos)
{
    Pos.x = Pos.x + newPos.x;
    Pos.y = Pos.y + newPos.y;
    Pos.z = Pos.z + newPos.z;
    if (SubModel) SubModel->AddPosHierarchy(newPos);
}

//--------------------------------------------
void Model::SetPosHierarchy(Vector3 newPos)
{
    Pos = newPos;
    if (SubModel) SubModel->SetPosHierarchy(newPos);
}

