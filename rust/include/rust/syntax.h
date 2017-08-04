
#pragma once

extern "C" {
	typedef void* PGSyntaxState;
	typedef void* PGSyntaxParser;
	typedef void* PGSyntaxSet;

	extern PGSyntaxSet PGLoadSyntaxSet(const char* directory);
	extern void PGDestroySyntaxSet(PGSyntaxSet);

	extern PGSyntaxParser PGCreateParser(PGSyntaxSet, const char* name);
	extern void PGDestroyParser(PGSyntaxParser parser);

	extern PGSyntaxState PGGetDefaultState(PGSyntaxParser parser);
	extern PGParserState PGCopyParserState(PGSyntaxState state);
	extern void PGDestroyParserState(PGSyntaxState state);

	extern int PGStateEquivalent(PGSyntaxState a, PGSyntaxState b);

	extern PGSyntaxState PGParseLine(PGSyntaxParser parser, PGSyntaxState state, const char* line);
}
