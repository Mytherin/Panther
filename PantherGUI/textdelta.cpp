
#include "textdelta.h"
#include "json.h"

using namespace nlohmann;

/*
{
	"type": "addtext",
	"cursors": [[0,1,2,3], [0,4,5,6]],
	"lines": ["hello", "world"]
}
*/

TextDelta* TextDelta::FromString(std::string) {
	return nullptr;
}

std::string AddText::ToString() {
	return "";
}

std::string RemoveText::ToString() {
	return "";
}

std::string RemoveSelection::ToString() {
	return "";
}

std::string InsertLineBefore::ToString() {
	return "";
}
