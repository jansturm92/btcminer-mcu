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

#include <libopencm3_util.h>
#include <sha256d.h>
#include <string.h>
#include <unity.h>

// simple workaround to avoid linking issues
void board_read_data(const char *buf, const int len) {
    (void)buf;
    (void)len;
}

static void hex2bin(uint8_t *bin, const char *hex) {
    int v = 0;
    while (sscanf(hex, "%02x", &v) > 0)
        *bin++ = v, hex += 2;
}

const char block_header_222222[] =
    "02000000426f46ed1c52cf2fff79f2812628701d2a1a7817f4aa89a50402000000000000eaedc860"
    "55f8961836c6b72dcce28ca55587998c9f16bb05c8a803b36316b914e22125515c98041ab868626"
    "4";

const char midstate_data_222222[] = "167ff5ad63ab786ce8fcb09136fff458ea016749b643beff9b0f"
                                    "750b5197565114b91663512521e21a04985c646268b8";

const char block_header_555444[] =
    "000000203c9568c0d8bf0e3eec9d8893e5bfc712b5578db13b630a00000000000000000043a3cbaf"
    "c3a3213a225783362b1cc02d077d9f3b1c8259ef03346125b5d0cd66b569225cf41e3717ce88e5f"
    "9";

const char midstate_data_555444[] = "48ba5d2cb31d73fa8194633412f1d424d2abb6ef7ef99e1f2877"
                                    "58f582a839ef66cdd0b55c2269b517371ef4f9e588ce";

static void test_sha256d(void) {

    uint8_t actual[32];
    uint8_t expected[32];
    uint8_t header[80];

    char data0[] = "123456";
    sha256d(actual, (const uint8_t *)data0, strlen(data0));
    hex2bin(expected, "ff7f73b854845fc02aa13b777ac090fb1d9ebfe16c8950c7d26499371dd0b479");
    TEST_ASSERT(!memcmp(expected, actual, 32));

    ++data0[0];
    sha256d(actual, (const uint8_t *)data0, strlen(data0));
    hex2bin(expected, "ff7f73b854845fc02aa13b777ac090fb1d9ebfe16c8950c7d26499371dd0b479");
    TEST_ASSERT(memcmp(expected, actual, 32));

    hex2bin(header, block_header_222222);
    sha256d(actual, (const uint8_t *)header, sizeof(header));
    hex2bin(expected,
            "78f6e6b279b4f21f251e8ab4c411a5c0b59449b1610b9db4b800000000000000'");
    TEST_ASSERT(!memcmp(expected, actual, 32));

    hex2bin(header, block_header_555444);
    sha256d(actual, (const uint8_t *)header, sizeof(header));
    hex2bin(expected,
            "42467b7f54df869a0d750b4fb69fa97f12e3c551d4fc16000000000000000000'");
    TEST_ASSERT(!memcmp(expected, actual, 32));
}

static void test_sha256d_ms(void) {

    SHA256D_MS_CTX ctx;
    uint32_t *nonce = &(ctx.data[3]);
    uint8_t buf[96];
    uint8_t hash[32];

    hex2bin(buf, midstate_data_222222);
    sha256d_ms_init(&ctx, (uint32_t *)buf);
    sha256d_ms((uint32_t *)hash, &ctx);
    TEST_ASSERT_EQUAL(0, *(uint32_t *)(hash + 28));

    hex2bin(buf, midstate_data_555444);
    sha256d_ms_init(&ctx, (uint32_t *)buf);
    sha256d_ms((uint32_t *)hash, &ctx);
    TEST_ASSERT_EQUAL(0, *(uint32_t *)(hash + 28));

    ++(*nonce);
    sha256d_ms((uint32_t *)hash, &ctx);
    TEST_ASSERT_NOT_EQUAL(0, *(uint32_t *)(hash + 28));
}

void setUp(void) {

#ifdef LIBOPENCM3
    libopencm3_board_setup(NULL, 0);
#endif
    sleep(UNITTEST_INITIAL_SLEEP_DURATION);
}
void tearDown(void) {}

int main(void) {

    UNITY_BEGIN();

    RUN_TEST(test_sha256d);
    RUN_TEST(test_sha256d_ms);

    return UNITY_END();
}
