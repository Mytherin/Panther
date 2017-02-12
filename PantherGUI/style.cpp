
#include "style.h"

#include <algorithm>


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
	// standard style set
	PGStyle vs;
	vs.SetColor(PGColorNotificationBackground, PGColor(51, 51, 51));
	vs.SetColor(PGColorNotificationText, PGColor(238, 238, 238));
	vs.SetColor(PGColorNotificationError, PGColor(190, 17, 0));
	vs.SetColor(PGColorNotificationWarning, PGColor(255, 140, 0));
	vs.SetColor(PGColorNotificationButton, PGColor(14, 99, 156));
	vs.SetColor(PGColorProjectExplorerText, PGColor(222, 222, 222));
	vs.SetColor(PGColorTextFieldBackground, PGColor(30, 30, 30));
	vs.SetColor(PGColorTextFieldText, PGColor(200, 200, 182));
	vs.SetColor(PGColorTextFieldSelection, PGColor(38, 79, 120));
	vs.SetColor(PGColorTextFieldCaret, PGColor(191, 191, 191));
	vs.SetColor(PGColorTextFieldLineNumber, PGColor(43, 145, 175));
	vs.SetColor(PGColorTextFieldError, PGColor(252, 64, 54));
	vs.SetColor(PGColorScrollbarBackground, PGColor(62, 62, 66));
	vs.SetColor(PGColorScrollbarForeground, PGColor(104, 104, 104));
	vs.SetColor(PGColorScrollbarHover, PGColor(158, 158, 158));
	vs.SetColor(PGColorScrollbarDrag, PGColor(239, 235, 239));
	vs.SetColor(PGColorMinimapHover, PGColor(255, 255, 255, 96));
	vs.SetColor(PGColorMinimapDrag, PGColor(255, 255, 255, 128));
	vs.SetColor(PGColorMenuBackground, PGColor(27, 27, 28));
	vs.SetColor(PGColorMenuText, PGColor(241, 241, 241));
	vs.SetColor(PGColorMenuDisabled, PGColor(101, 101, 101));
	vs.SetColor(PGColorMenuHover, PGColor(51, 51, 52));
	vs.SetColor(PGColorTabControlText, PGColor(224, 224, 224));
	vs.SetColor(PGColorTabControlBorder, PGColor(104, 104, 104));
	vs.SetColor(PGColorTabControlBackground, PGColor(30, 30, 30));
	vs.SetColor(PGColorTabControlUnsavedText, PGColor(222, 77, 77));
	vs.SetColor(PGColorTabControlHover, PGColor(28, 151, 234));
	vs.SetColor(PGColorTabControlSelected, PGColor(0, 122, 204));
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

	this->default_style = this->styles["Visual Studio (Dark)"];
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

PGBitmapHandle PGStyleManager::_GetImage(std::string path) {
	auto entry = images.find(path);
	if (entry != images.end()) {
		return images[path];
	}
	auto handle = PGLoadImage(path);
	if (!handle) {
		return nullptr;
	}
	images[path] = handle;
	return handle;
}
