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

#define NUM_TARGETS 200
#define NUM_BULLETS 30

float targetRadius = 0.25f;
float bulletRadius = 0.05f;

Model * targets[NUM_TARGETS];
bool targetPresent[NUM_TARGETS];
Model * bullets[NUM_BULLETS];
Vector3 bulletVel[NUM_BULLETS];
int currBullet = 0;

//-------------------------------------------------------------------------------------
void GAME_Init()
{
    for (int i = 0; i < NUM_TARGETS; i++)
    {
        TriangleSet walls;
        float scale = targetRadius;
        walls.AddCube(Vector3(-scale, -scale, -scale), Vector3(scale, scale, scale), 0xff00ff00);

        Vector3 loc;
        loc.x = (Maths::Rand01() - Maths::Rand01()) * 20;
        loc.y = Maths::Rand01() * 10;
        loc.z = -Maths::Rand01() * 300;

        targets[i] = new Model(&walls, loc, Quat4(),
            new Material(new Texture(false, 256, 256, Texture::AUTO_GRID, 1, 8), new Shader()));
        targetPresent[i] = true;
    }
    for (int i = 0; i < NUM_BULLETS; i++)
    {
        TriangleSet walls;
        float scale = bulletRadius;
        walls.AddCube(Vector3(-scale, -scale, -scale), Vector3(scale, scale, scale), 0xffff00ff);

        bullets[i] = new Model(&walls, Vector3(), Quat4(),
            new Material(new Texture(false, 256, 256, Texture::AUTO_FLOOR, 1, 8), new Shader()));

        bullets[i]->Pos = Vector3(0, 0, 0);
        bulletVel[i] = Vector3(0, 0, 0);
    }
}

//-------------------------------------------------------------------------------------
void GAME_Reset()
{
    for (int i = 0; i < NUM_TARGETS; i++)
    {
        targetPresent[i] = true;
    }
}

//-------------------------------------------------------------------------------------
void GAME_Process(Vector3 bulletOrigin, Quat4 bulletDir)
{
    // Lets jiggle them randomly
    for (int i = 0; i < NUM_TARGETS; i++)
    {
        targets[i]->Pos.x += 0.01f*(Maths::Rand01() - Maths::Rand01());
        targets[i]->Pos.y += 0.01f*(Maths::Rand01() - Maths::Rand01());
        targets[i]->Pos.z += 0.01f*(Maths::Rand01() - Maths::Rand01());
    }

    // Lets fire them from the current location,
    // But only every so often. 
    if ((frameIndex % 25) == 0)
    {
        currBullet++;
        if (currBullet == NUM_BULLETS)
            currBullet = 0;

        bullets[currBullet]->Pos = bulletOrigin + Vector3(0, -0.25f, 0);

        Vector3 forward = Vector3(0, 0, -1).Rotate(&bulletDir);
        bulletVel[currBullet] = forward * 0.5f;
    }

    // Advance all bullets, and see if we've hit the targets
    for (int b = 0; b < NUM_BULLETS; b++)
    {
        bullets[b]->Pos += bulletVel[b];

        for (int t = 0; t < NUM_TARGETS; t++)
        {
            Vector3 diffApart = bullets[b]->Pos - targets[t]->Pos;
            float distApart = diffApart.Length();
            if (distApart < (targetRadius + bulletRadius))
                targetPresent[t] = false;
        }
    }

}

//-------------------------------------------------------------------------------------
void GAME_Render(Matrix44 * projView)
{
    for (int i = 0; i < NUM_TARGETS; i++)
    {
        if (targetPresent[i])
            targets[i]->Render(projView, 1, 1, 1, 1, true);
    }
    for (int i = 0; i < NUM_BULLETS; i++)
    {
        bullets[i]->Render(projView, 1, 1, 1, 1, true);
    }
}

