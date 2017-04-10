
#include "textfieldcontainer.h"

TextFieldContainer::TextFieldContainer(PGWindowHandle window, std::vector<std::shared_ptr<TextFile>> textfiles) : PGContainer(window) {
	this->width = 0;
	this->height = TEXT_TAB_HEIGHT;
	this->textfield = new TextField(this->window, nullptr);
	textfield->SetAnchor(PGAnchorTop);
	textfield->percentage_height = 1;
	textfield->percentage_width = 1;
	this->tabcontrol = new TabControl(this->window, textfield, textfiles);
	tabcontrol->SetAnchor(PGAnchorTop | PGAnchorLeft);
	tabcontrol->fixed_height = TEXT_TAB_HEIGHT;
	tabcontrol->percentage_width = 1;
	this->AddControl(tabcontrol);
	this->AddControl(textfield);
	textfield->vertical_anchor = tabcontrol;
}