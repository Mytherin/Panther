#pragma once

#include "windowfunctions.h"

#include <map>

enum PGColorType {
	PGColorTextFieldBackground,
	PGColorTextFieldText,
	PGColorTextFieldSelection,
	PGColorTextFieldCaret,
	PGColorTextFieldLineNumber,
	PGColorScrollbarBackground,
	PGColorScrollbarForeground,
	PGColorScrollbarHover,
	PGColorScrollbarDrag,
	PGColorMinimapHover,
	PGColorMinimapDrag,
	PGColorTabControlText,
	PGColorTabControlUnsavedText,
	PGColorTabControlBackground,
	PGColorTabControlHover,
	PGColorTabControlSelected,
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
};

class PGStyleManager {
public:
	static PGStyleManager* GetInstance() {
		static PGStyleManager style;
		return &style;
	}

	static PGColor GetColor(PGColorType type, PGStyle* extra_style = nullptr) { return GetInstance()->_GetColor(type, extra_style); }
private:
	PGStyleManager();

	PGColor _GetColor(PGColorType type, PGStyle* extra_style);

	PGStyle default_style;
	PGStyle user_style;

	std::map<std::string, PGStyle> styles;
};

