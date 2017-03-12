
#include "control.h"

const PGAnchor PGAnchorNone = 0x00;
const PGAnchor PGAnchorLeft = 0x01;
const PGAnchor PGAnchorRight = 0x02;
const PGAnchor PGAnchorTop = 0x04;
const PGAnchor PGAnchorBottom = 0x08;

Control::Control(PGWindowHandle handle) : 
	percentage_width(-1), percentage_height(-1), fixed_height(-1), fixed_width(-1) {
	this->window = handle;
	this->x = 0;
	this->y = 0;
	this->anchor = PGAnchorNone;
	this->parent = nullptr;
	this->visible = true;
}

void Control::Draw(PGRendererHandle handle, PGIRect* rectangle) {
}

void Control::PeriodicRender(void) {
}

void Control::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {

}

void Control::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {

}

void Control::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {

}


void Control::MouseMove(int x, int y, PGMouseButton buttons) {

}

bool Control::AcceptsDragDrop(PGDragDropType type) {
	return false;
}

void Control::DragDrop(PGDragDropType type, int x, int y, void* data) {

}

void Control::PerformDragDrop(PGDragDropType type, int x, int y, void* data) {

}

void Control::ClearDragDrop(PGDragDropType type) {

}

void Control::LosesFocus(void) {

}

void Control::GainsFocus(void) {

}

bool Control::KeyboardButton(PGButton button, PGModifier modifier) {
	return false;
}

bool Control::KeyboardCharacter(char character, PGModifier modifier) {
	return false;
}

bool Control::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	return false;
}

void Control::Invalidate(bool redraw_now) {
	RefreshWindow(this->window, PGIRect((int)X() - 1, (int)Y() - 1, (int)this->width + 2, (int)this->height + 2), redraw_now);
}

void Control::Invalidate(PGIRect rectangle, bool redraw_now) {
	this->Invalidate(redraw_now);
}

void Control::Invalidate(PGRect rectangle, bool redraw_now) {
	this->Invalidate(PGIRect((int)rectangle.x, (int)rectangle.y, (int)rectangle.width, (int)rectangle.height), redraw_now);
}

bool Control::HasFocus() {
	return GetFocusedControl(this->window) == this;
}

void Control::OnResize(PGSize old_size, PGSize new_size) {

}

void Control::LoadWorkspace(nlohmann::json& j) {

}

void Control::WriteWorkspace(nlohmann::json& j) {

}

void Control::TriggerResize() {
	this->OnResize(PGSize(this->width, this->height), PGSize(this->width, this->height));
}

void Control::SetSize(PGSize size) {
	PGSize oldsize(this->width, this->height);
	this->width = size.width; 
	this->height = size.height;
	this->OnResize(oldsize, PGSize(this->width, this->height));
}

bool Control::IsDragging() {
	return false;
}

PGScalar Control::X() {
	return Position().x;
}

PGScalar Control::Y() {
	return Position().y;
}

PGPoint Control::Position() {
	PGPoint point = PGPoint(x, y);
	Control* p = parent;
	while (p) {
		point.x += p->x;
		point.y += p->y;
		p = p->parent;
	}
	return point;
}

void Control::MouseEnter() {

}

void Control::MouseLeave() {

}

void Control::ResolveSize(PGSize new_size) {
	if (size_resolved) return;
	PGSize current_size = PGSize(this->width, this->height);
	if (fixed_height >= 0 || percentage_height >= 0) {
		if (vertical_anchor == nullptr) {
			if (fixed_height > 0) {
				this->height = fixed_height;
			} else {
				this->height = percentage_height * new_size.height;
			}
			if (this->anchor & PGAnchorTop) {
				this->y = 0;
			}
			if (this->anchor & PGAnchorBottom) {
				// bottom anchor only makes sense with a fixed height
				this->y = new_size.height - this->height;
			}
		} else {
			vertical_anchor->ResolveSize(new_size);
			PGScalar remaining_height = 0;
			if (fixed_height > 0) {
				this->height = fixed_height;
			}
			if (this->anchor & PGAnchorTop) {
				this->y = vertical_anchor->y + vertical_anchor->height;
				remaining_height = new_size.height - this->y;
			}
			if (this->anchor & PGAnchorBottom) {
				remaining_height = vertical_anchor->y;
			}
			if (percentage_height > 0) {
				this->height = percentage_height * remaining_height;
			}
			if (this->anchor & PGAnchorBottom) {
				this->y = vertical_anchor->y - this->height;
			}
		}
	}
	if (fixed_width >= 0 || percentage_width >= 0) {
		if (horizontal_anchor == nullptr) {
			if (fixed_width >= 0) {
				this->width = fixed_width;
			} else {
				assert(percentage_width > 0);
				this->width = percentage_width * new_size.width;
			}
			if (this->anchor & PGAnchorLeft) {
				this->x = 0;
			}
			if (this->anchor & PGAnchorRight) {
				// right anchor only makes sense with a fixed width
				assert(this->fixed_width > 0);
				this->x = new_size.width - this->width;
			}
		} else {
			horizontal_anchor->ResolveSize(new_size);
			PGScalar remaining_width = 0;
			if (fixed_width > 0) {
				this->width = fixed_width;
				assert(percentage_width < 0);
			}
			if (this->anchor & PGAnchorLeft) {
				this->x = horizontal_anchor->x + horizontal_anchor->width;
				remaining_width = new_size.width - this->x;
			}
			if (this->anchor & PGAnchorRight) {
				remaining_width = horizontal_anchor->x;
			}
			if (percentage_width > 0) {
				this->width = percentage_width * remaining_width;
			}
			if (this->anchor & PGAnchorRight) {
				this->x = horizontal_anchor->x - this->width;
			}
			this->size_resolved = true;
		}
	}
	this->size_resolved = true;
	this->OnResize(current_size, PGSize(this->width, this->height));
}

bool Control::PressKey(std::map<PGKeyPress, PGKeyFunctionCall>& keybindings, PGButton button, PGModifier modifier) {
	PGKeyPress press;
	press.button = button;
	press.modifier = modifier;
	if (keybindings.count(press) > 0) {
		keybindings[press].Call(this);
		return true;
	}
	return false;
}

bool Control::PressCharacter(std::map<PGKeyPress, PGKeyFunctionCall>& keybindings, char character, PGModifier modifier) {
	PGKeyPress press;
	press.character = character;
	press.modifier = modifier;
	if (keybindings.find(press) != keybindings.end()) {
		keybindings[press].Call(this);
		return true;
	}
	return false;
}

bool Control::PressMouseButton(std::map<PGMousePress, PGMouseFunctionCall>& mousebindings, PGMouseButton button, PGPoint mouse, PGModifier modifier, int clicks, lng line, lng character) {
	PGMousePress press;
	press.button = button;
	press.modifier = modifier;
	press.clicks = clicks;
	if (mousebindings.find(press) != mousebindings.end()) {
		mousebindings[press].Call(this, button, mouse, line, character);
		return true;
	}
	return false;
}
