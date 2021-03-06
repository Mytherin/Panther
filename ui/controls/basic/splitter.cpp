
#include "splitter.h"
#include "style.h"


Splitter::Splitter(PGWindowHandle window, bool horizontal) : 
	Control(window), is_dragging(false), horizontal(horizontal) {
}

void Splitter::Draw(PGRendererHandle renderer) {
	RenderRectangle(renderer, PGRect(X(), Y(), this->width, this->height), PGStyleManager::GetColor(PGColorTabControlBackground), PGDrawStyleFill);
	Control::Draw(renderer);
}

void Splitter::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse(x - this->x, y - this->y);
	if (button & PGLeftMouseButton) {
		drag_offset = horizontal ? mouse.x : mouse.y;
		is_dragging = true;
	}
}

void Splitter::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	if (button & PGLeftMouseButton) {
		is_dragging = false;
	}
}

void Splitter::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	if (buttons & PGLeftMouseButton) {
		auto parent = this->parent.lock();
		if (horizontal) {
			PGScalar new_width = x - this->left_anchor->x - drag_offset;
			if (this->left_anchor->fixed_width >= 0) {
				// fixed width resize
				this->left_anchor->fixed_width = panther::clamp(new_width, this->left_anchor->minimum_width, this->left_anchor->maximum_width);
			} else {
				// percentage width resize
				// percentage_width * (this->parent->width - horizontal_anchor->x) == new_width
				this->left_anchor->percentage_width = new_width / (parent->width - left_anchor->x);
			}
			for (auto it = additional_anchors.begin(); it != additional_anchors.end(); it++) {
				if ((*it)->fixed_width >= 0) {
					(*it)->fixed_width = panther::clamp(new_width, (*it)->minimum_width, (*it)->maximum_width);
				} else {
					(*it)->percentage_width = new_width / (parent->width - (*it)->x);
				}
			}
		} else {
			PGScalar new_height = parent->height - y + drag_offset;
			if (this->bottom_anchor->fixed_height >= 0) {
				this->bottom_anchor->fixed_height = panther::clamp(new_height, this->bottom_anchor->minimum_height, this->bottom_anchor->maximum_height);
			} else {
				this->bottom_anchor->percentage_height = new_height / (parent->height);
			}
			for (auto it = additional_anchors.begin(); it != additional_anchors.end(); it++) {
				if ((*it)->fixed_height >= 0) {
					(*it)->fixed_height = panther::clamp(new_height, (*it)->minimum_height, (*it)->maximum_height);
				} else {
					(*it)->percentage_height = new_height / (parent->height);
				}
			}
		}
		parent->TriggerResize();
		parent->Invalidate();
	} else {
		is_dragging = false;
	}
}