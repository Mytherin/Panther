#pragma once

#include "utils.h"
#include "windowfunctions.h"

#include <map>

class Control;
struct PGKeyPress;
struct PGMousePress;
struct PGKeyFunctionCall;
struct PGMouseFunctionCall;

typedef void(*PGKeyFunction)(Control*);
typedef void(*PGKeyFunctionArgs)(Control*, std::map<std::string, std::string>);
typedef void(*PGMouseFunction)(Control*, PGMouseButton button, PGPoint mouse, lng line, lng character);

#define PG_CONTROL_KEYBINDINGS											\
static void InitializeKeybindings();									\
static std::map<std::string, PGKeyFunction> keybindings_noargs;			\
static std::map<std::string, PGKeyFunctionArgs> keybindings_varargs;	\
static std::map<std::string, PGMouseFunction> mousebindings_noargs;	    \
static std::map<std::string, PGBitmapHandle> keybindings_images;		\
static std::map<PGKeyPress, PGKeyFunctionCall> keybindings;             \
static std::map<PGMousePress, PGMouseFunctionCall> mousebindings


#define PG_CONTROL_INITIALIZE_KEYBINDINGS(control)								\
std::map<std::string, PGKeyFunction> control::keybindings_noargs;				\
std::map<std::string, PGKeyFunctionArgs> control::keybindings_varargs;			\
std::map<std::string, PGMouseFunction> control::mousebindings_noargs;	        \
std::map<std::string, PGBitmapHandle> control::keybindings_images;				\
std::map<PGKeyPress, PGKeyFunctionCall> control::keybindings;					\
std::map<PGMousePress, PGMouseFunctionCall> control::mousebindings

void PGPopupMenuInsertCommand(PGPopupMenuHandle handle, 
	std::string command_text, 
	std::string command, 
	std::map<std::string, PGKeyFunction>& keybindings_noargs, 
	std::map<PGKeyPress, PGKeyFunctionCall>& keybindings, 
	std::map<std::string, PGBitmapHandle>& images, 
	PGPopupMenuFlags flags = PGPopupMenuFlagsNone,
	PGPopupCallback callback = nullptr,
	PGPopupMenuHandle main_menu = nullptr);

struct PGKeyPress {
	PGModifier modifier = PGModifierNone;
	PGButton button = PGButtonNone;
	char character = '\0';

	PGKeyPress() : 
		modifier(PGModifierNone), button(PGButtonNone), character('\0') {
	}

	std::string ToString() const;

	// comparison function needed for std::map
	friend bool operator< (const PGKeyPress& lhs, const PGKeyPress& rhs) {
		return (static_cast<int>(lhs.button) < static_cast<int>(rhs.button)) || 
			((lhs.button == rhs.button) && 
				((lhs.modifier < rhs.modifier) || 
					(lhs.modifier == rhs.modifier && lhs.character < rhs.character)));
	}
};

struct PGMousePress {
	PGModifier modifier = PGModifierNone;
	PGMouseButton button = PGMouseButtonNone;
	int clicks = 0;

	PGMousePress() :
		modifier(PGModifierNone), button(PGMouseButtonNone), clicks(0) {
	}

	// comparison function needed for std::map
	friend bool operator< (const PGMousePress& lhs, const PGMousePress& rhs) {
		return (static_cast<int>(lhs.button) < static_cast<int>(rhs.button)) ||
			((lhs.button == rhs.button) &&
			((lhs.modifier < rhs.modifier) ||
				(lhs.modifier == rhs.modifier && lhs.clicks < rhs.clicks)));
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

struct PGMouseFunctionCall {
	void* function;

	void Call(Control* c, PGMouseButton button, PGPoint mouse, lng line, lng character) {
		((PGMouseFunction)function)(c, button, mouse, line, character);
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
	bool ParseMousePress(std::string keys, PGMousePress& keypress);
	void LoadSettings(std::string file);
	void LoadSettingsFromData(const char* data);
};

