#include <cstdint>
#include "../Header/libextra.h"

//---------------------------------------------
TextModel::TextModel(Font * argFont, int maxTris, DWORD extraFlags) : font(argFont)
{
    LivePolyStore = new TriangleSet(maxTris);
    Material * mat = new Material(font->texture, new Shader(), Material::MAT_NOCULL | Material::MAT_TRANS | extraFlags);
    LivePolyModel = new Model(maxTris * 3, maxTris * 3, Vector3(0, 0, 0), Quat4(0, 0, 0, 1), mat);
    Reset();
}

//------------------------------------------------
float TextModel::FindLength(char * text)
{
    float total = 0;
    int len = (int)strlen(text);
    for (int i = 0; i < len; i++)
    {
        total += ScaleX * (font->lookupTable[text[i]].width + font->lookupTable[Font::GapBetweenChars].width);
    }
    return(total);
}

//-----------------------------------------------------
TextModel * TextModel::Add(int alignment0L1C2R, char * fnt, ...)
{
    static char stringText[1000];
    va_list args;
    va_start(args, fnt);
    vsprintf_s(stringText, fnt, args);
    va_end(args);

    if (alignment0L1C2R == 1) OffsetX -= 0.5f*FindLength(stringText);
    if (alignment0L1C2R == 2) OffsetX -= 1.0f*FindLength(stringText);

    int len = (int)strlen(stringText);
    for (int i = 0; i < len; i++)
    {
        uint8_t letter = stringText[i];
        Font::CharData * cd = &font->lookupTable[letter];
        float width = ScaleX * cd->width;
        LivePolyStore->AddQuad(
            Vertex(Vector3(OffsetX + 0, OffsetY + 0, OffsetZ), Colour, cd->u_start, 1),
            Vertex(Vector3(OffsetX + width, OffsetY + 0, OffsetZ), Colour, cd->u_end, 1),
            Vertex(Vector3(OffsetX + 0, OffsetY - ScaleY, OffsetZ), Colour, cd->u_start, 0),
            Vertex(Vector3(OffsetX + width, OffsetY - ScaleY, OffsetZ), Colour, cd->u_end, 0));
        OffsetX += width + ScaleX*font->lookupTable[Font::GapBetweenChars].width;
    }
    return(this);
}

//-----------------------------------------------------
TextModel * TextModel::Offset(float x, float y, float z)
{
    OffsetX = x;
    OffsetY = y;
    OffsetZ = z;
    return(this);
}

//-----------------------------------------------------
TextModel * TextModel::Scale(float x, float y)
{
    ScaleX = x;
    ScaleY = y;
    return(this);
}

//-----------------------------------------------------
TextModel * TextModel::Col(DWORD colour)        
{ 
    Colour = colour;
    return(this);
}

//-----------------------------------------------------
TextModel * TextModel::CR(float multiplier)
{ 
    Offset(0, OffsetY - ScaleY*multiplier, OffsetZ);
    return(this);
}

//-----------------------------------------------------
void TextModel::UpdateModel()    
{ 
    LivePolyModel->ReplaceTriangleSet(LivePolyStore);
}

//-----------------------------------------------------
void TextModel::Render(Matrix44 * viewProjMat, float red, float gre, float blu, float alpha)
{ 
    LivePolyModel->Render(viewProjMat, red, gre, blu, alpha,true);
}

//-----------------------------------------------------
void TextModel::Reset()
{ 
    Offset(0, 0,0);
    Scale(1, 1);
    Col(0xffffffff);
    LivePolyStore->Empty();
}
