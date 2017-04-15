
#include "controlmanager.h"
#include "scrollbar.h"
#include "style.h"
#include "searchbox.h"

Scrollbar::Scrollbar(Control* parent, PGWindowHandle window, bool horizontal, bool arrows) :
	Control(window), horizontal(horizontal), arrows(arrows), drag_offset(0), drag_start(0) {
	this->parent = parent;
	if (horizontal) {
		this->width = parent->width;
		this->height = SCROLLBAR_SIZE;
	} else {
		this->width = SCROLLBAR_SIZE;
		this->height = parent->width;
	}
	scrollbar_area = PGIRect(0, 0, this->width, this->height);
	ControlManager* cm = GetControlManager(this);
	if (arrows) {
		arrow_regions[0] = PGIRect(0, 0, SCROLLBAR_ARROW_SIZE, SCROLLBAR_ARROW_SIZE);
		arrow_regions[1] = PGIRect(horizontal ? this->width - SCROLLBAR_ARROW_SIZE : 0, horizontal ? 0 : this->height - SCROLLBAR_ARROW_SIZE, SCROLLBAR_ARROW_SIZE, SCROLLBAR_ARROW_SIZE);
		cm->RegisterMouseRegion(&arrow_regions[0], this, [](Control* c, bool enter, void* data) {
			((Scrollbar*)c)->mouse_on_arrow[0] = enter;
			c->Invalidate();
		});
		cm->RegisterMouseRegion(&arrow_regions[1], this, [](Control* c, bool enter, void* data) {
			((Scrollbar*)c)->mouse_on_arrow[1] = enter;
			c->Invalidate();
		});
	}
	cm->RegisterMouseRegion(&scrollbar_area, this, [](Control* c, bool enter, void* data) {
		((Scrollbar*)c)->mouse_on_scrollbar = enter;
		c->Invalidate();
	});
}

Scrollbar::~Scrollbar() {
	ControlManager* cm = GetControlManager(this);
	cm->UnregisterMouseRegion(&scrollbar_area);
	if (arrows) {
		cm->UnregisterMouseRegion(&arrow_regions[0]);
		cm->UnregisterMouseRegion(&arrow_regions[1]);
	}
}

void Scrollbar::DrawBackground(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();
	// render the background
	if (horizontal) {
		RenderRectangle(renderer, PGIRect(x - bottom_padding, y, this->width + bottom_padding + top_padding, this->height), PGStyleManager::GetColor(PGColorScrollbarBackground), PGDrawStyleFill);
	} else {
		RenderRectangle(renderer, PGIRect(x, y - bottom_padding, this->width, this->height + bottom_padding + top_padding), PGStyleManager::GetColor(PGColorScrollbarBackground), PGDrawStyleFill);
	}
	if (arrows) {
		arrow_regions[0] = PGIRect(0, 0, SCROLLBAR_ARROW_SIZE, SCROLLBAR_ARROW_SIZE);
		arrow_regions[1] = PGIRect(horizontal ? this->width - SCROLLBAR_ARROW_SIZE : 0, horizontal ? 0 : this->height - SCROLLBAR_ARROW_SIZE, SCROLLBAR_ARROW_SIZE, SCROLLBAR_ARROW_SIZE);
		PGColor arrowColor;
		PGPoint a, b, c;
		// render the arrow above/left of the scrollbar
		arrowColor = drag_type == PGScrollbarDragFirstArrow ? PGStyleManager::GetColor(PGColorScrollbarDrag) :
			(mouse_on_arrow[0] ? PGStyleManager::GetColor(PGColorScrollbarHover) : PGStyleManager::GetColor(PGColorScrollbarForeground));
		a = PGPoint(x + arrow_regions[0].x + SCROLLBAR_ARROW_SIZE / 2,
			y + arrow_regions[0].y);
		b = PGPoint(x + arrow_regions[0].x,
			y + arrow_regions[0].y + SCROLLBAR_ARROW_SIZE);
		c = PGPoint(x + arrow_regions[0].x + SCROLLBAR_ARROW_SIZE,
			y + arrow_regions[0].y + SCROLLBAR_ARROW_SIZE);
		RenderTriangle(renderer, a, b, c, arrowColor, PGDrawStyleFill);
		// render the arrow below/right of the scrollbar
		arrowColor = drag_type == PGScrollbarDragSecondArrow ? PGStyleManager::GetColor(PGColorScrollbarDrag) :
			(mouse_on_arrow[1] ? PGStyleManager::GetColor(PGColorScrollbarHover) : PGStyleManager::GetColor(PGColorScrollbarForeground));
		a = PGPoint(x + arrow_regions[1].x,
			y + arrow_regions[1].y);
		b = PGPoint(x + arrow_regions[1].x + SCROLLBAR_ARROW_SIZE,
			y + arrow_regions[1].y);
		c = PGPoint(x + arrow_regions[1].x + SCROLLBAR_ARROW_SIZE / 2,
			y + arrow_regions[1].y + SCROLLBAR_ARROW_SIZE);
		RenderTriangle(renderer, a, b, c, arrowColor, PGDrawStyleFill);
	}
	PGScalar size = ComputeScrollbarSize();
	PGScalar offset = ComputeScrollbarOffset(size);
	// update the scrollbar area with the new size
	if (horizontal) {
		scrollbar_area.width = (int)size;
		scrollbar_area.x = (int)offset;
		scrollbar_area.y = SCROLLBAR_SIZE / 4;
		scrollbar_area.height = SCROLLBAR_SIZE / 2;
	} else {
		scrollbar_area.height = (int)size;
		scrollbar_area.y = (int)offset;
		scrollbar_area.x = SCROLLBAR_SIZE / 4;
		scrollbar_area.width = SCROLLBAR_SIZE / 2;
	}
}

void Scrollbar::DrawScrollbar(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();
	// render the actual scrollbar
	PGColor scrollbar_color = drag_type == PGScrollbarDragScrollbar ? PGStyleManager::GetColor(PGColorScrollbarDrag) :
		(mouse_on_scrollbar ? PGStyleManager::GetColor(PGColorScrollbarHover) : PGStyleManager::GetColor(PGColorScrollbarForeground));
	RenderRectangle(renderer,
		PGRect(x + scrollbar_area.x, y + scrollbar_area.y, scrollbar_area.width, scrollbar_area.height), scrollbar_color, PGDrawStyleFill);
}

void Scrollbar::Draw(PGRendererHandle renderer) {
	if (!visible) return;
	DrawBackground(renderer);
	DrawScrollbar(renderer);
	Control::Draw(renderer);
}

PGScalar Scrollbar::ComputeScrollbarSize() {
	return std::max((PGScalar)SCROLLBAR_MINIMUM_SIZE,
		(value_size * ((horizontal ? this->width : this->height) - (arrows ? SCROLLBAR_ARROW_SIZE * 2 : 0))) /
		(max_value + value_size));
}

PGScalar Scrollbar::ComputeScrollbarOffset(PGScalar scrollbar_size) {
	assert(min_value == 0); // min value other than 0 is not supported right now
	return (arrows ? SCROLLBAR_ARROW_SIZE : 0) +
		((current_value * ((horizontal ? this->width : this->height) - scrollbar_size - (arrows ? SCROLLBAR_ARROW_SIZE * 2 : 0))) /
			std::max((lng)1, max_value));
}

void Scrollbar::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	if (buttons & PGLeftMouseButton) {
		if (drag_type == PGScrollbarDragScrollbar) {
			lng current_offset = current_value;

			SetScrollbarOffset(horizontal ? mouse.x - drag_offset : mouse.y - drag_offset);
			if (current_offset != current_value) {
				UpdateScrollValue();
				this->Invalidate();
			}
		} else if (drag_type != PGScrollbarDragNone) {
			time_t time = GetTime();
			if (time - drag_start >= DOUBLE_CLICK_TIME) {
				lng current_offset = current_value;
				if (drag_type == PGScrollbarDragFirstArrow) {
					current_value = std::max(min_value, current_value - 1);
				} else if (drag_type == PGScrollbarDragSecondArrow) {
					current_value = std::min(max_value, current_value + 1);
				} else if (drag_type == PGScrollbarDragAboveScrollbar) {
					current_value = std::max(min_value, current_value - value_size);
				} else if (drag_type == PGScrollbarDragBelowScrollbar) {
					current_value = std::min(max_value, current_value + value_size);
				}
				if (current_value != current_offset) {
					UpdateScrollValue();
				}
			}
		}
	} else {
		drag_type = PGScrollbarDragNone;
	}
}

void Scrollbar::SetScrollbarOffset(PGScalar offset) {
	// compute lineoffset_y from scrollbar offset
	lng new_value = (lng)((std::max((lng)1, max_value) * (offset - (arrows ? SCROLLBAR_ARROW_SIZE : 0))) /
		((horizontal ? this->width - scrollbar_area.width : this->height - scrollbar_area.height) - (arrows ? 2 * SCROLLBAR_ARROW_SIZE : 0)));
	new_value = std::max(min_value, std::min(new_value, max_value));
	current_value = new_value;
	if (horizontal) {
		scrollbar_area.x = ComputeScrollbarOffset(scrollbar_area.width);
	} else {
		scrollbar_area.y = ComputeScrollbarOffset(scrollbar_area.height);
	}
}


void Scrollbar::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse = PGPoint(x - this->x, y - this->y);
	if (button == PGLeftMouseButton) {
		if (arrows) {
			if (PGRectangleContains(arrow_regions[0], mouse)) {
				drag_start = GetTime();
				drag_type = PGScrollbarDragFirstArrow;
				if (current_value > min_value) {
					current_value = std::max(min_value, current_value - 1);
					UpdateScrollValue();
				}
				return;
			} else if (PGRectangleContains(arrow_regions[1], mouse)) {
				drag_start = GetTime();
				drag_type = PGScrollbarDragSecondArrow;
				if (current_value < max_value) {
					current_value = std::min(max_value, current_value + 1);
					UpdateScrollValue();
				}
				return;
			}
		}
		if (modifier & PGModifierShift) {
			// shift clicking a scrollbar moves to the clicked-on value
			lng current_offset = current_value;
			drag_offset = horizontal ? scrollbar_area.width / 2.0f : scrollbar_area.height / 2.0f;

			SetScrollbarOffset(horizontal ? mouse.x - drag_offset : mouse.y - drag_offset);
			if (current_offset != current_value) {
				UpdateScrollValue();
				this->Invalidate();
			}
			this->Invalidate();
		} else {
			if (PGRectangleContains(scrollbar_area, mouse)) {
				drag_offset = horizontal ? mouse.x - scrollbar_area.x : mouse.y - scrollbar_area.y;
				drag_type = PGScrollbarDragScrollbar;
				this->Invalidate();
			} else if (horizontal ? mouse.x < scrollbar_area.x : mouse.y < scrollbar_area.y) {
				// mouse click above/left of scrollbar
				if (current_value > min_value) {
					drag_start = GetTime();
					drag_type = PGScrollbarDragAboveScrollbar;
					current_value = std::max(min_value, current_value - value_size);
					UpdateScrollValue();
				}
			} else {
				// mouse click to the right/below scrollbar
				if (current_value < max_value) {
					drag_start = GetTime();
					drag_type = PGScrollbarDragBelowScrollbar;
					current_value = std::min(max_value, current_value + value_size);
					UpdateScrollValue();
				}
			}
		}
	}
}

void Scrollbar::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (button & PGLeftMouseButton) {
		if (drag_type != PGScrollbarDragNone) {
			drag_type = PGScrollbarDragNone;
			this->Invalidate();
		}
	}
}

void Scrollbar::UpdateScrollValue() {
	if (callback)
		callback(this, current_value);
	this->Invalidate();
}

void Scrollbar::UpdateValues(lng min_value, lng max_value, lng value_size, lng current_value) {
	assert(min_value == 0);
	this->min_value = min_value;
	this->max_value = max_value;
	this->value_size = value_size;
	this->current_value = current_value;
}
