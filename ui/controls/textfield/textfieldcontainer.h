#pragma once

#include "container.h"
#include "textfield.h"
#include "tabcontrol.h"

class TextFieldContainer : public PGContainer {
public:
	TextFieldContainer(PGWindowHandle window, std::vector<std::shared_ptr<TextView>> textfiles);

	virtual PGControlType GetControlType() { return PGControlTypeTextFieldContainer; }
	
	virtual void Invalidate(bool initial_invalidate = true) {
		everything_dirty = true;
		PGContainer::Invalidate(initial_invalidate);
	}

	TextField* textfield;
	TabControl* tabcontrol;
};