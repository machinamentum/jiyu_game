#include <cstdint>
#include "../Header/libextra.h"

//-------------------------------------------------------------------------------------
Font::Font(char * textureFilename, char * filename, bool isItBlocky)
{
    // Load in the texture
    texture = LoadBMP(textureFilename, isItBlocky ? MakeBMPBlocky : MakeBMPRedAlpha, isItBlocky ? 1 : 8);

    // Load in data
    memset(lookupTable, 0, 256 * sizeof(CharData));
    FILE * file_ptr; fopen_s(&file_ptr, File.Path(filename,true), "r");
    int pixelW, pixelH; fscanf_s(file_ptr, "REFWIDTH=%d REFHEIGHT=%d\n", &pixelW,&pixelH);
    while(true)
    {
        uint8_t this_char;    int first_pixel, last_pixel;
        fscanf_s(file_ptr, "%c %d %d\n", &this_char, 65000, &first_pixel, &last_pixel);
        if (this_char == (uint8_t)'¬') this_char = ' '; // Special character for space
        if (this_char == (uint8_t)'[') this_char = GapBetweenChars;
        if (this_char == (uint8_t)'^') break;

        lookupTable[this_char].width = (last_pixel - first_pixel + 1.0f) / pixelH;
        lookupTable[this_char].u_start = (first_pixel - 0.5f) / pixelW;
        lookupTable[this_char].u_end   = (last_pixel + 0.5f)  / pixelW;
    }
    fclose(file_ptr);

    // Fill in underscore as space, rather than unknown and thus little gap
    lookupTable['_'] = lookupTable[' '];
}

