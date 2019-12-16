
#include "../Header/libextra.h"

//-----------------------------------------------------------------------
TriangleSet::TriangleSet(int maxTriangles) : maxBuffer(3 * maxTriangles)
{
    numVertices = numIndices = 0;
    Vertices = (Vertex *)_aligned_malloc(maxBuffer *sizeof(Vertex), 16);
    Indices = (short *)  _aligned_malloc(maxBuffer *sizeof(short), 16);
}

//-----------------------------------------------------------------------
TriangleSet::~TriangleSet()
{
    _aligned_free(Vertices);
    _aligned_free(Indices);
}

//-----------------------------------------------------------------------
void TriangleSet::AddQuad(Vertex v0, Vertex v1, Vertex v2, Vertex v3)
{
    AddTriangle(v0, v1, v2);
    AddTriangle(v3, v2, v1);
}
//-----------------------------------------------------------------------
void TriangleSet::AddTriangle(Vertex v0, Vertex v1, Vertex v2)
{
    Debug::Assert(numVertices <= (maxBuffer - 3), "Insufficient triangle set");
    for (int i = 0; i < 3; i++) Indices[numIndices++] = numVertices + i;
    Vertices[numVertices++] = v0;
    Vertices[numVertices++] = v1;
    Vertices[numVertices++] = v2;
}

//-----------------------------------------------------------------------
DWORD TriangleSet::ModifyColor(DWORD c, Vector3 pos)
{
    #define GetLengthLocal(v)  (sqrt(v.x*v.x + v.y*v.y + v.z*v.z))
    float dist1 = GetLengthLocal(Vector3(pos.x - (-2), pos.y - (4), pos.z - (-2)));
    float dist2 = GetLengthLocal(Vector3(pos.x - (3),  pos.y - (4), pos.z - (-3)));
    float dist3 = GetLengthLocal(Vector3(pos.x - (-4), pos.y - (3), pos.z - (25)));
    int   bri = rand() % 160;
    float R = ((c >> 16) & 0xff) * (bri + 192.0f*(0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
    float G = ((c >> 8) & 0xff) * (bri + 192.0f*(0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
    float B = ((c >> 0) & 0xff) * (bri + 192.0f*(0.65f + 8 / dist1 + 1 / dist2 + 4 / dist3)) / 255.0f;
    return( (c & 0xff000000) + ((R>255 ? 255 : (DWORD)R) << 16) + ((G>255 ? 255 : (DWORD)G) << 8) + (B>255 ? 255 : (DWORD)B));
}

//-----------------------------------------------------------------------
void TriangleSet::AddCylinder(float radius, float height, float segments, DWORD c)
{
    for (int i = 0; i < segments; i++)
    {
        float angleGap = (360.0f / segments) / Maths::RTOD();
        float angle0 = i*angleGap;
        float angle1 = (i + 1) * angleGap;
            
        // Side panels
        Vector3 v0 = Vector3(radius * sin(angle0), +0.5f*height, radius * cos(angle0));
        Vector3 v1 = Vector3(radius * sin(angle1), +0.5f*height, radius * cos(angle1));
        Vector3 v2 = Vector3(radius * sin(angle0), -0.5f*height, radius * cos(angle0));
        Vector3 v3 = Vector3(radius * sin(angle1), -0.5f*height, radius * cos(angle1));
        AddQuad(Vertex(v0, c, 0, 0), Vertex(v1, c, 1, 0), Vertex(v2, c, 0, 1), Vertex(v3, c, 1, 1));

        // Top and bottom
        Vector3 centre0 = Vector3(0, 0.5f*height, 0);
        Vector3 centre1 = Vector3(0, -0.5f*height, 0);
        AddTriangle(Vertex(v2, c, 0, 0), Vertex(v3, c, 1, 0), Vertex(centre1, c, 0, 1));
        AddTriangle(Vertex(v0, c, 0, 0), Vertex(centre0, c, 0, 1), Vertex(v1, c, 1, 0));
    }
}

//--------------------------------------------------------
void TriangleSet::SwapYandZ()
{
    for (int i = 0; i < numVertices; i++)
    {
        float temp = Vertices[i].Pos.z;
        Vertices[i].Pos.z = Vertices[i].Pos.y;
        Vertices[i].Pos.y = temp;
    }
}

//--------------------------------------------------------
void TriangleSet::Translate(Vector3 offset)
{
    for (int i = 0; i < numVertices; i++)
    {
        Vertices[i].Pos.x += offset.x;
        Vertices[i].Pos.y += offset.y;
        Vertices[i].Pos.z += offset.z;
    }
}

//--------------------------------------------------------
void TriangleSet::Scale(Vector3 scale)
{
    for (int i = 0; i < numVertices; i++)
    {
        Vertices[i].Pos.x *= scale.x;
        Vertices[i].Pos.y *= scale.y;
        Vertices[i].Pos.z *= scale.z;
    }
}

//-----------------------------------------------------------------------
void TriangleSet::AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c, float UVScale)
{
    float sc = UVScale;
    AddQuad(Vertex(Vector3(x1, y2, z1), ModifyColor(c, Vector3(x1, y2, z1)), z1*sc, x1*sc),
            Vertex(Vector3(x2, y2, z1), ModifyColor(c, Vector3(x2, y2, z1)), z1*sc, x2*sc),
            Vertex(Vector3(x1, y2, z2), ModifyColor(c, Vector3(x1, y2, z2)), z2*sc, x1*sc),
            Vertex(Vector3(x2, y2, z2), ModifyColor(c, Vector3(x2, y2, z2)), z2*sc, x2*sc));
    AddQuad(Vertex(Vector3(x2, y1, z1), ModifyColor(c, Vector3(x2, y1, z1)), z1*sc, x2*sc),
            Vertex(Vector3(x1, y1, z1), ModifyColor(c, Vector3(x1, y1, z1)), z1*sc, x1*sc),
            Vertex(Vector3(x2, y1, z2), ModifyColor(c, Vector3(x2, y1, z2)), z2*sc, x2*sc),
            Vertex(Vector3(x1, y1, z2), ModifyColor(c, Vector3(x1, y1, z2)), z2*sc, x1*sc));
    AddQuad(Vertex(Vector3(x1, y1, z2), ModifyColor(c, Vector3(x1, y1, z2)), z2*sc, y1*sc),
            Vertex(Vector3(x1, y1, z1), ModifyColor(c, Vector3(x1, y1, z1)), z1*sc, y1*sc),
            Vertex(Vector3(x1, y2, z2), ModifyColor(c, Vector3(x1, y2, z2)), z2*sc, y2*sc),
            Vertex(Vector3(x1, y2, z1), ModifyColor(c, Vector3(x1, y2, z1)), z1*sc, y2*sc));
    AddQuad(Vertex(Vector3(x2, y1, z1), ModifyColor(c, Vector3(x2, y1, z1)), z1*sc, y1*sc),
            Vertex(Vector3(x2, y1, z2), ModifyColor(c, Vector3(x2, y1, z2)), z2*sc, y1*sc),
            Vertex(Vector3(x2, y2, z1), ModifyColor(c, Vector3(x2, y2, z1)), z1*sc, y2*sc),
            Vertex(Vector3(x2, y2, z2), ModifyColor(c, Vector3(x2, y2, z2)), z2*sc, y2*sc));
    AddQuad(Vertex(Vector3(x1, y1, z1), ModifyColor(c, Vector3(x1, y1, z1)), x1*sc, y1*sc),
            Vertex(Vector3(x2, y1, z1), ModifyColor(c, Vector3(x2, y1, z1)), x2*sc, y1*sc),
            Vertex(Vector3(x1, y2, z1), ModifyColor(c, Vector3(x1, y2, z1)), x1*sc, y2*sc),
            Vertex(Vector3(x2, y2, z1), ModifyColor(c, Vector3(x2, y2, z1)), x2*sc, y2*sc));
    AddQuad(Vertex(Vector3(x2, y1, z2), ModifyColor(c, Vector3(x2, y1, z2)), x2*sc, y1*sc),
            Vertex(Vector3(x1, y1, z2), ModifyColor(c, Vector3(x1, y1, z2)), x1*sc, y1*sc),
            Vertex(Vector3(x2, y2, z2), ModifyColor(c, Vector3(x2, y2, z2)), x2*sc, y2*sc),
            Vertex(Vector3(x1, y2, z2), ModifyColor(c, Vector3(x1, y2, z2)), x1*sc, y2*sc));
}

//-----------------------------------------------------------------------
void TriangleSet::Empty(void)
{
    numVertices = numIndices = 0;
}

//-----------------------------------------------------------------------
void TriangleSet::AddCube(Vector3 n, Vector3 x, DWORD c, bool loX, bool hiX, bool loY, bool hiY, bool loZ, bool hiZ)
{
    if (hiZ) AddQuad(Vertex(Vector3(n.x, x.y, x.z), c, 0, 0), Vertex(Vector3(x.x, x.y, x.z), c, 1, 0),
        Vertex(Vector3(n.x, n.y, x.z), c, 0, 1), Vertex(Vector3(x.x, n.y, x.z), c, 1, 1));// Near
    if (loZ) AddQuad(Vertex(Vector3(x.x, x.y, n.z), c, 0, 0), Vertex(Vector3(n.x, x.y, n.z), c, 1, 0),
        Vertex(Vector3(x.x, n.y, n.z), c, 0, 1), Vertex(Vector3(n.x, n.y, n.z), c, 1, 1));// Far
    if (loX) AddQuad(Vertex(Vector3(n.x, x.y, n.z), c, 0, 0), Vertex(Vector3(n.x, x.y, x.z), c, 1, 0),
        Vertex(Vector3(n.x, n.y, n.z), c, 0, 1), Vertex(Vector3(n.x, n.y, x.z), c, 1, 1));// Left
    if (hiX) AddQuad(Vertex(Vector3(x.x, x.y, x.z), c, 0, 0), Vertex(Vector3(x.x, x.y, n.z), c, 1, 0),
        Vertex(Vector3(x.x, n.y, x.z), c, 0, 1), Vertex(Vector3(x.x, n.y, n.z), c, 1, 1));// Right
    if (hiY) AddQuad(Vertex(Vector3(n.x, x.y, n.z), c, 0, 0), Vertex(Vector3(x.x, x.y, n.z), c, 1, 0),
        Vertex(Vector3(n.x, x.y, x.z), c, 0, 1), Vertex(Vector3(x.x, x.y, x.z), c, 1, 1));// Top
    if (loY) AddQuad(Vertex(Vector3(n.x, n.y, x.z), c, 0, 0), Vertex(Vector3(x.x, n.y, x.z), c, 1, 0),
        Vertex(Vector3(n.x, n.y, n.z), c, 0, 1), Vertex(Vector3(x.x, n.y, n.z), c, 1, 1));// Bottom
}

//-----------------------------------------------------------------------
void TriangleSet::AddCube(Vector3 n, Vector3 x,
    DWORD nnn,    DWORD xnn,    DWORD nxn,    DWORD xxn,
    DWORD nnx,    DWORD xnx,    DWORD nxx,    DWORD xxx,
       bool loX, bool hiX, bool loY, bool hiY, bool loZ, bool hiZ)
{
    if (hiZ) AddQuad(
        Vertex(Vector3(n.x, x.y, x.z), nxx, 0, 0), Vertex(Vector3(x.x, x.y, x.z), xxx, 1, 0),
        Vertex(Vector3(n.x, n.y, x.z), nnx, 0, 1), Vertex(Vector3(x.x, n.y, x.z), xnx, 1, 1));// Near
    if (loZ) AddQuad(
        Vertex(Vector3(x.x, x.y, n.z), xxn, 0, 0), Vertex(Vector3(n.x, x.y, n.z), nxn, 1, 0),
        Vertex(Vector3(x.x, n.y, n.z), xnn, 0, 1), Vertex(Vector3(n.x, n.y, n.z), nnn, 1, 1));// Far
    if (loX) AddQuad(
        Vertex(Vector3(n.x, x.y, n.z), nxn, 0, 0), Vertex(Vector3(n.x, x.y, x.z), nxx, 1, 0),
        Vertex(Vector3(n.x, n.y, n.z), nnn, 0, 1), Vertex(Vector3(n.x, n.y, x.z), nnx, 1, 1));// Left
    if (hiX) AddQuad(
        Vertex(Vector3(x.x, x.y, x.z), xxx, 0, 0), Vertex(Vector3(x.x, x.y, n.z), xxn, 1, 0),
        Vertex(Vector3(x.x, n.y, x.z), xnx, 0, 1), Vertex(Vector3(x.x, n.y, n.z), xnn, 1, 1));// Right
    if (hiY) AddQuad(
        Vertex(Vector3(n.x, x.y, n.z), nxn, 0, 0), Vertex(Vector3(x.x, x.y, n.z), xxn, 1, 0),
        Vertex(Vector3(n.x, x.y, x.z), nxx, 0, 1), Vertex(Vector3(x.x, x.y, x.z), xxx, 1, 1));// Top
    if (loY) AddQuad(
        Vertex(Vector3(n.x, n.y, x.z), nnx, 0, 0), Vertex(Vector3(x.x, n.y, x.z), xnx, 1, 0),
        Vertex(Vector3(n.x, n.y, n.z), nnn, 0, 1), Vertex(Vector3(x.x, n.y, n.z), xnn, 1, 1));// Bottom
}

