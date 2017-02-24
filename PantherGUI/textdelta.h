#pragma once

#include "cursor.h"
#include "json.h"
#include "textbuffer.h"
#include "utils.h"
#include "regex.h"

#include <string>
#include <vector>

class TextDelta;
class TextFile;

typedef enum {
	PGDeltaAddTextPosition,
	PGDeltaRemoveTextPosition,
	PGDeltaReplaceTextPosition,
	PGDeltaReplaceText,
	PGDeltaRegexReplace,
	PGDeltaRemoveText,
	PGDeltaCursor,
	PGDeltaRemoveLine,
	PGDeltaRemoveCharacter,
	PGDeltaRemoveWord,
	PGDeltaAddEmptyLine,
	PGDeltaUnknown
} PGTextType;

class TextDelta {
public:
	PGTextType type;
	std::vector<PGCursorRange> stored_cursors;
	std::unique_ptr<TextDelta> next = nullptr;

	TextDelta(PGTextType type) : 
		type(type), next(nullptr) { }
	virtual ~TextDelta() { }
	PGTextType TextDeltaType() { return type; }
	static std::vector<TextDelta*> LoadWorkspace(nlohmann::json& j);
	virtual void WriteWorkspace(nlohmann::json& j) {};
	virtual size_t SerializedSize() { return 0; }
};

class PGReplaceText : public TextDelta {
public:
	std::vector<std::string> removed_text;
	std::string text;

	PGReplaceText(std::string text) :
		TextDelta(PGDeltaReplaceText), text(text) {
		}
	/*
	void WriteWorkspace(nlohmann::json& j);
	size_t SerializedSize();*/
};

class PGRegexReplace : public TextDelta {
public:
	std::vector<std::string> removed_text;
	std::vector<lng> added_text_size;
	std::vector<std::pair<std::string, int>> groups;
	PGRegexHandle regex;
	// returns either a PGRegexReplace if replacement_text contains valid group substitutions for <regex>
	// or a PGReplaceText if it does not
	static TextDelta* CreateRegexReplace(std::string replacement_text, PGRegexHandle regex);

	PGRegexReplace(std::string replacement_text, PGRegexHandle regex);
	~PGRegexReplace();
};

class RemoveText : public TextDelta {
public:
	std::vector<std::string> removed_text;

	RemoveText() : 
		TextDelta(PGDeltaRemoveText) { }
	void WriteWorkspace(nlohmann::json& j);
	size_t SerializedSize();
};

class RemoveSelection : public TextDelta {
public:
	PGDirection direction;
	RemoveSelection(PGDirection direction, PGTextType type) : 
		TextDelta(type), direction(direction) { }
	void WriteWorkspace(nlohmann::json& j);
	size_t SerializedSize();
};

class InsertLineBefore : public TextDelta {
public:
	PGDirection direction;
	InsertLineBefore(PGDirection direction) : 
		TextDelta(PGDeltaAddEmptyLine), direction(direction) { }
	void WriteWorkspace(nlohmann::json& j);
	size_t SerializedSize();
};


class AddTextPosition : public TextDelta {
public:
	struct AddTextPositionData {
		std::string text;
		lng line;
		lng character;

		AddTextPositionData(std::string text, lng line, lng character) : 
			text(text), line(line), character(character) {  }
	};

	std::vector<AddTextPositionData> data;
	AddTextPosition() :
		TextDelta(PGDeltaAddTextPosition) {
	}
};

class RemoveTextPosition : public TextDelta {
public:
	std::vector<PGCursorRange> data;
	std::vector<std::string> removed_text;
	RemoveTextPosition() :
		TextDelta(PGDeltaRemoveTextPosition) {
	}
};

class ReplaceTextPosition : public TextDelta {
public:
	std::vector<PGCursorRange> data;
	std::vector<std::string> replacement_text;
	std::vector<std::string> removed_text;
	ReplaceTextPosition() :
		TextDelta(PGDeltaReplaceTextPosition) {
	}
};
