#pragma once

#include "windowfunctions.h"
#include "utils.h"

typedef int PGAnchor;

extern const PGAnchor PGAnchorNone;
extern const PGAnchor PGAnchorLeft;
extern const PGAnchor PGAnchorRight;
extern const PGAnchor PGAnchorTop;
extern const PGAnchor PGAnchorBottom;

class Control {
public:
	Control(PGWindowHandle window, bool reg);

	virtual void MouseWheel(int x, int y, int distance, PGModifier modifier);
	// Returns true if the button has been consumed by the control, false if it has been ignored
	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual void KeyboardCharacter(char character, PGModifier modifier);
	virtual void KeyboardUnicode(char* character, PGModifier modifier);
	virtual void PeriodicRender(void);
	virtual void Draw(PGRendererHandle, PGIRect*);

	virtual void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);

	virtual void Invalidate();
	virtual void Invalidate(PGIRect);
	virtual void Invalidate(PGRect);

	void SetSize(PGSize size) { this->width = size.width; this->height = size.height; }
	void SetPosition(PGPoint point) { this->x = point.x; this->y = point.y; }
	void SetAnchor(PGAnchor anchor) { this->anchor = anchor; }
	PGSize GetSize() { return PGSize(this->width, this->height); }
	PGWindowHandle GetWindow() { return window; }

	PGRect GetRectangle() { 
		return PGRect(x, y, width, height); 
	}

	virtual void OnResize(PGSize old_size, PGSize new_size);

	void UpdateParentSize(PGSize old_size, PGSize new_size);

	virtual bool IsDragging();

	virtual PGCursorType GetCursor(PGPoint mouse) { return PGCursorStandard; }

	bool visible;
protected:
	Control* parent;

	PGScalar x, y;
	PGScalar width, height;
	PGAnchor anchor;

	PGWindowHandle window;
	bool HasFocus();
};
