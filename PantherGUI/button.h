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

	bool IsDragging() { return clicking; }

	void Invalidate() { parent->Invalidate(); }

	void OnPressed(PGButtonCallback callback, void* data = nullptr) { on_pressed = callback; pressed_data = data; }

	void SetImage(PGBitmapHandle image) { this->image = image; }
	void SetText(std::string text, PGFontHandle font);
	std::string GetText() { return text; }

	virtual PGControlType GetControlType() { return PGControlTypeButton; }

	void ResolveSize(PGSize new_size);

	PGColor background_color;
	PGColor background_color_hover;
	PGColor background_color_click;
	PGColor background_stroke_color;
	PGColor text_color;

	bool fixed_size = true;

	static Button* CreateFromCommand(Control* parent, std::string command_name, std::string tooltip_text,
		std::map<std::string, PGKeyFunction>& functions, std::map<std::string, PGBitmapHandle>& images);
private:
	PGBitmapHandle image = nullptr;
	std::string text;
	PGFontHandle font = nullptr;

	PGButtonCallback on_pressed = nullptr;
	void* pressed_data = nullptr;
};