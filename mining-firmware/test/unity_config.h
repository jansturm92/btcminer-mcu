// Copyright (C) 2022 Jan Sturm
//
// This program is free software: you can redistribute it and/or modify it under
// the terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or FITNESS
// FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef MINING_FIRMWARE_TEST_UNITY_CONFIG_H
#define MINING_FIRMWARE_TEST_UNITY_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

void unity_output_start(void);
void unity_output_char(char c);
void unity_output_flush(void);
void unity_output_complete(void);

#define UNITY_OUTPUT_START() unity_output_start()
#define UNITY_OUTPUT_CHAR(c) unity_output_char(c)
#define UNITY_OUTPUT_FLUSH() unity_output_flush()
#define UNITY_OUTPUT_COMPLETE() unity_output_complete()

#ifdef __cplusplus
}
#endif

#endif // MINING_FIRMWARE_TEST_UNITY_CONFIG_H
