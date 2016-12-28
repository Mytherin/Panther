
#include "encoding.h"
#include "utils.h"
#include "logger.h"

#include <malloc.h>

#include "unicode.h"
#include <unicode/ucnv.h>

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
	}
	return "";
}

static char* GetEncodingName(PGFileEncoding encoding) {
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
	}
	return nullptr;
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

	// create the converters
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

	return handle;
}

lng PGConvertText(PGEncoderHandle encoder, std::string input, char** output, lng* output_size, char** intermediate_buffer, lng* intermediate_size) {
	lng return_size = -1;
	char* result_buffer = nullptr;
	UChar* buffer = nullptr;
	UErrorCode error = U_ZERO_ERROR;

	// first we convert the source encoding to the internal ICU representation (UChars)
	size_t targetsize = *intermediate_size;
	buffer = *((UChar**)intermediate_buffer);
	targetsize = ucnv_toUChars(encoder->source, buffer, targetsize, input.c_str(), input.size(), &error);
	if (error == U_BUFFER_OVERFLOW_ERROR) {
		error = U_ZERO_ERROR;
		if (buffer)
			free(buffer);
		buffer = (UChar*)malloc(targetsize * sizeof(UChar));
		*intermediate_buffer = (char*)buffer;
		*intermediate_size = targetsize;
		targetsize = ucnv_toUChars(encoder->source, buffer, targetsize, input.c_str(), input.size(), &error);
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