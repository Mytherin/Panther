
#include "codecompletion.h"
#include "style.h"


CodeCompletion::CodeCompletion(PGWindowHandle window, std::vector<CodeSuggestion> suggestions) :
	Control(window), scroll_position(0), selected_suggestion(0) {
	this->font = PGStyleManager::GetFont(PGFontTypeTextField);
	SetTextFontSize(this->font, 13);
	SetSuggestions(suggestions);
}

void CodeCompletion::Draw(PGRendererHandle renderer) {
	PGScalar x = X(), y = Y();
	
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height),
		PGStyleManager::GetColor(PGColorTabControlBackground), PGDrawStyleFill);

	SetRenderBounds(renderer, PGRect(x, y, this->width, this->height));

	PGScalar current_y = y;
	for (size_t i = scroll_position; i < suggestions.size(); i++) {
		if (current_y > y + this->height) break;
		if (i == selected_suggestion) {
			RenderRectangle(renderer, PGRect(x, current_y, this->width, GetTextHeight(font)), 
				PGStyleManager::GetColor(PGColorTextFieldSelection), 
				this->ControlHasFocus() ? PGDrawStyleFill : PGDrawStyleStroke);
		}

		SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));

		RenderText(renderer, font, suggestions[i].text.c_str(), suggestions[i].text.size(), x, current_y);
		current_y += GetTextHeight(font);
	}
	ClearRenderBounds(renderer);

	if (scrollbar) {
		scrollbar->Draw(renderer);
	}
}

bool CodeCompletion::KeyboardButton(PGButton button, PGModifier modifier) {
	return false;
}

void CodeCompletion::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (scrollbar) {
		scrollbar->MouseWheel(mouse.x, mouse.y, hdistance, distance, modifier);
	}
}

void CodeCompletion::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse(x - this->x, y - this->y);
	if (scrollbar) {
		scrollbar->MouseDown(mouse.x, mouse.y, button, modifier, click_count);
	}
}

void CodeCompletion::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGPoint mouse(x - this->x, y - this->y);
	if (scrollbar) {
		scrollbar->MouseUp(mouse.x, mouse.y, button, modifier);
	}
}

void CodeCompletion::SetSuggestions(std::vector<CodeSuggestion> new_suggestions) {
	this->suggestions = new_suggestions;
}
