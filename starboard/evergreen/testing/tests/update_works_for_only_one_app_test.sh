#!/bin/bash
#
# Copyright 2020 The Cobalt Authors. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Unset the previous test's name and runner function.
unset TEST_NAME
unset TEST_FILE
unset -f run_test

TEST_NAME="UpdateOnlyWorksForOneApp"
TEST_FILE="test.html"

function run_test() {
  clear_storage

  start_cobalt "file:///tests/${TEST_FILE}?channel=test" "${TEST_NAME}.0.log" "update from test channel installed"

  if [[ $? -ne 0 ]]; then
    error "Failed to download and install the test package"
    return 1
  fi

  start_cobalt "file:///tests/${TEST_FILE}?channel=test" "${TEST_NAME}.1.log" "App is up to date"

  if [[ $? -ne 0 ]]; then
    error "Failed to run the downloaded installation"
    return 1
  fi

  start_cobalt "file:///tests/empty.html" "${TEST_NAME}.2.log" "quick update succeeded"

  if [[ $? -ne 0 ]]; then
    error "Failed to perform a quick update on different app"
    return 1
  fi

  FILENAME="$(get_bad_app_key_file_path "${TEST_NAME}.2.log")"

  if [[ -z "${FILENAME}" ]]; then
    error "Failed to find the bad app key file path"
    return 1
  fi

  create_file "${FILENAME}"

  start_cobalt "file:///tests/empty.html" "${TEST_NAME}.3.log" "RevertBack current_installation="

  if [[ $? -ne 0 ]]; then
    error "Failed to revert when the app's bad file exists"
    return 1
  fi

  return 0
}
