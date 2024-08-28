#ifndef DRAWHELPER_H_
#define ALLOW_ACCESS_TO_INTERNALS_OF_BITMAPS
#define DRAWHELPER_H_
#include <PalmOS.h>
#include "models.h"
#include "colors.h"

typedef struct ImageData {
    MemHandle resource;
    BitmapPtr bitmapPtr;
} ImageData;

typedef struct ImageSprite {
    int resourceId;
    Coordinate size;
    ImageData *imageData;
} ImageSprite;

typedef struct AnimationSprite {
    int resourceId;
    int frameCount;
    Coordinate size;
    ImageData *imageData;
} AnimationSprite;

ImageSprite imageSprite(int resourceId, Coordinate size);
AnimationSprite animationSprite(int resourceId, int frameCount, Coordinate size);

ImageData *drawhelper_loadImage(UInt16 bitmapId);
ImageData *drawhelper_loadAnimation(UInt16 bitmapId, int frameCount);
void drawhelper_loadAndDrawImage(UInt16 bitmapId, Coordinate coordinate);
void drawhelper_drawImage(ImageData *imageData, Coordinate coordinate);
void drawhelper_drawSprite(ImageSprite *imageSprite, Coordinate coordinate);
void drawhelper_drawAnimation(ImageData *imageDataContainer, Coordinate coordinate, int frameCount, int animationsPerSecond);
void drawhelper_drawAnimationSprite(AnimationSprite *animationSprite, Coordinate coordinate, int animationsPerSecond);
void drawhelper_releaseImage(ImageData *imageData);
void drawhelper_releaseAnimation(ImageData *imageData, int frameCount);

void drawhelper_applyForeColor(AppColor color);
void drawhelper_applyTextColor(AppColor color);
void drawhelper_applyBackgroundColor(AppColor color);
void drawhelper_drawText(char *text, Coordinate position);
void drawhelper_drawTextFromResource(UInt16 resourceId, Coordinate position);
void drawhelper_drawTextWithValue(char *text, int value, Coordinate position);
void drawhelper_drawLine(Line *line);
void drawhelper_drawBoxAround(Coordinate coordinate, int dimension);
void drawHelper_drawTriangle(Coordinate p1, Coordinate p2, Coordinate p3);

#endif