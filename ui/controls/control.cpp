
#include "control.h"

const PGAnchor PGAnchorNone = 0x00;
const PGAnchor PGAnchorLeft = 0x01;
const PGAnchor PGAnchorRight = 0x02;
const PGAnchor PGAnchorTop = 0x04;
const PGAnchor PGAnchorBottom = 0x08;

Control::Control(PGWindowHandle handle) :
	percentage_width(-1), percentage_height(-1), fixed_height(-1), fixed_width(-1),
	dirty(false), margin(PGPadding()), padding(PGPadding()), tooltip(nullptr),
	destroyed(false) {
	this->window = handle;
	this->x = 0;
	this->y = 0;
	this->anchor = PGAnchorNone;
	this->visible = true;
}

Control::~Control() {
	if (this->destroy_data.function) {
		this->destroy_data.function(this, destroy_data.data);
	}
	if (tooltip) {
		PGDestroyTooltip(tooltip);
	}
}

void Control::Initialize() {
}

void Control::Draw(PGRendererHandle handle) {
	this->dirty = false;
}

void Control::Update(void) {
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

void Control::Invalidate(bool initial_invalidate) {
	if (initial_invalidate) {
		RefreshWindow(this->window, PGIRect((int)X() - 1, (int)Y() - 1, (int)this->width + 2, (int)this->height + 2), false);
	}
	this->dirty = true;
	if (!this->parent.expired()) {
		this->parent.lock()->Invalidate(false);
	}
}

bool Control::ControlHasFocus() {
	if (this->parent.expired()) {
		return true;
	} else {
		auto parent = this->parent.lock();
		return (parent->ControlHasFocus() && parent->GetActiveControl() == this);
	}
}


bool Control::HasFocus() {
	return GetFocusedControl(this->window) == this;
}

void Control::OnResize(PGSize old_size, PGSize new_size) {
	this->Invalidate();
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
	auto p = parent.lock();
	while (p) {
		point.x += p->x;
		point.y += p->y;
		p = p->parent.lock();
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
		if (fixed_height >= 0) {
			this->height = fixed_height + padding.top + padding.bottom;
		}
		if (top_anchor != nullptr && anchor & PGAnchorTop) {
			assert(margin.bottom != PGMarginAuto && margin.top != PGMarginAuto);
			top_anchor->ResolveSize(new_size);
			this->y = top_anchor->y + top_anchor->height + top_anchor->margin.bottom + this->margin.top;
			if (percentage_height > 0) {
				PGScalar remaining_height = new_size.height - this->y - this->margin.bottom;
				if (bottom_anchor != nullptr) {
					bottom_anchor->ResolveSize(new_size);
					remaining_height = bottom_anchor->y - this->y - this->margin.bottom - bottom_anchor->margin.top;
				}
				this->height = percentage_height * remaining_height;
			}
		} else if (bottom_anchor != nullptr && anchor & PGAnchorBottom) {
			assert(margin.bottom != PGMarginAuto && margin.top != PGMarginAuto);
			bottom_anchor->ResolveSize(new_size);
			if (percentage_height > 0) {
				PGScalar remaining_height = bottom_anchor->y - this->margin.left;
				if (top_anchor != nullptr) {
					top_anchor->ResolveSize(new_size);
					remaining_height -= top_anchor->y + top_anchor->height - top_anchor->margin.bottom;
				}
				this->height = percentage_height * remaining_height;
			}
			this->y = bottom_anchor->y - this->height - bottom_anchor->margin.top - this->margin.bottom;
		} else {
			PGScalar remaining_height = (new_size.height 
				- (margin.top == PGMarginAuto ? 0 : margin.top) 
				- (margin.bottom == PGMarginAuto ? 0 : margin.bottom));
			if (this->percentage_height > 0) {
				this->height = remaining_height * percentage_height;
			}

			if (margin.top == PGMarginAuto && margin.bottom == PGMarginAuto) {
				remaining_height -= this->height;
				this->y = remaining_height / 2.0f;
			} else if (anchor & PGAnchorTop) {
				assert(margin.bottom != PGMarginAuto && margin.top != PGMarginAuto);
				this->y = margin.top;
			} else if (anchor & PGAnchorBottom) {
				assert(margin.bottom != PGMarginAuto && margin.top != PGMarginAuto);
				this->y = new_size.height - this->height - margin.bottom;
			} else {
				assert(0);
			}
		}
	}
	if (fixed_width >= 0 || percentage_width >= 0) {
		if (fixed_width >= 0) {
			this->width = fixed_width + padding.left + padding.right;
		}
		if (left_anchor != nullptr && anchor & PGAnchorLeft) {
			left_anchor->ResolveSize(new_size);
			this->x = left_anchor->x + left_anchor->width + left_anchor->margin.right + this->margin.left;
			if (percentage_width > 0) {
				PGScalar remaining_width = new_size.width - this->x - this->margin.right;
				if (right_anchor != nullptr) {
					right_anchor->ResolveSize(new_size);
					remaining_width = right_anchor->x - this->x - this->margin.right - right_anchor->margin.left;
				}
				this->width = percentage_width * remaining_width;
			}
		} else if (right_anchor != nullptr && anchor & PGAnchorRight) {
			right_anchor->ResolveSize(new_size);
			if (percentage_width > 0) {
				PGScalar remaining_width = right_anchor->x - this->margin.left;
				if (left_anchor != nullptr) {
					left_anchor->ResolveSize(new_size);
					remaining_width -= left_anchor->x + left_anchor->width - left_anchor->margin.right;
				}
				this->width = percentage_width * remaining_width;
			}
			this->x = right_anchor->x - this->width - right_anchor->margin.left - this->margin.right;
		} else {
			if (this->percentage_width > 0) {
				this->width = (new_size.width - margin.left - margin.right) * percentage_width;
			}
			if (anchor & PGAnchorLeft) {
				this->x = margin.left;
			} else if (anchor & PGAnchorRight) {
				this->x = new_size.width - this->width - margin.right;
			} else {
				assert(0);
			}
		}
	}
	if (tooltip) {
		PGUpdateTooltipRegion(tooltip, PGRect(X(), Y(), this->width, this->height));
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

void Control::SetTooltip(std::string tooltip) {
	if (this->tooltip) {
		PGDestroyTooltip(this->tooltip);
	}
	this->tooltip = PGCreateTooltip(this->window, PGRect(X(), Y(), this->width, this->height), tooltip);
}
