
#include "tabbedtextfield.h"


TabbedTextField::TabbedTextField(PGWindowHandle window, TextFile* file) : Control(window, true) {
	textfield = new TextField(window, file);
	this->width = 0;
	this->height = TEXT_TAB_HEIGHT;
	textfield->SetAnchor(PGAnchorBottom | PGAnchorLeft | PGAnchorRight);
	textfield->SetPosition(PGPoint(this->x, this->y + TEXT_TAB_HEIGHT));
	textfield->SetSize(PGSize(this->width, this->height - TEXT_TAB_HEIGHT));
	tabs = new TabControl(window, textfield);
	tabs->SetAnchor(PGAnchorLeft | PGAnchorRight);
	tabs->SetPosition(PGPoint(this->x, this->y));
	tabs->SetSize(PGSize(this->width, TEXT_TAB_HEIGHT));
}

bool TabbedTextField::KeyboardButton(PGButton button, PGModifier modifier) {
	if (tabs->KeyboardButton(button, modifier)) {
		return true;
	} else {
		return textfield->KeyboardButton(button, modifier);
	}
}

void TabbedTextField::KeyboardCharacter(char character, PGModifier modifier) {
	textfield->KeyboardCharacter(character, modifier);
}

void TabbedTextField::KeyboardUnicode(char* character, PGModifier modifier) {
	textfield->KeyboardUnicode(character, modifier);
}

void TabbedTextField::PeriodicRender(void) {
	textfield->PeriodicRender();
	tabs->PeriodicRender();
}

void TabbedTextField::Draw(PGRendererHandle renderer, PGIRect* rect) {
	tabs->Draw(renderer, rect);
	textfield->Draw(renderer, rect);
}

void TabbedTextField::MouseClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.y >= TEXT_TAB_HEIGHT) {
		textfield->MouseClick(x, y, button, modifier);
	} else {
		tabs->MouseClick(x, y, button, modifier);
	}
}

void TabbedTextField::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.y >= TEXT_TAB_HEIGHT) {
		textfield->MouseDown(x, y, button, modifier);
	} else {
		tabs->MouseDown(x, y, button, modifier);
	}
}

void TabbedTextField::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.y >= TEXT_TAB_HEIGHT) {
		textfield->MouseUp(x, y, button, modifier);
	} else {
		tabs->MouseUp(x, y, button, modifier);
	}
}

void TabbedTextField::MouseDoubleClick(int x, int y, PGMouseButton button, PGModifier modifier) {
	assert(0);
}

void TabbedTextField::MouseWheel(int x, int y, int distance, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.y >= TEXT_TAB_HEIGHT) {
		textfield->MouseWheel(x, y, distance, modifier);
	} else {
		tabs->MouseWheel(x, y, distance, modifier);
	}
}

void TabbedTextField::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.y >= TEXT_TAB_HEIGHT) {
		textfield->MouseMove(x, y, buttons);
	} else {
		tabs->MouseMove(x, y, buttons);
	}
}

void TabbedTextField::OnResize(PGSize old_size, PGSize new_size) {
	textfield->UpdateParentSize(old_size, new_size);
	tabs->UpdateParentSize(old_size, new_size);
}
