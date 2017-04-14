#pragma once

#pragma once

#include "control.h"

class Label : public Control {
public:
	Label(PGWindowHandle window, Control* parent);

	void Draw(PGRendererHandle renderer);

	void Invalidate() { parent->Invalidate(); }

	void SetText(std::string text, PGFontHandle font) {
		this->text = text;
		this->font = font;
	}
	std::string GetText() { return text; }

	virtual PGControlType GetControlType() { return PGControlTypeLabel; }

	void ResolveSize(PGSize new_size);

	PGColor background_color;
	PGColor text_color;

	bool fixed_size = true;
private:
	std::string text;
	PGFontHandle font = nullptr;
};
