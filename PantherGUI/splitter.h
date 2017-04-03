#pragma once


#include "control.h"

class Splitter : public Control {
public:
	Splitter(PGWindowHandle window, bool horizontal);

	void Draw(PGRendererHandle);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);

	bool IsDragging() { return is_dragging; }

	PGCursorType GetCursor(PGPoint mouse) { return horizontal ? PGCursorResizeHorizontal : PGCursorResizeVertical; }
	PGCursorType GetDraggingCursor() { return GetCursor(PGPoint(0, 0)); }

	PGControlType GetControlType() { return PGControlTypeSplitter; }

	bool horizontal;
	std::vector<Control*> additional_anchors;
private:
	PGScalar drag_offset;
	bool is_dragging;
};