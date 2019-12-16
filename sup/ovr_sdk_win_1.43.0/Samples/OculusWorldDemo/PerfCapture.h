/************************************************************************************
  Created     :   Jun 25, 2018
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

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <ostream>
#include <istream>

#include "OVR_CAPI.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Implements Oculus performance stats capture serializer
//
// --Serialized format--
// The serialized file format is simply a memory dump of std::vector<ovrPerfStats> to disk. 
// It is written as if like so:
//     file.write(statsVector.data(), statsVector.size() * sizeof(ovrPerfStats));
////////////////////////////////////////////////////////////////////////////////////////////////////

class PerfCaptureSerializer {
 public:
  enum class Units { 
    none,     // None specified
    ms,       // Milliseconds
    s,        // Seconds
    frames    // Frames
  };

  enum class Status {
    None, // No state set yet
    Error, // Failure to start capture
    NotStarted, // PerfCapture hasn't started
    Started, // PerfCapture has started at least one step.
    Complete // PerfCapture has completed.
  };

  PerfCaptureSerializer() 
    : PerfCaptureSerializer(0.0, Units::none, std::string()) {}

  PerfCaptureSerializer(double totalDuration, Units units, const std::string& filePath);

  ~PerfCaptureSerializer();

  void Reset();

  // Executes PerfCapture one step at a time. Call this periodically. At least once per frame.
  Status Step(ovrSession session);

  Status GetStatus() const {
    return CurrentStatus;
  }

  std::string GetFilePath() const {
    return FilePath;
  }

  void SetFilePath(const std::string& filePath) {
    FilePath = filePath;
  }

 protected:
  void StartCapture();
  void EndCapture();
  void SerializeBuffer();

protected:
  std::string FilePath;
  Units DurationUnits;
  double TotalDuration;

  double CompletionValue;
  Status CurrentStatus;

  const int NumBufferedStatsReserve = 16384; // at 90 Hz, enough to hold several minutes worth
  const int NumBufferedStatsSerializeTrigger = NumBufferedStatsReserve - 100;

  // Might benefit from a lockless buffer if the synchronous writer stalls things.
  std::vector<ovrPerfStats> BufferedPerfStats;

  std::ofstream SerializedFile;
  int CurrentFrame;
};

