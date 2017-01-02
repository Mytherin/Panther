
#include "text.h"
#include <cctype>

PGCharacterClass 
GetCharacterClass(char character) {
	if (std::isspace(character)) return PGCharacterTypeWhitespace;
	if (std::ispunct(character)) return PGCharacterTypePunctuation;
	if (character == '\n' || character == '\0') return PGCharacterTypeWhitespace;
	return PGCharacterTypeText;
}