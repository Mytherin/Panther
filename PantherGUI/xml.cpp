
#include "xml.h"
#include <vector>

enum PGParserXMLState {
	PGParserXMLDefault,
	PGParserXMLComment,
	PGParserXMLInsideStartTag,
	PGParserXMLInsideEndTag,
	PGParserXMLElementName,
	PGParserXMLInElement,
	PGParserXMLOpenValue,
	PGParserXMLSpecialName,
	PGParserXMLStartValue
};

const PGSyntaxType PGXMLElementName = 1;
const PGSyntaxType PGXMLAttributeName = 2;
const PGSyntaxType PGXMLAttributeValue = 3;
const PGSyntaxType PGXMLComment = 4;
const PGSyntaxType PGXMLEmpty = 255;

struct XMLToken {
	std::string name;
	XMLToken(std::string name) : name(name) { }
};

struct XMLParserState {
	PGParserXMLState state;
	std::string current_token;
	std::vector<XMLToken> open_tokens;
};

PGParserState XMLHighlighter::IncrementalParseLine(TextLine& line, ssize_t linenr, PGParserState s, PGParseErrors& errors) {
	XMLParserState* state = (XMLParserState*)s;
	char* text = line.GetLine();
	ssize_t size = line.GetLength();
	// free the current syntax of this line
	PGSyntax* current = &line.syntax;
	PGSyntax* prev = nullptr;
	PGSyntax* next = current->next;
	current->next = nullptr;
	while (next) {
		assert(next->next != next);
		PGSyntax* tmp = next->next;
		delete next;
		next = tmp;
	}
	current = &line.syntax;
	current->end = -1;
	for (ssize_t i = 0; i < size; i++) {
		if (text[i] == '<') {
			if (state->state == PGParserXMLDefault) {
				current->type = PGXMLEmpty;
				current->end = i + 1;
				current->next = new PGSyntax();
				prev = current;
				current = current->next;
				state->state = PGParserXMLElementName;
			} else if (state->state != PGParserXMLComment && state->state != PGParserXMLOpenValue && state->state != PGParserXMLDefault) {
				// Parse Error
				continue;
			}
		} else if (text[i] == '>') {
			if (state->state == PGParserXMLInElement) {
				state->state = PGParserXMLDefault;
				current->type = PGXMLEmpty;
				current->end = i + 1;
				prev = current;
				current->next = new PGSyntax();
				current = current->next;
			} else if (state->state == PGParserXMLElementName || state->state == PGParserXMLSpecialName) {
				current->type = PGXMLElementName;
				current->end = i;
				current->next = new PGSyntax();
				current = current->next;
				current->type = PGXMLEmpty;
				current->end = i + 1;
				prev = current;
				current->next = new PGSyntax();
				current = current->next;
				state->state = PGParserXMLDefault;
			} else if (state->state != PGParserXMLComment && state->state != PGParserXMLOpenValue) {
				// Parse Error
				continue;
			}
		} else if (text[i] == ' ' || text[i] == '\t') {
			if (state->state == PGParserXMLElementName || state->state == PGParserXMLSpecialName) {
				// Element Name
				current->type = PGXMLElementName;
				current->end = i;
				prev = current;
				current->next = new PGSyntax();
				current = current->next;
				state->state = PGParserXMLInElement;
			}
		} else if (text[i] == '=') {
			if (state->state == PGParserXMLInElement) {
				// Attribute Name
				current->type = PGXMLAttributeName;
				current->end = i;
				prev = current;
				current->next = new PGSyntax();
				current = current->next;
				state->state = PGParserXMLStartValue;
			}
		} else if (text[i] == '"' || text[i] == '\'') {
			if (state->state == PGParserXMLStartValue) {
				current->type = PGXMLEmpty;
				current->end = i;
				prev = current;
				current->next = new PGSyntax();
				current = current->next;
				state->state = PGParserXMLOpenValue;
			} else if (state->state == PGParserXMLOpenValue) {
				// Attribute Value
				current->type = PGXMLAttributeValue;
				current->end = i + 1;
				prev = current;
				current->next = new PGSyntax();
				current = current->next;
				state->state = PGParserXMLInElement;
			} else if (state->state != PGParserXMLComment && state->state != PGParserXMLOpenValue && state->state != PGParserXMLDefault) {
				// Parse Error
			}
		} else if (text[i] == '!') {
			if (state->state == PGParserXMLElementName) {
				state->state = PGParserXMLSpecialName;
			}
		} else if (text[i] == '-') {
			if (state->state == PGParserXMLSpecialName) {
				if (i + 1 < size && text[i + 1] == '-') {
					state->state = PGParserXMLComment;
					i++;
				}
			} else if (state->state == PGParserXMLComment) {
				if (i + 2 < size && text[i + 1] == '-' && text[i + 2] == '>') {
					state->state = PGParserXMLDefault;
					i += 2;
					current->type = PGXMLComment;
					current->end = i;
					prev = current;
					current->next = new PGSyntax();
					current = current->next;
				}
			}
		}
	}
	if (state->state == PGParserXMLComment) {
		current->type = PGXMLComment;
		current->end = size;
		current->next = new PGSyntax();
		current = current->next;
	} else {
		// FIXME: error
		state->state = PGParserXMLDefault;
	}
	return state;
}

PGParserState XMLHighlighter::GetDefaultState() {
	XMLParserState* state = new XMLParserState();
	state->state = PGParserXMLDefault;
	return state;
}

PGParserState XMLHighlighter::CopyParserState(const PGParserState inp) {
	XMLParserState* original = (XMLParserState*)inp;
	XMLParserState* state = new XMLParserState();
	state->state = original->state;
	state->open_tokens.insert(state->open_tokens.begin(), original->open_tokens.begin(), original->open_tokens.end());
	return state;
}

void XMLHighlighter::DeleteParserState(PGParserState inp) {
	delete inp;
}

bool XMLHighlighter::StateEquivalent(const PGParserState a, const PGParserState b) {
	XMLParserState* a_xml = (XMLParserState*)a;
	XMLParserState* b_xml = (XMLParserState*)b;
	return a_xml->state == b_xml->state;
}