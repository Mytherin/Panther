
#include "label.h"
#include "style.h"

Label::Label(PGWindowHandle window, Control* parent) : 
	Control(window) {
	this->parent = parent;

	this->background_color = PGStyleManager::GetColor(PGColorTabControlBackground);
	this->text_color = PGStyleManager::GetColor(PGColorTabControlText);
}

void Label::Draw(PGRendererHandle renderer) {
	PGRect render_rectangle = PGRect(X(), Y(), this->width, this->height);
	if (font) {
		SetTextColor(font, text_color);
		RenderText(renderer, font, text.c_str(), text.size(), render_rectangle.x + render_rectangle.width / 2, render_rectangle.y + (render_rectangle.height - GetTextHeight(font)) / 2 - 1, PGTextAlignHorizontalCenter);
	}
}

void Label::ResolveSize(PGSize new_size) {
	if (!fixed_size && font) {
		this->fixed_width = MeasureTextWidth(font, text);
	}
	Control::ResolveSize(new_size);
}
