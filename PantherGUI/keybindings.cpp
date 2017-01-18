
#include "keybindings.h"
#include "textfield.h"

#include "json.h"

using namespace nlohmann;

PGKeyBindingsManager::PGKeyBindingsManager() {
	// initialize all keybindings

	// initialize textfield keybindings
	TextField::InitializeKeybindings();

	LoadSettings("default-keybindings.json");
}

bool PGKeyBindingsManager::ParseKeyPress(std::string keys, PGKeyPress& keypress) {
	if (keys == "ctrl+shift+s") {
		keypress.character = 'S';
		keypress.modifier = PGModifierCtrlShift;
		return true;
	}
	return false;
}

void PGKeyBindingsManager::LoadSettings(std::string filename) {
	lng result_size;
	char* ptr = (char*) panther::ReadFile(filename, result_size);
	if (!ptr) {
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
			std::string control = it.key();
			panther::tolower(control);
			auto keys = it.value();
			std::map<PGKeyPress, PGKeyFunctionCall>* functions = nullptr;
			std::map<std::string, PGKeyFunction>* keybindings_noargs = nullptr;
			std::map<std::string, PGKeyFunctionArgs>* keybindings_varargs = nullptr;
			if (control == "textfield") {
				functions = &TextField::keybindings;
				keybindings_noargs = &TextField::keybindings_noargs;
				keybindings_varargs = &TextField::keybindings_varargs;
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
					std::string keys = keybinding["key"];
					std::string command = keybinding["command"];
					panther::tolower(keys);
					panther::tolower(command);
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
							assert(0);
							// fixme: assign function arguments
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
