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

#include "PerfCapture.h"
#include "Extras/OVR_Math.h"
#include "Kernel/OVR_System.h"
#include <iosfwd>
#include <sstream>
#include <string>
#include <cerrno>

//////////////////////////////////////////////////////////////////////////
/// PerfCaptureSerializer
//////////////////////////////////////////////////////////////////////////

PerfCaptureSerializer::PerfCaptureSerializer(double totalDuration, Units units, const std::string& filePath)
  : FilePath(filePath),
    DurationUnits(units),
    TotalDuration(totalDuration),
    CompletionValue(0.0),
    CurrentStatus(Status::None),
    BufferedPerfStats(),
    SerializedFile(),
    CurrentFrame(0) 
{}

PerfCaptureSerializer::~PerfCaptureSerializer() {
  Reset(); // This will EndCapture if needed.
}

void PerfCaptureSerializer::Reset() {
  if (CurrentStatus == Status::Started) {
    EndCapture();
  }

  // Return to the newly constructed state.
  FilePath.clear();
  DurationUnits = Units::none;
  TotalDuration = 0;
  CompletionValue = 0.0;
  CurrentStatus = Status::None;
  BufferedPerfStats.clear();

  if (SerializedFile.is_open()) {
    SerializedFile.close();
    SerializedFile.clear();
  }

  CurrentFrame = 0;
}

void PerfCaptureSerializer::SerializeBuffer() {
  if (CurrentStatus == Status::Started) {
    char* byteData = reinterpret_cast<char*>(&BufferedPerfStats[0]);
    size_t dataSize = BufferedPerfStats.size() * sizeof(ovrPerfStats);
    SerializedFile.write(byteData, dataSize);
  } else {
    OVR_FAIL();
  }

  // clear it out regardless to avoid memory bloat
  BufferedPerfStats.clear();
}

void PerfCaptureSerializer::StartCapture() {
  bool success = false;
  try {
    // create/open file
    SerializedFile.open(FilePath, std::ios::out | std::ios::binary);

    // ready buffer vector
    BufferedPerfStats.clear();
    BufferedPerfStats.reserve(NumBufferedStatsReserve);
    success = true;
  } catch (std::ios_base::failure&) {
    OVR_FAIL();
    success = false;
  }

  CurrentStatus = (success ? Status::Started : Status::Error);
}

void PerfCaptureSerializer::EndCapture() {
  if (CurrentStatus == Status::Started) {
    if (BufferedPerfStats.size() > 0) {
      SerializeBuffer();
    }

    CurrentStatus = Status::Complete;
  } else {
    CurrentStatus = Status::Error; // called end before start
  }

  if (SerializedFile.is_open()) {
    SerializedFile.close();
  }
}

PerfCaptureSerializer::Status PerfCaptureSerializer::Step(ovrSession session) {
  if (CurrentStatus == Status::None) {
    StartCapture();
  }

  if (CurrentStatus == Status::Started) {
    ovrPerfStats perfStats = {};
    ovr_GetPerfStats(session, &perfStats);

    if (perfStats.FrameStatsCount > 0) {
      CurrentFrame = perfStats.FrameStats[0].AppFrameIndex;
    }

    bool timeIsUp = false;

    // make sure we got some stats before we start
    if (CurrentFrame > 0) {
      BufferedPerfStats.push_back(perfStats);

      if (CompletionValue == 0) // If the capture hasn't started yet...
      {
        if (DurationUnits == PerfCaptureSerializer::Units::ms) {
          CompletionValue = (ovr_GetTimeInSeconds() + (TotalDuration / 1000));
        } else if (DurationUnits == PerfCaptureSerializer::Units::s) {
          CompletionValue = (ovr_GetTimeInSeconds() + TotalDuration);
        } else if (DurationUnits == PerfCaptureSerializer::Units::frames) {
          CompletionValue = (CurrentFrame + TotalDuration);
        } else {
          OVR_FAIL();
        }
      }

      if ((DurationUnits == PerfCaptureSerializer::Units::ms) ||
          (DurationUnits == PerfCaptureSerializer::Units::s)) {
        if (ovr_GetTimeInSeconds() >= CompletionValue) // If done
        {
          timeIsUp = true;
        }
      } else // PerfCaptureSerializer::Units::frames
      {
        if (CurrentFrame >= CompletionValue) // If done
        {
          timeIsUp = true;
        }
      }
    }

    if (timeIsUp) {
      EndCapture();
    } else {
      bool shouldSerialize = (int)BufferedPerfStats.size() > NumBufferedStatsSerializeTrigger;
      if (shouldSerialize) {
        SerializeBuffer();
      }
    }
  }

  return CurrentStatus;
}

