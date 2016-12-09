
#include "xml.h"

const PGParserState PGParserXMLComment = 1;
const PGParserState PGParserXMLElementName = 2;
const PGParserState PGParserXMLInsideElement = 3;
const PGParserState PGParserXMLOpenAttribute = 4;
const PGParserState PGParserXMLStartValue = 5;
const PGParserState PGParserXMLOpenValue = 6;

const PGSyntaxType PGXMLElementName = 1;
const PGSyntaxType PGXMLAttributeName = 2;
const PGSyntaxType PGXMLValue = 3;
const PGSyntaxType PGXMLBracket = 4;

PGParserState XMLHighlighter::IncrementalParseLine(TextLine& line, PGParserState state) {
	assert(state == PGParserDefaultState);
	char* text = line.GetLine();
	ssize_t size = line.GetLength();
	bool escaped = false;
	// free the current
	PGSyntax* prev = nullptr;
	PGSyntax* current = &line.syntax;
	while (current->next) {
		delete current->next;
		current = current->next;
	}
	current = &line.syntax;
	current->end = -1;
	for (ssize_t i = 0; i < size; i++) {
		if (text[i] == '<') {
			if (state == PGParserDefaultState) {
				current->type = PGXMLBracket;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state = PGParserXMLElementName;
			} else if (state != PGParserXMLComment && state != PGParserXMLOpenValue || state != PGParserDefaultState) {
				// Parse Error
				if (!prev || prev->type != PGSyntaxError) {
					current->type = PGSyntaxError;
					current->end = i + 1;
					current->next = new PGSyntax();
					prev = current;
					current = current->next;
				} else {
					prev->end = i + 1;
				}
			}
		} else if (text[i] == '>') {
			if (state == PGParserXMLInsideElement) {
				state = PGParserDefaultState;
				current->type = PGXMLBracket;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
			} else if (state == PGParserXMLElementName) {
				current->type = PGXMLElementName;
				current->end = i;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				current->type = PGXMLBracket;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state = PGParserDefaultState;
			} else if (state != PGParserXMLComment && state != PGParserXMLOpenValue || state != PGParserDefaultState) {
				// Parse Error
				if (!prev || prev->type != PGSyntaxError) {
					current->type = PGSyntaxError;
					current->end = i + 1;
					current->next = new PGSyntax();
					prev = current;
					current = current->next;
				} else {
					prev->end = i + 1;
				}
			}
		} else if (text[i] == ' ' || text[i] == '\t') {
			if (state == PGParserXMLElementName) {
				// Element Name
				current->type = PGXMLElementName;
				current->end = i;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state = PGParserXMLInsideElement;
			}
		} else if (text[i] == '=') {
			if (state == PGParserXMLInsideElement) {
				// Attribute Name
				current->type = PGXMLAttributeName;
				current->end = i;
				current->next = new PGSyntax();
				current = current->next;
				current->type = PGXMLBracket;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state = PGParserXMLStartValue;
			}
		} else if (text[i] == '"' || text[i] == '\'') {
			if (escaped) {
				escaped = false;
			} else if (state == PGParserXMLStartValue) {
				prev->end = i;
				state = PGParserXMLOpenValue;
			} else if (state == PGParserXMLOpenValue) {
				// Attribute Value
				current->type = PGXMLValue;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state = PGParserXMLInsideElement;
			} else if (state != PGParserXMLComment && state != PGParserXMLOpenValue || state != PGParserDefaultState) {
				// Parse Error
				if (!prev || prev->type != PGSyntaxError) {
					current->type = PGSyntaxError;
					current->end = i + 1;
					current->next = new PGSyntax();
					prev = current;
					current = current->next;
				} else {
					prev->end = i + 1;
				}
			}
		} else if (text[i] == '\\') {
			escaped = !escaped;
		}
	}
	line.state = state;
	return state;
}

PGParserState XMLHighlighter::BacktrackParseLine(TextLine& line) {
	return PGParserErrorState;
}