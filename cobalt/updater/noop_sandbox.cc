// Copyright 2020 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// This is a test app for Evergreen that does nothing.

#include "base/time/time.h"
#include "starboard/event.h"
#include "starboard/system.h"
#include "starboard/thread.h"

void SbEventHandle(const SbEvent* event) {
  // No-op app. Exit after 1s.
  SbThreadSleep(1 * base::Time::kMicrosecondsPerSecond);
  SbSystemRequestStop(0);
}
