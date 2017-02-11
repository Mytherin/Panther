#pragma once

#include "keywords.h"
#include "language.h"


class CHighlighter : public KeywordHighlighter {
public:
	CHighlighter();
};

class CLanguage : public PGLanguage {
	std::string GetName() { return "C"; }
	SyntaxHighlighter* CreateHighlighter() { return new CHighlighter(); }
	bool MatchesFileExtension(std::string extension) { return extension == "h" || extension == "c"; }
	std::string GetExtension() { return "C"; }
	PGColor GetColor() { return PGColor(239, 245, 43); }
};
