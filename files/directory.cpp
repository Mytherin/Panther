
#include "directory.h"
#include "textfile.h"

#include <rust/gitignore.h>

PGDirectory::PGDirectory(std::string path, bool show_all_files) :
	path(path), last_modified_time(-1), loaded_files(false), expanded(false) {
	this->lock = std::unique_ptr<PGMutex>(CreateMutex());

	PGIgnoreGlob glob = show_all_files ? nullptr : PGCreateGlobForDirectory(path.c_str());
	this->Update(glob);
	PGDestroyIgnoreGlob(glob);
}

PGDirectory::PGDirectory(std::string path, PGIgnoreGlob glob) :
	path(path), last_modified_time(-1), loaded_files(false), expanded(false) {
	this->lock = std::unique_ptr<PGMutex>(CreateMutex());

	this->Update(glob);
}

PGDirectory::~PGDirectory() {
	LockMutex(lock.get());
	for (auto it = directories.begin(); it != directories.end(); it++) {
		delete *it;
	}
}

void PGDirectory::GetFiles(std::vector<PGFile>& result) {
	LockMutex(lock.get());
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->GetFiles(result);
	}
	for (auto it = files.begin(); it != files.end(); it++) {
		PGFile file = *it;
		file.path = PGPathJoin(this->path, file.path);
		result.push_back(file);
	}
	UnlockMutex(lock.get());
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
			UnlockMutex(lock.get());
			return;
		}
		file_count += files;
	}
	UnlockMutex(lock.get());
	assert(this->expanded);
	lng entry = file_number - file_count;
	assert(entry >= 0 && entry < files.size());
	*file = files[entry];
	file->path = PGPathJoin(this->path, file->path);
}

lng PGDirectory::FindFile(std::string full_name, PGDirectory** directory, PGFile* file, bool search_only_expanded) {
	if (search_only_expanded && !expanded) {
		return -1;
	}
	if (full_name.size() < this->path.size() || 
		full_name.substr(0, this->path.size()) != this->path) {
		return -1;
	}
	std::string remainder = full_name.size() == this->path.size() ? "" : full_name.substr(this->path.size() + 1);
	if (std::find(remainder.begin(), remainder.end(), GetSystemPathSeparator()) == remainder.end()) {
		// this is the final directory
		if (remainder.size() == 0) {
			this->expanded = true;
			return 0;
		}
		lng entry = 1;
		LockMutex(lock.get());
		for (auto it = directories.begin(); it != directories.end(); it++) {
			if ((*it)->path == full_name) {
				this->expanded = true;
				(*it)->expanded = true;
				UnlockMutex(lock.get());
				return entry;
			}
			entry += (*it)->DisplayedFiles();
		}
		for (auto it = files.begin(); it != files.end(); it++) {
			if (it->path == remainder) {
				*directory = this;
				*file = *it;
				this->expanded = true;
				UnlockMutex(lock.get());
				return entry;
			}
			entry++;
		}
		UnlockMutex(lock.get());
	} else {
		lng entry = 1;
		LockMutex(lock.get());
		// this is not the final directory; search recursively in the other directories
		for (auto it = directories.begin(); it != directories.end(); it++) {
			lng directory_entry = (*it)->FindFile(full_name, directory, file, search_only_expanded);
			if (directory_entry >= 0) {
				this->expanded = true;
				UnlockMutex(lock.get());
				return entry + directory_entry;
			}
			entry += (*it)->DisplayedFiles();
		}
		UnlockMutex(lock.get());
	}
	return -1;
}


void PGDirectory::CollapseAll() {
	LockMutex(lock.get());
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->CollapseAll();
	}
	this->expanded = false;
	UnlockMutex(lock.get());
}

void PGDirectory::Update(PGIgnoreGlob glob) {
	if (PGFileIsIgnored(glob, this->path.c_str(), true)) {
		// file is ignored by the .gitignore glob, skip this path
		return;
	}
	files.clear();

	std::vector<PGFile> dirs;
	if (PGGetDirectoryFiles(path, dirs, files, glob) != PGDirectorySuccess) {
		loaded_files = false;
		return;
	}

	// for each directory, check if it is already present
	// if it is not we add it
	LockMutex(lock.get());
	std::vector<PGDirectory*> current_directores = std::vector<PGDirectory*>(directories.begin(), directories.end());
	for (auto it = dirs.begin(); it != dirs.end(); it++) {
		std::string path = PGPathJoin(this->path, it->path);
		bool found = false;
		for (auto it2 = current_directores.begin(); it2 != current_directores.end(); it2++) {
			if ((*it2)->path == path) {
				// if the directory is already known here, update it
				(*it2)->Update(glob);
				if ((*it2)->loaded_files) {
					// if we failed to load files for this directory, leave it in the "current_directores"
					// it will count as "not found" and be removed from the 
					current_directores.erase(it2);
				}
				found = true;
				break;
			}
		}
		if (!found) {
			// if the directory is not known add it and scan it
			PGDirectory* directory = new PGDirectory(path, glob);
			if (!directory->loaded_files) {
				// failed to load files from directory
				delete directory;
			} else {
				directories.push_back(directory);
			}
		}
	}
	// remove any directories we did not find (because they have been deleted or removed)
	for (auto it2 = current_directores.begin(); it2 != current_directores.end(); it2++) {
		directories.erase(std::find(directories.begin(), directories.end(), *it2));
		delete *it2;
	}
	UnlockMutex(lock.get());
	loaded_files = true;
	return;
}

lng PGDirectory::DisplayedFiles() {
	if (expanded) {
		lng recursive_files = 1;
		for (auto it = directories.begin(); it != directories.end(); it++) {
			recursive_files += (*it)->DisplayedFiles();
		}
		return recursive_files + files.size();
	} else {
		return 1;
	}
}


void PGDirectory::ListFiles(std::vector<PGFile>& result_files, PGGlobSet whitelist) {
	for (auto it = files.begin(); it != files.end(); it++) {
		if (whitelist && !PGGlobSetMatches(whitelist, it->path.c_str())) {
			// file does not match whitelist, ignore it
			continue;
		}
		result_files.push_back(PGFile(*it));
	}
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->ListFiles(result_files, whitelist);
	}
}

void PGDirectory::WriteWorkspace(nlohmann::json& j) {
	if (expanded) {
		LockMutex(lock.get());
		std::string filename = PGFile(this->path).Filename();
		j[filename] = nlohmann::json::object();
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

void PGDirectory::IterateOverFiles(PGDirectoryIterCallback callback, void* data) {
	LockMutex(lock.get());
	for (auto it = files.begin(); it != files.end(); it++) {
		callback(*it, data);
	}
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->IterateOverFiles(callback, data);
	}
	UnlockMutex(lock.get());
}
