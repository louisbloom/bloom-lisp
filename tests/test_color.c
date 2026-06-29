/* test_color.c - RGB ↔ 256 ↔ 16 ↔ 8 color conversion tests */

#include "flare_testkit.h"
#include <ditty/highlight.h>

static void test_rgb_to_256_pure_red(void)
{
    ASSERT_EQ(flare_color_rgb_to_256(255, 0, 0), 196);
}

static void test_rgb_to_256_pure_green(void)
{
    ASSERT_EQ(flare_color_rgb_to_256(0, 255, 0), 46);
}

static void test_rgb_to_256_pure_blue(void)
{
    ASSERT_EQ(flare_color_rgb_to_256(0, 0, 255), 21);
}

static void test_rgb_to_256_black(void)
{
    ASSERT_EQ(flare_color_rgb_to_256(0, 0, 0), 16);
}

static void test_rgb_to_256_white(void)
{
    ASSERT_EQ(flare_color_rgb_to_256(255, 255, 255), 231);
}

static void test_rgb_to_256_grayscale(void)
{
    int idx = flare_color_rgb_to_256(128, 128, 128);
    ASSERT_TRUE(idx >= 232 && idx <= 255);
}

static void test_rgb_to_256_idempotent_cube(void)
{
    int cube_idx = flare_color_rgb_to_256(255, 0, 0);
    uint8_t r, g, b;
    flare_color_256_to_rgb(cube_idx, &r, &g, &b);
    int back = flare_color_rgb_to_256(r, g, b);
    ASSERT_EQ(back, cube_idx);
}

static void test_rgb_to_8_pure_red(void)
{
    ASSERT_EQ(flare_color_rgb_to_8(255, 0, 0), 1);
}

static void test_rgb_to_8_pure_green(void)
{
    ASSERT_EQ(flare_color_rgb_to_8(0, 255, 0), 2);
}

static void test_rgb_to_8_black(void)
{
    ASSERT_EQ(flare_color_rgb_to_8(0, 0, 0), 0);
}

static void test_rgb_to_8_white(void)
{
    ASSERT_EQ(flare_color_rgb_to_8(255, 255, 255), 7);
}

static void test_rgb_to_8_dark_gray(void)
{
    ASSERT_EQ(flare_color_rgb_to_8(64, 64, 64), 0);
}

static void test_rgb_to_8_bright_red(void)
{
    ASSERT_EQ(flare_color_rgb_to_8(255, 80, 80), 1);
}

static void test_rgb_to_16_pure_red(void)
{
    ASSERT_EQ(flare_color_rgb_to_16(255, 0, 0), 9);
}

static void test_rgb_to_16_dark_red(void)
{
    ASSERT_EQ(flare_color_rgb_to_16(128, 0, 0), 1);
}

static void test_rgb_to_16_pure_white(void)
{
    ASSERT_EQ(flare_color_rgb_to_16(255, 255, 255), 15);
}

static void test_rgb_to_16_black(void)
{
    ASSERT_EQ(flare_color_rgb_to_16(0, 0, 0), 0);
}

static void test_rgb_to_16_gray(void)
{
    int idx = flare_color_rgb_to_16(128, 128, 128);
    ASSERT_TRUE(idx == 8);
}

static void test_rgb_to_16_bright_green(void)
{
    ASSERT_EQ(flare_color_rgb_to_16(0, 255, 0), 10);
}

static void test_rgb_to_16_dark_green(void)
{
    ASSERT_EQ(flare_color_rgb_to_16(0, 128, 0), 2);
}

static void test_color_256_to_rgb_cube(void)
{
    uint8_t r, g, b;
    flare_color_256_to_rgb(196, &r, &g, &b);
    ASSERT_EQ(r, 255);
    ASSERT_EQ(g, 0);
    ASSERT_EQ(b, 0);
}

static void test_color_256_to_rgb_grayscale(void)
{
    uint8_t r, g, b;
    flare_color_256_to_rgb(244, &r, &g, &b);
    ASSERT_EQ(r, g);
    ASSERT_EQ(g, b);
}

static void test_rgb_256_roundtrip_near_black(void)
{
    int idx = flare_color_rgb_to_256(5, 5, 5);
    uint8_t r, g, b;
    flare_color_256_to_rgb(idx, &r, &g, &b);
    ASSERT_TRUE(abs(r - 5) <= 50);
    ASSERT_TRUE(abs(g - 5) <= 50);
    ASSERT_TRUE(abs(b - 5) <= 50);
}

static void test_rgb_to_256_zero(void)
{
    ASSERT_EQ(flare_color_rgb_to_256(0, 0, 0), 16);
}

static void test_rgb_to_256_max(void)
{
    int idx = flare_color_rgb_to_256(255, 255, 255);
    ASSERT_TRUE(idx >= 16 && idx <= 255);
}

int main(void)
{
    RUN_TEST(test_rgb_to_256_pure_red);
    RUN_TEST(test_rgb_to_256_pure_green);
    RUN_TEST(test_rgb_to_256_pure_blue);
    RUN_TEST(test_rgb_to_256_black);
    RUN_TEST(test_rgb_to_256_white);
    RUN_TEST(test_rgb_to_256_grayscale);
    RUN_TEST(test_rgb_to_256_idempotent_cube);
    RUN_TEST(test_rgb_to_8_pure_red);
    RUN_TEST(test_rgb_to_8_pure_green);
    RUN_TEST(test_rgb_to_8_black);
    RUN_TEST(test_rgb_to_8_white);
    RUN_TEST(test_rgb_to_8_dark_gray);
    RUN_TEST(test_rgb_to_8_bright_red);
    RUN_TEST(test_rgb_to_16_pure_red);
    RUN_TEST(test_rgb_to_16_dark_red);
    RUN_TEST(test_rgb_to_16_pure_white);
    RUN_TEST(test_rgb_to_16_black);
    RUN_TEST(test_rgb_to_16_gray);
    RUN_TEST(test_rgb_to_16_bright_green);
    RUN_TEST(test_rgb_to_16_dark_green);
    RUN_TEST(test_color_256_to_rgb_cube);
    RUN_TEST(test_color_256_to_rgb_grayscale);
    RUN_TEST(test_rgb_256_roundtrip_near_black);
    RUN_TEST(test_rgb_to_256_zero);
    RUN_TEST(test_rgb_to_256_max);
    TEST_SUMMARY();
}
