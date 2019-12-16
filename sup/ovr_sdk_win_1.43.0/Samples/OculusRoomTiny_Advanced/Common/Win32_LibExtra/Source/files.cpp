
#include "../Header/libextra.h"

//------------------------------------
Files::Files(char * initialAssetPath)
{
    strcpy_s(AssetPath, sizeof(AssetPath), initialAssetPath);
}

//--------------------------------------------------------------------------------------------
char * Files::Path(char * originalName, bool insistExists)
{
    static char adaptedName[100];
    sprintf_s(adaptedName, "%s%s", AssetPath, originalName);
    if (insistExists)
    {
        // Debug::Assert(Exists(adaptedName), "File is missing %s", adaptedName);
        // Debug::Assert(false/*Exists(adaptedName)*/, "File is missing %d", 3, 6, 7);
        if (!Exists(adaptedName)) Debug::Error("File is missing %s", adaptedName);
    }
    return(adaptedName);
}

//---------------------------------------------------------------
bool Files::Exists(char * pathedName)
{
    FILE * fileptr;
    auto result = fopen_s(&fileptr, pathedName, "r");

    if (result) return(false);
    else
    {
        fclose(fileptr);
        return(true);
    }
}



