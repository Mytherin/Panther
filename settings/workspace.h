#pragma once

#include "settings.h"
#include "utils.h"
#include "windowfunctions.h"

#include "findtextmanager.h"

#include <vector>

class PGWorkspace {
public:
	PGWorkspace();
	void LoadWorkspace(std::string filename);
	std::string WriteWorkspace();

	std::vector<PGWindowHandle>& GetWindows() { return windows; }
	std::string GetName() { return workspace_name; }

	void AddWindow(PGWindowHandle window) { windows.push_back(window); }
	void RemoveWindow(PGWindowHandle window);
	
	FindTextManager& GetFindTextManager() { return findtext; }

	nlohmann::json settings;
private:
	std::string workspace_name;
	std::string filename;
	std::vector<PGWindowHandle> windows;

	FindTextManager findtext;
};

