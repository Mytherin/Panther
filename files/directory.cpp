
#include "directory.h"
#include "textfile.h"

#include <rust/gitignore.h>

#include "searchindex.h"

PGDirectory::PGDirectory(std::string path, PGDirectory* parent) :
	path(path), last_modified_time(-1), loaded_files(false), expanded(false),
	displayed_files(0), total_files(0), parent(parent) {
	this->lock = std::unique_ptr<PGMutex>(CreateMutex());
}

void PGDirectory::FindFile(lng file_number, PGDirectory** directory, PGFile* file) {
	lng entry;
	if (file_number == 0) {
		*directory = this;
		return;
	}
	lng file_count = 1;
	LockMutex(lock.get());
	for (auto it = directories.begin(); it != directories.end(); it++) {
		lng files = (*it)->DisplayedFiles();
		if (file_number >= file_count && file_number < file_count + files) {
			(*it)->FindFile(file_number - file_count, directory, file);
			goto unlock;
		}
		file_count += files;
	}
	assert(this->expanded);
	entry = file_number - file_count;
	assert(entry >= 0 && entry < files.size());
	*file = files[entry];
	file->path = PGPathJoin(this->path, file->path);
unlock:
	UnlockMutex(lock.get());
}

lng PGDirectory::FindFile(std::string full_name, PGDirectory** directory, PGFile* file, bool search_only_expanded) {
	if (search_only_expanded && !expanded) {
		return -1;
	}
	if (full_name.size() < this->path.size() ||
		full_name.substr(0, this->path.size()) != this->path) {
		return -1;
	}
	lng entry = 1;
	std::string remainder = full_name.size() == this->path.size() ? "" : full_name.substr(this->path.size() + 1);
	if (std::find(remainder.begin(), remainder.end(), GetSystemPathSeparator()) == remainder.end()) {
		// this is the final directory
		if (remainder.size() == 0) {
			this->expanded = true;
			return 0;
		}
		LockMutex(lock.get());
		for (auto it = directories.begin(); it != directories.end(); it++) {
			if ((*it)->path == full_name) {
				this->expanded = true;
				(*it)->expanded = true;
				goto unlock;
			}
			entry += (*it)->DisplayedFiles();
		}
		for (auto it = files.begin(); it != files.end(); it++) {
			if (it->path == remainder) {
				*directory = this;
				*file = *it;
				this->expanded = true;
				goto unlock;
			}
			entry++;
		}
	} else {
		LockMutex(lock.get());
		// this is not the final directory; search recursively in the other directories
		for (auto it = directories.begin(); it != directories.end(); it++) {
			lng directory_entry = (*it)->FindFile(full_name, directory, file, search_only_expanded);
			if (directory_entry >= 0) {
				this->expanded = true;
				entry += directory_entry;
				goto unlock;
			}
			entry += (*it)->DisplayedFiles();
		}
	}
	entry = -1;
unlock:
	UnlockMutex(lock.get());
	return entry;
}


void PGDirectory::CollapseAll() {
	LockMutex(lock.get());
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->CollapseAll();
	}
	UnlockMutex(lock.get());
	this->SetExpanded(false);
}

void PGDirectory::SetExpanded(bool expand) {
	if (this->expanded != expand) {
		this->expanded = expand;
		if (parent) {
			lng file_diff = expand ? displayed_files : -displayed_files;
			parent->AddDisplayedFiles(file_diff);
		}
	}
}

void PGDirectory::WriteWorkspace(nlohmann::json& j) {
	if (expanded) {
		std::string filename = PGFile(this->path).Filename();
		j[filename] = nlohmann::json::object();
		LockMutex(lock.get());
		for (auto it = directories.begin(); it != directories.end(); it++) {
			(*it)->WriteWorkspace(j[filename]);
		}
		UnlockMutex(lock.get());
	}
}

void PGDirectory::LoadWorkspace(nlohmann::json& j) {
	assert(j.is_object());
	std::string filename = PGFile(this->path).Filename();
	if (j.count(filename) > 0 && j[filename].is_object()) {
		this->expanded = true;
		LockMutex(lock.get());
		for (auto it = directories.begin(); it != directories.end(); it++) {
			(*it)->LoadWorkspace(j[filename]);
		}
		UnlockMutex(lock.get());
	}
}

bool PGDirectory::IterateOverFiles(PGDirectoryIterCallback callback, void* data, lng& filenr, lng total_files) {
	LockMutex(lock.get());
	auto files = std::vector<PGFile>(this->files.begin(), this->files.end());
	auto directories = std::vector<std::shared_ptr<PGDirectory>>(this->directories.begin(), this->directories.end());
	UnlockMutex(lock.get());
	for (auto it = files.begin(); it != files.end(); it++) {
		filenr++;
		if (!callback(PGFile(PGPathJoin(this->path, it->path)), data, filenr, total_files)) {
			return false;
		}
	}
	for (auto it = directories.begin(); it != directories.end(); it++) {
		if (!(*it)->IterateOverFiles(callback, data, filenr, total_files)) {
			return false;
		}
	}
	return true;
}

void PGDirectory::Update(PGIgnoreGlob glob, std::queue<std::shared_ptr<PGDirectory>>& open_directories, SearchIndex* index) {
	lng difference, previous_dircount, directory_difference = 0, file_difference = 0;
	loaded_files = false;
	if (PGFileIsIgnored(glob, this->path.c_str(), true)) {
		// file is ignored by the .gitignore glob, skip this path
		return;
	}

	LockMutex(lock.get());

	lng previous_filecount = files.size();
	files.clear();
	std::vector<PGFile> dirs;
	std::vector<std::shared_ptr<PGDirectory>> current_directories;
	if (PGGetDirectoryFiles(path, dirs, files, glob) != PGDirectorySuccess) {
		goto unlock;
	}
	difference = files.size() - previous_filecount;
	// FIXME: only add files that were not in previously
	if (index) {
		for (auto it = files.begin(); it != files.end(); it++) {
			std::string path = PGPathJoin(this->path, it->path);
			SearchEntry entry;
			entry.display_name = it->path;
			entry.display_subtitle = path;
			entry.text = path;
			entry.basescore = 0;
			entry.multiplier = 1;
			index->AddEntry(entry);
		}
	}
	this->AddFiles(difference);

	// for each directory, check if it is already present
	// if it is not we add it
	previous_dircount = directories.size();
	current_directories = std::vector<std::shared_ptr<PGDirectory>>(directories.begin(), directories.end());
	for (auto it = dirs.begin(); it != dirs.end(); it++) {
		std::string path = PGPathJoin(this->path, it->path);
		bool found = false;
		for (auto it2 = current_directories.begin(); it2 != current_directories.end(); it2++) {
			if ((*it2)->path == path) {
				open_directories.push(*it2);
				current_directories.erase(it2);
				found = true;
				break;
			}
		}
		if (!found) {
			// if the directory is not known add it and scan it
			auto new_directory = std::shared_ptr<PGDirectory>(new PGDirectory(path, this));
			directories.push_back(new_directory);
			open_directories.push(new_directory);
		}
	}
	// remove any directories we did not find (because they have been deleted or removed)
	for (auto it2 = current_directories.begin(); it2 != current_directories.end(); it2++) {
		file_difference += -(*it2)->total_files;
		if ((*it2)->expanded) {
			directory_difference += -(*it2)->displayed_files;
		}
		directories.erase(std::find(directories.begin(), directories.end(), *it2));
	}
	directory_difference += directories.size() - previous_dircount;
	this->AddFiles(file_difference);
	this->AddDisplayedFiles(difference + directory_difference);
	loaded_files = true;
unlock:
	UnlockMutex(lock.get());
}

void PGDirectory::AddFiles(lng files) {
	if (files == 0) return;

	this->total_files += files;
	if (parent) {
		parent->AddFiles(files);
	}
}

void PGDirectory::AddDisplayedFiles(lng files) {
	if (files == 0) return;

	this->displayed_files += files;
	if (this->expanded && parent) {
		parent->displayed_files += files;
	}
}
