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


#ifndef OWD_SCRIPT_h
#define OWD_SCRIPT_h


#include <string>
#include <stack>
#include <memory>
#include <vector>
#include "PerfCapture.h"

class OculusWorldDemoApp;


// Implements a basic command script interpreter. Scripts are merely a linear list of commands.
// There currently is no concept of variables (except a single numerical status), looping, 
// functions, etc. Scripts are text files or strings which are one one line per statement.
//    Command           args                                                       Notes
//    -----------------------------------------------------------------------------------------------------------------------
//    ExecuteMenu       <menu path string> <menu value>                            Executes a menu item by string path. Quoted strings as-needed. enum menu values can use the string or index.
//    ExecuteScriptFile <script file path string>                                  Recursively executes another script. If path is relative, then it's relative to parent script path.
//    SetPose           [<x pos> <y pos> <z pos> <x ori> <y ori> <z ori>] | [off]  Sets the HMD pose meters and degrees, or stops pose control and goes back to HMD-based poses.
//    Wait              <count> [ms|s|frame|frames]                                Pauses the script until N ms or frames.
//    PauseUntil        <6dof|3dof> <count> [ms|s|frame|frames]                    Pauses the script until either the HMD tracking mode(6dof or 3dof) or timeout(N ms or frames).
//    ControllersPauseUntil     <both6dof|notboth6dof> <count> [ms|s|frame|frames] Pauses the script until either both controllers tracking mode(6dof or not) or timeout(N ms or frames).
//    EnableTrackingStateLog    <true|false>                                       Logs to stdout whenever there is a tracked state change and logs the time.
//    PerfCapture       <count> [ms|s|frame|frames] <output file path>             Captures performance stats for specified duration and serializes them out to specified file.
//    Screenshot        <output file path>                                         Saves a mirror screenshot to disk. Currently limited to .bmp format.
//    WriteLog          <string>                                                   Writes to the OWD log.
//    Exit                                                                         Exits the process with a status code that reflects the script execution state: 0 or -1.
//    //                <comment>                                                  Specifies a comment line
//
// A wait for 1 frame results in the script continuing on the next frame (and not the 
// next script step). Thus a one frame wait is like a single frame no-op.
//
// In addition to built in functions, there is some useful scripting functionality available from 
// the OWD menu system. For example, the "ExecuteMenu Scene Content.Animation Enabled.<true/false>" 
// menu command can enable or disable animation, which is important for a predicable scene state.
// The OWD menu system provides a menu option to trace all available menu options. Note that for 
// trigger options, you can provide any value as the final menu path component, for example:
// "ExecuteMenu Scene.StartTiming.anything" will execute the Scene.StartTiming trigger.
// 
// Example command line usage:
//     C:> OculusWorldDemo.exe -scriptfile "C:\SomeDir\SomeScript.txt"
//
// Example programmatic usage:
//    OWDScript script;
//
//    script.LoadScript("SetPose 0, 0, 0, 90\n"
//                      "Wait 10.5 ms\n"
//                      "\"ExecuteMenu Scene Content.Draw Repeat.Low count\" 100\n"
//                      "// Save a screenshot for comparison:\n"
//                      "Screenshot owd.drawrepeat.100.bmp\n"
//                      "Exit");
//    void AppTick() {
//        ovrScript.Step();
//        ovr_WaitFrame();
//        ovr_BeginFrame();
//        ovr_EndFrame();
//    }

class OWDScript
{
public:
    OWDScript(OculusWorldDemoApp* owdApp);
   ~OWDScript() = default;

    enum class State {
        None,       // Script is not loaded.
        NotStarted, // Script loaded but not yet stepped.
        Started,    // Script has started at least one step.
        Complete    // Script has completed all steps.
    };

    // Must call shutdown first if you want to call while a script is already loaded.
    bool LoadScriptFile(const char* filePath);
    bool LoadScriptText(const char* scriptText);

    // Specifies a base diretory path to which script output (e.g. Screenshot) is written.
    bool SetScriptOutputPath(const char* directoryPath);

    // Returns the state to newly constructed state, which sets the interpreter to a state
    // ready to load a new script. Leaves the OculusWorldDemoApp member as-is.
    void Shutdown();

    // Executes the script interpreter one step at a time. Call this 
    // function periodically. At least once per frame.
    State Step();

    State GetState() const;

protected:
    enum class CommandType {
        ExecuteMenu,
        ExecuteScriptFile,
        SetPose,
        Wait,
        PauseUntil,
        ControllersPauseUntil,
        EnableTrackingStateLog,
        Screenshot,
        WriteLog,
        PerfCapture,
        Exit
    };

    enum class WaitUnit { none, ms, s, frames };

    struct Command {
        CommandType commandType;
    };

    struct CommandExecuteMenu : public Command {
        CommandExecuteMenu() : Command{CommandType::ExecuteMenu}{}
        std::string MenuPath;  // in a form like "parent.child.size"
        std::string MenuValue; // in a form like "100.23"
    };

    struct CommandExecuteScriptFile : public Command {
        CommandExecuteScriptFile() : Command{CommandType::ExecuteScriptFile}{}
        std::string ScriptFilePath;
    };

    struct CommandSetPose : public Command {
        CommandSetPose() : Command{CommandType::SetPose}, X(), Y(), Z(), OriX{}, OriY{}, OriZ{}, stop(false){}
        float X, Y, Z, OriX, OriY, OriZ;
        bool stop;
    };

    struct CommandWait : public Command {
        CommandWait() : Command{CommandType::Wait}, WaitAmount(0.0), WaitUnits(WaitUnit::none), CompletionValue(0.0){}
        double WaitAmount; // Relative amount.
        WaitUnit WaitUnits;
        
        // State data
        double CompletionValue;
    };

    struct CommandPauseUntil : public Command {
        CommandPauseUntil()
          : Command{CommandType::PauseUntil}
          , WaitMode(dof6)
          , WaitAmount(0.0)
          , WaitUnits(WaitUnit::none)
          , WaitStarted(false)
          , CompletionValue(0.0) {}

        enum { dof6, dof3 } WaitMode;
        double WaitAmount; // Relative amount.
        WaitUnit WaitUnits;

        // State data
        bool WaitStarted;
        double CompletionValue;
    };

    struct CommandControllersPauseUntil : public Command {
        CommandControllersPauseUntil()
            : Command{ CommandType::ControllersPauseUntil }
            , WaitMode(both6dof)
            , WaitAmount(0.0)
            , WaitUnits(WaitUnit::none)
            , WaitStarted(false)
            , CompletionValue(0.0) {}

        enum { both6dof, notboth6dof } WaitMode;
        double WaitAmount; // Relative amount.
        WaitUnit WaitUnits;

        // State data
        bool WaitStarted;
        double CompletionValue;
    };

    struct CommandEnableTrackingStateLog : public Command {
        CommandEnableTrackingStateLog() : Command{CommandType::EnableTrackingStateLog}, enable(false){}
        bool enable;
    };

    struct CommandScreenshot : public Command {
        CommandScreenshot() : Command{CommandType::Screenshot}{}
        std::string ScreenshotFilePath;
    };

    struct CommandPerfCapture : public Command {
      CommandPerfCapture()
        : Command{ CommandType::PerfCapture }
        , CaptureTotalDuration(0.0)
        , CaptureDurationUnits(PerfCaptureSerializer::Units::none)
        , CaptureStartPrinted(false)
        , CaptureCollector() {}

      double CaptureTotalDuration;
      PerfCaptureSerializer::Units CaptureDurationUnits;
      std::string CaptureFilePath;

      bool CaptureStartPrinted;
      std::shared_ptr<PerfCaptureSerializer> CaptureCollector;
    };

    struct CommandWriteLog : public Command {
        CommandWriteLog() : Command{CommandType::WriteLog}{}
        std::string LogText;
    };

    struct CommandExit : public Command {
        CommandExit() : Command{CommandType::Exit}{}
    };

    typedef std::shared_ptr<Command> CommandPtr;
    typedef std::vector<CommandPtr> CommandVector;

    bool LoadScriptText(const char* scriptText, CommandVector& commands);
    bool LoadScriptFile(const char* filePath, CommandVector& commands);
    bool ReadCommandLine(const std::string& line, CommandVector& commands);
    void CleanupOWDState();

protected:
    std::string ScriptFile; // May be empty if the script didn't originate from a file.
    std::string ScriptOutputPath; // The base directory to where images, etc. are written.
    State ScriptState;
    CommandVector Commands;
    size_t CurrentCommand;  // Index into Commands.
    int32_t Status; // If the Exit command is called then the exit code is set to this status.
    OculusWorldDemoApp* OWDApp;
};


#endif // OWD_SCRIPT_h



