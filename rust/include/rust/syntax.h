
#pragma once

extern "C" {
	typedef void* PGSyntaxState;
	typedef void* PGSyntaxParser;


	extern PGSyntaxParser PGCreateParser();
	extern void PGDestroyParser(PGSyntaxParser parser);

	extern PGSyntaxState PGGetDefaultState(PGSyntaxParser parser);
	extern PGParserState PGCopyParserState(PGSyntaxState state);
	extern void PGDestroyParserState(PGSyntaxState state);

	extern int PGStateEquivalent(PGSyntaxState a, PGSyntaxState b);

	extern PGSyntaxState PGParseLine(PGSyntaxParser parser, PGSyntaxState state, const char* line);
}
