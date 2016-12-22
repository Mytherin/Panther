
#include "windowfunctions.h"
#include "control.h"
#include "renderer.h"
#include "unicode.h"

struct PGRenderer {
	SkCanvas* canvas;
	SkPaint* paint;

	PGRenderer() : canvas(nullptr), paint(nullptr) {}
};

struct PGFont {
	SkPaint* textpaint;
	PGScalar character_width;
	PGScalar text_offset;
	int tabwidth = 4;
	std::vector<SkPaint*> fallback_paints;
};

static SkPaint::Style PGStyleConvert(PGStyle style) {
	if (style == PGStyleStroke) {
		return SkPaint::kStroke_Style;
	}
	return SkPaint::kStrokeAndFill_Style;
}

SkPaint* CreateTextPaint() {
	SkPaint* textpaint = new SkPaint();
	textpaint->setTextSize(SkIntToScalar(15));
	textpaint->setAntiAlias(true);
	textpaint->setStyle(SkPaint::kFill_Style);
	textpaint->setTextEncoding(SkPaint::kUTF8_TextEncoding);
	textpaint->setTextAlign(SkPaint::kLeft_Align);
	return textpaint;
}

static void CreateFallbackFonts(PGFontHandle font) {
	SkPaint* fallback_paint = CreateTextPaint();
	auto fallback_font = SkTypeface::MakeFromFile("NotoSansHans-Regular.otf");
	fallback_paint->setTypeface(fallback_font);
	font->fallback_paints.push_back(fallback_paint);
}

PGFontHandle PGCreateFont() {
	char* default_font = "Consolas";
	return PGCreateFont(default_font, false, false);
}

PGFontHandle PGCreateFont(char* fontname, bool italic, bool bold) {
	PGFontHandle font = new PGFont();

	SkFontStyle style(bold ? SkFontStyle::kBold_Weight : SkFontStyle::kNormal_Weight, SkFontStyle::kNormal_Width, italic ? SkFontStyle::kItalic_Slant : SkFontStyle::kUpright_Slant);

	font->textpaint = CreateTextPaint();
	auto main_font = SkTypeface::MakeFromName(fontname, style);
	font->textpaint->setTypeface(main_font);

	CreateFallbackFonts(font);

	return font;
}

PGFontHandle PGCreateFont(char* filename) {
	PGFontHandle font = new PGFont();

	font->textpaint = CreateTextPaint();
	auto main_font = SkTypeface::MakeFromFile(filename);
	font->textpaint->setTypeface(main_font);

	CreateFallbackFonts(font);

	return font;
}

void PGDestroyFont(PGFontHandle font) {
	delete font->textpaint;
	delete font;
}

PGRendererHandle InitializeRenderer() {
	SkPaint* paint = new SkPaint();
	PGRendererHandle renderer = new PGRenderer();
	paint->setStyle(SkPaint::kFill_Style);
	renderer->canvas = nullptr;
	renderer->paint = paint;
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

void RenderCircle(PGRendererHandle handle, PGCircle circle, PGColor color, PGStyle drawStyle) {
	handle->paint->setAntiAlias(true);
	handle->paint->setStyle(PGStyleConvert(drawStyle));
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->canvas->drawCircle(circle.x, circle.y, circle.radius, *handle->paint);
}

void RenderLine(PGRendererHandle handle, PGLine line, PGColor color, int width) {
	handle->paint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	handle->paint->setStrokeWidth(width);
	handle->canvas->drawLine(line.start.x, line.start.y, line.end.x, line.end.y, *handle->paint);
}

void RenderImage(PGRendererHandle renderer, void* image, int x, int y) {

}

void RenderText(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y) {
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
			if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
				// if not, we first render the previous characters using the main font
				if (position < i) {
					renderer->canvas->drawText(text + position, i - position, x, y + font->text_offset, *font->textpaint);
					x += font->textpaint->measureText(text + position, i - position);
				}
				position = i + offset;
				// then we switch to a fallback font to render this character
				bool found_fallback = false;
				for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
					if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
						renderer->canvas->drawText(text + i, offset, x, y + font->text_offset, **it);
						x += (*it)->measureText(text + i, offset);
						found_fallback = true;
						break;
					}
				}
				if (!found_fallback) {
					renderer->canvas->drawText("?", 1, x, y + font->text_offset, *font->textpaint);
					x += font->character_width;
				}
			}
		} else {
			if (text[i] == '\t') {
				if (position < i) {
					renderer->canvas->drawText(text + position, i - position, x, y + font->text_offset, *font->textpaint);
					x += font->textpaint->measureText(text + position, i - position);
				}
				position = i + offset;
				PGScalar offset = font->textpaint->measureText(" ", 1) * font->tabwidth;
				// if (render_spaces_always)
				// PGScalar lineheight = GetTextHeight(font);
				// RenderLine(renderer, PGLine(x + 1, y + lineheight / 2, x + offset - 1, y + lineheight / 2), PGColor(255, 255, 255, 100), 0.5f);
				x += offset;
			}
		}
		assert(offset > 0); // invalid UTF8 
		i += offset;
	}
	if (position < i) {
		renderer->canvas->drawText(text + position, i - position, x, y + font->text_offset, *font->textpaint);
	}
}

PGScalar RenderText(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y, PGTextAlign alignment) {
	PGScalar width = MeasureTextWidth(font, text, len);
	if (alignment & PGTextAlignRight) {
		x -= width;
	} else if (alignment & PGTextAlignHorizontalCenter) {
		x -= width / 2;
	}
	RenderText(renderer, font, text, len, x, y);
	return width;
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

PGScalar MeasureTextWidth(PGFontHandle font, const char* text, size_t length) {
	PGScalar text_size = 0;
	if (font->character_width > 0) {
		// main font is a monospace font
		int regular_elements = 0;
		for (size_t i = 0; i < length; ) {
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					regular_elements += font->tabwidth;
				} else {
					regular_elements++;
				}
			} else {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						regular_elements++;
				} else {
					regular_elements++;
				}
			}
			assert(offset > 0);
			i += offset;
		}
		text_size += regular_elements * font->character_width;
	} else {
		// main font is not monospace
		for (size_t i = 0; i < length; ) {
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					text_size += font->textpaint->measureText(" ", 1) * font->tabwidth;
				} else {
					text_size += font->textpaint->measureText(text + i, 1);
				}
			} else {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						text_size += font->textpaint->measureText("?", 1);
				} else {
					text_size += font->textpaint->measureText(text + i, offset);
				}
			}
			assert(offset > 0);
			i += offset;
		}
	}
	return text_size;
}

lng GetPositionInLine(PGFontHandle font, PGScalar x, const char* text, size_t length) {
	PGScalar text_size = 0;
	if (font->character_width > 0) {
		// main font is a monospace font
		for (size_t i = 0; i < length; ) {
			PGScalar old_size = text_size;
			int offset = utf8_character_length(text[i]);
			if (offset == 1) {
				if (text[i] == '\t') {
					text_size += font->character_width * font->tabwidth;
				} else {
					text_size += font->character_width;
				}
			} else {
				if (font->textpaint->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) == 0) {
					// if the main font does not support the current glyph, look into the fallback fonts
					bool found_fallback = false;
					for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
						if ((*it)->getTypeface()->charsToGlyphs(text + i, SkTypeface::kUTF8_Encoding, nullptr, 1) != 0) {
							text_size += (*it)->measureText(text + i, offset);
							found_fallback = true;
							break;
						}
					}
					if (!found_fallback)
						text_size += font->character_width;
				} else {
					text_size += font->character_width;
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


PGScalar GetTextHeight(PGFontHandle font) {
	SkPaint::FontMetrics metrics;
	font->textpaint->getFontMetrics(&metrics, 0);
	return metrics.fDescent - metrics.fAscent;
}

void RenderCaret(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y, lng characternr, PGScalar line_height, PGColor color) {
	PGScalar width = MeasureTextWidth(font, text, characternr);
	RenderLine(renderer, PGLine(x + width, y, x + width, y + line_height), color);
}

void RenderSelection(PGRendererHandle renderer, PGFontHandle font, const char *text, size_t len, PGScalar x, PGScalar y, lng start, lng end, PGColor selection_color, PGScalar line_height) {
	if (start == end) return;
	PGScalar selection_start = MeasureTextWidth(font, text, start);
	PGScalar selection_width = MeasureTextWidth(font, text, end > (lng) len ? len : end);
	PGScalar lineheight = GetTextHeight(font);
	if (end > (lng) len) {
		assert(end == len + 1);
		selection_width += font->character_width;
	}
	RenderRectangle(renderer, PGRect(x + selection_start, y, selection_width - selection_start, lineheight), selection_color, PGStyleFill);
	// if (!render_spaces_always)
	PGScalar cumsiz = selection_start;
	for (lng i = start; i < end;) {
		int offset = utf8_character_length(text[i]);
		if (offset == 1) {
			if (text[i] == '\t') {
				PGScalar text_size = font->textpaint->measureText(" ", 1) * font->tabwidth;
				PGScalar lineheight = GetTextHeight(font);
				RenderLine(renderer, PGLine(x + cumsiz + 1, y + lineheight / 2, x + cumsiz + text_size - 1, y + lineheight / 2), PGColor(255, 255, 255, 100), 1);
			} else if (text[i] == ' ') {
				char bullet_point[3] = { 0xE2, 0x88, 0x99 };
				PGColor color = GetTextColor(font);
				SetTextColor(font, PGColor(255, 255, 255, 64));
				RenderText(renderer, font, bullet_point, 3, x + cumsiz, y);
				SetTextColor(font, color);
			}
		}
		cumsiz += MeasureTextWidth(font, text + i, offset);
		i += offset;
	}
}

void SetTextColor(PGFontHandle font, PGColor color) {
	font->textpaint->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
		(*it)->setColor(SkColorSetARGB(color.a, color.r, color.g, color.b));
	}
}

PGColor GetTextColor(PGFontHandle font) {
	SkColor color = font->textpaint->getColor();
	return PGColor(SkColorGetR(color), SkColorGetG(color), SkColorGetB(color), SkColorGetA(color));
}

void SetTextFontSize(PGFontHandle font, PGScalar height) {
	font->textpaint->setTextSize(height);
	font->character_width = font->textpaint->measureText("i", 1);
	PGScalar max_width = font->textpaint->measureText("W", 1);
	if (PG::abs(font->character_width - max_width) > 0.1f) {
		// Set renderer->character_width to -1 for non-monospace fonts
		font->character_width = -1;
	}
	font->text_offset = font->textpaint->getFontBounds().height() / 2 + font->textpaint->getFontBounds().height() / 4;
	for (auto it = font->fallback_paints.begin(); it != font->fallback_paints.end(); it++) {
		(*it)->setTextSize(height);
	}
}