
#include "c.h"


CHighlighter::CHighlighter() {
	std::vector<PGKWEntry> keywords;
	keywords.push_back(PGKWEntry("auto", PGKWKeyword));
	keywords.push_back(PGKWEntry("break", PGKWKeyword));
	keywords.push_back(PGKWEntry("case", PGKWKeyword));
	keywords.push_back(PGKWEntry("char", PGKWKeyword));
	keywords.push_back(PGKWEntry("const", PGKWKeyword));
	keywords.push_back(PGKWEntry("continue", PGKWKeyword));
	keywords.push_back(PGKWEntry("default", PGKWKeyword));
	keywords.push_back(PGKWEntry("do", PGKWKeyword));
	keywords.push_back(PGKWEntry("double", PGKWKeyword));
	keywords.push_back(PGKWEntry("else", PGKWKeyword));
	keywords.push_back(PGKWEntry("enum", PGKWKeyword));
	keywords.push_back(PGKWEntry("extern", PGKWKeyword));
	keywords.push_back(PGKWEntry("float", PGKWKeyword));
	keywords.push_back(PGKWEntry("for", PGKWKeyword));
	keywords.push_back(PGKWEntry("goto", PGKWKeyword));
	keywords.push_back(PGKWEntry("if", PGKWKeyword));
	keywords.push_back(PGKWEntry("int", PGKWKeyword));
	keywords.push_back(PGKWEntry("long", PGKWKeyword));
	keywords.push_back(PGKWEntry("register", PGKWKeyword));
	keywords.push_back(PGKWEntry("return", PGKWKeyword));
	keywords.push_back(PGKWEntry("short", PGKWKeyword));
	keywords.push_back(PGKWEntry("signed", PGKWKeyword));
	keywords.push_back(PGKWEntry("sizeof", PGKWKeyword));
	keywords.push_back(PGKWEntry("static", PGKWKeyword));
	keywords.push_back(PGKWEntry("struct", PGKWKeyword));
	keywords.push_back(PGKWEntry("switch", PGKWKeyword));
	keywords.push_back(PGKWEntry("typedef", PGKWKeyword));
	keywords.push_back(PGKWEntry("union", PGKWKeyword));
	keywords.push_back(PGKWEntry("unsigned", PGKWKeyword));
	keywords.push_back(PGKWEntry("void", PGKWKeyword));
	keywords.push_back(PGKWEntry("volatile", PGKWKeyword));
	keywords.push_back(PGKWEntry("while", PGKWKeyword));
	keywords.push_back(PGKWEntry("#include", PGKWKeyword));
	keywords.push_back(PGKWEntry("#define", PGKWKeyword));


	std::vector<PGSyntaxPair> special_tokens;
	special_tokens.push_back(PGSyntaxPair("\"", "\"", PGParserKWSLString, '\\'));
	special_tokens.push_back(PGSyntaxPair("'", "'", PGParserKWSLString, '\\'));
	special_tokens.push_back(PGSyntaxPair("//", "", PGParserKWSLComment, '\0'));
	special_tokens.push_back(PGSyntaxPair("/*", "*/", PGParserKWMLComment, '\0'));

	this->GenerateIndex(keywords, special_tokens);
}