#include "drawhelper.h"
#include "colors.h"
#include "models.h"
#include <PalmOS.h>

#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS

ImageData *drawhelper_loadImage(UInt16 bitmapId) {
    MemHandle bitmapH;

    bitmapH = DmGetResource(bitmapRsc, bitmapId);
    if (bitmapH) {
        BitmapPtr bitmap = (BitmapPtr)MemHandleLock(bitmapH);
        ImageData *imageData = (ImageData *)MemPtrNew(sizeof(ImageData));
        MemSet(imageData, sizeof(ImageData), 0);
        imageData->bitmapPtr = bitmap;
        imageData->resource = bitmapH;
        return imageData;
    }
    return NULL;
}

ImageData *drawhelper_loadAnimation(UInt16 bitmapId, int frameCount) {
    int i;
    MemHandle bitmapH;
    ImageData *imageDataContainer = (ImageData *)MemPtrNew(sizeof(ImageData) * frameCount);
    MemSet(imageDataContainer, sizeof(ImageData) * frameCount, 0);
    for (i = 0; i < frameCount; i++) {
        bitmapH = DmGetResource(bitmapRsc, bitmapId + i);
        if (bitmapH) {
            BitmapPtr bitmap = (BitmapPtr)MemHandleLock(bitmapH);
            imageDataContainer[i].bitmapPtr = bitmap;
            imageDataContainer[i].resource = bitmapH;
        }
    }
    return imageDataContainer;
}

void drawhelper_releaseImage(ImageData *imageData) {
    MemHandleUnlock(imageData->resource);
    DmReleaseResource(imageData->resource);
    MemPtrFree(imageData);
}

void drawhelper_releaseAnimation(ImageData *imageData, int frameCount) {
    int i;
    for (i = 0; i < frameCount; i++) {
        MemHandleUnlock(imageData[i].resource);
        DmReleaseResource(imageData[i].resource);
    }
    MemPtrFree(imageData);
}

ImageSprite imageSprite(int resourceId, Coordinate size) {
    ImageSprite sprite;
    sprite.resourceId = resourceId;
    sprite.size = size;
    return sprite;
}

AnimationSprite animationSprite(int resourceId, int frameCount, Coordinate size) {
    AnimationSprite sprite;
    sprite.resourceId = resourceId;
    sprite.frameCount = frameCount;
    sprite.size = size;
    return sprite;
}

void drawhelper_loadAndDrawImage(UInt16 bitmapId, Coordinate coordinate) {

    ImageData *imageData = drawhelper_loadImage(bitmapId);
    drawhelper_drawImage(imageData, coordinate);
    drawhelper_releaseImage(imageData);
}

void drawhelper_drawImage(ImageData *imageData, Coordinate coordinate) {
    WinDrawBitmap(imageData->bitmapPtr, coordinate.x, coordinate.y);
}

void drawhelper_drawSprite(ImageSprite *imageSprite, Coordinate coordinate) {
    Coordinate updatedPosition;
    updatedPosition.x = coordinate.x - imageSprite->size.x / 2;
    updatedPosition.y = coordinate.y - imageSprite->size.y / 2;
    drawhelper_drawImage(imageSprite->imageData, updatedPosition);
}

void drawhelper_drawAnimation(ImageData *imageDataContainer, Coordinate coordinate, int frameCount, int animationsPerSecond) {
    int animationStep = (TimGetTicks() /  (SysTicksPerSecond() / animationsPerSecond)) % frameCount;
    WinDrawBitmap(imageDataContainer[animationStep].bitmapPtr, coordinate.x, coordinate.y);
}

void drawhelper_drawAnimationSprite(AnimationSprite *animationSprite, Coordinate coordinate, int animationsPerSecond) {
    Coordinate updatedPosition;
    updatedPosition.x = coordinate.x - animationSprite->size.x / 2;
    updatedPosition.y = coordinate.y - animationSprite->size.y / 2;
    drawhelper_drawAnimation(animationSprite->imageData, updatedPosition, animationSprite->frameCount, animationsPerSecond);
}

void drawhelper_applyForeColor(AppColor color) {
    WinSetForeColor(colors_reference[color]);
}

void drawhelper_applyTextColor(AppColor color) {
    WinSetTextColor(colors_reference[color]);
}

void drawhelper_applyBackgroundColor(AppColor color) {
    WinSetBackColor(colors_reference[color]);
}

void drawhelper_drawBoxAround(Coordinate coordinate, int dimension) {
  Line line;
  int offset = dimension / 2 + 2;
  // TOP
  line.startpoint.x = coordinate.x - offset;
  line.endpoint.x = coordinate.x + offset;
  line.startpoint.y = coordinate.y - offset;
  line.endpoint.y = coordinate.y - offset;
  drawhelper_drawLine(&line);
  // LEFT
  line.startpoint.x = coordinate.x - offset;
  line.endpoint.x = coordinate.x - offset;
  line.startpoint.y = coordinate.y - offset;
  line.endpoint.y = coordinate.y + offset;
  drawhelper_drawLine(&line);
  // BOTTOM
  line.startpoint.x = coordinate.x - offset;
  line.endpoint.x = coordinate.x + offset;
  line.startpoint.y = coordinate.y + offset;
  line.endpoint.y = coordinate.y + offset;
  drawhelper_drawLine(&line);
  // RIGHT
  line.startpoint.x = coordinate.x + offset;
  line.endpoint.x = coordinate.x + offset;
  line.startpoint.y = coordinate.y - offset;
  line.endpoint.y = coordinate.y + offset ;
  drawhelper_drawLine(&line);
}

void drawhelper_drawTextWithValue(char *text, int value, Coordinate position) {
    char finalText[20];
    char valueText[20];
    StrCopy(finalText, text);
    StrIToA(valueText, value);
    StrCat(finalText, valueText);
    WinDrawChars(finalText, StrLen(finalText), position.x, position.y);
}

void drawhelper_drawText(char *text, Coordinate position) {
    WinDrawChars(text, StrLen(text), position.x, position.y);
}

void drawhelper_drawTextFromResource(UInt16 resourceId, Coordinate position) {
    MemHandle resourceHandle = DmGetResource('tSTR', resourceId);
    char* text = (char*)MemHandleLock(resourceHandle);
    WinDrawChars(text, StrLen(text), position.x, position.y);
    MemHandleUnlock(resourceHandle);
    DmReleaseResource(resourceHandle);
}

void drawhelper_drawLine(Line *line) {
    WinDrawLine(line->startpoint.x,
                line->startpoint.y,
                line->endpoint.x,
                line->endpoint.y);
}

static Boolean drawhelper_isPointInTriangle(Coordinate p, Coordinate p1, Coordinate p2, Coordinate p3) {
    // Compute vectors
    int v0x = p3.x - p1.x;
    int v0y = p3.y - p1.y;
    int v1x = p2.x - p1.x;
    int v1y = p2.y - p1.y;
    int v2x = p.x - p1.x;
    int v2y = p.y - p1.y;

    // Compute dot products
    int dot00 = v0x * v0x + v0y * v0y;
    int dot01 = v0x * v1x + v0y * v1y;
    int dot02 = v0x * v2x + v0y * v2y;
    int dot11 = v1x * v1x + v1y * v1y;
    int dot12 = v1x * v2x + v1y * v2y;

    // Compute barycentric coordinates
    int invDenom = dot00 * dot11 - dot01 * dot01;
    float u = (float)(dot11 * dot02 - dot01 * dot12) / invDenom;
    float v = (float)(dot00 * dot12 - dot01 * dot02) / invDenom;

    // Check if point is in triangle
    return (u >= 0) && (v >= 0) && (u + v <= 1);
}

// The main function to iterate all pixels in a triangle
void drawHelper_drawTriangle(Coordinate p1, Coordinate p2, Coordinate p3) {
    int y, x;
    // Find bounding box for the triangle
    int minX = p1.x < p2.x ? (p1.x < p3.x ? p1.x : p3.x) : (p2.x < p3.x ? p2.x : p3.x);
    int minY = p1.y < p2.y ? (p1.y < p3.y ? p1.y : p3.y) : (p2.y < p3.y ? p2.y : p3.y);
    int maxX = p1.x > p2.x ? (p1.x > p3.x ? p1.x : p3.x) : (p2.x > p3.x ? p2.x : p3.x);
    int maxY = p1.y > p2.y ? (p1.y > p3.y ? p1.y : p3.y) : (p2.y > p3.y ? p2.y : p3.y);

    // Iterate over the bounding box
    for (y = minY; y <= maxY; y++) {
        for (x = minX; x <= maxX; x++) {
            // Create a point for the current pixel
            Coordinate p = coordinate(x, y);

            // Check if the pixel is inside the triangle
            if (drawhelper_isPointInTriangle(p, p1, p2, p3)) {
                WinDrawPixel(p.x, p.y);
            }
        }
    }
}
