
#include "workspace.h"

#include <fstream>

using namespace nlohmann;

PGWorkspace::PGWorkspace(PGWindowHandle window) : 
	window(window), filename("") {
	

}

void PGWorkspace::LoadWorkspace(std::string filename) {
	this->filename = filename;
	std::ifstream i(filename);
	if (!i) {
		return;
	}
	json j;
	if (!(i >> j)) {
		assert(0);
		return;
	}
	PGLoadWorkspace(window, j);
	this->settings = j;
}

void PGWorkspace::WriteWorkspace() {
	if (filename.size() == 0) return;

	json j = settings;

	PGWriteWorkspace(window, j);

	std::ofstream out(filename);
	if (!out) {
		return;
	}
	if (!(out << std::setw(4) << j)) {
		assert(0);
		return;
	}
}