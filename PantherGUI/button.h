#pragma once

#include "control.h"

class Button;

typedef void(*PGButtonCallback)(Button*);

class Button {
public:
	Button(Control* parent);
	~Button();

	bool hovering = false;
	bool clicking = false;
	PGIRect region;

	void DrawBackground(PGRendererHandle renderer, PGIRect* rect);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	void Invalidate() { parent->Invalidate(); }

	void OnPressed(PGButtonCallback callback) { on_pressed = callback; }

	Control* parent;
private:
	PGButtonCallback on_pressed = nullptr;
};