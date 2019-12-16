
// Includes
//===========
#include "d3d11.h"
#include <cstdint>
#include <vector>

// Libs
//======
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dinput8.lib") // For control
#pragma comment(lib, "dxguid.lib")  // For control


// Globals - need to be defined by an app
//========================================
extern struct DirectX11 DIRECTX;
extern struct Files File;  

#include "libExtraTypes.h"

//-----------------------------------------------------------
struct Camera
{
    Vector3 Pos;
    Quat4   Rot;
    float OptionalYaw; // temp
    Vector3 idealPos;
    Vector3 smoothedPos;

    Camera();
    Camera(Vector3 * pos, Quat4 * rot);
    Matrix44 GetViewMatrix();
    Matrix44 GetViewMatrixWithEyePose(Vector3 eyePos, Quat4 eyeQuat);
    Matrix44 GetViewProjMatrix(Matrix44 * projMat);
};

//-------------------------------------------------------------------
struct Control
{
    enum
    {
        XBOX_A = 240,     // We are going to override the last buttons on the Keypad 
        XBOX_B, XBOX_X, XBOX_Y,
        XBOX_DPAD_UP,   // Must remain first of the pads, and only pads after this.
        XBOX_DPAD_DOWN, XBOX_DPAD_LEFT, XBOX_DPAD_RIGHT,
        NUM_XBOX_BUTTONS,
    };

    bool LastKey[256];

    Control();
    void SaveLastKeys();
    bool KeyDown(int c);
    bool KeyPress(int c);
    bool AnyKeyDown(bool includeXBOXPads);
    void Process(void);
};

//------------------------------------------------------
struct CubeWorld
{
    enum
    {
        EMPTY,
        BLOCK,
        LIGHT,
        FLOOR,
    };
    // Dimensions are vert dims and are one more than the cube size
    int vDimX;
    int vDimY;
    int vDimZ;

    struct Element
    {
        float cubeRGB[3];
        DWORD vertCol;
        uint8_t type;
        uint8_t vertOffset[3];  
    } *data, *temp;

    // Helpers
    float ConvertUChar(uint8_t v);
    Vector3 FullOffset(uint8_t * array3);
    int Index(int x, int y, int z);
    Element * Ep(int x, int y, int z);
    void ConstrainIndices(int * x, int * y, int * z);
    CubeWorld(int argVDimX, int argVDimY, int argVDimZ);
    CubeWorld(char * filename);
    void FillTriangleSet(struct TriangleSet * t0, struct TriangleSet * tFloor, int firstCubeX, int firstCubeY, int firstCubeZ, int numX, int numY, int numZ);
    struct Model * MakeRecursiveModel(int blockSize, struct TriangleSet * useT0, struct TriangleSet * useTFloor, struct Material * mat0, struct Material * matFloor);
    void PropagateLight(int iterations);
    CubeWorld(int argVDimX, int argVDimY, int argVDimZ, struct FloatScape * fs);
    void ApplyLighting();
    void Save(char * filename);
    void Set(int x0, int y0, int z0, int numX, int numY, int numZ, int type);
};

//----------------------------------------------------------------
struct DataBuffer
{
    ID3D11Buffer * D3DBuffer;
    size_t         Size;
    DataBuffer(D3D11_BIND_FLAG use, const void* buffer, size_t size);
    ~DataBuffer();
};

//---------------------------------------------------------------------
struct Debug
{
    static void Error(char * errorMessage, ...);
    static void Assert(bool test, char * errorMessage = "No message", ...);
    static void Output(const char * fnt, ...);
};

//------------------------------------------------------------
struct DepthBuffer
{
    ID3D11DepthStencilView * TexDsv;
    DepthBuffer(int sizeW, int sizeH, int sampleCount = 1);
    ~DepthBuffer();
};

//---------------------------------------------------------------------
struct DirectX11
{
    HWND                     Window;
    bool                     Running;
    bool                     Key[256];
    int                      WinSizeW;
    int                      WinSizeH;
    ID3D11Device           * Device;
    ID3D11DeviceContext    * Context;
    IDXGISwapChain         * SwapChain;
    DepthBuffer            * MainDepthBuffer;
    ID3D11Texture2D        * BackBuffer;
    ID3D11RenderTargetView * BackBufferRT;
    // Fixed size buffer for shader constants, before copied into buffer
    static const int         UNIFORM_DATA_SIZE = 2000;
    unsigned char            UniformData[UNIFORM_DATA_SIZE];
    DataBuffer             * UniformBufferGen;
    HINSTANCE                hInstance;

    DirectX11();
    ~DirectX11();
    bool InitWindow(HINSTANCE hinst, LPCWSTR title);
    void CloseWindow();
    bool InitDevice(int vpW, int vpH, const LUID* pLuid, bool windowed = true);
    void SetAndClearRenderTarget(ID3D11RenderTargetView * rendertarget, struct DepthBuffer * depthbuffer, float R = 0, float G = 0, float B = 0, float A = 0, bool doWeClear = true);
    void SetViewport(float vpX, float vpY, float vpW, float vpH);
    bool LiveMoveOrStretchWindow(int vpX, int vpY, int vpW, int vpH);
    bool HandleMessages(void);
    void Run(bool(*MainLoop)(bool retryCreate));
    void ReleaseDevice();
};

//--------------------------------------------------
struct Files
{
    char AssetPath[100]; // This must be initialised
    Files(char * initialAssetPath);
    char * Path(char * originalName, bool insistExists); // Only use at point of opening
    bool Exists(char * pathedName);
};

//---------------------------------------------------------
struct FileParser
{
#define MAX_CHARS_IN_KEYWORD_AND_ARGUMENTS 2048
#define MAX_ARGUMENTS_PER_KEYWORD 32
#define MAX_ARGUMENT_STRING_LENGTH 256
#define MAX_KEYWORD_STRING_LENGTH 256

    struct ARGUMENT
    {
        char data_as_string[MAX_ARGUMENT_STRING_LENGTH];
        int data_as_int;
        float data_as_float;
    };
    struct KEYWORD
    {
        char keyword[MAX_KEYWORD_STRING_LENGTH];
        int num_arguments;
        ARGUMENT * argument[MAX_ARGUMENTS_PER_KEYWORD];
    };

private:
    int max_supply_arguments;
    int num_supply_arguments;
    ARGUMENT * supply_argument;// Needs allocating
    int max_keywords;
public:// This data can be read off directly
    int num_keywords;
    KEYWORD * keyword; // Needs allocating

    KEYWORD * GetKeyWord(char * name);
    int Int(char * name, int argumentNum = 0);
    float Float(char * name, int argumentNum = 0);
    char * String(char * name, int argumentNum = 0);
    FileParser(int arg_max_keywords, int arg_max_supply_arguments);
    void Fill_From_File(char * filename);
    void Debug_Output(void);
};

//--------------------------------------------------------
struct FloatScape
{
    float * a;
    int DimX, DimY, DimZ;

    int Index(int x, int y, int z);
    void Blur(int iterations);
    FloatScape(int dimX, int dimY, int dimZ, int iterations = 15, float min = 0.0f, float max = 1.0f, float topBotBias = 0);
    void Set(int x0, int y0, int z0, int numX, int numY, int numZ, float value);
    ~FloatScape();
private:
    float * t;
};

//--------------------------------------------------------------
struct Font
{
    struct Texture * texture;
    struct CharData
    {
        float u_start, u_end, width;  // Assuming height is 1.0f
    } lookupTable[256];
    enum { GapBetweenChars = 0 };

    Font(char * textureFilename, char * filename, bool isItBlocky = false);
};

//---------------------------------------------------------------------
struct Maths
{
    static float Rand01();
    static float Lerp(float a, float b, float propOfB);
    static Vector3 Lerp(Vector3 a, Vector3 b, float propOfB);
    static float Pi();
    static float RTOD();
    static DWORD ColourLerp(DWORD one, DWORD two, float propOfOne);
    static Vector3 GetEulerFromQuat(float x, float y, float z, float w);
    static Vector3 GetEulerFromQuat(Quat4 q);
    static void Limit(float * value, float min, float max);
};

//-----------------------------------------------------
struct Material
{
    struct Shader           * TheShader;
    Texture                 * Tex;
    ID3D11SamplerState      * SamplerState;
    ID3D11RasterizerState   * Rasterizer;
    ID3D11DepthStencilState * DepthState;
    ID3D11BlendState        * BlendState;

    enum { MAT_WRAP = 1, MAT_WIRE = 2, MAT_ZALWAYS = 4, MAT_NOCULL = 8, MAT_TRANS = 16, MAT_ZNOWRITE = 32, MAT_NOBLUR = 64 };
    Material(Texture * t, struct Shader * s, DWORD flags = MAT_WRAP | MAT_TRANS);
    void CreateNewSamplerState(DWORD flags);
    void CreateNewRasterizer(DWORD flags);
    void CreateNewDepthState(DWORD flags);
    void CreateNewBlendState(DWORD flags);
    ~Material();
};

//----------------------------------------------------------------------
struct Model
{
    Vector3      Pos;
    Quat4        Rot;
    Vector3      Scale;
    Material   * Mat;
    DataBuffer * VertexBuffer;
    DataBuffer * IndexBuffer;
    int          NumIndices;
    int          MaxVerts, MaxIndices;
    Model      * SubModel;

    void Init(TriangleSet * t);
    Model(TriangleSet * t, Vector3 argPos, Quat4 argRot, Material * argMat);
    Model(int maxVerts = 0, int maxIndices = 0, Vector3 argPos = Vector3(), Quat4 argRot = Quat4(), Material * argMat = nullptr);
    Model(Material * mat, float minx, float miny, float maxx, float maxy, float zDepth = 0);
    void AddSubModel(Model * subModel);
    ~Model();
    void Render(Matrix44 * projView, float R, float G, float B, float A, bool standardUniforms);
    void RenderElement(Matrix44 * projView, float R, float G, float B, float A, bool standardUniforms);
    void ReplaceTriangleSet(TriangleSet * t);
    void AddPosHierarchy(Vector3 newPos);
    void SetPosHierarchy(Vector3 newPos);
};

//----------------------------------------------------- 
template<typename T> void Release(T *&obj)
{
    if (!obj) return;
    obj->Release();
    obj = nullptr;
};

//---------------------------------------------------------------
struct Shader
{
    ID3D11VertexShader      * D3DVert;
    ID3D11PixelShader       * D3DPix;
    ID3D11InputLayout       * InputLayout;
    UINT                      VertexSize;
    Shader(char * fileName = "DefaultShader", D3D11_INPUT_ELEMENT_DESC * vertexDesc = NULL, int numVertexDesc = 3, char* vertexShader = NULL, char* pixelShader = NULL, int vertexSize = 24);
    ~Shader();
};

//----------------------------------------------------------------------
struct TextModel
{
    float OffsetX, OffsetY, OffsetZ, ScaleX, ScaleY;
    DWORD Colour;
    TriangleSet * LivePolyStore;
    Model       * LivePolyModel; // Orientate with the model, the text is always in xy plane
    Font * font;

    TextModel(Font * argFont, int maxTris, DWORD extraFlags = 0);
    float FindLength(char * text);
    TextModel * Add(int alignment0L1C2R, char * fnt, ...);
    TextModel * Offset(float x, float y, float z=0);
    TextModel * Scale(float x, float y);
    TextModel * Col(DWORD colour);
    TextModel * CR(float multiplier = 1.0f);
    void UpdateModel();
    void Render(Matrix44 * viewProjMat, float red = 1, float gre = 1, float blu = 1, float alpha = 1);
    void Reset();
};

//------------------------------------------------------------
struct Texture
{
    ID3D11Texture2D            * Tex;
    ID3D11ShaderResourceView   * TexSv;
    ID3D11RenderTargetView     * TexRtv;
    int                          SizeW, SizeH;

    enum {
        AUTO_WHITE = 1, AUTO_WALL, AUTO_FLOOR, AUTO_CEILING, AUTO_GRID, AUTO_GRID_CLEAR, AUTO_DOUBLEGRID_CLEAR, AUTO_GRID_GREENY, AUTO_CLEAR, AUTO_GREENY,    AUTO_GRADE_256
    };
    Texture();
    Texture(bool rendertarget, int sizeW, int sizeH, int autoFillData, int sampleCount, int mipLevels);
    Texture(DWORD(*colourFunc)(int, int, uint8_t *, int), int sizeW, int sizeH, uint8_t * optionalData, int mipLevels);
    ~Texture();
private:
    void FillTexture(DWORD * pix,int mipLevels);
    void AutoFillTexture(int autoFillData, int mipLevels);
    void Init(int sizeW, int sizeH, bool rendertarget, int mipLevels, int sampleCount);
};

DWORD MakeWall(int i, int j, uint8_t * unused1, int unused2);
DWORD FullWhite(int i, int j, uint8_t * unused1, int unused2);
DWORD MakeWhiteToBlack256(int i, int j, uint8_t * unused1, int unused2);
DWORD MakeBMPFullAlpha(int i, int j, uint8_t * data, int sizeW);
DWORD MakeBMPRedAlpha(int i, int j, uint8_t * data, int sizeW);
DWORD MakeBMPBlocky(int i, int j, uint8_t * data, int sizeW);
Texture *LoadBMP(char * filename, DWORD(*colFunc)(int, int, uint8_t *, int), int mipLevels);

//-----------------------------------------------------------------------
struct TriangleSet
{
    int       numVertices, numIndices, maxBuffer;
    struct Vertex    * Vertices;
    short     * Indices;
    TriangleSet(int maxTriangles = 2000);
    ~TriangleSet();
    void AddQuad(struct Vertex v0, struct Vertex v1, struct Vertex v2, struct Vertex v3);
    void AddTriangle(struct Vertex v0, struct Vertex v1, struct Vertex v2);
    void AddCylinder(float radius, float height, float segments, DWORD c);
    void SwapYandZ();
    void Scale(Vector3 scale);
    void Translate(Vector3 offset);
    
    DWORD ModifyColor(DWORD c, Vector3 pos);
    void AddSolidColorBox(float x1, float y1, float z1, float x2, float y2, float z2, DWORD c, float UVScale = 1.0f);
    void Empty(void);
    void AddCube(Vector3 n, Vector3 x, DWORD c, bool loX = 1, bool hiX = 1, bool loY = 1, bool hiY = 1, bool loZ = 1, bool hiZ = 1);
    void AddCube(Vector3 n, Vector3 x,
        DWORD nnn, DWORD xnn, DWORD nxn, DWORD xxn,
        DWORD nnx, DWORD xnx, DWORD nxx, DWORD xxx,
        bool loX = 1, bool hiX = 1, bool loY = 1, bool hiY = 1, bool loZ = 1, bool hiZ = 1);
};

// Left over from ORT, so ORT just slots in
#define VALIDATE(x, msg) if (!(x)) { MessageBoxA(NULL, (msg), "OculusRoomTiny", MB_ICONERROR | MB_OK); exit(-1); }


