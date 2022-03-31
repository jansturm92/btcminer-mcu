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

#include <sha256d.h>
#include <stm32_util.h>
#include <string.h>
#include <unity.h>

static void hex2bin(uint8_t *bin, const char *hex) {
    int v = 0;
    while (sscanf(hex, "%02x", &v) > 0)
        *bin++ = v, hex += 2;
}

const char block_header_222222[] =
    "02000000426f46ed1c52cf2fff79f2812628701d2a1a7817f4aa89a50402000000000000eaedc860"
    "55f8961836c6b72dcce28ca55587998c9f16bb05c8a803b36316b914e22125515c98041ab868626"
    "4";

const char block_header_555444[] =
    "000000203c9568c0d8bf0e3eec9d8893e5bfc712b5578db13b630a00000000000000000043a3cbaf"
    "c3a3213a225783362b1cc02d077d9f3b1c8259ef03346125b5d0cd66b569225cf41e3717ce88e5f"
    "9";

static void test_sha256d(void) {

    uint8_t actual[32];
    uint8_t expected[32];
    uint8_t header[80];

    char data0[] = "123456";
    sha256d(actual, (const uint8_t *)data0, strlen(data0));
    hex2bin(expected, "ff7f73b854845fc02aa13b777ac090fb1d9ebfe16c8950c7d26499371dd0b479");
    TEST_ASSERT(!memcmp(expected, actual, 32))

    ++data0[0];
    sha256d(actual, (const uint8_t *)data0, strlen(data0));
    hex2bin(expected, "ff7f73b854845fc02aa13b777ac090fb1d9ebfe16c8950c7d26499371dd0b479");
    TEST_ASSERT(memcmp(expected, actual, 32))

    hex2bin(header, block_header_222222);
    sha256d(actual, (const uint8_t *)header, sizeof(header));
    hex2bin(expected,
            "78f6e6b279b4f21f251e8ab4c411a5c0b59449b1610b9db4b800000000000000'");
    TEST_ASSERT(!memcmp(expected, actual, 32))

    hex2bin(header, block_header_555444);
    sha256d(actual, (const uint8_t *)header, sizeof(header));
    hex2bin(expected,
            "42467b7f54df869a0d750b4fb69fa97f12e3c551d4fc16000000000000000000'");
    TEST_ASSERT(!memcmp(expected, actual, 32))
}

void setUp(void) {}
void tearDown(void) {}

int main(void) {

    stm32_board_setup();
    sleep(STM32_UNITTEST_INITIAL_SLEEP_DURATION);

    UNITY_BEGIN();

    RUN_TEST(test_sha256d);

    UNITY_END();

    while (1) {
    }
}
