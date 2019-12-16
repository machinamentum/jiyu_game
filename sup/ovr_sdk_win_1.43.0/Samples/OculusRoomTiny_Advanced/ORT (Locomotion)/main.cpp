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

//Advise compiling with Debug and Win32. 

#include "general.h"

int whichTest = 1; 

// Globals that all tests use
ovrSession session;
ovrHmdDesc hmdDesc;
long frameIndex;
DirectX11 DIRECTX;

#ifdef BUILD_FOR_OLD_SDK
// Files File("Assets\\");
Files File("..\\..\\..\\Assets\\");
#else
Files File("..\\..\\..\\Assets\\");
#endif

Control CONTROL;

// Assets that most tests use, or at least can share.
Camera * mainCam;
Camera * localCam;
Camera * counterCam;
Model * roomModel;
Model * roomModelNoFurniture;
Model * corridorModel;
Model * skyModel;
Model * holoModel;
VRLayer * Layer0;
VRLayer * Layer1;
VRLayer * standardLayer0;
VRLayer * standardLayer1;

//---------------------------------------------
void Independent_Window_Render(void)
{
}

// Used to supply the relevent tests processing function. 
int(*processFn)(void);

//-------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hinst, HINSTANCE, LPSTR, int)
{
    // Make the HMD to get the resolutions
    ovr_Initialize(nullptr);
    ovrGraphicsLuid luid;
    ovr_Create(&session, &luid);
    hmdDesc = ovr_GetHmdDesc(session);

    // Setup Device and Graphics, mirror is 1/2 HMD res.
    DIRECTX.InitWindow(hinst, L"Oculus Room Tiny (DX11)");
    DIRECTX.InitDevice(hmdDesc.Resolution.w / 2, hmdDesc.Resolution.h / 2, reinterpret_cast<LUID*>(&luid));

    // Initialise the text capability
    INFOTEXT_Init();

    // Initialise shared assets
    roomModel = SCENE_CreateOriginalORTModel(true);
    roomModelNoFurniture = SCENE_CreateOriginalORTModel(false);
    corridorModel = SCENE_CreateCorridorModel(false); // Toggle this to generate;
    skyModel = SCENE_CreateSkyBoxModel();
    holoModel = SCENE_CreateHolodeckModel();
    mainCam = new Camera();
    localCam = new Camera();
    counterCam = new Camera();

    standardLayer0 = new VRLayer(session, hmdDesc.DefaultEyeFov[0], hmdDesc.DefaultEyeFov[1], 1.0f, true);
    standardLayer1 = new VRLayer(session, hmdDesc.DefaultEyeFov[0], hmdDesc.DefaultEyeFov[1], 1.0f, true);

    // Init simple mirror window
    VRMIRROR_Init(session, standardLayer0);

    // Init game for those that use it.
    GAME_Init();

    // This doubles as a general clock
    frameIndex = 0;

    // Main loop
    while (DIRECTX.HandleMessages())
    {
        
        // Recentre on press of 1
        // Or automatically in first 10 frames
        if ((DIRECTX.Key['1'])
            /*|| (DIRECTX.Key[' '])*/ // Could have it on every press of SPACE if required.
            || (frameIndex < 10))
            ovr_RecenterTrackingOrigin(session);

        INFOTEXT_StartAddingText();

        static int currentTest = 10000; // This triggers an auto update, to get in 1-9 range.
        static int totalTests = 1; // This gets updated on first loop by initialisation lower
        static int currentTestTimer = 0;
        static bool currentTestTextVisible = false;
        static char testTitle[200];
        static char testDesc[5][200];

        if (currentTestTextVisible)
        {
            // General Text that every test has
            INFOTEXT_GetModelToAdd()->Add(1, "%d/%d  \"%s\"", currentTest, totalTests, testTitle)->CR(2);
            INFOTEXT_GetModelToAdd()->Add(1, "%s", testDesc[0])->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "%s", testDesc[1])->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "%s", testDesc[2])->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "%s", testDesc[3])->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "%s", testDesc[4])->CR(3);
            INFOTEXT_GetModelToAdd()->Add(1, "GENERAL CONTROLS")->CR(2);
            INFOTEXT_GetModelToAdd()->Add(1, "Press '1' at any time to recenter.")->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "Press SPACE or 'X' to advance through tests ('Z' to go back).")->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "Press ARROW keys to rotate and move forward and back.")->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "Press WASD keys to strafe and move forward and back.")->CR();
            INFOTEXT_GetModelToAdd()->Add(1, "In many examples, hold '9' to show a lurch function to enhance potential discomfort to test.")->CR();

            if ((CONTROL.KeyPress(' '))
             || (CONTROL.KeyPress('Z'))
             || (CONTROL.KeyPress('X')))
                currentTestTextVisible = false;
        }
        else
        {
            if ((CONTROL.KeyPress(' '))
                || (CONTROL.KeyPress('Z'))
                || (CONTROL.KeyPress('X'))
                || (currentTest == 10000))
            {
                // OK, we have to initialise a new test
                if (CONTROL.KeyPress('Z')) currentTest--;
                else                         currentTest++;
                currentTestTimer = 0;
                currentTestTextVisible = true;
                GAME_Reset();

            doOver:
                int t = 0;
                if (currentTest == 0) currentTest = 1;

                t++;
                // Below is the complete description of each test.
			    if (currentTest == (t++))
                {
                    processFn = TESTBASE2_Init();
                    strcpy_s(testTitle, "WELCOME");
                    strcpy_s(testDesc[0], "This is series of experimental comfort techniques");
                    strcpy_s(testDesc[1], "for achieving locomotion within VR.");
                    strcpy_s(testDesc[2], "This is just the extended ORT scene.");
                    strcpy_s(testDesc[3], "Practice navigating here, note the general controls below.");
                    strcpy_s(testDesc[4], "Don't forget to press '1' to recenter.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTV_Init(false, true, true, true, false);
                    strcpy_s(testTitle, "WINDOW INTO THE MOVING WORLD 1");
                    strcpy_s(testDesc[0], "We start with mimicking the familiar scenario of a TV screen.");
                    strcpy_s(testDesc[1], "The moving VR world exists within the confines of a frame");
                    strcpy_s(testDesc[2], "with a static surrounding world, that grounds the user.");
                    strcpy_s(testDesc[3], "In this first example, we show a very simplistic single window,");
                    strcpy_s(testDesc[4], "and we note that the 3D world extends depthwise into the window.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTV_Init(false, true, true, false, false);
                    strcpy_s(testTitle, "WINDOW INTO THE MOVING WORLD 2");
                    strcpy_s(testDesc[0], "We now extend this concept further, and take out that depth");
                    strcpy_s(testDesc[1], "information of the moving scene, so it more truly represents");
                    strcpy_s(testDesc[2], "the familiar 'TV' environment.  Note how the moving VR world");
                    strcpy_s(testDesc[3], "now appears to have its depth at the 'screen'.");
                    strcpy_s(testDesc[4], "");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTV_Init(false, false, false, true, false);
                    strcpy_s(testTitle, "WINDOW INTO THE MOVING WORLD 3");
                    strcpy_s(testDesc[0], "We go further. Now we introduce more windows, such that more");
                    strcpy_s(testDesc[1], "of the moving VR world is visible. We still retain the grounding");
                    strcpy_s(testDesc[2], "static scene in order to retain comfort, but now there is more to see.");
                    strcpy_s(testDesc[3], "One can start to appreciate that with more windows, or more diverse windowing,");
                    strcpy_s(testDesc[4], "that one can closer and closer approximate to full VR, whilst retaining the stabilising effect.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTV_Init(false, false, false, false, false);
                    strcpy_s(testTitle, "WINDOW INTO THE MOVING WORLD 4");
                    strcpy_s(testDesc[0], "And once again we take off depth, so that the enhanced comfort and");
                    strcpy_s(testDesc[1], "and familiarity of a TV screen is achieved for the windows.");
                    strcpy_s(testDesc[2], "At this point, we introduce you to auxiliary control keys in this sample,");
                    strcpy_s(testDesc[3], "for you to experiment with.");
                    strcpy_s(testDesc[4], "Holding keys 2,3,4,5,6 and 9 generates different options.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTV_Init(false, false, true, false, false);
                    strcpy_s(testTitle, "WINDOW INTO THE MOVING WORLD 5");
                    strcpy_s(testDesc[0], "Here we expand these windows in one permutation,");
                    strcpy_s(testDesc[1], "to show how all-encompassing such windowing may be.");
                    strcpy_s(testDesc[2], "Holding keys 2,3,4,5,6 and 9 generates different options.");
                    strcpy_s(testDesc[3], "Note that '5' is allowing transparency of the windows content.");
                    strcpy_s(testDesc[4], "Note that '3' is the toggle to full depth, or window-level depth.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTV_Init(true, false, false, false,false);
                    strcpy_s(testTitle, "WINDOW INTO THE MOVING WORLD 6");
                    strcpy_s(testDesc[0], "There are also permutations of the quality of the static background");
                    strcpy_s(testDesc[1], "that is used to ground the player.  Here we show a holodeck style one.");
                    strcpy_s(testDesc[2], "Different applications might vary in how they portray this static");
                    strcpy_s(testDesc[3], "component.  A word at this juncture to suggest that the user can be ");
                    strcpy_s(testDesc[4], "aclimatised to VR comfort with different sizes or qualities of this effect, in the early stages of an app.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTV_Init(false, false, false, false,true);
                    strcpy_s(testTitle, "WINDOW INTO THE MOVING WORLD 7");
                    strcpy_s(testDesc[0], "Finally, in this series, we portray the potential");
                    strcpy_s(testDesc[1], "significance of the static world having more or less");
                    strcpy_s(testDesc[2], "realism as compared to the moving VR world.  Thus potentially");
                    strcpy_s(testDesc[3], "it has been beneficial to portray the static world as the most 'real'");
                    strcpy_s(testDesc[4], "and now we have flipped it to show what happens if the reverse is true.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPORTAL_Init(true,false, false);
                    strcpy_s(testTitle, "PORTALS INTO THE STATIC WORLD 1");
                    strcpy_s(testDesc[0], "With this series, we introduce the concept of portals in the moving VR world.");
                    strcpy_s(testDesc[1], "These are special, because they are actually plotted in the moving VR world,");
                    strcpy_s(testDesc[2], "so they have location in the world, and move like a 3D object.  They provide");
                    strcpy_s(testDesc[3], "visual access to the grounding static world, to promote comfort.");
                    strcpy_s(testDesc[4], "There are both regular located ones, and also more randomly located ones.  Check out both.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPORTAL_Init(false,false, false);
                    strcpy_s(testTitle, "PORTALS INTO THE STATIC WORLD 2");
                    strcpy_s(testDesc[0], "OK, now we are going to enhance this effect by promoting the realism of the");
                    strcpy_s(testDesc[1], "static world.  So that the user fully comprehends this aspect of the scene,");
                    strcpy_s(testDesc[2], "as 'reality', and the moving part is downgraded to a flat, depthless portrayal.");
                    strcpy_s(testDesc[3], "Such an undertaking can dramatically increase the effectiveness of the technique.");
                    strcpy_s(testDesc[4], "Use the '3' key to switch between them in realtime.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPORTAL_Init(false, true, false);
                    strcpy_s(testTitle, "PORTALS INTO THE STATIC WORLD 3");
                    strcpy_s(testDesc[0], "The choice is with the user on how significant, or otherwise, to make this effect.");
                    strcpy_s(testDesc[1], "The larger the visibility, the better, but potentially at the cost of immersion");
                    strcpy_s(testDesc[2], "or aesthetics.  As with many of these techniques, introducing the concept during ");
                    strcpy_s(testDesc[3], "early or tutorial stages, might allow the user to be aclimatized and progress");
                    strcpy_s(testDesc[4], "to achieiving comfort with less and less intrusive sizes and frequencies.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPORTAL_Init(false, false,true);
                    strcpy_s(testTitle, "PORTALS INTO THE STATIC WORLD 4");
                    strcpy_s(testDesc[0], "As with the 'windows' examples, it is expected that the realism of the moving scene");
                    strcpy_s(testDesc[1], "versus the static one, is significant.  Thus we have defaulted thus far to a more");
                    strcpy_s(testDesc[2], "realistic colouring of the static component, and a tinted moving part.");
                    strcpy_s(testDesc[3], "We now portray the reverse, to illustrate for comparison");
                    strcpy_s(testDesc[4], "Note 3,4,6,7 and 9 allow realtime options in the items in this series.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTEDGE_Init(true, false);
                    strcpy_s(testTitle, "EMERGING STATIC IN THE PERIPHERY 1");
                    strcpy_s(testDesc[0], "This is an extension on the more familiar theme, of decreasing the FOV when");
                    strcpy_s(testDesc[1], "discomforting acceleration or rotation is present. Now, the decreasing FOV reveals");
                    strcpy_s(testDesc[2], "a grounding, static world to stabilise the player.");
                    strcpy_s(testDesc[3], "In order to test the subconscious effect of this, a simple game has been introduced.");
                    strcpy_s(testDesc[4], "This serves to keep the players conscious mind occupied, to facilitate realistic testing.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTEDGE_Init(false, false);
                    strcpy_s(testTitle, "EMERGING STATIC IN THE PERIPHERY 2");
                    strcpy_s(testDesc[0], "In the prior example, the FOV of both eyes were balanced, such that the same area");
                    strcpy_s(testDesc[1], "appeared to be reduced.  This is a variation on that, where the FOV reduction does not");
                    strcpy_s(testDesc[2], "perfectly overlap.  Arguably this is less effective, but sometimes its useful to show");
                    strcpy_s(testDesc[3], "such lesser solutions, for clarity.");
                    strcpy_s(testDesc[4], "");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTEDGE_Init(true, true);
                    strcpy_s(testTitle, "EMERGING STATIC IN THE PERIPHERY 3");
                    strcpy_s(testDesc[0], "Note that in the prior items in this series, the transparency of the main moving scene");
                    strcpy_s(testDesc[1], "has faded off with discomforting conditions.  In this test, we've left the");
                    strcpy_s(testDesc[2], "tranparency off, to showcase a more solid main scene, that has advantages of immersion.");
                    strcpy_s(testDesc[3], "An option in this series is to hold '4', such that the colours and background are alternatives.");
                    strcpy_s(testDesc[4], "Hold '2' to permanently put on the periphery effect.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTCOUNTER_Init(360, false,true);
                    strcpy_s(testTitle, "COUNTERING OPTIC FLOW 1");
                    strcpy_s(testDesc[0], "There is a concept that if you can provide an equal-and-opposite balancing optic flow");
                    strcpy_s(testDesc[1], "then you can increase comfort, causing the brain to believe that the net effect is less,");
                    strcpy_s(testDesc[2], "or zero motion.  In this first instance of the series, we just show the effect");
                    strcpy_s(testDesc[3], "permanently on, so the effect is seen and appreciated.");
                    strcpy_s(testDesc[4], "Again, a simple game has been provided to showcase how the brain naturally operates to ignore the effect.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTCOUNTER_Init(360, false,false);
                    strcpy_s(testTitle, "COUNTERING OPTIC FLOW 2");
                    strcpy_s(testDesc[0], "Now we extend the prior test, to only provide the countering optic flow when the situation");
                    strcpy_s(testDesc[1], "requires it, otherwise it remains off, and leaves an ordinary VR scene.");
                    strcpy_s(testDesc[2], "You'll note that the countering visuals are actually the same scene, but with");
                    strcpy_s(testDesc[3], "reversed movement, and tinted to make it clear which is the actual game world.");
                    strcpy_s(testDesc[4], "As with many of these series and tests, try pressing '9' to see how it fares with artifically bad motion.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTCOUNTER_Init(100, false, false);
                    strcpy_s(testTitle, "COUNTER OPTIC FLOW 3");
                    strcpy_s(testDesc[0], "Using the game world itself as counter visuals, presents a problem in that you can move off from reasonable graphics.");
                    strcpy_s(testDesc[1], "Thus we continually automatically recenter the counter visuals, to ensure they are present, and a reasonable");
                    strcpy_s(testDesc[2], "counter portrayal of the main world.  In this test, we actually increase the frequency of that reset to");
                    strcpy_s(testDesc[3], "show the effect of that, and the app itself could find a frequency which is most effective.");
                    strcpy_s(testDesc[4], "Again, worth reiterating that many of these effects could be faded off once a user is aclimatized.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTCOUNTER_Init(10000, false, false);
                    strcpy_s(testTitle, "COUNTER OPTIC FLOW 4");
                    strcpy_s(testDesc[0], "To illustrate the need for resetting, here is a version that does not reset,");
                    strcpy_s(testDesc[1], "except for when the effect is off and not needed.");
                    strcpy_s(testDesc[2], "Note how it effectively 'loses' a useful counter flow if you turn down side tunnels,");
                    strcpy_s(testDesc[3], "or operate near the end of the main tunnel.");
                    strcpy_s(testDesc[4], "");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTCOUNTER_Init(360, true, false);
                    strcpy_s(testTitle, "COUNTER OPTIC FLOW 5");
                    strcpy_s(testDesc[0], "Finally in this series, we show a counter optic flow that");
                    strcpy_s(testDesc[1], "does not in fact use the game world itself, but an artificial");
                    strcpy_s(testDesc[2], "'counter' world, in order that its effectiveness can be judged.");
                    strcpy_s(testDesc[3], "Having a more simplistic world potentially creates less optical confusion,");
                    strcpy_s(testDesc[4], "and weirdness, but perhaps at the expense of effectiveness.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTDESENS_Init(0.03f);
                    strcpy_s(testTitle, "DESENSITIZING PROCESS 1");
                    strcpy_s(testDesc[0], "Hold '9' to activate. The constant unnatural motion temporarily weakens the brains link between eyes and ears,");
                    strcpy_s(testDesc[1], "for this environment.  Such that the brain no longer trusts its input, and becomes aclimatized.");
                    strcpy_s(testDesc[2], "The premise is that an app might initially present graphics such as this, in order");
                    strcpy_s(testDesc[3], "to aclimate the player to VR, such that when they play the main app, they are no");
                    strcpy_s(testDesc[4], "longer affected.  Try it for a while, by holding '9', then return to the welcome screen. ");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTDESENS_Init(0.13f);
                    strcpy_s(testTitle, "DESENSITIZING PROCESS 2");
                    strcpy_s(testDesc[0], "Hold '9' to activate. The motion of this process is inherently discomforting to some extent.");
                    strcpy_s(testDesc[1], "In this version, the motion is faster which for some might be more comfortable.");
                    strcpy_s(testDesc[2], "This is very experimental, and the hope is that there is a cap on how uncomfortable");
                    strcpy_s(testDesc[3], "this feels, but then guards and 'protects' the player against more significant or");
                    strcpy_s(testDesc[4], "severe discomfort in the main app. Needs more research.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTUNREAL_Init(false, false);
                    strcpy_s(testTitle, "UNREAL WORLD BEYOND STATIC COCKPIT 1");
                    strcpy_s(testDesc[0], "This is included for illustration, but as with so much in VR, things with theoretical promise");
                    strcpy_s(testDesc[1], "turn out not so well when actually implemented!");
                    strcpy_s(testDesc[2], "The theory here is that a static cockpit could be used to ground the external motion");
                    strcpy_s(testDesc[3], "by decreasing the 'reality' of the exterior.  Otherwise it would just be perceived that you AND the cockpit are moving.");
                    strcpy_s(testDesc[4], "Hold 3 to turn off the de-realism effect, which here is deliberately incorrect distortion.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTUNREAL_Init(true, false);
                    strcpy_s(testTitle, "UNREAL WORLD BEYOND STATIC COCKPIT 2");
                    strcpy_s(testDesc[0], "It has been suggested that a large cockpit might enhance the effectiveness,");
                    strcpy_s(testDesc[1], "so that is the variation here.");
                    strcpy_s(testDesc[2], "Again, hold 3 to compare with regular moving world, rather than artificially derealisation.");
                    strcpy_s(testDesc[3], "Other effects to downgrade reality of the moving world might be colour shaders or effects, or a lower framerate.");
                    strcpy_s(testDesc[4], "This doesn't seem to hold much promise at this stage, but included for completeness.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTUNREAL_Init(true, true);
                    strcpy_s(testTitle, "COUNTER COCKPIT TO MASK MOTION");
                    strcpy_s(testDesc[0], "A very lightly research area, compared to others in this suite. ");
                    strcpy_s(testDesc[1], "A quick example here to show the initial work.  The theory is that one can utilise");
                    strcpy_s(testDesc[2], "artificial counter motion on a cockpit, to trick the brain to effectively mask out motion");
                    strcpy_s(testDesc[3], "in the main scene. No motion perceived equals no discomfort.  To try it out, hold 9 to");
                    strcpy_s(testDesc[4], "get discomforting motion, then try holding keys 6,7,8 and note how they diminish perceptopn of the main motion.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPOLE_Init(false,false,+1);
                    strcpy_s(testTitle, "SKIPOLE WORLD MANIPULATION 1");
                    strcpy_s(testDesc[0], "The principle here is that if you can convince the brain that the world is moving, not you,");
                    strcpy_s(testDesc[1], "then you can eliminate discomfort.  To this end, imagine each Touch controller is a 'skipole'.");
                    strcpy_s(testDesc[2], "Grab the world with either trigger, or top button, and drag the world under you.");
                    strcpy_s(testDesc[3], "You should have a sense that you aren't in fact moving, from this action, and hence");
                    strcpy_s(testDesc[4], "no discomforting mismatch between eye input and inner eye input.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPOLE_Init(true,false,+1);
                    strcpy_s(testTitle, "SKIPOLE WORLD MANIPULATION 2");
                    strcpy_s(testDesc[0], "The reason for using a 'skipole' rather than direct hands, is that very little motion is");
                    strcpy_s(testDesc[1], "generated from hands, whereas the extra length helps with a pole.");
                    strcpy_s(testDesc[2], "Expanding the 'reach' of this technique further, we introduce momentum from your poling action.");
                    strcpy_s(testDesc[3], "Note that you can use both hands or poles in concert, to get a smooth and increased motion.");
                    strcpy_s(testDesc[4], "Rotation is achievable on the sticks.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPOLE_Init(true, true, +1);
                    strcpy_s(testTitle, "SKIPOLE WORLD MANIPULATION 3");
                    strcpy_s(testDesc[0], "We develop this theme further, by allowing the twisting of the Touch controller");
                    strcpy_s(testDesc[1], "to twist the moving VR world, further emphasising the mobile nature of the VR world");
                    strcpy_s(testDesc[2], "and how IT is moving under you, and being moved by you, rather than the discomforting");
                    strcpy_s(testDesc[3], "perception of you moving in the world without your inner ear reporting it.");
                    strcpy_s(testDesc[4], "Try exploring side alleys and really try to move naturally and intuitively through the world.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTPOLE_Init(true, true, -1);
                    strcpy_s(testTitle, "SKIPOLE WORLD MANIPULATION 4");
                    strcpy_s(testDesc[0], "The previous test introduced twisting, consistent with a skipole grabbing and rotating.");
                    strcpy_s(testDesc[1], "For some, this is unintuitive on some level.  If they are consciously turning left to go left,");
                    strcpy_s(testDesc[2], "then that won't happen in the prior test.  Because its the world moving. So we offer the portrayal");
                    strcpy_s(testDesc[3], "of a reverse twisting effect in this test, in order for you to experience this.");
                    strcpy_s(testDesc[4], "It is conceivable that such mixing of the intuitive responses may still work. Judge for yourself.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTILT_Init(true,true);
                    strcpy_s(testTitle, "ARTIFICIAL TILT 1");
                    strcpy_s(testDesc[0], "Many of you will be familiar with apps that employ head tilting as means to comfortably impart");
                    strcpy_s(testDesc[1], "accelerations within a VR world.  This has a scientific basis in that the component of gravity");
                    strcpy_s(testDesc[2], "'represents' the VR acceleration sufficiently.  But you have to tilt your head! Here we explore");
                    strcpy_s(testDesc[3], "if the acceleration comes from the game controls and player customary input, and then the associated");
                    strcpy_s(testDesc[4], "head tilt is simulated by artificially tilting the VR world. Employ all the control methods to test, including strafing.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTILT_Init(false,true);
                    strcpy_s(testTitle, "ARTIFICIAL TILT 2");
                    strcpy_s(testDesc[0], "The prior test was actually a constrained and approximated version of the type of artificial tilts");
                    strcpy_s(testDesc[1], "required to reflect ingame movement.  Here we present the true version, which as you can see, ");
                    strcpy_s(testDesc[2], "is rather more radical and creates counterproductively unsettling discomfort, by virtue of large ");
                    strcpy_s(testDesc[3], "head ROTATIONS that didn't exist in reality.  Therefore, it appears that approximating and using smaller");
                    strcpy_s(testDesc[4], "and more cautious (but representative) artificial tilts, is beneficial.");
                }
                else if (currentTest == (t++))
                {
                    processFn = TESTTILT_Init(true,false);
                    strcpy_s(testTitle, "ARTIFICIAL TILT 3");
                    strcpy_s(testDesc[0], "In the prior tests in this series, we employed a camera with rather smooth motion, in order that");
                    strcpy_s(testDesc[1], "resulting accelerations were smoother, and therefore smoother representative tilts were artificially added.");
                    strcpy_s(testDesc[2], "Slightly contrived, but not beyond the wit of a developer to do.  However, if we revert, as in this test,");
                    strcpy_s(testDesc[3], "to the camera motion of our choice, then we see more jarring tilting in response, but actually, more");
                    strcpy_s(testDesc[4], "comfortable for it!  So aesthetically less nice, but comfort wise, using the natural game controls works well.");
                }
				else if (currentTest == (t++))
				{
				processFn = TESTINVISIBLE_Init(false);
				strcpy_s(testTitle, "INVISIBLE IN PERIPHERY");
				strcpy_s(testDesc[0], "Using a custom shader, we simulate what it would be like");
				strcpy_s(testDesc[1], "for the periphery to be relatively invisible in the");
				strcpy_s(testDesc[2], "periphery of the eye view, by effectively generating uniform brightness.");
				strcpy_s(testDesc[3], "The idea is that there is almost no optic flow in the all important");
				strcpy_s(testDesc[4], "periphery, including for locomotion, since the rods of the eye can't see it.");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTUNREAL2_Init(true, 2, false, 1);
				strcpy_s(testTitle, "DIMINISHED REALITY BY FRAMERATE 1");
				strcpy_s(testDesc[0], "In order to promote the foreground cockpit as the");
				strcpy_s(testDesc[1], "stabilizing influence, we diminish the reality of the");
				strcpy_s(testDesc[2], "full VR world by updating it every other frame.");
				strcpy_s(testDesc[3], "This includes deliberately failing to update HMD tracking");
				strcpy_s(testDesc[4], "for the world, but importantly, NOT the cockpit.");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTUNREAL2_Init(true, 10, false,1);
				strcpy_s(testTitle, "DIMINISHED REALITY BY FRAMERATE 2");
				strcpy_s(testDesc[0], "Continuing this theme, we update the world every 10 frames");
				strcpy_s(testDesc[1], "thus over-enhancing the lack of reality of the world,");
				strcpy_s(testDesc[2], "but perhaps at the cost of comfort, as the effect");
				strcpy_s(testDesc[3], "is quite dramatic.");
				strcpy_s(testDesc[4], "");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTUNREAL2_Init(false, 4, true, 1);
				strcpy_s(testTitle, "DIMINISHED REALITY BY FRAMERATE 3");
				strcpy_s(testDesc[0], "Now a more balanced scenario with every 4 frames updated");
				strcpy_s(testDesc[1], "so promoting the preferred grounding of the cockpit");
				strcpy_s(testDesc[2], "but without the distracting excessive low framerate update of");
				strcpy_s(testDesc[3], "tracking.  We also diminish the impact of the cockpit in the");
				strcpy_s(testDesc[4], "hope that the cockpit doesn't need to be too large.");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTUNREAL2_Init(false, 1, true, 10);
				strcpy_s(testTitle, "DIMINISHED REALITY BY FRAMERATE 4");
				strcpy_s(testDesc[0], "Granted, it can be jarring with the low framerate of tracking");
				strcpy_s(testDesc[1], "even though this ensures that the 'unreality' of the VR");
				strcpy_s(testDesc[2], "world remains front of mind.  As an alternative, this");
				strcpy_s(testDesc[3], "demo considers only a lower framerate of update of the VR world");
				strcpy_s(testDesc[4], "and locomotion of that world, once per 10 frames.  ");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTUNREAL2_Init(false, 1, true, 3);
				strcpy_s(testTitle, "DIMINISHED REALITY BY FRAMERATE 5");
				strcpy_s(testDesc[0], "Now a more subtle version of the previous test");
				strcpy_s(testDesc[1], "where we are now updating the world (not the tracking)");
				strcpy_s(testDesc[2], "once every 3rd frame, effectively showing a 30fps");
				strcpy_s(testDesc[3], "experience - a framerate that has historically been");
				strcpy_s(testDesc[4], "acceptable in gaming.");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTCOUNTER2_Init(true,0,0.25f);
				strcpy_s(testTitle, "PERIPHERY COUNTER OPTIC FLOW 1");
				strcpy_s(testDesc[0], "This is a combination of counter optic flow,");
				strcpy_s(testDesc[1], "but now limited to the periphery to avoid dramatic immersion-breaking graphics.");
				strcpy_s(testDesc[2], "We are showing an exaggerated version here, with the periphery quite wide");
				strcpy_s(testDesc[3], "and have the optic flow reset fairly promptly at every 10 frames, in order");
				strcpy_s(testDesc[4], "to more closely retain the visual characteristics of the scene in the periphery.");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTCOUNTER2_Init(true, 0, 0.33f);
				strcpy_s(testTitle, "PERIPHERY COUNTER OPTIC FLOW 2");
				strcpy_s(testDesc[0], "We enhance the prior test, with a now more sublte size of periphery");
				strcpy_s(testDesc[1], "in order to promote immersion but ideally hold on to the");
				strcpy_s(testDesc[2], "benefits of the counter optic flow effect.");
				strcpy_s(testDesc[3], "Note that the counter flow is tinted in order to draw attention");
				strcpy_s(testDesc[4], "to it not being the main VR world of interest.");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTCOUNTER2_Init(true, 1, 0.25f);
				strcpy_s(testTitle, "PERIPHERY COUNTER OPTIC FLOW 3");
				strcpy_s(testDesc[0], "This test is as before, but now we remove the tinting and allow");
				strcpy_s(testDesc[1], "counter flow to have the same graphics, thus diminishing its");
				strcpy_s(testDesc[2], "anti-immersion properties.  The reset update of the counter flow");
				strcpy_s(testDesc[3], "is reduced to every 4th frame, hopefully holding onto the effect");
				strcpy_s(testDesc[4], "whilst increasing immersion further.  Initially with big periphery.");
				}
				else if (currentTest == (t++))
				{
				processFn = TESTCOUNTER2_Init(true, 1, 0.33f);
				strcpy_s(testTitle, "PERIPHERY COUNTER OPTIC FLOW 4");
				strcpy_s(testDesc[0], "And now the prior test with a more subtle periphery");
				strcpy_s(testDesc[1], "whose smaller impact again lessens loss of immersion.");
				strcpy_s(testDesc[2], "");
				strcpy_s(testDesc[3], "");
				strcpy_s(testDesc[4], "");
				}
				else // Last one
                {
                    currentTest = 1;
                    totalTests = t-1;
                    goto doOver;
                }
            }
        }
        currentTestTimer++; 

        // Run the correct test processing/render function, noting the number of desired layers.
        int numLayers = processFn();

        // Submit all the layers to the compositor
        ovrLayerHeader* layerHeaders[10];
        layerHeaders[0] = &Layer0->ld.Header;
        layerHeaders[1] = &Layer1->ld.Header;
        ovr_SubmitFrame(session, frameIndex, nullptr, layerHeaders, numLayers);

        // Mirror window.
        VRMIRROR_Render(session, 0, Independent_Window_Render);

        // Important swapchain
        DIRECTX.SwapChain->Present(0, 0);

        // Housekeeping
        frameIndex++;
        CONTROL.SaveLastKeys();
    }

    ovr_Shutdown();
}

