
#include "xml.h"

const PGParserState PGParserXMLComment = 1;
const PGParserState PGParserXMLElementName = 2;
const PGParserState PGParserXMLInElement = 3;
const PGParserState PGParserXMLOpenAttribute = 4;
const PGParserState PGParserXMLStartValue = 5;
const PGParserState PGParserXMLOpenValue = 6;
const PGParserState PGParserXMLSpecialName = 7;

const PGSyntaxType PGXMLElementName = 1;
const PGSyntaxType PGXMLAttributeName = 2;
const PGSyntaxType PGXMLValue = 3;
const PGSyntaxType PGXMLBracket = 4;
const PGSyntaxType PGXMLComment = 5;

PGParserState XMLHighlighter::IncrementalParseLine(TextLine& line, PGParserState state) {
	char* text = line.GetLine();
	ssize_t size = line.GetLength();
	// free the current
	PGSyntax* prev = nullptr;
	PGSyntax* current = &line.syntax;
	PGSyntax* next = current->next;
	current->next = nullptr;
	while (next) {
		PGSyntax* tmp = next->next;
		delete next;
		next = tmp;
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
			} else if (state != PGParserXMLComment && state != PGParserXMLOpenValue && state != PGParserDefaultState) {
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
			if (state == PGParserXMLInElement) {
				state = PGParserDefaultState;
				current->type = PGXMLBracket;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
			} else if (state == PGParserXMLElementName || state == PGParserXMLSpecialName) {
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
			} else if (state != PGParserXMLComment && state != PGParserXMLOpenValue) {
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
			if (state == PGParserXMLElementName || state == PGParserXMLSpecialName) {
				// Element Name
				current->type = PGXMLElementName;
				current->end = i;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state = PGParserXMLInElement;
			}
		} else if (text[i] == '=') {
			if (state == PGParserXMLInElement) {
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
			if (state == PGParserXMLStartValue) {
				prev->end = i;
				state = PGParserXMLOpenValue;
			} else if (state == PGParserXMLOpenValue) {
				// Attribute Value
				current->type = PGXMLValue;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state = PGParserXMLInElement;
			} else if (state != PGParserXMLComment && state != PGParserXMLOpenValue && state != PGParserDefaultState) {
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
		} else if (text[i] == '!') {
			if (state == PGParserXMLElementName) {
				state = PGParserXMLSpecialName;
			}
		} else if (text[i] == '-') {
			if (state == PGParserXMLSpecialName) {
				if (i + 1 < size && text[i + 1] == '-') {
					state = PGParserXMLComment;
					i++;
				}
			} else if (state == PGParserXMLComment) {
				if (i + 2 < size && text[i + 1] == '-' && text[i + 2] == '>') {
					state = PGParserDefaultState;
					i += 2;
					current->type = PGXMLComment;
					current->end = i;
					current->next = new PGSyntax();
					current = current->next;
				}
			}
		}
	}
	if (state == PGParserXMLComment || state == PGParserXMLOpenValue) {
		current->type = state == PGParserXMLComment ? PGXMLComment : PGXMLValue;
		current->end = size;
		current->next = new PGSyntax();
		current = current->next;
	} else {
		state = PGParserDefaultState;
	}
	line.state = state;
	return state;
}
