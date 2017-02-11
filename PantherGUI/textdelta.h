#pragma once

#include "cursor.h"
#include "json.h"
#include "textbuffer.h"
#include "utils.h"

#include <string>
#include <vector>

class TextDelta;
class TextFile;

typedef enum {
	PGDeltaReplaceText,
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
	std::vector<CursorData> stored_cursors;
	TextDelta* next = nullptr;

	TextDelta(PGTextType type) : 
		type(type), next(nullptr) { }
	PGTextType TextDeltaType() { return type; }
	static std::vector<TextDelta*> LoadWorkspace(nlohmann::json& j);
	virtual void WriteWorkspace(nlohmann::json& j) {};
	virtual size_t SerializedSize() { return 0; }
};

class ReplaceDelta : public TextDelta {
public:
	std::vector<std::string> removed_text;
	std::string text;

	ReplaceDelta(std::string text) :
		TextDelta(PGDeltaReplaceText), text(text) {
		}
	/*
	void WriteWorkspace(nlohmann::json& j);
	size_t SerializedSize();*/
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
