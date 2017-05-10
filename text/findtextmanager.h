#pragma once

#define MAXIMUM_FIND_HISTORY 5

#include "json.h"
#include "utils.h"

class FindTextManager {
public:
	FindTextManager();

	bool regex = false;
	bool matchcase = false;
	bool wholeword = false;
	bool wrap = true;
	bool highlight = true;
	bool ignore_binary_files = false;
	bool respect_gitignore = false;

	void LoadWorkspace(nlohmann::json& j);
	void WriteWorkspace(nlohmann::json& j);

	std::vector<std::string> find_history;
	std::vector<std::string> filter_history;
	std::vector<std::string> replace_history;
};
