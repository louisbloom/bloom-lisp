/* color.c - RGB ↔ 256-color ↔ 16-color conversion (canonical implementation) */

#include "../include/bloom-lisp/highlight.h"
#include <math.h>

int bflare_color_rgb_to_256(int r, int g, int b)
{
    /* Greyscale ramp: indices 232-255 hold 24 shades from (8,8,8) to (238,238,238) */
    if (r == g && g == b) {
        if (r < 8)
            return 16; /* black */
        if (r > 248)
            return 231; /* white in cube */
        /* Map [8..248] → [232..255] (24 steps, step size ~10) */
        int gray = (r - 8) * 24 / 241 + 232;
        if (gray > 255)
            gray = 255;
        return gray;
    }

    /* 6×6×6 color cube: indices 16-231 */
    int ri = (r == 0) ? 0 : (r - 35) / 40 + 1;
    int gi = (g == 0) ? 0 : (g - 35) / 40 + 1;
    int bi = (b == 0) ? 0 : (b - 35) / 40 + 1;
    if (ri > 5)
        ri = 5;
    if (gi > 5)
        gi = 5;
    if (bi > 5)
        bi = 5;

    int cube = 16 + ri * 36 + gi * 6 + bi;

    /* Check if grayscale is closer than cube color */
    int gray_idx = 232 + (r + g + b) / 3 * 24 / 256;
    if (gray_idx > 255)
        gray_idx = 255;
    if (gray_idx < 232)
        gray_idx = 232;

    uint8_t cr, cg, cb, gr2, gg2, gb2;
    bflare_color_256_to_rgb(cube, &cr, &cg, &cb);
    bflare_color_256_to_rgb(gray_idx, &gr2, &gg2, &gb2);

    int cube_dist = (cr - r) * (cr - r) + (cg - g) * (cg - g) + (cb - b) * (cb - b);
    int gray_dist = (gr2 - r) * (gr2 - r) + (gg2 - g) * (gg2 - g) + (gb2 - b) * (gb2 - b);

    return (gray_dist < cube_dist) ? gray_idx : cube;
}

int bflare_color_rgb_to_8(int r, int g, int b)
{
    /* Map to the nearest ANSI 8-color by finding which primary is strongest */
    int idx = 0;
    if (r > 128)
        idx |= 1;
    if (g > 128)
        idx |= 2;
    if (b > 128)
        idx |= 4;
    return idx;
}

/* Chroma-style 16-color palette for nearest-match mapping.
 * Normal colors (0-7) use half-intensity, bright colors (8-15) use full.
 * Matches the reference RGB values returned by bflare_color_256_to_rgb(0..15). */
static const uint8_t ansi16[16][3] = {
    { 0, 0, 0 }, { 128, 0, 0 }, { 0, 128, 0 }, { 128, 128, 0 }, { 0, 0, 128 }, { 128, 0, 128 }, { 0, 128, 128 }, { 192, 192, 192 }, { 128, 128, 128 }, { 255, 0, 0 }, { 0, 255, 0 }, { 255, 255, 0 }, { 0, 0, 255 }, { 255, 0, 255 }, { 0, 255, 255 }, { 255, 255, 255 }
};

int bflare_color_rgb_to_16(int r, int g, int b)
{
    int best = 0;
    int best_dist = 0x7fffffff;
    for (int i = 0; i < 16; i++) {
        int dr = (int)ansi16[i][0] - r;
        int dg = (int)ansi16[i][1] - g;
        int db = (int)ansi16[i][2] - b;
        int dist = dr * dr + dg * dg + db * db;
        if (dist < best_dist) {
            best_dist = dist;
            best = i;
        }
    }
    return best;
}

void bflare_color_256_to_rgb(int idx, uint8_t *r, uint8_t *g, uint8_t *b)
{
    if (idx < 16) {
        /* Standard 16 colors — just return approximate RGB */
        static const uint8_t std16[16][3] = {
            { 0, 0, 0 }, { 128, 0, 0 }, { 0, 128, 0 }, { 128, 128, 0 }, { 0, 0, 128 }, { 128, 0, 128 }, { 0, 128, 128 }, { 192, 192, 192 }, { 128, 128, 128 }, { 255, 0, 0 }, { 0, 255, 0 }, { 255, 255, 0 }, { 0, 0, 255 }, { 255, 0, 255 }, { 0, 255, 255 }, { 255, 255, 255 }
        };
        *r = std16[idx][0];
        *g = std16[idx][1];
        *b = std16[idx][2];
        return;
    }
    if (idx < 232) {
        /* 6×6×6 color cube */
        idx -= 16;
        int bi = idx % 6;
        int gi = (idx / 6) % 6;
        int ri = idx / 36;
        static const uint8_t cube_vals[6] = { 0, 95, 135, 175, 215, 255 };
        *r = cube_vals[ri];
        *g = cube_vals[gi];
        *b = cube_vals[bi];
        return;
    }
    /* Grayscale ramp 232-255 */
    int v = 8 + (idx - 232) * 10;
    if (v > 238)
        v = 238;
    *r = *g = *b = (uint8_t)v;
}
