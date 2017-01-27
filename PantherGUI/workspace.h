#pragma once

#include "settings.h"
#include "utils.h"
#include "windowfunctions.h"

#include <vector>

class PGWorkspace {
public:
	PGWorkspace(PGWindowHandle window);
	void LoadWorkspace(std::string filename);
	void WriteWorkspace();
private:
	std::string filename;
	PGWindowHandle window;
};

