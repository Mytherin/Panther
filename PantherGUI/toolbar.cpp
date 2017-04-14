
#include "toolbar.h"
#include "style.h"
#include "workspace.h"

PGToolbar::PGToolbar(PGWindowHandle window) : 
	PGContainer(window) {
	font = PGCreateFont("myriad", false, false);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));
}

PGToolbar::~PGToolbar() {

}

void PGToolbar::Draw(PGRendererHandle renderer) {
	std::string name = "Active Project: " + PGGetWorkspace(this->window)->GetName();
	RenderRectangle(renderer, this->GetRectangle(), PGStyleManager::GetColor(PGColorMainMenuBackground), PGDrawStyleFill, 1.0f);
	RenderLine(renderer, PGLine(X(), Y() + this->height - 1, X() + this->width, Y() + this->height - 1), PGColor(255, 255, 255), 1);
	RenderText(renderer, font, name.c_str(), name.size(), X(), Y());
	PGContainer::Draw(renderer);
}

PGCursorType PGToolbar::GetCursor(PGPoint mouse) {
	PGCursorType c = PGContainer::GetCursor(mouse);
	return c == PGCursorNone ? PGCursorStandard : c;
}