
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
	std::string errmsg;
	if (filename.size() == 0) return;

	json j = settings;

	PGWriteWorkspace(window, j);

	// we write the workspace to a temporary file first (workspace.json.tmp)
	std::string temp_filename = filename + ".tmp";

	std::ofstream out(temp_filename);
	if (!out) {
		return;
	}
	if (!(out << std::setw(4) << j)) {
		out.close();
		goto cleanup;
	}
	out.close();

	// after writing to the temporary file, we move it over the old workspace file 
	auto ret = PGRenameFile(temp_filename, filename);
	if (ret == PGIOSuccess) {
		return;
	}
cleanup:
	PGRemoveFile(temp_filename);
}