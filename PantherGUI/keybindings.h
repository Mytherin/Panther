#pragma once

#include "utils.h"
#include "windowfunctions.h"

#include <map>

class Control;

typedef void(*PGKeyFunction)(Control*);
typedef void(*PGKeyFunctionArgs)(Control*, std::map<std::string, std::string>);

struct PGKeyPress {
	PGModifier modifier = PGModifierNone;
	PGButton button = PGButtonNone;
	char character = '\0';

	PGKeyPress() : 
		modifier(PGModifierNone), button(PGButtonNone), character('\0') {
	}

	// comparison function needed for std::map
	friend bool operator< (const PGKeyPress& lhs, const PGKeyPress& rhs) {
		return (static_cast<int>(lhs.button) < static_cast<int>(rhs.button)) || 
			((lhs.button == rhs.button) && 
				((lhs.modifier < rhs.modifier) || 
					(lhs.modifier == rhs.modifier && lhs.character < rhs.character)));
	}
};

struct PGKeyFunctionCall {
	void* function;
	bool has_args;
	std::map<std::string, std::string> arguments;

	void Call(Control* c) {
		if (has_args) {
			((PGKeyFunctionArgs)function)(c, arguments);
		} else {
			((PGKeyFunction)function)(c);
		}
	}
};

class PGKeyBindingsManager {
public:
	static PGKeyBindingsManager* GetInstance() {
		static PGKeyBindingsManager manager;
		return &manager;
	}

	static void Initialize() { (void) GetInstance(); }
private:
	PGKeyBindingsManager();

	bool ParseKeyPress(std::string keys, PGKeyPress& keypress);
	void LoadSettings(std::string file);
};

