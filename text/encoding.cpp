
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
	AddEncoding(PGEncodingNordicISO8859_10, "ISO-8859-10", "iso-8859_10-1998");
	AddEncoding(PGEncodingCelticISO8859_14, "ISO-8859-14", "iso-8859_14-1998");
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
				output_buffer[++pos] = ' ';
			}
			output_buffer[pos] = '\0';
			assert(pos < output_buffer_size);
			*output = output_buffer;
			*output_size = output_buffer_size;
			return output_buffer_size;
		} else if (encoder->target_encoding == PGEncodingBinary) {
			assert(encoder->source_encoding == PGEncodingUTF8);
			assert(0);
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

bool PGTryConvertToUTF8(char* input_text, size_t input_size, char** output_text, lng* output_size, PGFileEncoding* result_encoding) {
	UCharsetDetector* csd = nullptr;
	const UCharsetMatch *ucm = nullptr;
	UErrorCode status = U_ZERO_ERROR;

	if (((unsigned char*)input_text)[0] == 0xEF &&
		((unsigned char*)input_text)[1] == 0xBB &&
		((unsigned char*)input_text)[2] == 0xBF) {
		// UTF8 BOM, use UTF8
		*result_encoding = PGEncodingUTF8;
		*output_text = input_text;
		*output_size = input_size;
		return true;
	}

	if (input_size == 0) {
		// empty file, use UTF8
		*result_encoding = PGEncodingUTF8;
		*output_text = input_text;
		*output_size = input_size;
		return true;
	}

	*output_text = nullptr;
	*output_size = 0;

	// try to determine the encoding from a sample of the text
	lng sample_size = std::min((size_t)MAXIMUM_TEXT_SAMPLE, input_size);
	csd = ucsdet_open(&status);
	if (U_FAILURE(status)) {
		return false;
	}
	ucsdet_setText(csd, input_text, sample_size, &status);
	if (U_FAILURE(status)) {
		return false;
	}
	ucm = ucsdet_detect(csd, &status);
	if (U_FAILURE(status)) {
		return false;
	}
	const char* encoding = ucsdet_getName(ucm, &status);
	if (U_FAILURE(status) || encoding == nullptr) {
		return false;
	}
	ucsdet_close(csd);
	// convert the predicted encoding
	PGFileEncoding source_encoding = GetEncodingFromName(encoding);
	auto encoder = PGCreateEncoder(source_encoding, PGEncodingUTF8);
	if (!encoder) {
		return false;
	}
	if (source_encoding == PGEncodingWesternISO8859_1) {
		// ICU likes to assign ascii text to ISO-8859-1
		// we prefer UTF-8 for ASCII encoding
		// thus if we encounter ISO-8859-1 encoding, check if the sample contains only ASCII text
		// if it does, we use UTF-8 instead of ISO-8859-1
		bool utf8 = true;
		for (lng i = 0; i < sample_size;) {
			int character_offset = utf8_character_length(input_text[i]);
			if (character_offset != 1) {
				utf8 = false;
				break;
			}
			i += character_offset;
		}
		if (utf8) {
			source_encoding = PGEncodingUTF8;
		}
	}

	*result_encoding = source_encoding;
	if (source_encoding == PGEncodingUTF8) {
		// source encoding is UTF8: just directly return the input text
		*output_text = input_text;
		*output_size = input_size;
		return true;
	}
	// if we do not have UTF8 we first convert from the (predicted) source encoding to UTF8
	char* intermediate_buffer = nullptr;
	lng intermediate_size = 0;
	if (PGConvertText(encoder, input_text, input_size, output_text, output_size, &intermediate_buffer, &intermediate_size) > 0) {
		return true;
	}
	return false;
}

std::string utf8_tolower(std::string str) {
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