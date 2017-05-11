#pragma once

#include "control.h"

class Scrollbar;

#define SCROLLBAR_ARROW_SIZE 16
#define SCROLLBAR_SIZE 16
#define SCROLLBAR_MINIMUM_SIZE 16
#define SCROLLBAR_PADDING 4

enum PGScrollbarDragType {
	PGScrollbarDragNone,
	PGScrollbarDragScrollbar,
	PGScrollbarDragFirstArrow,
	PGScrollbarDragSecondArrow,
	PGScrollbarDragAboveScrollbar,
	PGScrollbarDragBelowScrollbar
};

typedef void(*PGScrollbarCallback)(Scrollbar*, lng value);

class Scrollbar : public Control {
public:
	Scrollbar(std::shared_ptr<Control> parent, PGWindowHandle window, bool horizontal, bool arrows);
	Scrollbar(Control* parent, PGWindowHandle window, bool horizontal, bool arrows) : Scrollbar(parent->shared_from_this(), window, horizontal, arrows) { }
	
	virtual ~Scrollbar();

	virtual void Draw(PGRendererHandle renderer);

	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	bool IsDragging() { return drag_type != PGScrollbarDragNone; }
	
	void OnScrollChanged(PGScrollbarCallback callback) { this->callback = callback; }

	void Invalidate();

	lng min_value = 0;
	lng max_value = 1;
	lng current_value = 0;
	lng value_size = 1;

	void UpdateValues(lng min_value, lng max_value, lng value_size, lng current_value);

	bool mouse_on_arrow[2]{ false, false };
	bool mouse_on_scrollbar = false;

	virtual PGControlType GetControlType() { return PGControlTypeScrollbar; }
protected:
	PGScalar Width() { return this->width - padding.left - padding.right; }
	PGScalar Height() { return this->height - padding.top - padding.bottom; }

	void DrawBackground(PGRendererHandle renderer);
	void DrawScrollbar(PGRendererHandle renderer);

	PGScrollbarCallback callback;
	PGScrollbarDragType drag_type = PGScrollbarDragNone;

	PGIRect scrollbar_area;
	PGIRect arrow_regions[2];
	
	bool horizontal = false;
	bool arrows = true;

	time_t drag_start;
	PGScalar drag_offset;

	PGScalar ComputeScrollbarSize();
	PGScalar ComputeScrollbarOffset(PGScalar scrollbar_size);
	void SetScrollbarOffset(PGScalar offset);

	void UpdateScrollValue();
};