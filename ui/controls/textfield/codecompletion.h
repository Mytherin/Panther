#pragma once

#include "control.h"
#include "scrollbar.h"

enum PGSuggestionType {
	PGSuggestionTypeNone,
	PGSuggestionTypeVariable,
	PGSuggestionTypeFunction,
	PGSuggestionTypeEnum,
	PGSuggestionTypeKeyword
};

struct CodeSuggestion {
	std::string text;
	PGSuggestionType type;

	CodeSuggestion() : text(), type(PGSuggestionTypeNone) { }
	CodeSuggestion(std::string text, PGSuggestionType type) : text(text), type(type) { }
};

class CodeCompletion : public Control {
public:
	CodeCompletion(PGWindowHandle window, std::vector<CodeSuggestion> suggestions);

	void Draw(PGRendererHandle);

	bool KeyboardButton(PGButton button, PGModifier modifier);

	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	void SetSuggestions(std::vector<CodeSuggestion> new_suggestions);

	PGControlType GetControlType() { return PGControlCodeCompletion; }

	PGFontHandle font;
private:
	lng scroll_position;
	lng selected_suggestion;

	
	std::unique_ptr<Scrollbar> scrollbar;

	std::vector<CodeSuggestion> suggestions;
};