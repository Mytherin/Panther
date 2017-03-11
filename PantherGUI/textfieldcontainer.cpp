
#include "textfieldcontainer.h"

TextFieldContainer::TextFieldContainer(PGWindowHandle window, std::vector<std::shared_ptr<TextFile>> textfiles) : PGContainer(window) {
	this->width = 0;
	this->height = TEXT_TAB_HEIGHT;
	TextField* textfield = new TextField(this->window, textfiles[0]);
	textfield->SetAnchor(PGAnchorTop);
	textfield->percentage_height = 1;
	textfield->percentage_width = 1;
	TabControl* tabs = new TabControl(this->window, textfield, textfiles);
	tabs->SetAnchor(PGAnchorTop | PGAnchorLeft);
	tabs->fixed_height = TEXT_TAB_HEIGHT;
	tabs->percentage_width = 1;
	this->AddControl(tabs);
	this->AddControl(textfield);
	textfield->vertical_anchor = tabs;
}