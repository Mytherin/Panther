
#include "keybindings.h"

#include "controlmanager.h"
#include "textfield.h"
#include "tabcontrol.h"
#include "simpletextfield.h"

#include "json.h"

using namespace nlohmann;

PGKeyBindingsManager::PGKeyBindingsManager() {
	// initialize all keybindings
	BasicTextField::InitializeKeybindings();
	ControlManager::InitializeKeybindings();
	SimpleTextField::InitializeKeybindings();
	TabControl::InitializeKeybindings();
	TextField::InitializeKeybindings();

#ifdef WIN32
	LoadSettings("default-keybindings." + GetOSName() + ".json");
#else
	LoadSettings("/Users/myth/Programs/Panther/PantherGUI/default-keybindings." + GetOSName() + ".json");
#endif
}


void ExtractModifier(std::string& keys, PGModifier& modifier, std::string modifier_text, PGModifier added_modifier) {
	size_t pos = keys.find(modifier_text.c_str());
	if (pos != std::string::npos) {
		modifier |= added_modifier;
		keys = keys.erase(pos, modifier_text.size());
	}
}


bool PGKeyBindingsManager::ParseKeyPress(std::string keys, PGKeyPress& keypress) {
	PGModifier modifier = PGModifierNone;

	ExtractModifier(keys, modifier, "ctrl+", PGModifierCtrl);
	ExtractModifier(keys, modifier, "shift+", PGModifierShift);
	ExtractModifier(keys, modifier, "alt+", PGModifierAlt);
	ExtractModifier(keys, modifier, "option+", PGModifierAlt);
	ExtractModifier(keys, modifier, "cmd+", PGModifierCmd);
	ExtractModifier(keys, modifier, "super+", PGModifierCmd);

	keypress.modifier = modifier;
	if (keys.size() == 1) {
		// character
		char c = keys[0];
		if (c >= 33 && c <= 126) {
			keys = panther::toupper(keys);
			keypress.character = keys[0];
			return true;
		} else {
			return false;
		}
	} else {
		if (keys == "insert") {
			keypress.button = PGButtonInsert;
		} else if (keys == "home") {
			keypress.button = PGButtonHome;
		} else if (keys == "pageup") {
			keypress.button = PGButtonPageUp;
		} else if (keys == "pagedown") {
			keypress.button = PGButtonPageDown;
		} else if (keys == "delete") {
			keypress.button = PGButtonDelete;
		} else if (keys == "end") {
			keypress.button = PGButtonEnd;
		} else if (keys == "printscreen") {
			keypress.button = PGButtonPrintScreen;
		} else if (keys == "scrolllock") {
			keypress.button = PGButtonScrollLock;
		} else if (keys == "break") {
			keypress.button = PGButtonPauseBreak;
		} else if (keys == "escape") {
			keypress.button = PGButtonEscape;
		} else if (keys == "tab") {
			keypress.button = PGButtonTab;
		} else if (keys == "capslock") {
			keypress.button = PGButtonCapsLock;
		} else if (keys == "f1") {
			keypress.button = PGButtonF1;
		} else if (keys == "f2") {
			keypress.button = PGButtonF2;
		} else if (keys == "f3") {
			keypress.button = PGButtonF3;
		} else if (keys == "f4") {
			keypress.button = PGButtonF4;
		} else if (keys == "f5") {
			keypress.button = PGButtonF5;
		} else if (keys == "f6") {
			keypress.button = PGButtonF6;
		} else if (keys == "f7") {
			keypress.button = PGButtonF7;
		} else if (keys == "f8") {
			keypress.button = PGButtonF8;
		} else if (keys == "f9") {
			keypress.button = PGButtonF9;
		} else if (keys == "f10") {
			keypress.button = PGButtonF10;
		} else if (keys == "f11") {
			keypress.button = PGButtonF11;
		} else if (keys == "f12") {
			keypress.button = PGButtonF12;
		} else if (keys == "numlock") {
			keypress.button = PGButtonNumLock;
		} else if (keys == "down") {
			keypress.button = PGButtonDown;
		} else if (keys == "up") {
			keypress.button = PGButtonUp;
		} else if (keys == "left") {
			keypress.button = PGButtonLeft;
		} else if (keys == "right") {
			keypress.button = PGButtonRight;
		} else if (keys == "backspace") {
			keypress.button = PGButtonBackspace;
		} else if (keys == "enter") {
			keypress.button = PGButtonEnter;
		} else {
			return false;
		}
		return true;
	}
	return false;
}

#define INITIALIZE_CONTROL(control)       \
	functions = &(control::keybindings);   \
	keybindings_noargs = &(control::keybindings_noargs);  \
	keybindings_varargs = &(control::keybindings_varargs)

std::string ParseEscapeCharacters(std::string str) {
	if (str.size() == 0) return "";
	char* temporary_buffer = (char*) malloc((str.size() + 1) * sizeof(char));
	if (!temporary_buffer) {
		return "";
	}
	lng index = 0;
	size_t i;
	for (i = 0; i < str.size() - 1; i++) {
		if (str[i] == '\\') {
			if (str[i + 1] == 'n') {
				temporary_buffer[index++] = '\n';
				i++;
			} else if (str[i + 1] == '\\') {
				temporary_buffer[index++] = '\\';
				i++;
			} else if (str[i + 1] == 't') {
				temporary_buffer[index++] = '\t';
				i++;
			}
		} else {
			temporary_buffer[index++] = str[i];
		}
	}
	if (i == str.size() - 1) {
		temporary_buffer[index++] = str[str.size() - 1];
	}
	std::string result = std::string(temporary_buffer, index);
	free(temporary_buffer);
	return result;
}

void PGKeyBindingsManager::LoadSettings(std::string filename) {
	lng result_size;
	char* ptr = (char*)panther::ReadFile(filename, result_size);
	if (!ptr) {
		// FIXME:
		assert(0);
		return;
	}
	json j;
	//try {
	j = json::parse(ptr);
	/*} catch(...) {
		return;
	}*/

	for (auto it = j.begin(); it != j.end(); it++) {
		if (it.value().is_array()) {
			std::string control = panther::tolower(it.key());
			auto keys = it.value();
			std::map<PGKeyPress, PGKeyFunctionCall>* functions = nullptr;
			std::map<std::string, PGKeyFunction>* keybindings_noargs = nullptr;
			std::map<std::string, PGKeyFunctionArgs>* keybindings_varargs = nullptr;

			if (control == "global") {
				INITIALIZE_CONTROL(ControlManager);
			} else if (control == "textfield") {
				INITIALIZE_CONTROL(TextField);
			} else if (control == "simpletextfield") {
				INITIALIZE_CONTROL(SimpleTextField);
			} else if (control == "basictextfield") {
				INITIALIZE_CONTROL(BasicTextField);
			} else if (control == "tabcontrol") {
				INITIALIZE_CONTROL(TabControl);
			}

			if (functions && keybindings_noargs && keybindings_varargs) {
				for (auto key = keys.begin(); key != keys.end(); key++) {
					auto keybinding = key.value();
					if (keybinding.find("key") == keybinding.end()) {
						// no key found
						continue;
					}
					if (keybinding.find("command") == keybinding.end()) {
						// no command found
						continue;
					}
					std::string keys = panther::tolower(keybinding["key"]);
					std::string command = panther::tolower(keybinding["command"]);
					// parse the key
					PGKeyPress keypress;
					if (!ParseKeyPress(keys, keypress)) {
						// could not parse key press
						continue;
					}

					PGKeyFunctionCall function;
					if (keybinding.find("args") == keybinding.end()) {
						// command without args
						if (keybindings_noargs->count(command) != 0) {
							function.has_args = false;
							function.function = (void*)(*keybindings_noargs)[command];
						} else {
							// command not found
							continue;
						}
					} else {
						// command with args
						if (keybindings_varargs->count(command) != 0) {
							function.has_args = true;
							function.function = (void*)(*keybindings_varargs)[command];
							auto args = keybinding["args"];
							for (auto arg = args.begin(); arg != args.end(); arg++) {
								function.arguments[arg.key()] = ParseEscapeCharacters(StripQuotes(arg.value().dump()));
							}
						} else {
							// command not found
							continue;
						}
					}
					(*functions)[keypress] = function;
				}
			} else {
				// FIXME: unrecognized control type
			}
		}
	}
}
