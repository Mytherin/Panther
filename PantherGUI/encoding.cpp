
#include "encoding.h"
#include "utils.h"
#include "logger.h"

#include <malloc.h>
#include <algorithm>

#include "unicode.h"
#include <unicode/ucnv.h>
#include <unicode/ucsdet.h>

struct PGEncoder {
	PGFileEncoding source_encoding;
	PGFileEncoding target_encoding;
	UConverter* source = nullptr;
	UConverter* target = nullptr;
};

std::string PGEncodingToString(PGFileEncoding encoding) {
	switch (encoding) {
		case PGEncodingUTF8:
			return "UTF-8";
		case PGEncodingUTF8BOM:
			return "UTF-8 with BOM";
		case PGEncodingUTF16:
			return "UTF-16";
		case PGEncodingUTF16Platform:
			return "UTF-16 Platform";
		case PGEncodingUTF32:
			return "UTF-32";
		case PGEncodingUTF16BE:
			return "UTF-16 BE";
		case PGEncodingUTF16BEBOM:
			return "UTF-16 BE with BOM";
		case PGEncodingUTF16LE:
			return "UTF-16 LE";
		case PGEncodingUTF16LEBOM:
			return "UTF-16 LE with BOM";
		case PGEncodingUTF32BE:
			return "UTF-32 BE";
		case PGEncodingUTF32BEBOM:
			return "UTF-32 BE with BOM";
		case PGEncodingUTF32LE:
			return "UTF-32 LE";
		case PGEncodingUTF32LEBOM:
			return "UTF-32 LE with BOM";
		case PGEncodingWesternISO8859_1:
			return "ISO-8859-1";
		case PGEncodingWesternWindows1252:
			return "Windows-1252";
		default:
			return "Unknown";
	}
	return "";
}

static const char* GetEncodingName(PGFileEncoding encoding) {
	switch (encoding) {
		case PGEncodingUTF8:
		case PGEncodingUTF8BOM:
			return "UTF-8";
		case PGEncodingUTF16:
			return "UTF-16";
		case PGEncodingUTF16Platform:
			return "UTF16_PlatformEndian";
		case PGEncodingUTF32:
			return "UTF-32";
		case PGEncodingUTF16BE:
		case PGEncodingUTF16BEBOM:
			return "UTF-16BE";
		case PGEncodingUTF16LE:
		case PGEncodingUTF16LEBOM:
			return "UTF-16LE";
		case PGEncodingUTF32BE:
		case PGEncodingUTF32BEBOM:
			return "UTF-16BE";
		case PGEncodingUTF32LE:
		case PGEncodingUTF32LEBOM:
			return "UTF-32LE";
		case PGEncodingWesternISO8859_1:
			return "ISO-8859-1";
		case PGEncodingNordicISO8859_10:
			return "iso-8859_10-1998";
		case PGEncodingCelticISO8859_14:
			return "iso-8859_14-1998";
		case PGEncodingWesternWindows1252:
			return "windows-1252";
		case PGEncodingBinary:
			return "Binary";
		default:
			assert(0);
	}
	return nullptr;
}

static PGFileEncoding GetEncodingFromName(std::string encoding) {
	if (encoding == "UTF-8") {
		return PGEncodingUTF8;
	} else if (encoding == "UTF-16") {
		return PGEncodingUTF16;
	} else if (encoding == "UTF16_PlatformEndian") {
		return PGEncodingUTF16Platform;
	} else if (encoding == "UTF-32") {
		return PGEncodingUTF32;
	} else if (encoding == "UTF-16BE") {
		return PGEncodingUTF16BE;
	} else if (encoding == "UTF-16LE") {
		return PGEncodingUTF16LE;
	} else if (encoding == "UTF-32BE") {
		return PGEncodingUTF32BE;
	} else if (encoding == "UTF-32LE") {
		return PGEncodingUTF32LE;
	} else if (encoding == "ISO-8859-1") {
		return PGEncodingWesternISO8859_1;
	} else if (encoding == "iso-8859_10-1998") {
		return PGEncodingNordicISO8859_10;
	} else if (encoding == "iso-8859_14-1998") {
		return PGEncodingCelticISO8859_14;
	} else if (encoding == "windows-1252") {
		return PGEncodingWesternWindows1252;
	} else if (encoding == "Binary") {
		return PGEncodingBinary;
	}
	//assert(0);
	return PGEncodingBinary;
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
	UErrorCode error = U_ZERO_ERROR;

	// assure that both encodings are implemented
	assert(GetEncodingName(source_encoding) && GetEncodingName(target_encoding));

	PGEncoderHandle handle = new PGEncoder();
	handle->source_encoding = source_encoding;
	handle->target_encoding = target_encoding;

	if (handle->source_encoding != PGEncodingBinary && handle->target_encoding != PGEncodingBinary) {
		// create the converters
		// binary encoding is handled by us, not by ICU
		// so we don't create an ICU converter for that encoding
		handle->source = ucnv_open(GetEncodingName(source_encoding), &error);
		if (U_FAILURE(error)) {
			// failed to create a converter
			delete handle;
			return nullptr;
		}
		handle->target = ucnv_open(GetEncodingName(target_encoding), &error);
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