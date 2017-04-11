#pragma once

#include "settings.h"
#include "utils.h"
#include "windowfunctions.h"

#include <vector>

class PGWorkspace {
public:
	PGWorkspace();
	void LoadWorkspace(std::string filename);
	void WriteWorkspace();

	std::vector<PGWindowHandle>& GetWindows() { return windows; }
	
	void AddWindow(PGWindowHandle window) { windows.push_back(window); }
	void RemoveWindow(PGWindowHandle window) { windows.erase(std::find(windows.begin(), windows.end(), window)); }

	nlohmann::json settings;
private:
	std::string filename;
	std::vector<PGWindowHandle> windows;
};

