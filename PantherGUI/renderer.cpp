
#include "windowfunctions.h"
#include "control.h"
#include "renderer.h"
#include "unicode.h"

const char* _spaces_array = "                                                                                                           ";

struct PGRenderer {
	SkCanvas* canvas;
	SkPaint* paint;
	SkPaint* textpaint;
	PGScalar character_width;
	PGScalar text_offset;
	sk_sp<SkTypeface> main_font;
	std::vector<sk_sp<SkTypeface>> fallback_fonts;
	int tabwidth;

	PGRenderer() : canvas(nullptr), paint(nullptr), tabwidth(4) {}
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

PGRendererHandle InitializeRenderer(char* default_font) {
	SkPaint* paint = new SkPaint();
	SkPaint* textpaint = new SkPaint();
	PGRendererHandle renderer = new PGRenderer();
	textpaint->setTextSize(SkIntToScalar(15));
	textpaint->setAntiAlias(true);
	textpaint->setStyle(SkPaint::kFill_Style);
	textpaint->setTextEncoding(SkPaint::kUTF8_TextEncoding);
	textpaint->setTextAlign(SkPaint::kLeft_Align);
	SkFontStyle style(SkFontStyle::kNormal_Weight, SkFontStyle::kNormal_Width, SkFontStyle::kUpright_Slant);
	renderer->main_font = SkTypeface::MakeFromName(default_font, style);
	textpaint->setTypeface(renderer->main_font);
	auto fallback = SkTypeface::MakeFromFile("NotoSansHans-Regular.otf");
	renderer->fallback_fonts.push_back(fallback);

	paint->setStyle(SkPaint::kFill_Style);
	renderer->canvas = nullptr;
	renderer->paint = paint;
	renderer->textpaint = textpaint;
	return renderer;
}

void RenderControlsToBitmap(PGRendererHandle renderer, SkBitmap& bitmap, PGIRect rect, ControlManager* manager) {
	bitmap.allocN32Pixels(rect.width, rect.height);
	bitmap.allocPixels();

	SkCanvas canvas(bitmap);
	canvas.clear(SkColorSetRGB(30, 30, 30));

	renderer->canvas = &canvas;

	manager->Draw(renderer, &rect);
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
	/*if (len == 0) {
		len = 1;
		text = " ";
	}*/
	size_t position = 0;
	size_t i = 0;
	for ( ; i < len; ) {
		int offset = utf8_character_length(text[i]);
		if (offset > 1) {
			// for special unicode characters, we check if the main font can render the character
			if (renderer->main_font->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
				// if not, we first render the previous characters using the main font
				if (position != i) {
					renderer->textpaint->setTypeface(renderer->main_font);
					renderer->canvas->drawText(text + position, i - position, x, y + renderer->text_offset, *renderer->textpaint);
					x += renderer->textpaint->measureText(text + position, i - position);
				}
				position = i + offset;
				// then we switch to a fallback font to render this character
				bool found_fallback = false;
				for (auto it = renderer->fallback_fonts.begin(); it != renderer->fallback_fonts.end(); it++) {
					if ((*it)->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
						renderer->textpaint->setTypeface(*it);
						renderer->canvas->drawText(text + i, offset, x, y + renderer->text_offset, *renderer->textpaint);
						x += renderer->textpaint->measureText(text + i, offset);
						found_fallback = true;
						break;
					}
				}
				assert(found_fallback);
			}
		} else {
			if (text[i] == '\t') {
				renderer->textpaint->setTypeface(renderer->main_font);
				if (position != i) {
					renderer->canvas->drawText(text + position, i - position, x, y + renderer->text_offset, *renderer->textpaint);
					x += renderer->textpaint->measureText(text + position, i - position);
				}
				position = i + offset;
				assert(renderer->tabwidth > 0 && renderer->tabwidth < 100);
				renderer->canvas->drawText(_spaces_array, renderer->tabwidth, x, y, *renderer->textpaint);
				x += renderer->textpaint->measureText(_spaces_array, renderer->tabwidth);
			}
		}
		assert(offset > 0); // invalid UTF8 
		i += offset;
	}
	renderer->textpaint->setTypeface(renderer->main_font);
	if (position != i) {
		renderer->canvas->drawText(text + position, i - position, x, y + renderer->text_offset, *renderer->textpaint);
	}
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
	PGScalar text_size = 0;
	if (renderer->character_width > 0) {
		// main font is a monospace font
		int regular_elements = 0;
		for (size_t i = 0; i < length; ) {
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					regular_elements += renderer->tabwidth;
				} else {
					regular_elements++;
				}
			} else {
				if (renderer->main_font->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = renderer->fallback_fonts.begin(); it != renderer->fallback_fonts.end(); it++) {
						if ((*it)->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							renderer->textpaint->setTypeface(*it);
							text_size += renderer->textpaint->measureText(text + i, offset);
							renderer->textpaint->setTypeface(renderer->main_font);
							found_fallback = true;
							break;
						}
					}
					assert(found_fallback);
				} else {
					regular_elements++;
				}
			}
			assert(offset > 0);
			i += offset;
		}
		text_size += regular_elements * renderer->character_width;
	} else {
		// FIXME: main font is not monospace
		assert(0);
	}
	return text_size;
}

lng GetPositionInLine(PGRendererHandle renderer, PGScalar x, const char* text, size_t length) {
	PGScalar text_size = 0;
	if (renderer->character_width > 0) {
		// main font is a monospace font
		for (size_t i = 0; i < length; ) {
			PGScalar old_size = text_size;
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					text_size += renderer->character_width * renderer->tabwidth;
				} else {
					text_size += renderer->character_width;
				}
			} else {
				if (renderer->main_font->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = renderer->fallback_fonts.begin(); it != renderer->fallback_fonts.end(); it++) {
						if ((*it)->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							renderer->textpaint->setTypeface(*it);
							text_size += renderer->textpaint->measureText(text + i, offset);
							renderer->textpaint->setTypeface(renderer->main_font);
							found_fallback = true;
							break;
						}
					}
					assert(found_fallback);
				} else {
					text_size += renderer->character_width;
				}
			}
			assert(offset > 0);
			if (text_size > x) {
				return i;
			}
			i += offset;
		}
	} else {
		// FIXME: main font is not monospace
		assert(0);
	}
	return length;
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
	renderer->character_width = renderer->textpaint->measureText("i", 1);
	if (std::abs(renderer->character_width - renderer->textpaint->measureText("W", 1)) > 0.1) {
		// only monospace fonts are supported for now
		assert(0);
		// non monospace font, set character_width to a negative number
		renderer->character_width = -1;
	}
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
