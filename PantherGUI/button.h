#pragma once

#include "control.h"

class Button;

typedef void(*PGButtonCallback)(Button*);

class Button : public Control {
public:
	Button(PGWindowHandle window, Control* parent);
	~Button();

	bool hovering = false;
	bool clicking = false;

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseEnter();
	void MouseLeave();

	bool IsDragging() { return clicking; }

	void Invalidate() { parent->Invalidate(); }

	void OnPressed(PGButtonCallback callback) { on_pressed = callback; }
private:
	PGButtonCallback on_pressed = nullptr;
};