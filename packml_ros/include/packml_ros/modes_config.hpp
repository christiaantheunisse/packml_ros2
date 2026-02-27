// Copyright (c) 2024 PackML Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <iostream>
#include <map>
#include <string>

#include <yaml-cpp/yaml.h>

#include "packml_sm/common.hpp"

namespace packml_ros {

/// Parse a modes YAML configuration file and return per-mode state masks.
///
/// The YAML file must contain a top-level "modes" mapping (name → int value)
/// and an optional "state_masks" mapping (name → state_name → bool).
/// States not listed under a mode default to true (available).
///
/// @param yaml_file  Path to the YAML configuration file.
/// @return           Map from ModeType value to AvailableStates.  Returns an
///                   empty map if the file cannot be opened or has no masks.
inline std::map<packml_sm::ModeType, packml_sm::AvailableStates>
parse_modes_config(const std::string & yaml_file)
{
  std::map<packml_sm::ModeType, packml_sm::AvailableStates> result;

  // Build the complete map of PackML state names → State enum values.
  const std::map<std::string, packml_sm::State> state_name_map = {
    {"ABORTING",     packml_sm::State::ABORTING},
    {"ABORTED",      packml_sm::State::ABORTED},
    {"CLEARING",     packml_sm::State::CLEARING},
    {"STOPPING",     packml_sm::State::STOPPING},
    {"STOPPED",      packml_sm::State::STOPPED},
    {"RESETTING",    packml_sm::State::RESETTING},
    {"IDLE",         packml_sm::State::IDLE},
    {"STARTING",     packml_sm::State::STARTING},
    {"EXECUTE",      packml_sm::State::EXECUTE},
    {"HOLDING",      packml_sm::State::HOLDING},
    {"HELD",         packml_sm::State::HELD},
    {"UNHOLDING",    packml_sm::State::UNHOLDING},
    {"SUSPENDING",   packml_sm::State::SUSPENDING},
    {"SUSPENDED",    packml_sm::State::SUSPENDED},
    {"UNSUSPENDING", packml_sm::State::UNSUSPENDING},
    {"COMPLETING",   packml_sm::State::COMPLETING},
    {"COMPLETE",     packml_sm::State::COMPLETE},
  };

  // All-true defaults used when a state is not listed under a mode mask.
  packml_sm::AvailableStates all_true;
  for (const auto & [name, state] : state_name_map) {
    all_true[state] = true;
  }

  YAML::Node root;
  try {
    root = YAML::LoadFile(yaml_file);
  } catch (const YAML::Exception & e) {
    std::cerr << "[modes_config] Failed to load YAML file '" << yaml_file
              << "': " << e.what() << std::endl;
    return result;
  }

  // Parse mode names → integer values.
  std::map<std::string, packml_sm::ModeType> mode_values;
  if (root["modes"] && root["modes"].IsMap()) {
    for (const auto & entry : root["modes"]) {
      try {
        mode_values[entry.first.as<std::string>()] =
          entry.second.as<packml_sm::ModeType>();
      } catch (const YAML::Exception & e) {
        std::cerr << "[modes_config] Failed to parse mode value for '"
                  << entry.first.as<std::string>() << "': " << e.what() << std::endl;
      }
    }
  }

  // Parse per-mode state masks.
  if (!root["state_masks"] || !root["state_masks"].IsMap()) {
    std::cerr << "[modes_config] No 'state_masks' section found in '" << yaml_file
              << "', all states will default to available" << std::endl;
    return result;
  }

  for (const auto & mode_entry : root["state_masks"]) {
    const std::string mode_name = mode_entry.first.as<std::string>();
    auto mode_it = mode_values.find(mode_name);
    if (mode_it == mode_values.end()) {
      std::cerr << "[modes_config] state_masks entry '" << mode_name
                << "' has no matching entry in 'modes', skipping" << std::endl;
      continue;
    }

    // Start from all-true defaults so unlisted states remain available.
    packml_sm::AvailableStates avail = all_true;

    if (mode_entry.second && mode_entry.second.IsMap()) {
      for (const auto & state_entry : mode_entry.second) {
        const std::string state_name = state_entry.first.as<std::string>();
        auto state_it = state_name_map.find(state_name);
        if (state_it == state_name_map.end()) {
          std::cerr << "[modes_config] Unknown state name '" << state_name
                    << "' in mask for mode '" << mode_name << "', skipping" << std::endl;
          continue;
        }
        try {
          avail[state_it->second] = state_entry.second.as<bool>();
        } catch (const YAML::Exception & e) {
          std::cerr << "[modes_config] Invalid boolean value for state '"
                    << state_name << "' in mode '" << mode_name
                    << "': " << e.what() << ", defaulting to true" << std::endl;
        }
      }
    }

    result[mode_it->second] = avail;
  }

  return result;
}

}  // namespace packml_ros
