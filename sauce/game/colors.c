#include "colors.h"
#include <PalmOS.h>

IndexedColorType colors_reference[5];

static RGBColorType colors_rgb(Int16 red, Int16 green, Int16 blue) {
    RGBColorType color;
    color.r = red;
    color.g = green;
    color.b = blue;
    return color;
}

static RGBColorType colors_alizarin() {
    return colors_rgb(231, 76, 60);
}

static RGBColorType colors_belizeHole() {
    return colors_rgb(41, 128, 185);
}

static RGBColorType colors_emerald() {
    return colors_rgb(46, 204, 113);
}

static RGBColorType colors_clouds() {
    return colors_rgb(255, 255, 255);
}

static RGBColorType colors_carrot() {
    return colors_rgb(230, 126, 34);
}

static RGBColorType colors_asbestos() {
    return colors_rgb(127, 140, 141);
}

static RGBColorType colors_dirt() {
    return colors_rgb(219, 197, 165);
}

void colors_setupReferenceColors(Boolean colorSupport, UInt32 depth) {
    RGBColorType color;
    if (colorSupport) {
        color = colors_alizarin();
        colors_reference[ALIZARIN] = WinRGBToIndex(&color);
        color = colors_belizeHole();
        colors_reference[BELIZEHOLE] = WinRGBToIndex(&color);
        color = colors_carrot();
        colors_reference[CARROT] = WinRGBToIndex(&color);
        color = colors_emerald();
        colors_reference[EMERALD] = WinRGBToIndex(&color);
        color = colors_clouds();
        colors_reference[CLOUDS] = WinRGBToIndex(&color);
        color = colors_asbestos();
        colors_reference[ASBESTOS] = WinRGBToIndex(&color);
        color = colors_dirt();
        colors_reference[DIRT] = WinRGBToIndex(&color);
    } else {
        int i;
        colors_reference[ALIZARIN] = 15;
        colors_reference[BELIZEHOLE] = 6;
        colors_reference[CARROT] = 12;
        colors_reference[EMERALD] = 9;
        colors_reference[CLOUDS] = 0;
        colors_reference[ASBESTOS] = 13;
        colors_reference[DIRT] = 7;

        if (depth == 8) {
            for (i = 0; i < 7; i++) {
                colors_reference[i] *= 2;
            }
        }
    }
}
