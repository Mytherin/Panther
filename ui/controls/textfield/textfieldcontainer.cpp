
#include "textfieldcontainer.h"

TextFieldContainer::TextFieldContainer(PGWindowHandle window, std::vector<std::shared_ptr<TextView>> textfiles) : PGContainer(window) {
	this->width = 0;
	this->height = TEXT_TAB_HEIGHT;

	auto textfield = make_shared_control<TextField>(this->window, nullptr);
	this->textfield = textfield.get();
	textfield->SetAnchor(PGAnchorTop | PGAnchorLeft);
	textfield->percentage_height = 1;
	textfield->percentage_width = 1;
	auto tabcontrol = make_shared_control<TabControl>(this->window, textfield.get(), textfiles);
	this->tabcontrol = tabcontrol.get();
	tabcontrol->SetAnchor(PGAnchorTop | PGAnchorLeft);
	tabcontrol->fixed_height = TEXT_TAB_HEIGHT;
	tabcontrol->percentage_width = 1;
	this->AddControl(tabcontrol);
	this->AddControl(textfield);
	textfield->top_anchor = tabcontrol.get();
}