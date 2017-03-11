#pragma once

#include "container.h"
#include "textfield.h"
#include "tabcontrol.h"

class TextFieldContainer : public PGContainer {
public:
	TextFieldContainer(PGWindowHandle window, std::vector<std::shared_ptr<TextFile>> textfiles);

	virtual PGControlType GetControlType() { return PGControlTypeTextFieldContainer; }

	TextField* textfield;
	TabControl* tabcontrol;
};