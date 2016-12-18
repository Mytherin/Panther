
#include "encoding.h"
#include "utils.h"
#include "logger.h"

#include <malloc.h>

#include <unicode/ucnv.h>

char* GetEncodingName(PGFileEncoding encoding) {
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

lng PGConvertText(std::string input, char** output, PGFileEncoding source_encoding, PGFileEncoding target_encoding) {
	// assure that both encodings are implemented
	assert(GetEncodingName(source_encoding) && GetEncodingName(target_encoding));
	UErrorCode error;
	UChar* buffer = nullptr;
	char* result_buffer = nullptr;
	UConverter* conv = nullptr;
	lng return_size = -1;

	*output = nullptr;

	size_t length = input.size();
	// first we convert the source encoding to the internal ICU representation (UChars)
	// create the converter
	conv = ucnv_open(GetEncodingName(source_encoding), &error);
	if (U_FAILURE(error)) {
		// failed to create a converter
		goto wrapup;
	}
	size_t targetsize = length * sizeof(UChar);
	buffer = (UChar*)malloc(targetsize * sizeof(UChar));
	targetsize = ucnv_toUChars(conv, buffer, targetsize, input.c_str(), input.size(), &error);
	if (error == U_BUFFER_OVERFLOW_ERROR) {
		error = U_ZERO_ERROR;
		free(buffer);
		buffer = (UChar*)malloc(targetsize * sizeof(UChar));
		targetsize = ucnv_toUChars(conv, buffer, targetsize, input.c_str(), input.size(), &error);
	}
	if (U_FAILURE(error)) {
		// failed source conversion
		goto wrapup;
	}
	ucnv_close(conv); 
	conv = ucnv_open(GetEncodingName(target_encoding), &error);
	if (U_FAILURE(error)) {
		// failed to create a converter
		goto wrapup;
	}
	size_t result_size = targetsize * 4;
	// now convert the source to the target encoding
	result_buffer = (char*)malloc(targetsize * 4);
	result_size = ucnv_fromUChars(conv, result_buffer, result_size, buffer, targetsize, &error);
	if (U_FAILURE(error)) {
		// failed source conversion
		if (result_buffer) free(result_buffer);
		return_size = -1;
		goto wrapup;
	}

	*output = result_buffer;
	return_size = result_size;
wrapup:
	if (buffer)
		free(buffer);
	if (conv)
		ucnv_close(conv); 
	return return_size;
}
