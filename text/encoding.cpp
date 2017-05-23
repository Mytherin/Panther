
#include "encoding.h"
#include "utils.h"
#include "logger.h"

#include <malloc.h>
#include <algorithm>

#include "unicode.h"
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>

#include <map>

std::map<PGFileEncoding, std::string> readable_name_map;
std::map<std::string, PGFileEncoding> readable_name_map_inverse;
std::map<PGFileEncoding, std::string> icuname_map;
std::map<std::string, PGFileEncoding> icuname_map_inverse;

struct PGEncoder {
	PGFileEncoding source_encoding;
	PGFileEncoding target_encoding;
	UConverter* source = nullptr;
	UConverter* target = nullptr;
};


static void AddEncoding(PGFileEncoding encoding, std::string readable_name, std::string icuname) {
	readable_name_map[encoding] = readable_name;
	readable_name_map_inverse[readable_name] = encoding;
	icuname_map[encoding] = icuname;
	icuname_map_inverse[icuname] = encoding;
}

void PGInitializeEncodings() {
	AddEncoding(PGEncodingUTF8, "UTF-8", "UTF-8");
	AddEncoding(PGEncodingUTF8BOM, "UTF-8 with BOM", "UTF-8");
	AddEncoding(PGEncodingUTF16, "UTF-16", "UTF-16");
	AddEncoding(PGEncodingUTF16Platform, "UTF-16 Platform", "UTF16_PlatformEndian");
	AddEncoding(PGEncodingUTF32, "UTF-32", "UTF-32");
	AddEncoding(PGEncodingUTF16BE, "UTF-16 BE", "UTF-16BE");
	AddEncoding(PGEncodingUTF16BEBOM, "UTF-16 BE with BOM", "UTF-16BE");
	AddEncoding(PGEncodingUTF16LE, "UTF-16 LE", "UTF-16LE");
	AddEncoding(PGEncodingUTF16LEBOM, "UTF-16 LE with BOM", "UTF-16LE");
	AddEncoding(PGEncodingUTF32BE, "UTF-32 BE", "UTF-32BE");
	AddEncoding(PGEncodingUTF32BEBOM, "UTF-32 BE with BOM", "UTF-32BE");
	AddEncoding(PGEncodingUTF32LE, "UTF-32 LE", "UTF-32LE");
	AddEncoding(PGEncodingUTF32LEBOM, "UTF-32 LE with BOM", "UTF-32LE");
	AddEncoding(PGEncodingWesternISO8859_1, "ISO-8859-1", "ISO-8859-1");
	AddEncoding(PGEncodingWesternISO8859_2, "ISO-8859-2", "ISO-8859-2");
	AddEncoding(PGEncodingWesternISO8859_9, "ISO-8859-9", "ISO-8859-9");
	AddEncoding(PGEncodingNordicISO8859_10, "ISO-8859-10", "iso-8859_10-1998");
	AddEncoding(PGEncodingCelticISO8859_14, "ISO-8859-14", "iso-8859_14-1998");
	AddEncoding(PGEncodingWesternWindows1250, "Windows-1250", "windows-1250");
	AddEncoding(PGEncodingWesternWindows1252, "Windows-1252", "windows-1252");
	AddEncoding(PGEncodingBinary, "Binary", "Binary");
	AddEncoding(PGEncodingUnknown, "Unknown", "Unknown");
}

std::string PGEncodingToString(PGFileEncoding encoding) {
	auto entry = readable_name_map.find(encoding);
	if (entry != readable_name_map.end()) {
		return entry->second;
	}
	return "Unknown";
}

PGFileEncoding PGEncodingFromString(std::string encoding) {
	auto entry = readable_name_map_inverse.find(encoding);
	if (entry != readable_name_map_inverse.end()) {
		return entry->second;
	}
	return PGEncodingUnknown;
}

static std::string GetEncodingName(PGFileEncoding encoding) {
	auto entry = icuname_map.find(encoding);
	if (entry != icuname_map.end()) {
		return entry->second;
	}
	return "Unknown";
}

static PGFileEncoding GetEncodingFromName(std::string encoding) {
	auto entry = icuname_map_inverse.find(encoding);
	if (entry != icuname_map_inverse.end()) {
		return entry->second;
	}
	return PGEncodingUnknown;
}

void LogAvailableEncodings() {
	/* Returns count of the number of available names */
	int count = ucnv_countAvailable();

	/* get the canonical name of the 36th available converter */
	for (int i = 0; i < count; i++) {
		Logger::WriteLogMessage(std::string(ucnv_getAvailableName(i)));
	}
}

PGEncoderHandle PGCreateEncoder(PGFileEncoding source_encoding, PGFileEncoding target_encoding) {
	assert(source_encoding != PGEncodingUnknown && target_encoding != PGEncodingUnknown);
	UErrorCode error = U_ZERO_ERROR;

	// assure that both encodings are implemented
	std::string source = GetEncodingName(source_encoding);
	std::string target = GetEncodingName(target_encoding);

	assert(source != "Unknown" && target != "Unknown");

	PGEncoderHandle handle = new PGEncoder();
	handle->source_encoding = source_encoding;
	handle->target_encoding = target_encoding;

	if (handle->source_encoding != PGEncodingBinary && handle->target_encoding != PGEncodingBinary) {
		// create the converters
		// binary encoding is handled by us, not by ICU
		// so we don't create an ICU converter for that encoding
		handle->source = ucnv_open(source.c_str(), &error);
		if (U_FAILURE(error)) {
			// failed to create a converter
			delete handle;
			return nullptr;
		}
		handle->target = ucnv_open(target.c_str(), &error);
		if (U_FAILURE(error)) {
			// failed to create a converter
			ucnv_close(handle->source);
			delete handle;
			return nullptr;
		}
	}

	return handle;
}

lng PGConvertText(PGEncoderHandle encoder, std::string input, char** output, lng* output_size, char** intermediate_buffer, lng* intermediate_size) {
	return PGConvertText(encoder, input.c_str(), input.size(), output, output_size, intermediate_buffer, intermediate_size);
}

static inline bool is_hex(char value) {
	return (value > '0' && value < '9') ||
		(value > 'A' && value < 'F') ||
		(value > 'a' && value < 'f');
}

static inline char hex2byte(char value) {
	if (value > '0' && value < '9') {
		return value - '0';
	}
	if (value > 'A' && value < 'F') {
		return value - 'A' + 10;
	}
	if (value > 'a' && value < 'f') {
		return value - 'a' + 10;
	}
	assert(0);
	return '\0';
}

static bool hex2byte(size_t& input_position, const char* input, size_t input_size, char* output_buffer) {
	// first skip all non-hex bytes
	while (input_position < input_size && !is_hex(input[input_position])) input_position++;
	// now read all the hex bytes
	lng start = input_position;
	while (input_position < input_size && is_hex(input[input_position])) input_position++;
	if (input_position >= input_size) return false;
	assert(input_position > start);
	size_t multiplier = 1;
	size_t value = 0;
	for (lng i = input_position - 1; i >= start; i--) {
		value += hex2byte(input[i]) * multiplier;
		multiplier *= 16;
	}
	if (value > 255) value = 0;
	*output_buffer = value;
	return true;
}

lng PGConvertText(PGEncoderHandle encoder, const char* input_text, size_t input_size, char** output, lng* output_size, char** intermediate_buffer, lng* intermediate_size) {
	if (encoder->source_encoding == PGEncodingBinary || encoder->target_encoding == PGEncodingBinary) {
		if (encoder->source_encoding == PGEncodingBinary) {
			assert(encoder->target_encoding == PGEncodingUTF8);
			lng output_buffer_size = input_size * 3 + 1;
			char* output_buffer = (char*)malloc(output_buffer_size);
			lng pos = -1;
			const char * hex = "0123456789ABCDEF";
			for (lng i = 0; i < input_size; i++) {
				output_buffer[++pos] = hex[(input_text[i] >> 4) & 0xF];
				output_buffer[++pos] = hex[input_text[i] & 0xF];
				output_buffer[++pos] = (i + 1) % 16 == 0 ? '\n' : ' ';
			}
			output_buffer[pos] = '\0';
			assert(pos < output_buffer_size);
			*output = output_buffer;
			*output_size = output_buffer_size;
			return output_buffer_size;
		} else if (encoder->target_encoding == PGEncodingBinary) {
			assert(encoder->source_encoding == PGEncodingUTF8);
			lng output_buffer_size = input_size / 2;
			char* output_buffer = (char*)malloc(output_buffer_size);
			size_t input_position = 0, output_position = 0;
			while (hex2byte(input_position, input_text, input_size, output_buffer + output_position)) {
				output_position++;
				assert(output_position < output_buffer_size);
			}
			output_buffer_size = output_position;
			*output = output_buffer;
			*output_size = output_buffer_size;
			return output_buffer_size;
		}
	}


	lng return_size = -1;
	char* result_buffer = nullptr;
	UChar* buffer = nullptr;
	UErrorCode error = U_ZERO_ERROR;

	// first we convert the source encoding to the internal ICU representation (UChars)
	size_t targetsize = *intermediate_size;
	buffer = *((UChar**)intermediate_buffer);
	targetsize = ucnv_toUChars(encoder->source, buffer, targetsize, input_text, input_size, &error);
	if (error == U_BUFFER_OVERFLOW_ERROR) {
		error = U_ZERO_ERROR;
		if (buffer)
			free(buffer);
		buffer = (UChar*)malloc(targetsize * sizeof(UChar));
		*intermediate_buffer = (char*)buffer;
		*intermediate_size = targetsize;
		targetsize = ucnv_toUChars(encoder->source, buffer, targetsize, input_text, input_size, &error);
	}
	if (U_FAILURE(error)) {
		// failed source conversion
		return -1;
	}
	// now convert the source to the target encoding
	size_t result_size = targetsize * 4;
	if (*output_size < result_size) {
		result_buffer = (char*)malloc(result_size);
		*output_size = result_size;
		*output = result_buffer;
	} else {
		result_buffer = *output;
	}
	result_size = ucnv_fromUChars(encoder->target, result_buffer, result_size, buffer, targetsize, &error);
	if (U_FAILURE(error)) {
		// failed source conversion
		return -1;
	}
	return result_size;
}


lng PGConvertText(PGEncoderHandle encoder, std::string input, char** output) {
	lng output_size = 0;
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;

	lng result_size = PGConvertText(encoder, input, output, &output_size, &intermediate_buffer, &intermediate_size);
	if (intermediate_buffer) free(intermediate_buffer);
	return result_size;
}

void PGDestroyEncoder(PGEncoderHandle handle) {
	if (handle) {
		if (handle->source) {
			ucnv_close(handle->source);
		}
		if (handle->target) {
			ucnv_close(handle->target);
		}
		delete handle;
	}
}

lng PGConvertText(std::string input, char** output, PGFileEncoding source_encoding, PGFileEncoding target_encoding) {
	PGEncoderHandle encoder = PGCreateEncoder(source_encoding, target_encoding);
	if (!encoder) return -1;
	lng return_size = PGConvertText(encoder, input, output);
	PGDestroyEncoder(encoder);
	return return_size;
}

#define MAXIMUM_TEXT_SAMPLE 1024

PGFileEncoding PGGuessEncoding(char* input_text, size_t input_size) {
	if (input_size == 0) {
		// default encoding is UTF-8
		return PGEncodingUTF8;
	}

	// check for common known bit patterns

	// application/postscript
	if (input_size > 10 && !memcmp(input_text, "%!PS-Adobe-", 11))
		return PGEncodingUnknown;

	// image/png
	if (input_size > 7 && !memcmp(input_text, "\x89PNG\x0D\x0A\x1A\x0A", 8))
		return PGEncodingBinary;

	// image/gif
	if (input_size > 5) {
		if (!memcmp(input_text, "GIF87a", 6))
			return PGEncodingBinary;
		if (!memcmp(input_text, "GIF89a", 6))
			return PGEncodingBinary;
	}

	// image/jpeg
	if (input_size > 2 && !memcmp(input_text, "\xFF\xD8\xFF", 3))
		return PGEncodingBinary;

	if (input_size > 4 && !memcmp(input_text, "%PDF-", 5))
		return PGEncodingBinary;

	if (input_size > 2 && !memcmp(input_text, "\xef\xbb\xbf", 3)) {
		// UTF-8 with BOM
		return PGEncodingUTF8BOM;
	}
	if (input_size > 1) {
		// UTF-16BE
		if (!memcmp(input_text, "\xfe\xff", 2))
			return PGEncodingUTF16BE;
		// UTF-16LE
		if (!memcmp(input_text, "\xff\xfe", 2))
			return PGEncodingUTF16LE;
	}

	if (input_size > 3) {
		// UTF-32BE
		if (!memcmp(input_text, "\0\0\xfe\xff", 4))
			return PGEncodingUTF32BE;

		// UTF-32LE
		if (!memcmp(input_text, "\xff\xfe\0\0", 4))
			return PGEncodingUTF32LE;
	}

	// if no common headers are found, check the data for valid UTF data
	// first check for valid UTF-8
	bool valid_utf8 = true;
	for (size_t i = 0; i < input_size; i++) {
		if (input_text[i] == '\0') {
			valid_utf8 = false;
			break;
		}
		int charlen = utf8_character_length(input_text[i]);
		if (charlen <= 0) {
			valid_utf8 = false;
			break;
		} else if (i + charlen - 1 >= input_size) {
			break;
		} else {
			for (size_t j = 0; j < charlen - 1; j++) {
				if ((input_text[++i] & 0b11000000) != 0b10000000) {
					valid_utf8 = false;
					break;
				}
			}
		}
	}
	if (valid_utf8) {
		return PGEncodingUTF8;
	}
	// check for valid UTF-16BE and UTF-16LE
	// because UTF-16BE and UTF-16LE are so lenient we have an additional requirement
	// the space character must exist for the encoding to be chosen
	bool valid_utf16be = true; bool utf16be_pair = false;
	int utf16be_spaces = 0, utf16le_spaces = 0;
	bool valid_utf16le = true; bool utf16le_pair = false;
	for (size_t i = 0; i < input_size - 1; i += 2) {
		if (!valid_utf16be && !valid_utf16le) break;
		if (valid_utf16be) {
			if (!utf16be_pair) {
				// the first byte of the pair must either be
				// (1) smaller than 0xD8
				// (2) start with 0b110110
				if (input_text[i + 1] >= 0xD8) {
					if (input_text[i + 1] & 0b11111100 == 0b11011000) {
						// first byte of four-byte pair
						// next byte may be anything (0-255)
						// but byte afterwards must start with 0b110111
						utf16be_pair = true;
					} else {
						valid_utf16be = false;
					}
				} else {
					if (input_text[i + 1] == 0x20 && input_text[i] == 0x00) {
						utf16be_spaces++;
					}
				}
			} else {
				if (input_text[i + 1] & 0b11111100 != 0b11011100) {
					valid_utf16be = false;
				}
				utf16be_pair = false;
			}
		}
		if (valid_utf16le) {
			if (!utf16le_pair) {
				// the second byte of the pair must either be
				// (1) smaller than 0xD8
				// (2) start with 0b110110
				if (input_text[i] >= 0xD8) {
					if (input_text[i] & 0b11111100 == 0b11011000) {
						// first byte of four-byte pair
						// next byte may be anything (0-255)
						// but byte afterwards must start with 0b110111
						utf16be_pair = true;
					} else {
						valid_utf16be = false;
					}
				} else {
					if (input_text[i] == 0x20 && input_text[i + 1] == 0x00) {
						utf16le_spaces++;
					}
				}
			} else {
				if (input_text[i] & 0b11111100 != 0b11011100) {
					valid_utf16le = false;
				}
				utf16le_pair = false;
			}
		}
	}
	if (utf16be_spaces == 0) valid_utf16be = false;
	if (utf16le_spaces == 0) valid_utf16le = false;

	if (valid_utf16be && valid_utf16le) {
		return utf16be_spaces > utf16le_spaces ? PGEncodingUTF16BE : PGEncodingUTF16LE;
	}
	if (valid_utf16be) {
		return PGEncodingUTF16BE;
	}
	if (valid_utf16le) {
		return PGEncodingUTF16LE;
	}
	// if no common headers are found, check for NULL bytes in the data
	// if a NULL byte is present we assume the text is binary
	// otherwise we guess the encoding using ICU
	if (!!memchr(input_text, 0, input_size)) {
		return PGEncodingBinary;
	}
	UCharsetDetector* csd = nullptr;
	const UCharsetMatch *ucm = nullptr;
	UErrorCode status = U_ZERO_ERROR;
	// if we still have no idea, use ICU to guess
	csd = ucsdet_open(&status);
	if (U_FAILURE(status)) {
		return PGEncodingUnknown;
	}
	ucsdet_setText(csd, input_text, input_size, &status);
	if (U_FAILURE(status)) {
		return PGEncodingUnknown;
	}
	ucm = ucsdet_detect(csd, &status);
	if (U_FAILURE(status)) {
		return PGEncodingUnknown;
	}
	const char* encoding = ucsdet_getName(ucm, &status);
	if (U_FAILURE(status) || encoding == nullptr) {
		return PGEncodingUnknown;
	}
	ucsdet_close(csd);
	// convert the predicted encoding
	PGFileEncoding result_encoding = GetEncodingFromName(encoding);
	if (result_encoding == PGEncodingUnknown) {
		// if we don't recognize ICU's encoding, we just use Binary Encoding
		result_encoding = PGEncodingBinary;
	}
	return result_encoding;
}

bool PGTryConvertToUTF8(char* input_text, size_t input_size, char** output_text, lng* output_size, PGFileEncoding* result_encoding, bool ignore_binary) {
	*output_text = nullptr;
	*output_size = 0;

	// try to determine the encoding from a sample of the text
	lng sample_size = std::min((size_t)MAXIMUM_TEXT_SAMPLE, input_size);
	// guess the encoding from the sample
	PGFileEncoding source_encoding = PGGuessEncoding(input_text, sample_size);
	*result_encoding = source_encoding;
	if (source_encoding == PGEncodingUTF8 ||
		source_encoding == PGEncodingUTF8BOM) {
		// for UTF-8 files we do not perform any conversion
		*output_text = input_text;
		*output_size = input_size;
		return true;
	}
	if (ignore_binary && source_encoding == PGEncodingBinary) {
		return false;
	}

	// if we do not have UTF8 we first convert from the (predicted) source encoding to UTF8
	auto encoder = PGCreateEncoder(source_encoding, PGEncodingUTF8);
	if (!encoder) {
		return false;
	}
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;
	lng result_size = PGConvertText(encoder, input_text, input_size, output_text, output_size, &intermediate_buffer, &intermediate_size);
	if (intermediate_buffer) {
		free(intermediate_buffer);
	}
	*output_size = result_size;
	return result_size > 0;
}

std::string utf8_tolower(std::string str) {
	return panther::tolower(str);
	UErrorCode status = U_ZERO_ERROR;
	UCaseMap* casemap = ucasemap_open(nullptr, 0, &status);

	size_t destsize = str.size() * sizeof(char) + 1;
	char* dest = (char*)malloc(destsize);
	size_t result_size = ucasemap_utf8ToLower(casemap, dest, destsize, str.c_str(), str.size(), &status);
	assert(result_size <= destsize);
	std::string result = std::string(dest, result_size);
	free(dest);
	ucasemap_close(casemap);
	return result;
}