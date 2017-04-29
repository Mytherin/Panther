
#include "globalsettings.h"
#include "windowfunctions.h"

#include <fstream>

using namespace nlohmann;

PGGlobalSettings::PGGlobalSettings(std::string path) {
	this->path = path;
	std::ifstream i(path);
	this->settings = json::object();
	if (!i) {
		return;
	}
	if (!(i >> settings)) {
		return;
	}
}

void PGGlobalSettings::_WriteSettings() {
	if (path.size() == 0) return;

	std::string temp_filename = path + ".tmp";

	std::ofstream out(path);
	if (!out) {
		return;
	}
	if (!(out << std::setw(4) << settings)) {
		out.close();
		goto cleanup;
	}
	out.close();

	{
		// after writing to the temporary file, we move it over the old settings file 
		auto ret = PGRenameFile(temp_filename, path);
		if (ret == PGIOSuccess) {
			return;
		}
	}
cleanup:
	PGRemoveFile(temp_filename);
}
