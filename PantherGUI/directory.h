#pragma once

#include "regex.h"
#include "utils.h"
#include "windowfunctions.h"

#include <rust/globset.h>

struct PGDirectory {
	std::string path;
	std::vector<PGDirectory*> directories;
	std::vector<PGFile> files;
	lng last_modified_time;
	bool loaded_files;
	bool expanded;

	void ListFiles(std::vector<PGFile>& result_files, PGGlobSet whitelist);
	void FindFile(lng file_number, PGDirectory** directory, PGFile* file);
	lng FindFile(std::string full_name, PGDirectory** directory, PGFile* file);

	PGDirectory(std::string path);
	~PGDirectory();
	// Returns the number of files displayed by this directory
	lng DisplayedFiles();
	void Update();
	void GetFiles(std::vector<PGFile>& files);
};
