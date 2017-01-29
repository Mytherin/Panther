#pragma once

#include <SkBitmap.h>
#include <SkDevice.h>
#include <SkPaint.h>
#include <SkRect.h>
#include <SkData.h>
#include <SkImage.h>
#include <SkStream.h>
#include <SkSurface.h>
#include <SkPath.h>
#include <SkTypeface.h>
#include "controlmanager.h"

struct PGRenderer {
	SkCanvas* canvas;
	SkPaint* paint;

	PGRenderer() : canvas(nullptr), paint(nullptr) {}
};

struct PGFont {
	SkPaint* textpaint = nullptr;
	SkPaint* normaltext = nullptr;
	SkPaint* boldtext = nullptr;
	SkPaint* italictext = nullptr;
	PGScalar character_width;
	PGScalar text_offset;
	int tabwidth = 4;
	std::vector<SkPaint*> fallback_paints;

	PGFont() : normaltext(nullptr), boldtext(nullptr), italictext(nullptr) {}
};

struct PGBitmap {
	SkBitmap* bitmap;
};

void RenderControlsToBitmap(PGRendererHandle renderer, SkBitmap& bitmap, PGIRect rect, ControlManager* manager);
PGRendererHandle InitializeRenderer();
SkBitmap* PGGetBitmap(PGBitmapHandle);