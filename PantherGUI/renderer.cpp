
#include "windowfunctions.h"
#include "control.h"
#include "renderer.h"

struct PGRenderer {
	SkCanvas* canvas;
	SkPaint* paint;
	SkPaint* textpaint;
	PGScalar character_width;
	PGScalar text_offset;

	PGRenderer() : canvas(nullptr), paint(nullptr) {}
};

struct PGFont {
	int x;
};

static SkPaint::Style PGStyleConvert(PGStyle style) {
	if (style == PGStyleStroke) {
		return SkPaint::kStroke_Style;
	}
	return SkPaint::kFill_Style;
}

void RenderControlsToBitmap(SkBitmap& bitmap, PGIRect rect, ControlManager* manager, char* default_font) {
	SkPaint paint;
	SkPaint textpaint;
	PGRenderer renderer;

	bitmap.allocN32Pixels(rect.width, rect.height);
	bitmap.allocPixels();

	SkCanvas canvas(bitmap);
	textpaint.setTextSize(SkIntToScalar(15));
	textpaint.setAntiAlias(true);
	textpaint.setStyle(SkPaint::kFill_Style);
	textpaint.setTextEncoding(SkPaint::kUTF8_TextEncoding);
	textpaint.setTextAlign(SkPaint::kLeft_Align);
	SkFontStyle style(SkFontStyle::kNormal_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant);
	auto font = SkTypeface::MakeFromName(default_font, style);
	textpaint.setTypeface(font);
	paint.setStyle(SkPaint::kFill_Style);
	canvas.clear(SkColorSetRGB(30, 30, 30));

	renderer.canvas = &canvas;
	renderer.paint = &paint;
	renderer.textpaint = &textpaint;

	manager->Draw(&renderer, &rect);
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
