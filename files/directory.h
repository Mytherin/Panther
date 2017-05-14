#pragma once

#include "regex.h"
#include "utils.h"
#include "windowfunctions.h"

#include "thread.h"

#include <queue>

#include <rust/globset.h>
#include <rust/gitignore.h>

class SearchIndex;

typedef bool(*PGDirectoryIterCallback)(PGFile f, void* data, lng filenr, lng total_files);

struct PGDirectory {
	friend class DirectoryIterator;
	friend class ProjectExplorer;
public:
	std::string path;
	lng last_modified_time;
	bool loaded_files;

	PGDirectory(std::string path, PGDirectory* parent);

	void WriteWorkspace(nlohmann::json& j);
	void LoadWorkspace(nlohmann::json& j);

	void FindFile(lng file_number, PGDirectory** directory, PGFile* file);

	lng FindFile(std::string full_name, PGDirectory** directory, PGFile* file, bool search_only_expanded = false);

	bool IsExpanded() { return expanded; }
	void SetExpanded(bool expand);
	void CollapseAll();

	// Returns the number of files displayed by this directory
	lng DisplayedFiles() { return 1 + (expanded ? displayed_files : 0); }
	// Returns the total number of files in this directory
	lng TotalFiles() { return total_files; }

	bool IterateOverFiles(PGDirectoryIterCallback callback, void* data, lng& files, lng total_files);

	void Update(PGIgnoreGlob glob, std::queue<std::shared_ptr<PGDirectory>>& open_directories, SearchIndex* index);
private:
	bool expanded;
	std::unique_ptr<PGMutex> lock;

	void AddFiles(lng files);
	void AddDisplayedFiles(lng files);

	std::vector<std::shared_ptr<PGDirectory>> directories;
	std::vector<PGFile> files;

	// FIXME: this should probably be a weak ptr
	PGDirectory* parent;

	// total amount of displayed files for this directory (including directories)
	lng displayed_files;
	// total amount of files in this directory (excluding directories)
	lng total_files;
};
