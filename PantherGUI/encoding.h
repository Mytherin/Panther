#pragma once

typedef enum {
	PGEncodingUTF8,
	PGEncodingUTF8BOM,
	PGEncodingUTF16LE,
	PGEncodingUTF16LEBOM,
	PGEncodingUTF16GE,
	PGEncodingUTF16GEBOM,
	PGEncodingWesternWindows1252,
	PGEncodingWesternISO8859_1,
	PGEncodingWesternISO8859_3,
	PGEncodingWesternISO8859_15,
	PGEncodingWesternMacRoman,
	PGEncodingDOS,
	PGEncodingArabicWindows1256,
	PGEncodingArabicISO8859_6,
	PGEncodingBalticWindows1257,
	PGEncodingBalticISO8859_4,
	PGEncodingCelticISO8859_14,
	PGEncodingCentralEuropeanWindows1250,
	PGEncodingCentralEuropeanISO8859_2,
	PGEncodingCyrillicWindows1251,
	PGEncodingCyrillicWindows866,
	PGEncodingCyrillicISO8859_5,
	PGEncodingCyrillicKOI8_R,
	PGEncodingCyrillicKOI8_U,
	PGEncodingEstonianISO8859_13,
	PGEncodingGreekWindows1253,
	PGEncodingGreekISO8859_7,
	PGEncodingHebrewWindows1255,
	PGEncodingHebrewISO8859_8,
	PGEncodingNordicISO8859_10,
	PGEncodingRomanianISO8859_16,
	PGEncodingTurkishWindows1254,
	PGEncodingTurkishISO8859_9,
	PGEncodingVietnameseWindows1258
} PGFileEncoding;
