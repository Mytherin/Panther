
#include "toolbar.h"
#include "style.h"
#include "workspace.h"

#include "button.h"

PGToolbar::PGToolbar(PGWindowHandle window) : 
	PGContainer(window) {
	font = PGCreateFont(PGFontTypeUI);
	SetTextFontSize(font, 13);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));
}

PGToolbar::~PGToolbar() {

}

void PGToolbar::Initialize() {
	std::vector<std::string> image_paths;
	image_paths.push_back("data/icons/undo.png");
	image_paths.push_back("data/icons/redo.png");
	image_paths.push_back("data/icons/back.png");
	image_paths.push_back("data/icons/next.png");
	image_paths.push_back("data/icons/reload.png");
	image_paths.push_back("data/icons/save.png");
	image_paths.push_back("data/icons/saveas.png");
	image_paths.push_back("data/icons/newproject.png");
	image_paths.push_back("data/icons/removeproject.png");
	image_paths.push_back("data/icons/commentselection.png");
	image_paths.push_back("data/icons/decreasecomment.png");
	image_paths.push_back("data/icons/increaseindent.png");
	image_paths.push_back("data/icons/decreaseindent.png");
	image_paths.push_back("data/icons/showallfiles.png");
	image_paths.push_back("data/icons/collapseall.png");
	
	Button* prev_button = nullptr;
	for (int i = 0; i < image_paths.size(); i++) {
		Button* button = new Button(window, this);
		button->SetImage(PGStyleManager::GetImage(image_paths[i]));
		button->padding = PGPadding(4, 4, 4, 4);
		button->fixed_width = TOOLBAR_HEIGHT - button->padding.left - button->padding.right;
		button->fixed_height = TOOLBAR_HEIGHT - button->padding.top - button->padding.bottom;
		button->SetAnchor(PGAnchorLeft | PGAnchorTop);
		button->left_anchor = prev_button;
		button->margin.left = 2;
		button->margin.right = 2;
		button->background_color = PGColor(0, 0, 0, 0);
		button->background_stroke_color = PGColor(0, 0, 0, 0);
		button->background_color_hover = PGStyleManager::GetColor(PGColorTextFieldSelection);
		this->AddControl(std::shared_ptr<Control>(button));
		prev_button = button;
	}
}

void PGToolbar::Draw(PGRendererHandle renderer) {
	std::string name = "Active Project: " + PGGetWorkspace(this->window)->GetName();
	RenderRectangle(renderer, this->GetRectangle(), PGStyleManager::GetColor(PGColorMainMenuBackground), PGDrawStyleFill, 1.0f);
	//RenderLine(renderer, PGLine(X(), Y() + this->height - 1, X() + this->width, Y() + this->height - 1), PGColor(255, 255, 255), 1);
	//RenderText(renderer, font, name.c_str(), name.size(), X(), Y());
	PGContainer::Draw(renderer);
}

PGCursorType PGToolbar::GetCursor(PGPoint mouse) {
	PGCursorType c = PGContainer::GetCursor(mouse);
	return c == PGCursorNone ? PGCursorStandard : c;
}