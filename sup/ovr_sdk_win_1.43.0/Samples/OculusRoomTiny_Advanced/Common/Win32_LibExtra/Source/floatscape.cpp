
#include "../Header/libextra.h"

//-------------------------------------------------------------------------------
int FloatScape::Index(int x, int y, int z)
{
    return(x + (DimX * (y + DimY * z)));
}

//--------------------------------------------------------------------------
void FloatScape::Blur(int iterations)
{
    for (int i = 0; i < iterations; i++)
    {
        for (int x = 0; x<DimX; x++)
        for (int y = 0; y<DimY; y++)
        for (int z = 0; z<DimZ; z++)
        {
            float total = a[Index(x, y, z)];    int num = 1;
            if (x>0)            { total += a[Index(x - 1, y, z)]; num++; };
            if (y>0)            { total += a[Index(x, y - 1, z)]; num++; };
            if (z>0)            { total += a[Index(x, y, z - 1)]; num++; };
            if (x < (DimX - 1)) { total += a[Index(x + 1, y, z)]; num++; };
            if (y < (DimY - 1)) { total += a[Index(x, y + 1, z)]; num++; };
            if (z < (DimZ - 1)) { total += a[Index(x, y, z + 1)]; num++; };
            t[Index(x, y, z)] = total / ((float)num);
        }
        int entries = DimX * DimY * DimZ;
        for (int e = 0; e < entries; e++)
            a[e] = t[e];
    }
}

//--------------------------------------------------------------------------------
FloatScape::FloatScape(int dimX, int dimY, int dimZ, int iterations, float min, float max, float topBotBias) : DimX(dimX), DimY(dimY), DimZ(dimZ)
{
    int entries = dimX * dimY * dimZ;
    a = (float *)malloc(entries * sizeof(float));
    t = (float *)malloc(entries * sizeof(float));

    for (int x = 0; x < dimX; x++)
    for (int y = 0; y < dimY; y++)
    for (int z = 0; z < dimZ; z++)
    {
        float rand = Maths::Lerp(min, max, Maths::Rand01());
        if (y == (dimY - 1)) rand -= topBotBias;
        if (y == 0)          rand += topBotBias;
        a[Index(x, y, z)] = rand;
    }
    Blur(iterations);
    
}

//------------------------------------------------------------------------------
// Not sure we even need this one, since we're doing cubeworld ones.
void FloatScape::Set(int x0, int y0, int z0, int numX, int numY, int numZ, float value)
{
    for (int x = x0; x < (x0 + numX); x++)
    for (int y = y0; y < (y0 + numY); y++)
    for (int z = z0; z < (z0 + numZ); z++)
    {
        a[Index(x, y, z)] = value;
    }
}

//-------------------------------------------------------------------------------
FloatScape::~FloatScape()
{
    free(a);
    free(t);
}
