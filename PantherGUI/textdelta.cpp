
#include "textdelta.h"
#include "json.h"

using namespace nlohmann;

static void StoreCursors(nlohmann::json& j, std::vector<CursorData>& cursors) {
	j["cursors"] = json::array();
	json& cursor = j["cursors"];
	lng index = 0;
	for (auto it = cursors.begin(); it != cursors.end(); it++) {
		cursor[index] = json::array();
		json& current_cursor = cursor[index];
		current_cursor[0] = it->start_line;
		current_cursor[1] = it->start_position;
		current_cursor[2] = it->end_line;
		current_cursor[3] = it->end_position;
		index++;
	}
}

static void LoadCursors(nlohmann::json& j, std::vector<CursorData>& stored_cursors) {
	if (j.count("cursors") > 0) {
		json& cursors = j["cursors"];
		if (cursors.is_array()) {
			for (auto it = cursors.begin(); it != cursors.end(); it++) {
				int size = it->size();
				if (it->is_array() && it->size() == 4 &&
					(*it)[0].is_number() && (*it)[1].is_number() && (*it)[2].is_number() && (*it)[3].is_number()) {

					int start_line = (*it)[0];
					int start_pos = (*it)[1];
					int end_line = (*it)[2];
					int end_pos = (*it)[3];

					stored_cursors.push_back(CursorData(start_line, start_pos, end_line, end_pos));
				}
			}
		}
	}
}

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
					AddText* text = new AddText(std::vector<std::string>());
					if (k.count("lines") > 0) {
						json& lines = k["lines"];
						LoadStrings(lines, text->lines);
					}
					LoadCursors(k, text->stored_cursors);
					if (text->lines.size() == 0 || text->stored_cursors.size() == 0) {
						delete text;
						continue;
					}
					deltas.push_back(text);
				} else if (k["type"] == "removetext") {
					RemoveText* text = new RemoveText();
					if (k.count("removed_text") > 0) {
						json& lines = k["removed_text"];
						LoadStrings(lines, text->removed_text);
					}
					LoadCursors(k, text->stored_cursors);
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
						LoadCursors(k, text->stored_cursors);
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
						LoadCursors(k, text->stored_cursors);
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

void AddText::WriteWorkspace(nlohmann::json& j) {
	j["type"] = "addtext";
	StoreCursors(j, this->stored_cursors);
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

size_t AddText::SerializedSize() {
	size_t size = 40;
	for (auto it = lines.begin(); it != lines.end(); it++) {
		size += it->size();
	}
	size += stored_cursors.size() * 20;
	return size;
}

void RemoveText::WriteWorkspace(nlohmann::json& j) {
	j["type"] = "removetext";
	StoreCursors(j, this->stored_cursors);
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
	StoreCursors(j, this->stored_cursors);
	j["direction"] = direction == PGDirectionLeft ? "left" : "right";
}

size_t RemoveSelection::SerializedSize() {
	size_t size = 50;
	size += stored_cursors.size() * 20;
	return size;
}

void InsertLineBefore::WriteWorkspace(nlohmann::json& j) {
	j["type"] = "insertline";
	StoreCursors(j, this->stored_cursors);
	j["direction"] = direction == PGDirectionLeft ? "left" : "right";
}

size_t InsertLineBefore::SerializedSize() {
	size_t size = 50;
	size += stored_cursors.size() * 20;
	return size;
}
