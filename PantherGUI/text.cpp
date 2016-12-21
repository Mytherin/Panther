
#include "text.h"
#include <cctype>

PGCharacterClass 
GetCharacterClass(char character) {
	if (std::isspace(character)) return PGCharacterTypeWhitespace;
	if (std::ispunct(character)) return PGCharacterTypePunctuation;
	return PGCharacterTypeText;
}