
#include "findtextmanager.h"

FindTextManager::FindTextManager() :
	regex(false), matchcase(false), wholeword(false), wrap(true), highlight(true),
	ignore_binary_files(false), respect_gitignore(false) {

}

void FindTextManager::LoadWorkspace(nlohmann::json& j) {
	if (j.count("find_text") > 0) {
		nlohmann::json& settings = j["find_text"];
		if (!settings.is_object()) return;

		if (settings.count("toggle_regex") && settings["toggle_regex"].is_boolean())
			regex = settings["toggle_regex"];
		if (settings.count("toggle_matchcase") && settings["toggle_matchcase"].is_boolean())
			matchcase = settings["toggle_matchcase"];
		if (settings.count("toggle_wholeword") && settings["toggle_wholeword"].is_boolean())
			wholeword = settings["toggle_wholeword"];
		if (settings.count("toggle_wrap") && settings["toggle_wrap"].is_boolean())
			wrap = settings["toggle_wrap"];
		if (settings.count("toggle_highlight") && settings["toggle_highlight"].is_boolean())
			highlight = settings["toggle_highlight"];
		if (settings.count("toggle_ignorebinary") && settings["toggle_ignorebinary"].is_boolean())
			ignore_binary_files = settings["toggle_ignorebinary"];
		if (settings.count("toggle_respect_gitignore") && settings["toggle_respect_gitignore"].is_boolean())
			respect_gitignore = settings["toggle_respect_gitignore"];
		if (settings.count("find_history") && settings["find_history"].is_array()) {
			nlohmann::json& history = settings["find_history"];;
			for (auto it = history.begin(); it != history.end(); it++)
				if (it->is_string())
					find_history.push_back(*it);
		}
		if (settings.count("replace_history") && settings["replace_history"].is_array()) {
			nlohmann::json& history = settings["replace_history"];;
			for (auto it = history.begin(); it != history.end(); it++)
				if (it->is_string())
					replace_history.push_back(*it);
		}
		if (settings.count("filter_history") && settings["filter_history"].is_array()) {
			nlohmann::json& history = settings["filter_history"];;
			for (auto it = history.begin(); it != history.end(); it++)
				if (it->is_string())
					filter_history.push_back(*it);
		}
	}
}

void FindTextManager::WriteWorkspace(nlohmann::json& j) {
	j["find_text"] = nlohmann::json::object();
	nlohmann::json& settings = j["find_text"];
	settings["toggle_regex"] = regex;
	settings["toggle_matchcase"] = matchcase;
	settings["toggle_wholeword"] = wholeword;
	settings["toggle_wrap"] = wrap;
	settings["toggle_highlight"] = highlight;
	settings["toggle_ignorebinary"] = ignore_binary_files;
	settings["toggle_respect_gitignore"] = respect_gitignore;
	settings["find_history"] = nlohmann::json::array();
	{
		nlohmann::json& history = settings["find_history"];
		for (auto it = find_history.begin(); it != find_history.end(); it++) {
			history.push_back(*it);
		}
	}
	settings["replace_history"] = nlohmann::json::array();
	{
		nlohmann::json& history = settings["replace_history"];
		for (auto it = replace_history.begin(); it != replace_history.end(); it++) {
			history.push_back(*it);
		}
	}
	settings["filter_history"] = nlohmann::json::array();
	{
		nlohmann::json& history = settings["filter_history"];
		for (auto it = filter_history.begin(); it != filter_history.end(); it++) {
			history.push_back(*it);
		}
	}
}


