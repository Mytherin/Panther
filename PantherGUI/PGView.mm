
#include "textfile.h"
#include "control.h"
#include "textfield.h"
#include "time.h"
#include "scheduler.h"

#import "PGView.h"

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

#include "control.h"

#import "PGDrawBitmap.h"

struct PGWindow {
	PGView* view;
	std::vector<Control*> controls;
	Control *focused_control;
	PGIRect invalidated_area;
	bool invalidated;

	PGWindow() : invalidated_area(0,0,0,0) { }
};

struct PGRenderer {
	SkCanvas* canvas;
	SkPaint* paint;
	SkPaint* textpaint;
	PGScalar character_width;
	PGScalar text_offset;

	PGRenderer() : canvas(nullptr), paint(nullptr) {}
};


PGWindowHandle global_handle;

#define MAX_REFRESH_FREQUENCY 1000/30

void PeriodicWindowRedraw(void) {
	for (auto it = global_handle->controls.begin(); it != global_handle->controls.end(); it++) {
		(*it)->PeriodicRender();
	}
	if (global_handle->invalidated) {
		RedrawWindow(global_handle);
	} else if (global_handle->invalidated_area.width != 0) {
		RedrawWindow(global_handle, global_handle->invalidated_area);
	}
	global_handle->invalidated = 0;
	global_handle->invalidated_area.width = 0;
}

@implementation PGView : NSView

- (instancetype)initWithFrame:(NSRect)frameRect 
{
    if(self = [super initWithFrame:frameRect]) {
    	PGWindowHandle res = new PGWindow();
		res->view = self;

		//CreateTimer(MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);
		
		Scheduler::Initialize();
		Scheduler::SetThreadCount(8);

		CreateTimer(MAX_REFRESH_FREQUENCY, PeriodicWindowRedraw, PGTimerFlagsNone);
	
		TextField* textField = new TextField(res, "/Users/myth/cities.xml");
		textField->width = 1000;
		textField->height = 670;
		textField->x = 0;
		textField->y = 30;
		textField->anchor = PGAnchorBottom | PGAnchorLeft | PGAnchorRight;

		global_handle = res;
    }
    return self;
}

- (void)drawRect:(NSRect)invalidateRect
{
	SkBitmap bitmap;
	SkPaint paint;
	SkPaint textpaint;

	PGRenderer renderer;
	PGIRect rect(
		invalidateRect.origin.x,
		invalidateRect.origin.y,
		invalidateRect.size.width,
		invalidateRect.size.width);

	bitmap.allocN32Pixels(rect.width, rect.height);
	bitmap.allocPixels();

	SkCanvas canvas(bitmap);
	textpaint.setTextSize(SkIntToScalar(15));
	textpaint.setAntiAlias(true);
	textpaint.setStyle(SkPaint::kFill_Style);
	textpaint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
	textpaint.setTextAlign(SkPaint::kLeft_Align);
	SkFontStyle style(SkFontStyle::kNormal_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant);
	auto font = SkTypeface::MakeFromName("Courier New", style);
	textpaint.setTypeface(font);
	paint.setStyle(SkPaint::kFill_Style);
	canvas.clear(SkColorSetRGB(30, 30, 30));

	renderer.canvas = &canvas;
	renderer.paint = &paint;
	renderer.textpaint = &textpaint;

	for (auto it = global_handle->controls.begin(); it != global_handle->controls.end(); it++) {
		(*it)->Draw(&renderer, &rect);
	}

	// draw bitmap
	CGContextRef context = (CGContextRef) [[NSGraphicsContext currentContext] graphicsPort];
	SkCGDrawBitmap(context, bitmap, 0, 0);
}

-(NSRect)getBounds {
	return self.bounds;
}
/*- 
- (void)mouseDown:(NSEvent *)event;
- (void)mouseUp:(NSEvent *)event;
- (void)mouseMoved:(NSEvent *)event;
- (void)keyDown:(NSEvent *)event;
- (void)keyUp:(NSEvent *)event;*/

@end

PGPoint GetMousePosition(PGWindowHandle window) {
	// FIXME
	return PGPoint(0,0);
}


PGWindowHandle PGCreateWindow(void) {
	assert(0);
	return nullptr;
}


void RefreshWindow(PGWindowHandle window) {
	RedrawWindow(window);
}

void RefreshWindow(PGWindowHandle window, PGIRect rectangle) {
	RedrawWindow(window, rectangle);
}


void RedrawWindow(PGWindowHandle window) {
	[window->view needsToDrawRect:[window->view getBounds]];
}

void RedrawWindow(PGWindowHandle window, PGIRect rectangle) {
	[window->view needsToDrawRect:[window->view getBounds]];
}


static SkPaint::Style PGStyleConvert(PGStyle style) {
	if (style == PGStyleStroke) {
		return SkPaint::kStroke_Style;
	}
	return SkPaint::kFill_Style;
}

void RenderTriangle(PGRendererHandle handle, PGPoint a, PGPoint b, PGPoint c, PGColor color, PGStyle drawStyle) {
	SkPath path;
	SkPoint points[] = {{a.x, a.y}, {b.x, b.y}, {c.x, c.y}}; // triangle
	path.addPoly(points, 3, true);
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->paint->setStyle(PGStyleConvert(drawStyle));
	handle->canvas->drawPath(path, *handle->paint);
}

void RenderRectangle(PGRendererHandle handle, PGRect rectangle, PGColor color, PGStyle drawStyle) {
	SkRect rect;
	rect.fLeft = rectangle.x;
	rect.fTop = rectangle.y;
	rect.fRight = rectangle.x + rectangle.width;
	rect.fBottom = rectangle.y + rectangle.height;
	handle->paint->setStyle(PGStyleConvert(drawStyle));
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->canvas->drawRect(rect, *handle->paint);
}

void RenderLine(PGRendererHandle handle, PGLine line, PGColor color) {
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, *handle->paint);
}

void RenderImage(PGRendererHandle renderer, void* image, int x, int y) {

}

void RenderText(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y) {
	if (len == 0) {
		len = 1;
		text = " ";
	}
	// FIXME: render tabs correctly
	renderer->canvas->drawText(text, len, x, y + renderer->text_offset, *renderer->textpaint);
}

void RenderSquiggles(PGRendererHandle renderer, PGScalar width, PGScalar x, PGScalar y, PGColor color) {
	SkPath path;
	PGScalar offset = 3; // FIXME: depend on text height
	PGScalar end = x + width;
	path.moveTo(x, y);
	while (x < end) {
		path.quadTo(x + 1, y + offset, x + 2, y);
		offset = -offset;
		x += 2;
	}
	renderer->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	renderer->paint->setAntiAlias(true);
	renderer->paint->setAutohinted(true);
	renderer->canvas->drawPath(path, *renderer->paint);
}

PGScalar MeasureTextWidth(PGRendererHandle renderer, const char* text, size_t length) {
	//return renderer->textpaint->measureText(text, length);
	int size = 0;
	for (size_t i = 0; i < length; i++) {
		if (text[i] == '\t') {
			size += 5; // FIXME: tab width
		} else {
			size += 1;
		}
	}
	return size * renderer->character_width;
}

PGScalar GetTextHeight(PGRendererHandle renderer) {
	SkPaint::FontMetrics metrics;
	renderer->textpaint->getFontMetrics(&metrics, 0);
	return metrics.fDescent - metrics.fAscent;
}

void RenderCaret(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, lng characternr, PGScalar line_height) {
	PGScalar width = MeasureTextWidth(renderer, text, characternr);
	SkColor color = renderer->textpaint->getColor();
	RenderLine(renderer, PGLine(x + width, y, x + width, y + line_height), PGColor(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color)));
}

void RenderSelection(PGRendererHandle renderer, const char *text, size_t len, PGScalar x, PGScalar y, lng start, lng end, PGColor selection_color, PGScalar line_height) {
	if (start == end) return;
	PGScalar selection_start = MeasureTextWidth(renderer, text, start);
	PGScalar selection_width = MeasureTextWidth(renderer, text, end > (lng) len ? len : end);
	if (end > (lng) len) {
		assert(end == len + 1);
		selection_width += renderer->character_width;
	}
	RenderRectangle(renderer, PGRect(x + selection_start, y, selection_width - selection_start, GetTextHeight(renderer)), selection_color, PGStyleFill);
}

void SetTextColor(PGRendererHandle renderer, PGColor color) {
	renderer->textpaint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
}

void SetTextFont(PGRendererHandle renderer, PGFontHandle font, PGScalar height) {
	renderer->textpaint->setTextSize(height);
	renderer->character_width = renderer->textpaint->measureText("x", 1);
	renderer->text_offset = renderer->textpaint->getFontBounds().height() / 2 + renderer->textpaint->getFontBounds().height() / 4;
}

void SetTextAlign(PGRendererHandle renderer, PGTextAlign alignment) {
	if (alignment & PGTextAlignBottom) {
		assert(0);
	} else if (alignment & PGTextAlignTop) {
		assert(0);
	} else if (alignment & PGTextAlignVerticalCenter) {
		assert(0);
	}

	if (alignment & PGTextAlignLeft) {
		renderer->textpaint->setTextAlign(SkPaint::kLeft_Align);
	} else if (alignment & PGTextAlignRight) {
		renderer->textpaint->setTextAlign(SkPaint::kRight_Align);
	} else if (alignment & PGTextAlignHorizontalCenter) {
		renderer->textpaint->setTextAlign(SkPaint::kCenter_Align);
	}
}

Control* GetFocusedControl(PGWindowHandle window) {
	return window->focused_control;
}

void RegisterRefresh(PGWindowHandle window, Control *control) {
	window->focused_control = control;
	window->controls.push_back(control);
}

PGTime GetTime() {
	assert(0);
	return -1;
}

void SetWindowTitle(PGWindowHandle window, char* title) {
	assert(0);
}

void SetClipboardText(PGWindowHandle window, std::string text) {
	assert(0);
}

std::string GetClipboardText(PGWindowHandle window) {
	assert(0);
	return std::string("");
}

PGLineEnding GetSystemLineEnding() {
	return PGLineEndingMacOS;
}


bool WindowHasFocus(PGWindowHandle window) {
	// FIXME
	return true;
}

void SetCursor(PGWindowHandle window, PGCursorType type) {
	return;
}

struct PGTimer {

};

PGTimerHandle CreateTimer(int ms, PGTimerCallback callback, PGTimerFlags flags) {
	dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(ms * NSEC_PER_MSEC));
	dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
		callback();
		CreateTimer(ms, callback, flags);
	});
	return nullptr;
}

void DeleteTimer(PGTimerHandle handle) {

}