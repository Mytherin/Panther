
#include "label.h"
#include "style.h"
#include "textline.h"

Label::Label(PGWindowHandle window, Control* parent) : 
	Control(window), wrap_text(false) {
	this->parent = parent;

	this->background_color.a = 0;
	this->text_color = PGStyleManager::GetColor(PGColorTabControlText);
}

void Label::Draw(PGRendererHandle renderer) {
	PGRect render_rectangle = PGRect(X(), Y(), this->width, this->height);
	if (this->background_color.a != 0) {
		RenderRectangle(renderer, render_rectangle, this->background_color, PGDrawStyleFill);
	}
	if (font) {
		SetTextColor(font, text_color);
		if (wrap_text) {
			render_rectangle.x += padding.left;
			render_rectangle.y += padding.top;
			render_rectangle.width -= padding.left + padding.right;

			TextLine l = TextLine((char*)text.c_str(), text.size());
			lng linecount = 1;
			lng start_wrap = 0, end_wrap = text.size();
			PGScalar current_y = render_rectangle.y;
			while (l.WrapLine(font, render_rectangle.width, start_wrap, end_wrap)) {
				RenderText(renderer, font, text.c_str() + start_wrap, end_wrap - start_wrap, render_rectangle.x, current_y);
				current_y += GetTextHeight(font);
				start_wrap = end_wrap;
			}
			RenderText(renderer, font, text.c_str() + start_wrap, end_wrap - start_wrap, render_rectangle.x, current_y);
			this->fixed_height = this->height = GetTextHeight(font) * linecount;
		} else {
			RenderText(renderer, font, text.c_str(), text.size(), render_rectangle.x + render_rectangle.width / 2, render_rectangle.y + (render_rectangle.height - GetTextHeight(font)) / 2 - 1, PGTextAlignHorizontalCenter);
		}
	}
}

void Label::ResolveSize(PGSize new_size) {
	if (!fixed_size && font) {
		if (wrap_text) {
			Control::ResolveSize(new_size);
			TextLine l = TextLine((char*)text.c_str(), text.size());
			lng linecount = 1;
			lng start_wrap = 0, end_wrap;
			while (l.WrapLine(font, this->width, start_wrap, end_wrap)) {
				linecount++;
				start_wrap = end_wrap;
			}
			this->fixed_height = this->height = GetTextHeight(font) * linecount + padding.top + padding.bottom;
			return;
		} else {
			this->fixed_width = MeasureTextWidth(font, text) + padding.left + padding.right;
		}
	}
	Control::ResolveSize(new_size);
}
