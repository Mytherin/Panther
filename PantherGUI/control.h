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
	Control(PGWindowHandle window);
	virtual ~Control() { }

	virtual void MouseWheel(int x, int y, int distance, PGModifier modifier);
	// Returns true if the button has been consumed by the control, false if it has been ignored
	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual bool KeyboardCharacter(char character, PGModifier modifier);
	virtual bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	virtual void PeriodicRender(void);
	virtual void Draw(PGRendererHandle, PGIRect*);

	virtual void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);
	// In order for a control to get MouseEnter() and MouseLeave() events
	// it must register with the ControlManager
	virtual void MouseEnter();
	virtual void MouseLeave();

	virtual void Invalidate();
	virtual void Invalidate(PGIRect);
	virtual void Invalidate(PGRect);

	virtual Control* GetActiveControl() { return nullptr; }
	virtual bool ControlTakesFocus() { return false; }
	virtual bool ControlHasFocus() { return !parent ? true : (parent->ControlHasFocus() && parent->GetActiveControl() == this); }

	void SetSize(PGSize size);
	void SetPosition(PGPoint point) { this->x = point.x; this->y = point.y; }
	void SetAnchor(PGAnchor anchor) { this->anchor = anchor; }
	PGSize GetSize() { return PGSize(this->width, this->height); }
	PGWindowHandle GetWindow() { return window; }

	PGRect GetRectangle() { 
		return PGRect(this->x, this->y, width, height); 
	}

	virtual void OnResize(PGSize old_size, PGSize new_size);
	void TriggerResize() { this->OnResize(PGSize(this->width, this->height), PGSize(this->width, this->height)); }

	virtual void ResolveSize(PGSize new_size);


	virtual bool IsDragging();

	virtual PGCursorType GetCursor(PGPoint mouse) { return PGCursorStandard; }

	bool visible;

	PGScalar X();
	PGScalar Y();
	PGPoint Position();

	Control* parent = nullptr;

	bool size_resolved = false;
	PGScalar x = 0, y = 0;
	PGScalar width, height;
	PGScalar fixed_width = -1, fixed_height = -1;
	PGScalar percentage_width = -1, percentage_height = -1;
	PGAnchor anchor;
	Control* vertical_anchor = nullptr;
	Control* horizontal_anchor = nullptr;

	PGWindowHandle window;
	bool HasFocus();

};
