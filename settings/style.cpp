
#include "style.h"

#include <algorithm>

PGFontHandle PGStyleManager::default_font = nullptr;

void PGStyle::SetColor(PGColorType type, PGColor color) {
	colors[type] = color;
}

PGColor* PGStyle::GetColor(PGColorType type) {
	if (colors.find(type) != colors.end()) {
		return &colors[type];
	}
	return nullptr;
}

PGStyleManager::PGStyleManager() {
	// default fonts
	default_font = PGCreateFont(PGFontTypeTextField);
	textfield_font = PGCreateFont(PGFontTypeTextField);
	menu_font = PGCreateFont(PGFontTypeUI);
	popup_font = PGCreateFont(PGFontTypePopup);

	// standard style set
	PGStyle vs;
	vs.SetColor(PGColorToggleButtonToggled, PGColor(51, 51, 51));
	vs.SetColor(PGColorNotificationBackground, PGColor(51, 51, 51));
	vs.SetColor(PGColorNotificationText, PGColor(238, 238, 238));
	vs.SetColor(PGColorNotificationError, PGColor(190, 17, 0));
	vs.SetColor(PGColorNotificationInProgress, PGColor(65, 105, 225));
	vs.SetColor(PGColorNotificationWarning, PGColor(255, 140, 0));
	vs.SetColor(PGColorNotificationButton, PGColor(14, 99, 156));
	vs.SetColor(PGColorProjectExplorerText, PGColor(222, 222, 222));
	vs.SetColor(PGColorTextFieldBackground, PGColor(30, 30, 30));
	vs.SetColor(PGColorTextFieldText, PGColor(200, 200, 182));
	vs.SetColor(PGColorTextFieldSelection, PGColor(38, 79, 120));
	vs.SetColor(PGColorTextFieldCaret, PGColor(191, 191, 191));
	vs.SetColor(PGColorTextFieldLineNumber, PGColor(43, 145, 175));
	vs.SetColor(PGColorTextFieldError, PGColor(252, 64, 54));
	vs.SetColor(PGColorTextFieldFindMatch, PGColor(101, 51, 6));
	vs.SetColor(PGColorScrollbarBackground, PGColor(62, 62, 66));
	vs.SetColor(PGColorScrollbarForeground, PGColor(104, 104, 104));
	vs.SetColor(PGColorScrollbarHover, PGColor(158, 158, 158));
	vs.SetColor(PGColorScrollbarDrag, PGColor(239, 235, 239));
	vs.SetColor(PGColorMinimapHover, PGColor(255, 255, 255, 96));
	vs.SetColor(PGColorMinimapDrag, PGColor(255, 255, 255, 128));
	vs.SetColor(PGColorMainMenuBackground, PGColor(45, 45, 45));
	vs.SetColor(PGColorMenuBackground, PGColor(27, 27, 28));
	vs.SetColor(PGColorMenuText, PGColor(255, 255, 255));
	vs.SetColor(PGColorMenuDisabled, PGColor(101, 101, 101));
	vs.SetColor(PGColorMenuHover, PGColor(51, 51, 52));
	vs.SetColor(PGColorTabControlText, PGColor(224, 224, 224));
	vs.SetColor(PGColorTabControlBorder, PGColor(104, 104, 104));
	vs.SetColor(PGColorTabControlBackground, PGColor(30, 30, 30));
	vs.SetColor(PGColorTabControlUnsavedText, PGColor(222, 77, 77));
	vs.SetColor(PGColorTabControlHover, PGColor(28, 151, 234));
	vs.SetColor(PGColorTabControlSelected, PGColor(0, 122, 204));
	vs.SetColor(PGColorTabControlTemporary, PGColor(104, 33, 122));
	vs.SetColor(PGColorStatusBarBackground, PGColor(0, 122, 204));
	vs.SetColor(PGColorStatusBarText, PGColor(255, 255, 255));
	vs.SetColor(PGColorSyntaxString, PGColor(214, 157, 110));
	vs.SetColor(PGColorSyntaxConstant, PGColor(181, 206, 168));
	vs.SetColor(PGColorSyntaxComment, PGColor(78, 166, 74));
	vs.SetColor(PGColorSyntaxKeyword, PGColor(86, 156, 214));
	vs.SetColor(PGColorSyntaxOperator, *vs.GetColor(PGColorTextFieldText));
	vs.SetColor(PGColorSyntaxFunction, *vs.GetColor(PGColorTextFieldText));
	vs.SetColor(PGColorSyntaxClass1, PGColor(78, 201, 176));
	vs.SetColor(PGColorSyntaxClass2, PGColor(189, 99, 197));
	vs.SetColor(PGColorSyntaxClass3, PGColor(127, 127, 127));
	vs.SetColor(PGColorSyntaxClass4, *vs.GetColor(PGColorSyntaxClass1));
	vs.SetColor(PGColorSyntaxClass5, *vs.GetColor(PGColorSyntaxClass1));
	vs.SetColor(PGColorSyntaxClass6, *vs.GetColor(PGColorSyntaxClass1));
	this->styles["Visual Studio (Dark)"] = vs;

	PGStyle bp; // Black Panther
	bp.SetColor(PGColorToggleButtonToggled, PGColor(51, 51, 51));
	bp.SetColor(PGColorNotificationBackground, PGColor(51, 51, 51));
	bp.SetColor(PGColorNotificationText, PGColor(238, 238, 238));
	bp.SetColor(PGColorNotificationError, PGColor(190, 17, 0));
	bp.SetColor(PGColorNotificationInProgress, PGColor(65, 105, 225));
	bp.SetColor(PGColorNotificationWarning, PGColor(255, 140, 0));
	bp.SetColor(PGColorNotificationButton, PGColor(14, 99, 156));
	bp.SetColor(PGColorProjectExplorerText, PGColor(222, 222, 222));
	bp.SetColor(PGColorTextFieldBackground, PGColor(45, 45, 45));
	bp.SetColor(PGColorTextFieldText, PGColor(211, 208, 200));
	bp.SetColor(PGColorTextFieldSelection, PGColor(63, 63, 63));
	bp.SetColor(PGColorTextFieldCaret, PGColor(212, 208, 200));
	bp.SetColor(PGColorTextFieldLineNumber, PGColor(128, 127, 123));
	bp.SetColor(PGColorTextFieldError, PGColor(252, 64, 54));
	bp.SetColor(PGColorTextFieldFindMatch, PGColor(128, 64, 32));
	bp.SetColor(PGColorScrollbarBackground, PGColor(62, 62, 66));
	bp.SetColor(PGColorScrollbarForeground, PGColor(104, 104, 104));
	bp.SetColor(PGColorScrollbarHover, PGColor(158, 158, 158));
	bp.SetColor(PGColorScrollbarDrag, PGColor(239, 235, 239));
	bp.SetColor(PGColorMinimapHover, PGColor(255, 255, 255, 96));
	bp.SetColor(PGColorMinimapDrag, PGColor(255, 255, 255, 128));
	bp.SetColor(PGColorMainMenuBackground, PGColor(45, 45, 45));
	bp.SetColor(PGColorMenuBackground, PGColor(27, 27, 28));
	bp.SetColor(PGColorMenuText, PGColor(255, 255, 255));
	bp.SetColor(PGColorMenuDisabled, PGColor(101, 101, 101));
	bp.SetColor(PGColorMenuHover, PGColor(51, 51, 52));
	bp.SetColor(PGColorTabControlText, PGColor(224, 224, 224));
	bp.SetColor(PGColorTabControlBorder, PGColor(104, 104, 104));
	bp.SetColor(PGColorTabControlBackground, PGColor(30, 30, 30));
	bp.SetColor(PGColorTabControlUnsavedText, PGColor(222, 77, 77));
	bp.SetColor(PGColorTabControlHover, PGColor(28, 151, 234));
	bp.SetColor(PGColorTabControlSelected, PGColor(0, 122, 204));
	bp.SetColor(PGColorStatusBarBackground, PGColor(87, 87, 87));
	bp.SetColor(PGColorTabControlTemporary, PGColor(104, 33, 122));
	bp.SetColor(PGColorStatusBarText, PGColor(255, 255, 255));
	bp.SetColor(PGColorSyntaxString, PGColor(153, 204, 153));
	bp.SetColor(PGColorSyntaxConstant, PGColor(249, 145, 87));
	bp.SetColor(PGColorSyntaxComment, PGColor(116, 115, 105));
	bp.SetColor(PGColorSyntaxKeyword, PGColor(240, 128, 128));
	bp.SetColor(PGColorSyntaxOperator, *bp.GetColor(PGColorSyntaxString));
	bp.SetColor(PGColorSyntaxFunction, PGColor(166, 182, 255));
	bp.SetColor(PGColorSyntaxClass1, PGColor(240, 128, 128));
	bp.SetColor(PGColorSyntaxClass2, PGColor(247, 246, 146));
	bp.SetColor(PGColorSyntaxClass3, PGColor(204, 153, 204));
	bp.SetColor(PGColorSyntaxClass4, *bp.GetColor(PGColorSyntaxClass1));
	bp.SetColor(PGColorSyntaxClass5, *bp.GetColor(PGColorSyntaxClass1));
	bp.SetColor(PGColorSyntaxClass6, *bp.GetColor(PGColorSyntaxClass1));
	this->styles["Black Panther"] = bp;

	this->default_style = this->styles["Black Panther"];
}

PGColor PGStyleManager::_GetColor(PGColorType type, PGStyle* extra_style) {
	PGColor* color = nullptr;
	if (extra_style) {
		color = extra_style->GetColor(type);
		if (color) return *color;
	}
	color = user_style.GetColor(type);
	if (color) return *color;

	color = default_style.GetColor(type);
	if (color) return *color;

	return PGColor(255, 255, 255);
}

PGFontHandle PGStyleManager::GetFont(PGFontType type) {
	switch (type) {
		case PGFontTypeTextField:
			return GetInstance()->textfield_font;
		case PGFontTypeUI:
			return GetInstance()->menu_font;
		case PGFontTypePopup:
			return GetInstance()->popup_font;
	}
	return nullptr;
}

PGBitmapHandle PGStyleManager::_GetImage(std::string path) {
	auto entry = images.find(path);
	if (entry != images.end()) {
		return images[path];
	}
	auto handle = PGLoadImage(PGPathJoin(PGApplicationPath(), path));
	if (!handle) {
		return nullptr;
	}
	images[path] = handle;
	return handle;
}
