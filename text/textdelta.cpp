
#include "textdelta.h"
#include "json.h"

using namespace nlohmann;

static void LoadStrings(nlohmann::json& j, std::vector<std::string>& strings) {
	if (j.is_string()) {
		std::string line = j;
		strings.push_back(line);
	} else if (j.is_array()) {
		for (auto it = j.begin(); it != j.end(); it++) {
			std::string line = *it;
			strings.push_back(line);
		}
	}
}

std::vector<TextDelta*> TextDelta::LoadWorkspace(nlohmann::json& j) {
	std::vector<TextDelta*> deltas;
	if (j.is_array()) {
		for (auto it = j.begin(); it != j.end(); it++) {
			json& k = *it;
			if (k.count("type") > 0) {
				if (k["type"] == "addtext") {
					assert(0);
					/*
					ReplaceText* text = new ReplaceText(std::string>());
					if (k.count("lines") > 0) {
						json& lines = k["lines"];
						LoadStrings(lines, text->lines);
					}
					Cursor::LoadCursors(k, text->stored_cursors);
					if (text->lines.size() == 0 || text->stored_cursors.size() == 0) {
						delete text;
						continue;
					}
					deltas.push_back(text);*/
				} else if (k["type"] == "removetext") {
					RemoveText* text = new RemoveText();
					if (k.count("removed_text") > 0) {
						json& lines = k["removed_text"];
						LoadStrings(lines, text->removed_text);
					}
					Cursor::LoadCursors(k, text->stored_cursors);
					if (text->removed_text.size() == 0 || text->stored_cursors.size() == 0) {
						delete text;
						continue;
					}
					deltas.push_back(text);
				} else if (k["type"] == "removeselection") {
					if (k.count("direction") > 0 && k.count("remove_type") > 0) {
						PGDirection direction = k["direction"] == "left" ? PGDirectionLeft : PGDirectionRight;
						PGTextType type;
						std::string remove_type = k["remove_type"];
						if (remove_type == "remove_line") {
							type = PGDeltaRemoveLine;
						} else if (remove_type == "remove_character") {
							type = PGDeltaRemoveCharacter;
						} else if (remove_type == "remove_word") {
							type = PGDeltaRemoveWord;
						}
						RemoveSelection* text = new RemoveSelection(direction, type);
						Cursor::LoadCursors(k, text->stored_cursors);
						if (text->stored_cursors.size() == 0) {
							delete text;
							continue;
						}
						deltas.push_back(text);
					}
				} else if (k["type"] == "insertline") {
					if (k.count("direction") > 0) {
						PGDirection direction = k["direction"] == "left" ? PGDirectionLeft : PGDirectionRight;
						InsertLineBefore* text = new InsertLineBefore(direction);
						Cursor::LoadCursors(k, text->stored_cursors);
						if (text->stored_cursors.size() == 0) {
							delete text;
							continue;
						}
						deltas.push_back(text);
					}

				}
			}
		}
	}
	return deltas;
}

/*
void ReplaceText::WriteWorkspace(nlohmann::json& j) {
	j["type"] = "addtext";
	Cursor::StoreCursors(j, this->stored_cursors);
	if (lines.size() == 1) {
		j["lines"] = lines[0];
	} else {
		j["lines"] = json::array();
		json& lines = j["lines"];
		lng index = 0;
		for (auto it = lines.begin(); it != lines.end(); it++) {
			lines[index++] = *it;
		}
	}
}

size_t ReplaceText::SerializedSize() {
	size_t size = 40;
	for (auto it = lines.begin(); it != lines.end(); it++) {
		size += it->size();
	}
	size += stored_cursors.size() * 20;
	return size;
}*/

void RemoveText::WriteWorkspace(nlohmann::json& j) {
	j["type"] = "removetext";
	Cursor::StoreCursors(j, this->stored_cursors);
	j["removed_text"] = json::array();
	json& lines = j["removed_text"];
	lng index = 0;
	for (auto it = removed_text.begin(); it != removed_text.end(); it++) {
		lines[index++] = *it;
	}
}

size_t RemoveText::SerializedSize() {
	size_t size = 40;
	for (auto it = removed_text.begin(); it != removed_text.end(); it++) {
		size += it->size();
	}
	size += stored_cursors.size() * 20;
	return size;
}

void RemoveSelection::WriteWorkspace(nlohmann::json& j) {
	switch (type) {
	case PGDeltaRemoveLine:
		j["remove_type"] = "remove_line";
		break;
	case PGDeltaRemoveCharacter:
		j["remove_type"] = "remove_character";
		break;
	case PGDeltaRemoveWord:
		j["remove_type"] = "remove_word";
		break;
	default:
		assert(0);
		return;
	}
	j["type"] = "removeselection";
	Cursor::StoreCursors(j, this->stored_cursors);
	j["direction"] = direction == PGDirectionLeft ? "left" : "right";
}

size_t RemoveSelection::SerializedSize() {
	size_t size = 50;
	size += stored_cursors.size() * 20;
	return size;
}

void InsertLineBefore::WriteWorkspace(nlohmann::json& j) {
	j["type"] = "insertline";
	Cursor::StoreCursors(j, this->stored_cursors);
	j["direction"] = direction == PGDirectionLeft ? "left" : "right";
}

size_t InsertLineBefore::SerializedSize() {
	size_t size = 50;
	size += stored_cursors.size() * 20;
	return size;
}

#include "regex.h"

TextDelta* PGRegexReplace::CreateRegexReplace(std::string replacement_text, PGRegexHandle regex) {
	if (!regex) {
		return new PGReplaceText(replacement_text);
	}
	int capturing_groups = PGRegexNumberOfCapturingGroups(regex);
	if (capturing_groups < 0) {
		PGDeleteRegex(regex);
		return new PGReplaceText(replacement_text);
	}
	PGRegexReplace* replace = new PGRegexReplace(replacement_text, regex);
	if (replace->groups.size() == 0 || replace->groups[0].second < 0) {
		// no capturing groups in the replacement
		delete replace;
		return new PGReplaceText(replacement_text);
	}
	return replace;
}

PGRegexReplace::PGRegexReplace(std::string replacement_text, PGRegexHandle regex) :
	TextDelta(PGDeltaRegexReplace), regex(regex) {
	// parse replacement_text
	if (replacement_text.size() == 0) return;

	lng prev = 0;
	lng i;
	int capturing_groups = PGRegexNumberOfCapturingGroups(regex);
	for (i = 0; i < replacement_text.size() - 1; i++) {
		if (replacement_text[i] == '\\' || replacement_text[i] == '$') {
			lng j;
			for (j = i + 1; j < replacement_text.size(); j++) {
				if (!panther::is_digit(replacement_text[j])) {
					break;
				}
			}
			if (j == i + 1) continue; // no number following the character
			int number = strtol(replacement_text.c_str() + i + 1, nullptr, 0);
			if (number >= 0 && number <= capturing_groups) {
				// valid capturing group identifier
				groups.push_back(std::pair<std::string, int>(replacement_text.substr(prev, i - prev), number));
				prev = j;
			}

			i = j - 1;
		}
	}
	if (i != replacement_text.size()) {
		groups.push_back(std::pair<std::string, int>(replacement_text.substr(prev, replacement_text.size() - prev), -1));
	}
}

PGRegexReplace::~PGRegexReplace() {
	PGDeleteRegex(regex);
}
