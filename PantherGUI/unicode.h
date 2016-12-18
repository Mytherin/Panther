#pragma once

#include <string>
#include "utils.h"

//! Returns the amount of characters in the utf8 formatted string, returns -1 if the string is not a valid utf8 string
/* Arguments:
utf8_str: A utf8 formatted string.
type: returns the type of the string (invalid, ASCII or UTF8)
*/
lng utf8_strlen(std::string& utf8_str);

//! Returns the length in bytes of a single utf8 character [1,2,3 or 4] based on the signature of the first byte, returns -1 if the character is not a valid utf8 character
/* Arguments:
utf8_char: The first byte of a utf8 character.
*/
int utf8_character_length(unsigned char utf8_char);

//! Returns the position of the previous character in the UTF8 string
lng utf8_prev_character(char* text, lng current_character);


