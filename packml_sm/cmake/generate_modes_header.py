#!/usr/bin/env python3
# Copyright (c) 2024 PackML Contributors
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

"""
Generate a C++ header file with PackML mode constants from a YAML definition file.

Usage: generate_modes_header.py <yaml_file> <output_header>

The YAML file should have the following format:
  modes:
    Invalid: 0
    Production: 1
    Maintenance: 2

The generated header will define constants in the packml_modes namespace:
  namespace packml_modes {
    constexpr packml_sm::ModeType Invalid = 0;
    constexpr packml_sm::ModeType Production = 1;
    constexpr packml_sm::ModeType Maintenance = 2;
  }  // namespace packml_modes
"""

import os
import sys


def generate_modes_header(yaml_file, output_file):
    try:
        import yaml
    except ImportError:
        # Minimal YAML parser for simple key: value maps
        yaml = None

    with open(yaml_file, 'r') as f:
        content = f.read()

    if yaml is not None:
        data = yaml.safe_load(content)
    else:
        # Fallback: parse simple "modes:\n  Name: value" format manually
        data = {'modes': {}}
        in_modes = False
        for line in content.splitlines():
            stripped = line.strip()
            if stripped == 'modes:':
                in_modes = True
                continue
            if in_modes and stripped and not stripped.startswith('#'):
                if ':' in stripped:
                    name, value = stripped.split(':', 1)
                    try:
                        data['modes'][name.strip()] = int(value.strip())
                    except ValueError:
                        pass

    modes = data.get('modes', {})
    if not modes:
        raise ValueError(f"No 'modes' key found in YAML file: {yaml_file}")

    os.makedirs(os.path.dirname(os.path.abspath(output_file)), exist_ok=True)

    header_guard = os.path.basename(output_file).upper().replace('.', '_').replace('-', '_')

    with open(output_file, 'w') as f:
        f.write(f'// Auto-generated from {os.path.basename(yaml_file)} -- do not edit manually\n')
        f.write(f'#ifndef {header_guard}_\n')
        f.write(f'#define {header_guard}_\n\n')
        f.write('#include "packml_sm/common.hpp"\n\n')
        f.write('namespace packml_modes {\n\n')
        for name, value in modes.items():
            f.write(f'constexpr packml_sm::ModeType {name} = {value};\n')
        f.write('\n}  // namespace packml_modes\n\n')
        f.write(f'#endif  // {header_guard}_\n')


if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f'Usage: {sys.argv[0]} <yaml_file> <output_header>', file=sys.stderr)
        sys.exit(1)
    generate_modes_header(sys.argv[1], sys.argv[2])
