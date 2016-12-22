
#include "c.h"


CHighlighter::CHighlighter() {
	std::vector<PGKWEntry> keywords;
	keywords.push_back(PGKWEntry("auto", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("break", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("case", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("char", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("const", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("continue", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("default", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("do", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("double", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("else", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("enum", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("extern", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("float", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("for", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("goto", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("if", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("int", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("long", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("register", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("return", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("short", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("signed", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("sizeof", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("static", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("struct", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("switch", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("typedef", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("union", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("unsigned", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("void", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("volatile", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("while", PGSyntaxKeyword));
	keywords.push_back(PGKWEntry("#include", PGSyntaxClass3));
	keywords.push_back(PGKWEntry("#define", PGSyntaxClass3));


	std::vector<PGSyntaxPair> special_tokens;
	special_tokens.push_back(PGSyntaxPair("\"", "\"", PGParserKWSLString, '\\'));
	special_tokens.push_back(PGSyntaxPair("'", "'", PGParserKWSLString, '\\'));
	special_tokens.push_back(PGSyntaxPair("//", "", PGParserKWSLComment, '\0'));
	special_tokens.push_back(PGSyntaxPair("/*", "*/", PGParserKWMLComment, '\0'));

	this->GenerateIndex(keywords, special_tokens);
}