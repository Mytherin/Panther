#pragma once

#include "regex.h"
#include "utils.h"
#include "windowfunctions.h"

#include "thread.h"

#include <rust/globset.h>
#include <rust/gitignore.h>

typedef void(*PGDirectoryIterCallback)(PGFile f, void* data);

struct PGDirectory {
	std::string path;
	std::vector<PGDirectory*> directories;
	std::vector<PGFile> files;
	lng last_modified_time;
	bool loaded_files;
	bool expanded;

	std::unique_ptr<PGMutex> lock;

	PGDirectory(std::string path, bool show_all_files);
	PGDirectory(std::string path, PGIgnoreGlob glob = nullptr);
	~PGDirectory();

	void WriteWorkspace(nlohmann::json& j);
	void LoadWorkspace(nlohmann::json& j);

	void FindFile(lng file_number, PGDirectory** directory, PGFile* file);

	lng FindFile(std::string full_name, PGDirectory** directory, PGFile* file, bool search_only_expanded = false);

	void CollapseAll();

	// Returns the number of files displayed by this directory
	lng DisplayedFiles();
	void Update(PGIgnoreGlob glob);
	void GetFiles(std::vector<PGFile>& files);
	void ListFiles(std::vector<PGFile>& result_files, PGGlobSet whitelist);
	void IterateOverFiles(PGDirectoryIterCallback callback, void* data);
private:
	void ActualUpdate(PGIgnoreGlob glob);
};
