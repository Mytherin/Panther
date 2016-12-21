#pragma once

typedef enum {
	PGCharacterTypeText,
	PGCharacterTypeWhitespace,
	PGCharacterTypePunctuation
} PGCharacterClass;

PGCharacterClass GetCharacterClass(char character);