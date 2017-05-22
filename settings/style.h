#pragma once

#include "windowfunctions.h"

#include <map>

enum PGColorType {
	PGColorToggleButtonToggled,
	PGColorNotificationBackground,
	PGColorNotificationError,
	PGColorNotificationInProgress,
	PGColorNotificationWarning,
	PGColorNotificationButton,
	PGColorNotificationText,
	PGColorProjectExplorerText,
	PGColorTextFieldBackground,
	PGColorTextFieldText,
	PGColorTextFieldSelection,
	PGColorTextFieldCaret,
	PGColorTextFieldLineNumber,
	PGColorTextFieldError,
	PGColorTextFieldFindMatch,
	PGColorScrollbarBackground,
	PGColorScrollbarForeground,
	PGColorScrollbarHover,
	PGColorScrollbarDrag,
	PGColorMinimapHover,
	PGColorMinimapDrag,
	PGColorMainMenuBackground,
	PGColorMenuBackground,
	PGColorMenuText,
	PGColorMenuDisabled,
	PGColorMenuHover,
	PGColorTabControlText,
	PGColorTabControlUnsavedText,
	PGColorTabControlBackground,
	PGColorTabControlHover,
	PGColorTabControlSelected,
	PGColorTabControlTemporary,
	PGColorTabControlBorder,
	PGColorStatusBarBackground,
	PGColorStatusBarText,
	PGColorSyntaxString,
	PGColorSyntaxConstant,
	PGColorSyntaxComment,
	PGColorSyntaxOperator,
	PGColorSyntaxFunction,
	PGColorSyntaxKeyword,
	PGColorSyntaxClass1,
	PGColorSyntaxClass2,
	PGColorSyntaxClass3,
	PGColorSyntaxClass4,
	PGColorSyntaxClass5,
	PGColorSyntaxClass6
};

class PGStyle {
public:
	void SetColor(PGColorType, PGColor);
	PGColor* GetColor(PGColorType);

	std::map<PGColorType, PGColor> colors;

	static PGStyle LoadStyle(PGStyle base, nlohmann::json& j);
};

class PGStyleManager {
public:
	static PGStyleManager* GetInstance() {
		static PGStyleManager style;
		return &style;
	}

	static PGFontHandle default_font;

	static PGBitmapHandle GetImage(std::string path) { return GetInstance()->_GetImage(path); }
	static PGColor GetColor(PGColorType type, PGStyle* extra_style = nullptr) { return GetInstance()->_GetColor(type, extra_style); }
	static PGFontHandle GetFont(PGFontType);
private:
	PGStyleManager();

	PGFontHandle textfield_font;
	PGFontHandle menu_font;
	PGFontHandle popup_font;

	PGBitmapHandle _GetImage(std::string path);
	PGColor _GetColor(PGColorType type, PGStyle* extra_style);

	void LoadStyles(const char* text);

	PGStyle default_style;
	PGStyle user_style;

	std::map<std::string, PGBitmapHandle> images;
	std::map<std::string, PGStyle> styles;
};

