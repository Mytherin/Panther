
#include "directory.h"
#include "textfile.h"

#include <rust/gitignore.h>

PGDirectory::PGDirectory(std::string path) :
	path(path), last_modified_time(-1), loaded_files(false), expanded(false), root(false) {
	this->lock = std::unique_ptr<PGMutex>(CreateMutex());
}

void PGDirectory::FindFile(lng file_number, PGDirectory** directory, PGFile* file) {
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
	lng entry = file_number - file_count;
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
	this->expanded = false;
}

lng PGDirectory::DisplayedFiles() {
	if (expanded) {
		lng recursive_files = 1;
		LockMutex(lock.get());
		for (auto it = directories.begin(); it != directories.end(); it++) {
			recursive_files += (*it)->DisplayedFiles();
		}
		UnlockMutex(lock.get());
		return recursive_files + files.size();
	} else {
		return 1;
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

bool PGDirectory::IterateOverFiles(PGDirectoryIterCallback callback, void* data) {
	LockMutex(lock.get());
	auto files = std::vector<PGFile>(this->files.begin(), this->files.end());
	auto directories = std::vector<std::shared_ptr<PGDirectory>>(this->directories.begin(), this->directories.end());
	UnlockMutex(lock.get());
	for (auto it = files.begin(); it != files.end(); it++) {
		if (!callback(PGFile(PGPathJoin(this->path, it->path)), data)) {
			return false;
		}
	}
	for (auto it = directories.begin(); it != directories.end(); it++) {
		if (!(*it)->IterateOverFiles(callback, data)) {
			return false;
		}
	}
	return true;
}

void PGDirectory::Update(PGIgnoreGlob glob, std::queue<std::shared_ptr<PGDirectory>>& open_directories) {
	loaded_files = false;
	if (PGFileIsIgnored(glob, this->path.c_str(), true)) {
		// file is ignored by the .gitignore glob, skip this path
		return;
	}

	LockMutex(lock.get());

	files.clear();
	std::vector<PGFile> dirs;
	std::vector<std::shared_ptr<PGDirectory>> current_directories;
	if (PGGetDirectoryFiles(path, dirs, files, glob) != PGDirectorySuccess) {
		goto unlock;
	}

	// for each directory, check if it is already present
	// if it is not we add it
	current_directories = std::vector<std::shared_ptr<PGDirectory>>(directories.begin(), directories.end());
	for (auto it = dirs.begin(); it != dirs.end(); it++) {
		std::string path = PGPathJoin(this->path, it->path);
		bool found = false;
		for (auto it2 = current_directories.begin(); it2 != current_directories.end(); it2++) {
			if ((*it2)->path == path) {
				current_directories.erase(it2);
				open_directories.push(*it2);
				found = true;
				break;
			}
		}
		if (!found) {
			// if the directory is not known add it and scan it
			auto new_directory = std::shared_ptr<PGDirectory>(new PGDirectory(path));
			directories.push_back(new_directory);
			open_directories.push(new_directory);
		}
	}
	// remove any directories we did not find (because they have been deleted or removed)
	for (auto it2 = current_directories.begin(); it2 != current_directories.end(); it2++) {
		directories.erase(std::find(directories.begin(), directories.end(), *it2));
	}
	loaded_files = true;
unlock:
	UnlockMutex(lock.get());
}
