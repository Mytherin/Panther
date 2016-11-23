
#include "control.h"

Control::Control(PGWindowHandle handle) {
	this->window = handle;
	this->x = 0;
	this->y = 0;
}

void Control::Draw(PGRendererHandle handle, PGRect* rectangle) {
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

void Control::KeyboardButton(PGButton button, PGModifier modifier) {

}

void Control::KeyboardCharacter(char character, PGModifier modifier) {

}

void Control::KeyboardUnicode(char *character, PGModifier modifier) {

}

void Control::Invalidate() {
	RefreshWindow(this->window);
}

void Control::Invalidate(PGRect rectangle) {
	RefreshWindow(this->window, rectangle);

}

bool Control::HasFocus() {
	return GetFocusedControl(this->window) == this;
}