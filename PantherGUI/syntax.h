#pragma once

#include "utils.h"

typedef void* PGParserState;
typedef short PGSyntaxType;

extern const PGParserState PGParserErrorState;

extern const PGSyntaxType PGSyntaxError;
extern const PGSyntaxType PGSyntaxNone;
extern const PGSyntaxType PGSyntaxString;
extern const PGSyntaxType PGSyntaxConstant;
extern const PGSyntaxType PGSyntaxComment;
extern const PGSyntaxType PGSyntaxOperator;
extern const PGSyntaxType PGSyntaxFunction;
extern const PGSyntaxType PGSyntaxKeyword;
extern const PGSyntaxType PGSyntaxClass1;
extern const PGSyntaxType PGSyntaxClass2;
extern const PGSyntaxType PGSyntaxClass3;
extern const PGSyntaxType PGSyntaxClass4;
extern const PGSyntaxType PGSyntaxClass5;
extern const PGSyntaxType PGSyntaxClass6;

struct PGSyntax {
	PGSyntaxType type;
	lng end = -1;
	PGSyntax* next = nullptr;

	PGSyntax() : type(0), end(-1), next(nullptr) { }
	~PGSyntax() { }

	void Delete() {
		PGSyntax* next = this->next;
		while (next) {
			assert(next->next != next);
			PGSyntax* tmp = next->next;
			delete next;
			next = tmp;
		}
	}
};
