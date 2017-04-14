#pragma once

#include "control.h"

class Button;

typedef void(*PGButtonCallback)(Button*, void*);

class Button : public Control {
public:
	Button(PGWindowHandle window, Control* parent);
	virtual ~Button();

	bool hovering = false;
	bool clicking = false;

	void Draw(PGRendererHandle renderer);

	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	void MouseEnter();
	void MouseLeave();
	void ShowTooltip();

	bool IsDragging() { return clicking; }

	void Invalidate() { parent->Invalidate(); }

	void OnPressed(PGButtonCallback callback, void* data = nullptr) { on_pressed = callback; pressed_data = data; }

	void SetText(std::string text, PGFontHandle font);
	std::string GetText() { return text; }

	virtual PGControlType GetControlType() { return PGControlTypeButton; }

	PGColor background_color;
	PGColor background_color_hover;
	PGColor background_color_click;
	PGColor background_stroke_color;
	PGColor text_color;
private:
	std::string text;
	PGFontHandle font = nullptr;

	PGButtonCallback on_pressed = nullptr;
	void* pressed_data = nullptr;
};