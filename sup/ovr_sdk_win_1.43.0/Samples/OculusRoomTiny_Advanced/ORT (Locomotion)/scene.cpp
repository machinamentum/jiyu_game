/************************************************************************************
Content     :   Locomotion research application for Oculus Rift
Created     :   19th July 2017
Authors     :   Tom Heath
Copyright   :   Copyright (c) Facebook Technologies, LLC and its affiliates. All rights reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*************************************************************************************/

#include "general.h"

//-----------------------------
Model * SCENE_CreateHolodeckModel(void)
{
    Model * m = new Model();

    TriangleSet walls;
    float scale = 12;
    walls.AddSolidColorBox(+scale, scale, +scale, -scale, -scale, -scale, 0xffffff00, 0.25f);
    m->AddSubModel(new Model(&walls, Vector3(), Quat4(),
        new Material(new Texture(false, 256, 256, Texture::AUTO_GRID, 1, 8), new Shader())));
    return(m);
}

//-----------------------------
Model * SCENE_CreateTouchModel(float poleLength)
{
    Model * m;

    TriangleSet walls;
    float scale = 0.05f;
    float mscale = 0.005f;
    walls.AddSolidColorBox(-scale, -scale, -scale, +scale, +scale, +scale, 0xff00ffff, 2.0f);
    walls.AddSolidColorBox(-mscale, -poleLength, -mscale, +mscale, 0, +mscale, 0xff00ffff, 2.0f);
    m = new Model(&walls, Vector3(), Quat4(),
        new Material(new Texture(false, 256, 256, Texture::AUTO_FLOOR, 1, 8), new Shader()));
    return(m);
}

//-----------------------------
Model * SCENE_CreateOriginalORTModel(bool includeFurniture)
    {
    Model * m = new Model();

    TriangleSet walls;
    walls.AddSolidColorBox(10.1f, 0.0f, 20.0f, 10.0f, 4.0f, -20.0f, 0xff808080);  // Left Wall
    walls.AddSolidColorBox(10.0f, -0.1f, 20.1f, -10.0f, 4.0f, 20.0f, 0xff808080); // Back Wall
    walls.AddSolidColorBox(-10.0f, -0.1f, 20.0f, -10.1f, 4.0f, -20.0f, 0xff808080);   // Right Wall
    if (!includeFurniture)
        walls.AddSolidColorBox(10.0f, -0.1f, -20.1f, -10.0f, 4.0f, -20.0f, 0xff808080); // Add temporary front wall if not furniture
    m->AddSubModel(new Model(&walls, Vector3(), Quat4(),
        new Material(new Texture(false, 256, 256, Texture::AUTO_WALL, 1, 8), new Shader())));

    TriangleSet floors;
    floors.AddSolidColorBox(10.0f, -0.1f, 20.0f, -10.0f, 0.0f, -20.1f, 0xff808080);
    m->AddSubModel(new Model(&floors, Vector3(), Quat4(),
        new Material(new Texture(false, 256, 256, Texture::AUTO_FLOOR, 1, 8), new Shader())));

    TriangleSet ceiling;
    ceiling.AddSolidColorBox(10.0f, 4.0f, 20.0f, -10.0f, 4.1f, -20.1f, 0xff808080);
    m->AddSubModel(new Model(&ceiling, Vector3(), Quat4(),
        new Material(new Texture(false, 256, 256, Texture::AUTO_CEILING, 1, 8), new Shader())));

    if (includeFurniture)
    {
        TriangleSet furniture;
        furniture.AddSolidColorBox(-9.5f, 0.75f, -3.0f, -10.1f, 2.5f, -3.1f, 0xff383838);    // Right side shelf// Verticals
        furniture.AddSolidColorBox(-9.5f, 0.95f, -3.7f, -10.1f, 2.75f, -3.8f, 0xff383838);   // Right side shelf
        furniture.AddSolidColorBox(-9.55f, 1.20f, -2.5f, -10.1f, 1.30f, -3.75f, 0xff383838); // Right side shelf// Horizontals
        furniture.AddSolidColorBox(-9.55f, 2.00f, -3.05f, -10.1f, 2.10f, -4.2f, 0xff383838); // Right side shelf
        furniture.AddSolidColorBox(-5.0f, 1.1f, -20.0f, -10.0f, 1.2f, -20.1f, 0xff383838);   // Right railing   
        furniture.AddSolidColorBox(10.0f, 1.1f, -20.0f, 5.0f, 1.2f, -20.1f, 0xff383838);   // Left railing  
        for (float f = 5; f <= 9; f += 1)
            furniture.AddSolidColorBox(-f, 0.0f, -20.0f, -f - 0.1f, 1.1f, -20.1f, 0xff505050); // Left Bars
        for (float f = 5; f <= 9; f += 1)
            furniture.AddSolidColorBox(f, 1.1f, -20.0f, f + 0.1f, 0.0f, -20.1f, 0xff505050); // Right Bars
        furniture.AddSolidColorBox(1.8f, 0.8f, -1.0f, 0.0f, 0.7f, 0.0f, 0xff505000);  // Table
        furniture.AddSolidColorBox(1.8f, 0.0f, 0.0f, 1.7f, 0.7f, -0.1f, 0xff505000); // Table Leg 
        furniture.AddSolidColorBox(1.8f, 0.7f, -1.0f, 1.7f, 0.0f, -0.9f, 0xff505000); // Table Leg 
        furniture.AddSolidColorBox(0.0f, 0.0f, -1.0f, 0.1f, 0.7f, -0.9f, 0xff505000);  // Table Leg 
        furniture.AddSolidColorBox(0.0f, 0.7f, 0.0f, 0.1f, 0.0f, -0.1f, 0xff505000);  // Table Leg 
        furniture.AddSolidColorBox(1.4f, 0.5f, 1.1f, 0.8f, 0.55f, 0.5f, 0xff202050);  // Chair Set
        furniture.AddSolidColorBox(1.401f, 0.0f, 1.101f, 1.339f, 1.0f, 1.039f, 0xff202050); // Chair Leg 1
        furniture.AddSolidColorBox(1.401f, 0.5f, 0.499f, 1.339f, 0.0f, 0.561f, 0xff202050); // Chair Leg 2
        furniture.AddSolidColorBox(0.799f, 0.0f, 0.499f, 0.861f, 0.5f, 0.561f, 0xff202050); // Chair Leg 2
        furniture.AddSolidColorBox(0.799f, 1.0f, 1.101f, 0.861f, 0.0f, 1.039f, 0xff202050); // Chair Leg 2
        furniture.AddSolidColorBox(1.4f, 0.97f, 1.05f, 0.8f, 0.92f, 1.10f, 0xff202050); // Chair Back high bar
        for (float f = 3.0f; f <= 6.6f; f += 0.4f)
            furniture.AddSolidColorBox(3, 0.0f, -f, 2.9f, 1.3f, -f - 0.1f, 0xff404040); // Posts
        m->AddSubModel(new Model(&furniture, Vector3(), Quat4(),
            new Material(new Texture(false, 256, 256, Texture::AUTO_WHITE, 1, 8), new Shader())));
    }

    return(m);
}

//-----------------------------
Model * SCENE_CreateSkyBoxModel()
{
    Model * m = new Model();
    TriangleSet floors;
    float size = 500.0f;
    floors.AddSolidColorBox(size, size, size, -size, -size, -size, 0xff0000ff);
    m->AddSubModel(new Model(&floors, Vector3(), Quat4(),
        new Material(new Texture(false, 256, 256, Texture::AUTO_FLOOR, 1, 8), new Shader())));
    return(m);
}

//-----------------------------
Model * SCENE_CreateCockpitModel(bool extendedLength)
{
    Model * m = new Model();
    TriangleSet floors;
    
    floors.AddSolidColorBox(-5.0f, 5.0f, 5.0f, 5.0f, 6.0f, 6.0f, 0xff80ff80);   // Top, back left to right
    floors.AddSolidColorBox(-5.0f, 5.0f, -6.0f, 5.0f, 6.0f, -5.0f, 0xff80ff80); // Top, front left to right
    floors.AddSolidColorBox(-5.0f, extendedLength ? -26.0f : -6.0f, 5.0f, 5.0f, -5.0f, 6.0f, 0xff80ff80);   // Bot, back left to right
    floors.AddSolidColorBox(-5.0f, extendedLength ? -26.0f : -6.0f, -6.0f, 5.0f, -5.0f, -5.0f, 0xff80ff80); // Bot, front left to right

    floors.AddSolidColorBox(-6.0f, -5.0f, -6.0f, -5.0f, 5.0f, -5.0f, 0xff80ff80); // Front left vertical strut
    floors.AddSolidColorBox(5.0f, -5.0f, -6.0f, 6.0f, 5.0f, -5.0f, 0xff80ff80); // Front right vertical strut
    floors.AddSolidColorBox(-6.0f, -5.0f, 5.0f, -5.0f, 5.0f, 6.0f, 0xff80ff80); // Back left vertical strut
    floors.AddSolidColorBox(5.0f, -5.0f, 5.0f, 6.0f, 5.0f, 6.0f, 0xff80ff80); // Back right vertical strut

    floors.AddSolidColorBox(-6.0f, 5.0f, -5.0f, -5.0f, 6.0f, 5.0f, 0xff80ff80); // top left horiz strut
    floors.AddSolidColorBox(-6.0f, extendedLength ? -26.0f : -6.0f, -5.0f, -5.0f, -5.0f, 5.0f, 0xff80ff80); // bot left horiz strut
    floors.AddSolidColorBox(5.0f, 5.0f, -5.0f, 6.0f, 6.0f, 5.0f, 0xff80ff80); // top right horiz strut
    floors.AddSolidColorBox(5.0f, extendedLength ? -26.0f : -6.0f, -5.0f, 6.0f, -5.0f, 5.0f, 0xff80ff80); // bot right horiz strut

    floors.Scale(Vector3(0.07f, 0.07f, 0.14f));

    m->AddSubModel(new Model(&floors, Vector3(), Quat4(),
        new Material(new Texture(false, 256, 256, Texture::AUTO_FLOOR, 1, 8), new Shader())));
    return(m);
}

//--------------------------------------
Model * SCENE_CreateCorridorModel(bool rebuild)
{
    Model * m = new Model();

    // Textures
    Texture * wallTex = new Texture(false, 128, 128, Texture::AUTO_WALL, 1, 8);
    Texture * crateTex = LoadBMP("crate.bmp", MakeBMPFullAlpha, 8);

    // Shaders
	//Overbright, normal range up to 0.5, then becomes increasingly solid to 1.0
	char overBrightAbove50PercentPShader[] =
		"Texture2D Texture   : register(t0); SamplerState Linear : register(s0); "
		"float4 main(in float4 Position : SV_Position, in float4 mainCol: COLOR0, in float2 TexCoord : TEXCOORD0) : SV_Target"
		"{                                                       "
		"    float4 texCol = Texture.Sample(Linear, TexCoord);                        "
		"    mainCol.rgb *= 2;														  "
		"    float4 c = texCol*mainCol;												  "
		"    if (mainCol.r > 1) c.r = lerp(texCol.r, 1, (mainCol.r - 1.0f));	  "
		"    if (mainCol.g > 1) c.g = lerp(texCol.g, 1, (mainCol.g - 1.0f));	  "
		"    if (mainCol.b > 1) c.b = lerp(texCol.b, 1, (mainCol.b - 1.0f));	  "
		"    return (c);                                                              "
		"}                                                       ";

    Shader * obShader = new Shader("ShaderOverBright50", 0, 3, 0, overBrightAbove50PercentPShader);

    // Materials
    Material * overBrightWallMat = new Material(wallTex, obShader, Material::MAT_WRAP);
    Material * overBrightCrateMat = new Material(crateTex, obShader, Material::MAT_WRAP);

    // Big model
    TriangleSet generalTS0(20 * 20 * 20 * 12);
    TriangleSet generalTS1(20 * 20 * 20 * 12);
    CubeWorld * corridorCubeWorld;

    // Optionally grow a long corridor 
    #define CORRIDOR_LENGTH 300 // Vary it if you wish, but need to delete the corridor.txt file in assets
    if ((!File.Exists(File.Path("corridor.txt",false)))
     || (rebuild)) 
    {
        CubeWorld * cw;
        int x = 30;
        int y = 20;
        int z = CORRIDOR_LENGTH;
        FloatScape fs(x, y, z, 15, 0, 1, 0.0f);
        fs.Set(0, 0, 0, x, 1, z, 1.0f);  // Make the floor solid
        fs.Set((x / 2) - 1, 1, 0, 4, 4, z, 0.0f);  // Carve a 4x4 path just off floor
        cw = new CubeWorld(x + 1, y + 1, z + 1, &fs);
        cw->Set(0, 0, 0, x, 1, z, CubeWorld::FLOOR);  // Make the solid floor, a 'floor'
        cw->ApplyLighting();
        cw->Save("corridor.txt");
    }

    corridorCubeWorld = new CubeWorld("corridor.txt");
    m = corridorCubeWorld->MakeRecursiveModel(10, &generalTS0, &generalTS1, overBrightWallMat, overBrightCrateMat);
    m->AddPosHierarchy(Vector3(-16, -1, -21-CORRIDOR_LENGTH));

    return(m);
}


