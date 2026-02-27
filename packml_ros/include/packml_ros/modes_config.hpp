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

#include <fstream>
#include <map>
#include <string>

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

  std::ifstream f(yaml_file);
  if (!f.is_open()) {
    std::cerr << "[modes_config] Cannot open modes config file: " << yaml_file << std::endl;
    return result;
  }

  // Build the complete map of PackML state names → State enum values.
  const std::map<std::string, packml_sm::State> state_name_map = {
    {"ABORTING",    packml_sm::State::ABORTING},
    {"ABORTED",     packml_sm::State::ABORTED},
    {"CLEARING",    packml_sm::State::CLEARING},
    {"STOPPING",    packml_sm::State::STOPPING},
    {"STOPPED",     packml_sm::State::STOPPED},
    {"RESETTING",   packml_sm::State::RESETTING},
    {"IDLE",        packml_sm::State::IDLE},
    {"STARTING",    packml_sm::State::STARTING},
    {"EXECUTE",     packml_sm::State::EXECUTE},
    {"HOLDING",     packml_sm::State::HOLDING},
    {"HELD",        packml_sm::State::HELD},
    {"UNHOLDING",   packml_sm::State::UNHOLDING},
    {"SUSPENDING",  packml_sm::State::SUSPENDING},
    {"SUSPENDED",   packml_sm::State::SUSPENDED},
    {"UNSUSPENDING", packml_sm::State::UNSUSPENDING},
    {"COMPLETING",  packml_sm::State::COMPLETING},
    {"COMPLETE",    packml_sm::State::COMPLETE},
  };

  // All-true defaults.
  packml_sm::AvailableStates all_true;
  for (const auto & [name, state] : state_name_map) {
    all_true[state] = true;
  }

  // --- Minimal line-by-line YAML parser ---
  // Tracks indentation depth:
  //   indent 0  → top-level section key ("modes:" / "state_masks:")
  //   indent 2  → mode name key under a section
  //   indent 4  → state name key under a mode name
  enum class Section { NONE, MODES, STATE_MASKS };

  std::map<std::string, packml_sm::ModeType> mode_values;  // name → int
  // name → (state → bool)
  std::map<std::string, packml_sm::AvailableStates> raw_masks;

  Section section = Section::NONE;
  std::string current_mode_name;

  auto trim = [](const std::string & s) -> std::string {
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
  };

  auto indent_of = [](const std::string & s) -> int {
    int n = 0;
    for (char c : s) {
      if (c == ' ') ++n;
      else if (c == '\t') n += 2;
      else break;
    }
    return n;
  };

  std::string line;
  while (std::getline(f, line)) {
    // Strip comment
    auto comment_pos = line.find('#');
    if (comment_pos != std::string::npos) {
      line = line.substr(0, comment_pos);
    }
    if (trim(line).empty()) continue;

    int indent = indent_of(line);
    std::string content = trim(line);

    // Find colon separator
    auto colon = content.find(':');
    if (colon == std::string::npos) continue;

    std::string key = trim(content.substr(0, colon));
    std::string val = trim(content.substr(colon + 1));

    if (indent == 0) {
      if (key == "modes") {
        section = Section::MODES;
      } else if (key == "state_masks") {
        section = Section::STATE_MASKS;
      } else {
        section = Section::NONE;
      }
      current_mode_name.clear();
      continue;
    }

    if (indent == 2) {
      if (section == Section::MODES) {
        if (!val.empty()) {
          try {
            mode_values[key] = std::stoi(val);
          } catch (const std::exception & e) {
            std::cerr << "[modes_config] Failed to parse mode value for '" << key
                      << "': " << e.what() << std::endl;
          }
        }
      } else if (section == Section::STATE_MASKS) {
        current_mode_name = key;
        // Initialise with all-true defaults so unlisted states still work.
        if (raw_masks.find(current_mode_name) == raw_masks.end()) {
          raw_masks[current_mode_name] = all_true;
        }
      }
      continue;
    }

    if (indent == 4) {
      if (section == Section::STATE_MASKS && !current_mode_name.empty()) {
        auto it = state_name_map.find(key);
        if (it != state_name_map.end()) {
          if (val == "true" || val == "1" || val == "yes") {
            raw_masks[current_mode_name][it->second] = true;
          } else if (val == "false" || val == "0" || val == "no") {
            raw_masks[current_mode_name][it->second] = false;
          } else {
            std::cerr << "[modes_config] Unrecognised boolean value '" << val
                      << "' for state '" << key << "' in mode '" << current_mode_name
                      << "', defaulting to true" << std::endl;
            raw_masks[current_mode_name][it->second] = true;
          }
        }
      }
      continue;
    }
  }

  // Combine modes + raw_masks → result (indexed by ModeType value).
  for (const auto & [mode_name, avail] : raw_masks) {
    auto it = mode_values.find(mode_name);
    if (it != mode_values.end()) {
      result[it->second] = avail;
    }
  }

  return result;
}

}  // namespace packml_ros
