#pragma once

#include "container.h"
#include "searchbox.h"
#include "textfield.h"

struct ScrollData;

class PGGotoAnything : public PGContainer {
public:
	PGGotoAnything(TextField* textfield, PGWindowHandle window, PGGotoType type);
	~PGGotoAnything();

	void Initialize();

	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);

	void Draw(PGRendererHandle renderer);

	void OnResize(PGSize old_size, PGSize new_size);

	void SetType(PGGotoType type);

	void Cancel(bool success);
	void Close(bool success);

	bool ControlTakesFocus() { return true; }

	PG_CONTROL_KEYBINDINGS;

	virtual PGControlType GetControlType() { return PGControlTypeGotoAnything; }
private:
	PGFontHandle font;

	TextField* textfield;

	PGGotoType type;

	SimpleTextField* field;
	SearchBox* box;

	ToggleButton* goto_command;
	ToggleButton* goto_line;
	ToggleButton* goto_file;
	ToggleButton* goto_definition;

	ScrollData* scroll_data = nullptr;
	std::shared_ptr<TextView> current_textfile = nullptr;
	std::shared_ptr<TextView> preview = nullptr;
};