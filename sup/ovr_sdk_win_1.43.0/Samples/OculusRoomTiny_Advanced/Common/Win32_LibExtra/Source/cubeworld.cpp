#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "../Header/libextra.h"
#include <algorithm>

//---------------------------------------------------------------------
float CubeWorld::ConvertUChar(uint8_t v)
{
    return(0.025f*((float)v - 128.0f)); // Range from +/- 3.2 (is it enough)
}  

//---------------------------------------------------------------------
Vector3 CubeWorld::FullOffset(uint8_t * array3)
{
    return(Vector3(ConvertUChar(array3[0]), ConvertUChar(array3[1]), ConvertUChar(array3[2])));
}

//---------------------------------------------------------------------
int CubeWorld::Index(int x, int y, int z)
{
    return(x + (vDimX * (y + vDimY * z)));
}

//---------------------------------------------------------------------
CubeWorld::Element * CubeWorld::Ep(int x, int y, int z)
{
    return(&data[x + (vDimX * (y + vDimY * z))]);
}

//---------------------------------------------------------------------
void CubeWorld::ConstrainIndices(int * x, int * y, int * z)
{
    if (*x < 0) *x = 0;
    if (*y < 0) *y = 0;
    if (*z < 0) *z = 0;
    if (*x >(vDimX - 1)) *x = vDimX - 1;
    if (*y >(vDimY - 1)) *y = vDimY - 1;
    if (*z >(vDimZ - 1)) *z = vDimZ - 1;
}

// Allocate memory-------------------------------------------------------------------
CubeWorld::CubeWorld(int argVDimX, int argVDimY, int argVDimZ) : vDimX(argVDimX), vDimY(argVDimY), vDimZ(argVDimZ)
{
    data = (Element *)malloc(vDimX * vDimY * vDimZ * sizeof(Element));
    temp = (Element *)malloc(vDimX * vDimY * vDimZ * sizeof(Element));
}

// Loads in from a file----------------------------------------------------------------------
CubeWorld::CubeWorld(char * filename)
{
    FILE * file; fopen_s(&file, File.Path(filename,true), "r");
    int x, y, z;
    fscanf_s(file, "%d %d %d ", &x, &y, &z);
    *this = CubeWorld(x, y, z);
    fread(data, sizeof(Element), vDimX * vDimY * vDimZ, file);
    fclose(file);
}

// Fill it into triset, ready for models.----------------------------------------------------
void CubeWorld::FillTriangleSet(TriangleSet * t0,
                        TriangleSet * tFloor,
                        int firstCubeX, int firstCubeY, int firstCubeZ,
                        int numX, int numY, int numZ)
{
    for (int x = firstCubeX; x < (firstCubeX + numX); x++)
    for (int y = firstCubeY; y < (firstCubeY + numY); y++)
    for (int z = firstCubeZ; z < (firstCubeZ + numZ); z++)
    {
        if (Ep(x, y, z)->type == EMPTY) continue;
        TriangleSet * useT;
        if (Ep(x, y, z)->type == FLOOR) useT = tFloor;
        else                            useT = t0;
            
        Vector3 centre(0.5f + x, 0.5f + y, +(0.5f + z));
        bool loX = (x < 1)         || (Ep(x - 1,y,z)->type == 0) ? 1 : 0;
        bool hiX = (x > vDimX - 3) || (Ep(x + 1,y,z)->type == 0) ? 1 : 0;
        bool loY = (y < 1)         || (Ep(x,y - 1,z)->type == 0) ? 1 : 0;
        bool hiY = (y > vDimY - 3) || (Ep(x,y + 1,z)->type == 0) ? 1 : 0;
        bool loZ = (z < 1)         || (Ep(x,y,z - 1)->type == 0) ? 1 : 0;
        bool hiZ = (z > vDimZ - 3) || (Ep(x,y,z + 1)->type == 0) ? 1 : 0;
        float radius = 0.5f;
        useT->AddCube(Vector3(centre.x - radius, centre.y - radius, centre.z - radius),
            Vector3(centre.x + radius, centre.y + radius, centre.z + radius),
            Ep(x + 0, y + 0, z + 0)->vertCol,
            Ep(x + 1, y + 0, z + 0)->vertCol,
            Ep(x + 0, y + 1, z + 0)->vertCol,
            Ep(x + 1, y + 1, z + 0)->vertCol,
            Ep(x + 0, y + 0, z + 1)->vertCol,
            Ep(x + 1, y + 0, z + 1)->vertCol,
            Ep(x + 0, y + 1, z + 1)->vertCol,
            Ep(x + 1, y + 1, z + 1)->vertCol,
            loX, hiX, loY, hiY, loZ, hiZ);
    }
}

// Fill into a model------------------------------------------------------
Model * CubeWorld::MakeRecursiveModel(int blockSize, TriangleSet * useT0, TriangleSet * sweetie, Material * mat0, Material * matFloor)
{
    float debugSpread = 0.0f;// 0.1f;
    Model * m = new Model();
    Debug::Assert((((vDimX - 1) % blockSize) == 0), "Doesn't divide in X");
    Debug::Assert((((vDimY - 1) % blockSize) == 0), "Doesn't divide in Y");
    Debug::Assert((((vDimZ - 1) % blockSize) == 0), "Doesn't divide in Z");
    int tree = (vDimX - 1) / blockSize;
    int moon = (vDimY - 1) / blockSize;
    int cow = (vDimZ - 1) / blockSize;

    for (int x = 0; x < tree; x++)
    for (int y = 0; y < moon; y++)
    for (int z = 0; z < cow; z++)
    {
        useT0->Empty();
        sweetie->Empty();
        FillTriangleSet(useT0, sweetie, x*blockSize, y*blockSize, z*blockSize, blockSize, blockSize, blockSize);
        // General
        m->AddSubModel(new Model(useT0, Vector3(debugSpread*x, debugSpread*y, debugSpread*z), Quat4(), mat0));
        // Floor
        m->AddSubModel(new Model(sweetie, Vector3(debugSpread*x, debugSpread*y, debugSpread*z), Quat4(), matFloor));
    }
    return (m);
}

//---------------------------------------------------------------------
void CubeWorld::PropagateLight(int iterations)
{
    // If the current cube is empty, then it takes max of itself and 50% of its neighbours
    // If the current cube is solid, it takes max of itself and 0% of its neighbours
    float propagationFromType[3] = { 0.84f, 0.0f, 0.0f };

    for (int i = 0; i < iterations; i++)
    {
        for (int x = 0; x < (vDimX - 1); x++)
        for (int y = 0; y < (vDimY - 1); y++)
        for (int z = 1; z < (vDimZ - 1); z++)
        for (int c = 0; c<3; c++)
        {
            int ind = Index(x, y, z);
            float propagation = propagationFromType[data[ind].type];
            temp[ind].cubeRGB[c] = data[ind].cubeRGB[c];
            if (x > 0)           temp[ind].cubeRGB[c] = std::max(temp[ind].cubeRGB[c], propagation * data[Index(x - 1, y, z)].cubeRGB[c]);
            if (y > 0)           temp[ind].cubeRGB[c] = std::max(temp[ind].cubeRGB[c], propagation * data[Index(x, y - 1, z)].cubeRGB[c]);
            if (z > 0)           temp[ind].cubeRGB[c] = std::max(temp[ind].cubeRGB[c], propagation * data[Index(x, y, z - 1)].cubeRGB[c]);
            if (x < (vDimX - 2)) temp[ind].cubeRGB[c] = std::max(temp[ind].cubeRGB[c], propagation * data[Index(x + 1, y, z)].cubeRGB[c]);
            if (y < (vDimY - 2)) temp[ind].cubeRGB[c] = std::max(temp[ind].cubeRGB[c], propagation * data[Index(x, y + 1, z)].cubeRGB[c]);
            if (z < (vDimZ - 2)) temp[ind].cubeRGB[c] = std::max(temp[ind].cubeRGB[c], propagation * data[Index(x, y, z + 1)].cubeRGB[c]);
        }

        // Replace brightness
        for (int x = 0; x < (vDimX - 1); x++)
        for (int y = 0; y < (vDimY - 1); y++)
        for (int z = 1; z < (vDimZ - 1); z++)
        for (int c = 0; c < 3; c++)
        {
            data[Index(x, y, z)].cubeRGB[c] = temp[Index(x, y, z)].cubeRGB[c];
        }
    }
}

//---------------------------------------------------------------------
// Make a solid one, with a flat base.------------------------------------------
CubeWorld::CubeWorld(int argVDimX, int argVDimY, int argVDimZ, FloatScape * fs) : CubeWorld(argVDimX, argVDimY, argVDimZ)
{
    // Init and do basic shape
    for (int x = 0; x < vDimX; x++)
    for (int y = 0; y < vDimY; y++)
    for (int z = 0; z < vDimZ; z++)
    {
        int index = Index(x, y, z);
        Element * p = &data[index];

        // Init everything
        p->type = EMPTY;
        p->vertCol = 0xffff00ff; //purple
        p->cubeRGB[0] = 0.0f;
        p->cubeRGB[1] = 0.0f;
        p->cubeRGB[2] = 0.0f;
        p->vertOffset[0] = 0x00;
        p->vertOffset[1] = 0x00;
        p->vertOffset[2] = 0x00;

        // Cubes, so no edge
        if ((x == vDimX - 1) || (y == vDimY - 1) || (z == vDimZ - 1)) continue;

        // General 0 and 1 according to floatscape
        if (fs->a[fs->Index(x, y, z)] > 0.5f)     p->type = BLOCK;
        else                                    p->type = EMPTY;

    }
}
// Apply lighting AFTER any basic shape modifiers from the app----------------------
void CubeWorld::ApplyLighting()
{
    // Make the shape PART 2
    // Only when the shape is set, can we set the lights, as they are codependent
    // Make the shape, by setting types,
    for (int x = 0; x < vDimX - 1; x++)
    for (int y = 0; y < vDimY - 1; y++)
    for (int z = 0; z < vDimZ - 1; z++)
    {
        int index = Index(x, y, z);
        Element * p = &data[index];

        // Regular lights on base (type==2)
        // Random other lights, in solid sections, and as long as
        // there's at least one adjacent empty space
        // Allows buried ones, but they should play no part yet 
        if ((Maths::Rand01() < 0.01f) && (p->type != EMPTY))
        {
            if (((x != vDimX - 2) && (data[Index(x + 1, y, z)].type == 0))
                || ((x != 0) && (data[Index(x - 1, y, z)].type == 0))
                || ((y != (vDimY - 2)) && (data[Index(x, y + 1, z)].type == 0))
                || ((y != 0) && (data[Index(x, y - 1, z)].type == 0))
                || ((z != vDimZ - 2) && (data[Index(x, y, z + 1)].type == 0))
                || ((z != 0) && (data[Index(x, y, z - 1)].type == 0)))
                p->type = LIGHT;
        }
    }

    // Set the colour in the cubes, by responding to the types
    for (int x = 0; x < vDimX; x++)
    for (int y = 0; y < vDimY; y++)
    for (int z = 0; z < vDimZ; z++)
    {
        int index = Index(x, y, z);
        Element * p = &data[index];
        // Cubes, so no edge
        if ((x == vDimX - 1) || (y == vDimY - 1) || (z == vDimZ - 1)) continue;

        // Set to bright only with type 2
        if (p->type == LIGHT)
        {
            p->cubeRGB[0] = p->cubeRGB[1] = p->cubeRGB[2] = 255.0f;// 255.0f;
        }
    }

    // Propagate Light
    PropagateLight(30);

    // Set vertex colours from cube colours
    for (int x = 0; x < vDimX; x++)
    for (int y = 0; y < vDimY; y++)
    for (int z = 0; z < vDimZ; z++)
    {
        int index = Index(x, y, z);
        Element * p = &data[index];

        float maxRGB[3] = { 0, 0, 0 };
        for (int i = -1; i <= 0; i++)
        for (int j = -1; j <= 0; j++)
        for (int k = -1; k <= 0; k++)
        for (int c = 0; c < 3; c++)
        {
            int xi = x + i;
            int yj = y + j;
            int zk = z + k;

            // There's no valid data beyond edge
            if ((xi == -1) || (yj == -1) || (zk == -1))  continue;
            if ((xi == vDimX - 1) || (yj == vDimY - 1) || (zk == vDimZ - 1))  continue;

            maxRGB[c] = std::max(maxRGB[c], data[Index(xi, yj, zk)].cubeRGB[c]);
        }

        p->vertCol = 0xff000000 + (((DWORD)maxRGB[0]) << 16) + (((DWORD)maxRGB[1]) << 8) + (((DWORD)maxRGB[2]) << 0);
    }
}
//------------------------------------------------------------------------
void CubeWorld::Save(char * filename)
{
    FILE * file; fopen_s(&file, File.Path(filename,false), "w");
    fprintf(file, "%d %d %d ", vDimX, vDimY, vDimZ);
    fwrite(data, sizeof(Element), vDimX * vDimY * vDimZ, file);
    fclose(file);
}
//------------------------------------------------------------------------------
void CubeWorld::Set(int x0, int y0, int z0, int numX, int numY, int numZ, int type)
{
    for (int x = x0; x < (x0 + numX); x++)
    for (int y = y0; y < (y0 + numY); y++)
    for (int z = z0; z < (z0 + numZ); z++)
    {
        data[Index(x, y, z)].type = type;
    }
}


