
#include "control.h"

const PGAnchor PGAnchorNone = 0x00;
const PGAnchor PGAnchorLeft = 0x01;
const PGAnchor PGAnchorRight = 0x02;
const PGAnchor PGAnchorTop = 0x04;
const PGAnchor PGAnchorBottom = 0x08;

Control::Control(PGWindowHandle handle, bool reg) {
	if (reg) RegisterControl(handle, this);
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

void Control::MouseWheel(int x, int y, int distance, PGModifier modifier) {

}

void Control::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void Control::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void Control::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void Control::MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier) {

}

void Control::MouseMove(int x, int y, PGMouseButton buttons) {

}

bool Control::KeyboardButton(PGButton button, PGModifier modifier) {
	return false;
}

void Control::KeyboardCharacter(char character, PGModifier modifier) {

}

void Control::KeyboardUnicode(char *character, PGModifier modifier) {

}

void Control::Invalidate() {
	RefreshWindow(this->window);
}

void Control::Invalidate(PGIRect rectangle) {
	RefreshWindow(this->window, rectangle);
}

void Control::Invalidate(PGRect rectangle) {
	this->Invalidate(PGIRect((int)rectangle.x, (int)rectangle.y, (int)rectangle.width, (int)rectangle.height));
}

bool Control::HasFocus() {
	return GetFocusedControl(this->window) == this;
}

void Control::OnResize(PGSize old_size, PGSize new_size) {

}

void Control::UpdateParentSize(PGSize old_size, PGSize new_size) {
	PGSize current_size = PGSize(this->width, this->height);
	int width = new_size.width;
	int height = new_size.height;
	if (anchor & PGAnchorLeft && anchor & PGAnchorRight) {
		this->width = width;
	} else if (anchor & PGAnchorRight) {
		this->width = width - this->x;
	} else if (anchor & PGAnchorLeft) {
		int old = old_size.width - (this->x + this->width);
		this->width = (old - width) + this->x;
	}

	if (anchor & PGAnchorBottom && anchor & PGAnchorTop) {
		this->height = height;
	} else if (anchor & PGAnchorBottom) {
		this->height = height - this->y;
	} else if (anchor & PGAnchorTop) {
		int old = old_size.width - (this->y + this->width);
		this->height = (old - height) + this->y;
	}
	if (this->width != current_size.width || this->height != current_size.height) {
		this->OnResize(current_size, PGSize(this->width, this->height));
	}
}
