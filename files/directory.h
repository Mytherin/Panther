#pragma once

#include "regex.h"
#include "utils.h"
#include "windowfunctions.h"

#include "thread.h"

#include <queue>

#include <rust/globset.h>
#include <rust/gitignore.h>

typedef bool(*PGDirectoryIterCallback)(PGFile f, void* data);

struct PGDirectory {
	friend class ProjectExplorer;

	std::string path;
	lng last_modified_time;
	bool loaded_files;
	bool expanded;
	bool root = false;

	PGDirectory(std::string path);

	void WriteWorkspace(nlohmann::json& j);
	void LoadWorkspace(nlohmann::json& j);

	void FindFile(lng file_number, PGDirectory** directory, PGFile* file);

	lng FindFile(std::string full_name, PGDirectory** directory, PGFile* file, bool search_only_expanded = false);

	void CollapseAll();

	// Returns the number of files displayed by this directory
	lng DisplayedFiles();

	bool IterateOverFiles(PGDirectoryIterCallback callback, void* data);

	void Update(PGIgnoreGlob glob, std::queue<std::shared_ptr<PGDirectory>>& open_directories);
private:
	std::unique_ptr<PGMutex> lock;

	std::vector<std::shared_ptr<PGDirectory>> directories;
	std::vector<PGFile> files;
};
