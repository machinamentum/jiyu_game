/************************************************************************************
Created     :   Feb 23, 2018
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

#include "Script.h"
#include "OculusWorldDemo.h"
#include "Extras/OVR_Math.h"
#include "../CommonSrc/Util/OptionMenu.h"
#include "../CommonSrc/Util/Logger.h" // WriteLog
#include "../CommonSrc/Render/Render_Device.h"
#include "Util/Util_D3D11_Blitter.h" // From OVRKernel
#include <fstream>
#include <sstream>
#include <string>
#include <cerrno>
#include <codecvt>
#include <filesystem> // Pre-release C++ filesystem support.



#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4996) // 'sscanf': This function or variable may be unsafe.
#endif

#ifdef _WIN32
#define OVR_SCRIPT_PATH_SEPARATOR '\\'
#define OVR_SCRIPT_PATH_SEPARATORS "/\\"
#else
#define OVR_SCRIPT_PATH_SEPARATOR '/'
#define OVR_SCRIPT_PATH_SEPARATORS "/"
#endif


static bool EnsureDirectoryExists(const char* directoryPath) {
  namespace fs = std::experimental::filesystem::v1;

  std::error_code errorCode; // Will be success if the direcory already existed.
  fs::path p(directoryPath);
  bool result = (fs::create_directories(p, errorCode) || !errorCode);
  return result; // True if the directory path already existed.
}

// Remove any reading and trailing space, tab, newline
static void Trim(std::string& str, const char* whitespace = " \t\r\n")
{
    str.erase(0, str.find_first_not_of(whitespace));
    str.erase(str.find_last_not_of(whitespace)+1); // Works because C++ requires (string::npos == -1)
}

// Removes leading and trailing quotes. If there's only a leading or only a trailing quote 
// then it is removed; matches are not required for the removal to occur.
static void Dequote(std::string& str)
{
    if(!str.empty() && (str[0] == '\"'))
        str.erase(str.begin());
    if(!str.empty() && (str.back() == '\"'))
        str.pop_back();
}

static void TrimAndDequote(std::string& str)
{
    Trim(str);
    Dequote(str);
}


OWDScript::OWDScript(OculusWorldDemoApp* owdApp)
    : ScriptFile(), ScriptOutputPath(), ScriptState(State::None), Commands(), CurrentCommand(), Status(), OWDApp(owdApp)
{
}


// Does some checking and calls the lower level LoadScriptFile function.
bool OWDScript::LoadScriptFile(const char* filePath)
{
    bool success = false;

    if(ScriptState == State::None) // If not already executing script...
    {
        ScriptFile = filePath; // Save this for possible use during LoadScriptFile.
        TrimAndDequote(ScriptFile);
        success = LoadScriptFile(filePath, Commands);
    }

    return success;
}


// Does some checking and calls the lower level LoadScriptText function.
bool OWDScript::LoadScriptText(const char* scriptText)
{
    if(ScriptState == State::None) // If not already executing script...
        return LoadScriptText(scriptText, Commands);

    return false;
}


bool OWDScript::SetScriptOutputPath(const char* directoryPath)
{
    ScriptOutputPath = directoryPath;

    // Make sure ScriptOutputPath has a trailing directory path separator.
    if(ScriptOutputPath.find_last_of(OVR_SCRIPT_PATH_SEPARATORS) != (ScriptOutputPath.size() - 1))
        ScriptOutputPath += OVR_SCRIPT_PATH_SEPARATOR;

    // Create the directory if it doesn't already exist
    return EnsureDirectoryExists(directoryPath);
}

void OWDScript::Shutdown()
{
    CleanupOWDState();

    // Return to the newly constructed state.
    ScriptFile.clear();
    Commands.clear();
    CurrentCommand = 0;
    ScriptState = State::None;
    Status = 0;
    // Do not clear OWDApp, as it's set on construction.
}


// Loads the file and calls the lower level LoadScriptText function.
bool OWDScript::LoadScriptFile(const char* filePath, CommandVector& commands)
{
    bool success = false;

    std::string filePathStr(filePath);
    Dequote(filePathStr);

    std::ifstream file(filePathStr.c_str(), std::ios::in);

    if (!file)
    {
        // If the file open failed, let's check to see if the filePath was a 
        // relative file path to the current ScriptFile file path (e.g. script_2.txt).
        // For example, ScriptFile is /somedir/somedir/script_1.txt
        size_t pos = ScriptFile.find_last_of("/\\");

        if(pos != std::string::npos)
        {
            filePathStr = ScriptFile;   // e.g. /somedir/somedir/script_1.txt
            filePathStr.resize(pos+1);  // e.g. /somedir/somedir/
            filePathStr += filePath;    // e.g. /somedir/somedir/script_2.txt
        }

        file.open(filePathStr.c_str(), std::ios::in);
    }

    if (file)
    {
        std::ostringstream scriptText;
        scriptText << file.rdbuf();
        file.close();

        success = LoadScriptText(scriptText.str().c_str(), commands);
    }
    
    return success;
}


bool OWDScript::LoadScriptText(const char* scriptText, CommandVector& commands)
{
    bool success = true;

    if(scriptText)
    {
        std::istringstream stream{scriptText};
        std::string line;

        while (success && std::getline(stream, line))
        {
            Trim(line);

            if (!ReadCommandLine(line, commands))
            {
                success = false;
                commands.clear();
            }
        }
    }

    ScriptState = (success ? State::NotStarted : State::None);

    return success;
}


// The input line is expected to be trimmed of leading and trailing whitespace.
bool OWDScript::ReadCommandLine(const std::string& line, CommandVector& commands)
{
    bool success = true;
    char commandName[32];
    int result = sscanf(line.c_str(), "%31s", commandName); // Get the command name (e.g. "Wait")

    if(result == 1)
    {
        if(OVR_stristr(commandName, "ExecuteMenu") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandExecuteMenu>();
            CommandExecuteMenu* p = static_cast<CommandExecuteMenu*>(commandPtr.get());

            // We have a line like so: " ExecuteMenu   \"parent.child.size\" 100.23"
            p->MenuPath = line;
            p->MenuPath.erase(0, strlen("ExecuteMenu"));
            Trim(p->MenuPath);

            // Now we have a line like so: "\"parent.child.size\" \"some value\""
            // We implement a dequoting scheme that has limitations in its flexibility of handling
            // escaped quotes, but it should work for all our known value string use cases.
            size_t lastLineQuote = p->MenuPath.rfind('\"');
            size_t firstLineQuote = p->MenuPath.find('\"');

            if(lastLineQuote == (p->MenuPath.size() - 1) &&  // If the last argument is quoted...
               (firstLineQuote != lastLineQuote)) // And 
            {
                firstLineQuote = p->MenuPath.rfind('\"', lastLineQuote - 1);
                p->MenuValue.assign(p->MenuPath, firstLineQuote, lastLineQuote + 1 - firstLineQuote);
                p->MenuPath.erase(firstLineQuote);
            }
            else
            {
                size_t lastLineSpace = p->MenuPath.find_last_of(" \t");
                if(lastLineSpace != std::string::npos)
                {  
                    p->MenuValue.assign(p->MenuPath, lastLineSpace);
                    p->MenuPath.erase(lastLineSpace);
                }
            }

            TrimAndDequote(p->MenuPath);
            TrimAndDequote(p->MenuValue);

            if (!p->MenuPath.empty() && !p->MenuValue.empty())
                commands.push_back(commandPtr);
            else
            {
                WriteLog("[OWDScript] Invalid ExecuteMenu command: %s. Expect ExecuteMenu <menu path string> <value>\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "ExecuteScriptFile") == commandName)
        {
            std::string filePath = (line.c_str() + strlen("ExecuteScriptFile"));
            TrimAndDequote(filePath);

            CommandVector commandsTemp;

            if (LoadScriptFile(filePath.c_str(), commandsTemp))
                commands.insert(commands.end(), commandsTemp.begin(), commandsTemp.end());
            else
            {
                WriteLog("[OWDScript] Invalid or unloadable ExecuteScriptFile command: %s. Expect ExecuteScriptFile <script file path string>\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "SetPose") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandSetPose>();
            CommandSetPose* p = static_cast<CommandSetPose*>(commandPtr.get());
            char buffer[8];

            int argCount = sscanf(line.c_str(), "%*s %f %f %f %f %f %f", &p->X, &p->Y, &p->Z, &p->OriX, &p->OriY, &p->OriZ);
            
            if (argCount == 6)
                commands.push_back(commandPtr);
            else if((sscanf(line.c_str(), "%*s %8s", buffer) == 1) && (OVR_stricmp(buffer, "off") == 0))
            {
                p->stop = true;
                commands.push_back(commandPtr);
            }
            else
            {
                WriteLog("[OWDScript] Invalid SetPose command: %s. Expect SetPose [<x pos> <y pos> <z pos> <x ori> <y ori> <z ori>] | off\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "Wait") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandWait>();
            CommandWait* p = static_cast<CommandWait*>(commandPtr.get());

            char waitUnits[16];
            int argCount = sscanf(line.c_str(), "%*s %lf %15s", &p->WaitAmount, waitUnits);
            
            if (argCount == 2)
            {
                if(OVR_stristr(waitUnits, "ms") == waitUnits)
                    p->WaitUnits = WaitUnit::ms;
                else if(OVR_stristr(waitUnits, "s") == waitUnits)
                    p->WaitUnits = WaitUnit::s;
                else if(OVR_stristr(waitUnits, "frame") == waitUnits) // Covers both "frame" and "frames".
                    p->WaitUnits = WaitUnit::frames;
                else
                {
                    WriteLog("[OWDScript] Invalid wait units: %s", waitUnits);
                    success = false;
                }

                if ((p->WaitAmount > 0) && (p->WaitUnits != WaitUnit::none))
                    commands.push_back(commandPtr);
            }
            else
            {
                WriteLog("[OWDScript] Invalid Wait command: %s. Expect Wait <amount> [ms|s|frame|frames]\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "PauseUntil") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandPauseUntil>();
            CommandPauseUntil* p = static_cast<CommandPauseUntil*>(commandPtr.get());
            
            char waitMode[16];
            char waitUnits[16];
            int argCount =
                sscanf(line.c_str(), "%*s %15s %lf %15s", waitMode, &p->WaitAmount, waitUnits);
            
            if(argCount >= 2)
            {
                // wait mode
                if(OVR_stristr(waitMode, "6dof") == waitMode) {
                    p->WaitMode = CommandPauseUntil::dof6;
                }
                else if(OVR_stristr(waitMode, "3dof") == waitMode) {
                    p->WaitMode = CommandPauseUntil::dof3;
                }
                else {
                    WriteLog("[OWDScript] Invalid PauseUntil mode: %s", waitMode);
                    success = false;
                }
            
                if(argCount == 3) {
                    // wait timeout
                    if(OVR_stristr(waitUnits, "ms") == waitUnits) {
                        p->WaitUnits = WaitUnit::ms;
                    }
                    else if(OVR_stristr(waitUnits, "s") == waitUnits) {
                        p->WaitUnits = WaitUnit::s;
                    }
                    else if(OVR_stristr(waitUnits, "frame") == waitUnits) {
                        // Covers both "frame" and "frames".
                        p->WaitUnits = WaitUnit::frames;
                    }
                    else {
                        WriteLog("[OWDScript] Invalid PauseUntil units: %s", waitUnits);
                        success = false;
                    }
                }
            
                if(success && (p->WaitAmount > 0)) {
                    // push valid command
                    commands.push_back(commandPtr);
                }
            }
            else {
                WriteLog(
                    "[OWDScript] Invalid PauseUntil command: %s. Expect PauseUntil <6dof|3dof> <count> [ms|s|frame|frames]\n",
                    line.c_str());
                success = false;
            }
        }
        else if (OVR_stristr(commandName, "ControllersPauseUntil") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandControllersPauseUntil>();
            CommandControllersPauseUntil* p = static_cast<CommandControllersPauseUntil*>(commandPtr.get());

            char waitMode[16];
            char waitUnits[16];
            int argCount =
                sscanf(line.c_str(), "%*s %15s %lf %15s", waitMode, &p->WaitAmount, waitUnits);

            if (argCount >= 2)
            {
                // wait mode
                if (OVR_stristr(waitMode, "both6dof") == waitMode) {
                    p->WaitMode = CommandControllersPauseUntil::both6dof;
                }
                else if (OVR_stristr(waitMode, "notboth6dof") == waitMode) {
                    p->WaitMode = CommandControllersPauseUntil::notboth6dof;
                }
                else {
                    WriteLog("[OWDScript] Invalid ControllersPauseUntil mode: %s", waitMode);
                    success = false;
                }

                if (argCount == 3) {
                    // wait timeout
                    if (OVR_stristr(waitUnits, "ms") == waitUnits) {
                        p->WaitUnits = WaitUnit::ms;
                    }
                    else if (OVR_stristr(waitUnits, "s") == waitUnits) {
                        p->WaitUnits = WaitUnit::s;
                    }
                    else if (OVR_stristr(waitUnits, "frame") == waitUnits) {
                        // Covers both "frame" and "frames".
                        p->WaitUnits = WaitUnit::frames;
                    }
                    else {
                        WriteLog("[OWDScript] Invalid ControllersPauseUntil units: %s", waitUnits);
                        success = false;
                    }
                }

                if (success && (p->WaitAmount > 0)) {
                    // push valid command
                    commands.push_back(commandPtr);
                }
            }
            else {
                WriteLog(
                    "[OWDScript] Invalid ControllersPauseUntil command: %s. Expect ControllersPauseUntil <both6dof|notboth6dof> <count> [ms|s|frame|frames]\n",
                    line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "EnableTrackingStateLog") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandEnableTrackingStateLog>();
            CommandEnableTrackingStateLog* p =
                static_cast<CommandEnableTrackingStateLog*>(commandPtr.get());
            
            char enableStr[16];
            int argCount = sscanf(line.c_str(), "%*s %15s", enableStr);
            
            if(argCount == 1) {
                if(OVR_stricmp(enableStr, "true") == 0) {
                    p->enable = true;
                    commands.push_back(commandPtr);
                }
                else if(OVR_stricmp(enableStr, "false") == 0) {
                    p->enable = false;
                    commands.push_back(commandPtr);
                }
                else {
                    WriteLog("[OWDScript] Invalid EnableTrackingStateLog string: %s", enableStr);
                    success = false;
                }
            }
            else {
                WriteLog(
                    "[OWDScript] Invalid EnableTrackingStateLog command: %s. Expect EnableTrackingStateLog <true/false>\n",
                    line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "Screenshot") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandScreenshot>();
            CommandScreenshot* p = static_cast<CommandScreenshot*>(commandPtr.get());

            p->ScreenshotFilePath = line.c_str() + strlen("Screenshot");
            TrimAndDequote(p->ScreenshotFilePath);
            if (!p->ScreenshotFilePath.empty())
                commands.push_back(commandPtr);
            else
            {
                WriteLog("[OWDScript] Invalid Screenshot command: %s. Expect Screenshot <output file path>\n", line.c_str());
                success = false;
            }
        }
        else if (OVR_stristr(commandName, "PerfCapture") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandPerfCapture>();
            CommandPerfCapture* p = static_cast<CommandPerfCapture*>(commandPtr.get());
            
            char unitsStr[16];
            int cmdStrLen;
            const char* lineCStr = line.c_str();
            int argCount = sscanf(lineCStr, "%*s %lf %15s%n", &p->CaptureTotalDuration, unitsStr, &cmdStrLen);
            
            // Read rest of the string into variable. We do this to pull in white spaces.
            int fileNameStartOffset = cmdStrLen + 1;
            if ((int)strlen(lineCStr) > fileNameStartOffset)
            {
              p->CaptureFilePath = lineCStr + fileNameStartOffset;
              TrimAndDequote(p->CaptureFilePath);
            
              if (!p->CaptureFilePath.empty())
                argCount++;
            }
            
            if (argCount == 3)
            {
                if (OVR_stristr(unitsStr, "ms") == unitsStr)
                    p->CaptureDurationUnits = PerfCaptureSerializer::Units::ms;
                else if (OVR_stristr(unitsStr, "s") == unitsStr)
                    p->CaptureDurationUnits = PerfCaptureSerializer::Units::s;
                else if (OVR_stristr(unitsStr, "frame") == unitsStr) // Covers both "frame" and "frames".
                    p->CaptureDurationUnits = PerfCaptureSerializer::Units::frames;
                else
                {
                    WriteLog("OWDScript: Invalid duration units: %s", unitsStr);
                    success = false;
                }
                
                if ((p->CaptureTotalDuration > 0) && (p->CaptureDurationUnits != PerfCaptureSerializer::Units::none))
                {
                    if (p->CaptureCollector == nullptr)
                    {
                      // since we need the session, we delay the capture 
                      p->CaptureCollector = std::make_shared<PerfCaptureSerializer>(
                        p->CaptureTotalDuration, p->CaptureDurationUnits, p->CaptureFilePath);
                    }
                    
                    commands.push_back(commandPtr);
                }
            }
            else 
            {
                WriteLog("[OWDScript] Invalid PerfCapture command: %s. Expect PerfCapture <amount> " \
                       "[ms|s|frame|frames] <output file path>\n", line.c_str());
                success = false;
            }
        }
        else if(OVR_stristr(commandName, "WriteLog") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandWriteLog>();
            CommandWriteLog* p = static_cast<CommandWriteLog*>(commandPtr.get());

            p->LogText = line.c_str() + strlen("WriteLog");
            TrimAndDequote(p->LogText); // Unfortunately, we don't have a way of telling if the quote char is intentional.
            if (!p->LogText.empty())
                commands.push_back(commandPtr);
            // Do we print an error that the log text was empty?
        }
        else if(OVR_stristr(commandName, "Exit") == commandName)
        {
            CommandPtr commandPtr = std::make_shared<CommandExit>();
            commands.push_back(commandPtr);
        }
        else if(line.empty() || (line[0] == '/' && line[1] == '/'))
        {
            // Ignore empty or comment lines.
        }
        else
        {
            WriteLog("OWDScript: Invalid command line: %s\n", line.c_str());
            success = false;
        }
    }

    return success;
}

#ifdef _WIN32

// To consider: Move WriteTexture to an OWD member function.
static bool WriteTexture(ID3D11Device* d3dDevice, OVR::Render::D3D11::Texture* texture, const std::string& filePathOutput)
{
    using namespace OVR::D3DUtil;

    std::wstring filePathOutputW = OVR::UTF8StringToUCSString(filePathOutput);

    D3DTextureWriter textureWriter(d3dDevice);

    D3DTextureWriter::Result result = textureWriter.SaveTexture(
          texture->Tex, 0, true, filePathOutputW.c_str(), nullptr, nullptr);

    return (result == D3DTextureWriter::Result::SUCCESS);
}

#endif

void OWDScript::CleanupOWDState()
{
    // We reset anything we may have set in OWD.
    // If animation is to be unilaterally resumed, then we need to execute "Scene Content.Animation Enabled.false"

    // Disabled until we get the ovri_GetTrackingOverride change checked in.
}

OWDScript::State OWDScript::Step()
{
    // Currently we execute no more than one instruction per step. We could possibly execute
    // multiple instructions in a step for instructions that complete during the step.

    if((ScriptState == State::NotStarted) || (ScriptState == State::Started))
    {
        ScriptState = State::Started; // Set it started if not started already.

        if (CurrentCommand < Commands.size())
        {
            Command& command = *Commands[CurrentCommand].get(); // Convert shared_ptr to a reference.

            switch (command.commandType)
            {
                case CommandType::ExecuteMenu:{
                    CommandExecuteMenu& c = static_cast<CommandExecuteMenu&>(command);

                    OptionMenuItem* menuItem = OWDApp->Menu.FindMenuItem(c.MenuPath.c_str());

                    if (menuItem)
                    {
                        bool success = menuItem->SetValue(c.MenuValue);
                        if(success)
                            WriteLog("[OWDScript] ExecuteMenu: %s->%s\n", c.MenuPath.c_str(), c.MenuValue.c_str());
                        else
                            WriteLog("[OWDScript] ExecuteMenu: set of option failed: %s -> %s. Is the option available with this build or configuration?\n", c.MenuPath.c_str(), c.MenuValue.c_str());
                    }
                    else
                        WriteLog("[OWDScript] ExecuteMenu: Menu not found: %s -> %s. Is the menu available with this build or configuration?\n", c.MenuPath.c_str(), c.MenuValue.c_str());

                    CurrentCommand++;

                    break;
                }

                case CommandType::ExecuteScriptFile:
                    // This shouldn't happen because we expanded the script file on load.
                    break;

                case CommandType::SetPose:{
                    CommandSetPose& c = static_cast<CommandSetPose&>(command);

                        WriteLog("[OWDScript] SetPose (not supported): x:%f y:%f z:%f OriX:%f OriY:%f OriZ:%f\n", 
                            c.X, c.Y, c.Z, c.OriX, c.OriY, c.OriZ);

                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }

                case CommandType::Wait:{
                    CommandWait& c = static_cast<CommandWait&>(command);

                    if(c.CompletionValue == 0) // If the wait isn't started yet...
                    {
                        if (c.WaitUnits == WaitUnit::ms)
                        {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + (c.WaitAmount / 1000));
                            WriteLog("[OWDScript] Wait for %f ms started\n", c.WaitAmount);
                        }
                        else if(c.WaitUnits == WaitUnit::s)
                        {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + c.WaitAmount);
                            WriteLog("[OWDScript] Wait for %f s started\n", c.WaitAmount);
                        }
                        else // CommandWait::frames
                        {
                            c.CompletionValue = (OWDApp->TotalFrameCounter + c.WaitAmount);
                            WriteLog("[OWDScript] Wait for %f frame(s) started\n", c.WaitAmount);
                        }
                    }

                    if ((c.WaitUnits == WaitUnit::ms) ||
                        (c.WaitUnits == WaitUnit::s))
                    {
                        if(ovr_GetTimeInSeconds() >= c.CompletionValue) // If done waiting
                        {
                            WriteLog("[OWDScript] WaitMs wait for %f %s completed\n", c.WaitAmount, 
                                (c.WaitUnits == WaitUnit::ms) ? "ms" : "s");
                            CurrentCommand++; // Move onto the next instruction.
                        }
                    }
                    else // CommandWait::frames
                    {
                        if(OWDApp->TotalFrameCounter >= c.CompletionValue) // If done waiting
                        {
                            WriteLog("[OWDScript] Wait for %f frame(s) completed\n", c.WaitAmount);
                            CurrentCommand++; // Move onto the next instruction.
                        }
                    }

                    break;
                }
                
                case CommandType::PauseUntil: {
                    CommandPauseUntil& c = static_cast<CommandPauseUntil&>(command);
                    OVR_ASSERT(OWDApp);

                    std::string waitModeStr;
                    if (c.WaitMode == CommandPauseUntil::dof6) {
                        waitModeStr = "6dof";
                    }
                    else if (c.WaitMode == CommandPauseUntil::dof3) {
                        waitModeStr = "3dof";
                    }
                    else {
                        OVR_FAIL(); // should not reach here
                    }

                    if (!(c.WaitStarted)) // If the wait isn't started yet...
                    {
                        if (c.WaitUnits == WaitUnit::ms) {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + (c.WaitAmount / 1000.0));
                            WriteLog(
                                "[OWDScript] PauseUntil %s or %f ms started\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                        }
                        else if (c.WaitUnits == WaitUnit::s) {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + c.WaitAmount);
                            WriteLog(
                                "[OWDScript] PauseUntil %s or %f s started\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                        }
                        else // CommandWait::frames
                        {
                            c.CompletionValue = (OWDApp->TotalFrameCounter + c.WaitAmount);
                            WriteLog(
                                "[OWDScript] PauseUntil %s or %f frame(s) started\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                        }
                        c.WaitStarted = true;
                    }

                    // check wait mode
                    if (OWDApp->HaveHMDVisionTracking && (c.WaitMode == CommandPauseUntil::dof6))
                    {
                        WriteLog("[OWDScript] PauseUntil completed on %s\n", waitModeStr.c_str());
                        CurrentCommand++; // Move onto the next instruction.
                    }
                    else if (!(OWDApp->HaveHMDVisionTracking) && (c.WaitMode == CommandPauseUntil::dof3))
                    {
                          WriteLog("[OWDScript] PauseUntil completed on %s\n", waitModeStr.c_str());
                          CurrentCommand++; // Move onto the next instruction.
                    }
                    else if ((c.WaitUnits == WaitUnit::ms) ||
                        (c.WaitUnits == WaitUnit::s)) // check wait timeout
                    {
                        if (ovr_GetTimeInSeconds() >= c.CompletionValue) // If timeout
                        {
                            WriteLog(
                                "[OWDScript] PauseUntil %s completed on %f %s timeout\n",
                                waitModeStr.c_str(),
                                c.WaitAmount,
                                (c.WaitUnits == WaitUnit::ms) ? "ms" : "s");
                            CurrentCommand++; // Move onto the next instruction.
                         }
                    }
                    else if (c.WaitUnits == WaitUnit::frames)
                    {
                        if (OWDApp->TotalFrameCounter >= c.CompletionValue) // If timeout
                        {
                            WriteLog(
                                "[OWDScript] PauseUntil %s completed on %f frame(s) timeout\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                            CurrentCommand++; // Move onto the next instruction.
                         }
                    }

                    break;
                }

                case CommandType::ControllersPauseUntil: {
                    CommandControllersPauseUntil& c = static_cast<CommandControllersPauseUntil&>(command);
                    OVR_ASSERT(OWDApp);

                    std::string waitModeStr;
                    if (c.WaitMode == CommandControllersPauseUntil::both6dof) {
                        waitModeStr = "both6dof";
                    }
                    else if (c.WaitMode == CommandControllersPauseUntil::notboth6dof) {
                        waitModeStr = "notboth6dof";
                    }
                    else {
                        OVR_FAIL(); // should not reach here
                    }

                    if (!(c.WaitStarted)) // If the wait isn't started yet...
                    {
                        if (c.WaitUnits == WaitUnit::ms) {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + (c.WaitAmount / 1000.0));
                            WriteLog(
                                "[OWDScript] ControllersPauseUntil %s or %f ms started\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                        }
                        else if (c.WaitUnits == WaitUnit::s) {
                            c.CompletionValue = (ovr_GetTimeInSeconds() + c.WaitAmount);
                            WriteLog(
                                "[OWDScript] ControllersPauseUntil %s or %f s started\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                        }
                        else // CommandWait::frames
                        {
                            c.CompletionValue = (OWDApp->TotalFrameCounter + c.WaitAmount);
                            WriteLog(
                                "[OWDScript] ControllersPauseUntil %s or %f frame(s) started\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                        }
                        c.WaitStarted = true;
                    }

                    // check wait mode
                    if (OWDApp->HaveBothControllersVisionTracking && (c.WaitMode == CommandControllersPauseUntil::both6dof))
                    {
                        WriteLog("[OWDScript] ControllersPauseUntil completed on %s\n", waitModeStr.c_str());
                        CurrentCommand++; // Move onto the next instruction.
                    }
                    else if (!(OWDApp->HaveBothControllersVisionTracking) && (c.WaitMode == CommandControllersPauseUntil::notboth6dof))
                    {
                        WriteLog("[OWDScript] ControllersPauseUntil completed on %s\n", waitModeStr.c_str());
                        CurrentCommand++; // Move onto the next instruction.
                    }
                    else if ((c.WaitUnits == WaitUnit::ms) ||
                        (c.WaitUnits == WaitUnit::s)) // check wait timeout
                    {
                        if (ovr_GetTimeInSeconds() >= c.CompletionValue) // If timeout
                        {
                            WriteLog(
                                "[OWDScript] ControllersPauseUntil %s completed on %f %s timeout\n",
                                waitModeStr.c_str(),
                                c.WaitAmount,
                                (c.WaitUnits == WaitUnit::ms) ? "ms" : "s");
                            CurrentCommand++; // Move onto the next instruction.
                        }
                    }
                    else if (c.WaitUnits == WaitUnit::frames)
                    {
                        if (OWDApp->TotalFrameCounter >= c.CompletionValue) // If timeout
                        {
                            WriteLog(
                                "[OWDScript] ControllersPauseUntil %s completed on %f frame(s) timeout\n",
                                waitModeStr.c_str(),
                                c.WaitAmount);
                            CurrentCommand++; // Move onto the next instruction.
                        }
                    }

                    break;
                }

                case CommandType::EnableTrackingStateLog:{
                    CommandEnableTrackingStateLog& c =
                      static_cast<CommandEnableTrackingStateLog&>(command);
                    OVR_ASSERT(OWDApp);
                    OWDApp->EnableTrackingStateLog = c.enable;
                    WriteLog("[OWDScript] EnableTrackingStateLog: %s", c.enable ? "true" : "false");
                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }

                case CommandType::Screenshot:{
                    CommandScreenshot& c = static_cast<CommandScreenshot&>(command);
                    #ifdef _WIN32
                    std::string screenshotFilePath(ScriptOutputPath + c.ScreenshotFilePath);
                    OVR_ASSERT(OWDApp);
                    OVR_ASSERT(OWDApp->pRender);
                    bool success = WriteTexture(static_cast<OVR::Render::D3D11::RenderDevice*>(OWDApp->pRender)->Device, 
                        static_cast<OVR::Render::D3D11::Texture*>(OWDApp->MirrorTexture.GetPtr()), screenshotFilePath);
                    #else
                    bool success = false;
                    #endif

                    WriteLog("[OWDScript] Screenshot: Save to %s %s\n", c.ScreenshotFilePath.c_str(), success ? "succeeded" : "failed");
                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }

                case CommandType::PerfCapture:{
                    CommandPerfCapture& c = static_cast<CommandPerfCapture&>(command);

                    OVR_ASSERT(c.CaptureCollector);

                    // Prepend the script output path to the relative file path that was specified in the script.
                    // We do this here instead of earlier because we don't necessarily know the script output
                    // path at the time we loaded the script.
                    if((c.CaptureCollector->GetStatus() == PerfCaptureSerializer::Status::None) ||
                       (c.CaptureCollector->GetStatus() == PerfCaptureSerializer::Status::NotStarted))
                    {
                        std::string captureFilePath(ScriptOutputPath + c.CaptureCollector->GetFilePath());
                        c.CaptureCollector->SetFilePath(captureFilePath);
                    }

                    auto serializerStatus = c.CaptureCollector->Step(OWDApp->Session);

                    if (serializerStatus == PerfCaptureSerializer::Status::Complete)
                    {
                        WriteLog("[OWDScript] PerfCapture to %s completed\n", c.CaptureFilePath.c_str());
                        CurrentCommand++; // Move onto the next instruction.
                    }
                    else if (serializerStatus == PerfCaptureSerializer::Status::Error)
                    {
                        WriteLog("[OWDScript] PerfCapture to %s FAILED\n", c.CaptureFilePath.c_str());
                        CurrentCommand++; // Move onto the next instruction.
                    }
                    else if (!c.CaptureStartPrinted && serializerStatus == PerfCaptureSerializer::Status::Started)
                    {
                        WriteLog("[OWDScript] PerfCapture to %s started\n", c.CaptureFilePath.c_str());
                        c.CaptureStartPrinted = true;
                    }

                    break;
                }

                case CommandType::WriteLog:{
                    CommandWriteLog& c = static_cast<CommandWriteLog&>(command);
                    //WriteLog("[OWDScript] WriteLog: %s", c.LogText.c_str()); // This would be redundant.
                    WriteLog("[OWDScript] %s", c.LogText.c_str());
                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }

                case CommandType::Exit:{
                    CleanupOWDState();
                    OWDApp->Exit(Status); // The actual exit will occur in the near future.
                    WriteLog("[OWDScript] Exit: %d\n", Status);
                    CurrentCommand++; // Move onto the next instruction.
                    break;
                }
            }
        }

        if (CurrentCommand == Commands.size())
            ScriptState = State::Complete;
    }

    return ScriptState;
}


OWDScript::State OWDScript::GetState() const
{
    return ScriptState;
}








