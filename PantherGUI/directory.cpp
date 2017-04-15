
#include "directory.h"
#include "textfile.h"

#include <rust/gitignore.h>

void PGDirectory::GetFiles(std::vector<PGFile>& result) {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->GetFiles(result);
	}
	for (auto it = files.begin(); it != files.end(); it++) {
		PGFile file = *it;
		file.path = PGPathJoin(this->path, file.path);
		result.push_back(file);
	}
}

void PGDirectory::FindFile(lng file_number, PGDirectory** directory, PGFile* file) {
	if (file_number == 0) {
		*directory = this;
		return;
	}
	lng file_count = 1;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		lng files = (*it)->DisplayedFiles();
		if (file_number >= file_count && file_number < file_count + files) {
			(*it)->FindFile(file_number - file_count, directory, file);
			return;
		}
		file_count += files;
	}
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
	if (full_name.substr(0, this->path.size()) != this->path) {
		return -1;
	}
	std::string remainder = full_name.substr(this->path.size() + 1);
	if (std::find(remainder.begin(), remainder.end(), GetSystemPathSeparator()) == remainder.end()) {
		// this is the final directory
		lng entry = 1;
		for (auto it = directories.begin(); it != directories.end(); it++) {
			entry += (*it)->DisplayedFiles();
		}
		for (auto it = files.begin(); it != files.end(); it++) {
			if (it->path == remainder) {
				*directory = this;
				*file = *it;
				this->expanded = true;
				return entry;
			}
			entry++;
		}
	} else {
		lng entry = 1;
		// this is not the final directory; search recursively in the other directories
		for (auto it = directories.begin(); it != directories.end(); it++) {
			lng directory_entry = (*it)->FindFile(full_name, directory, file, search_only_expanded);
			if (directory_entry >= 0) {
				this->expanded = true;
				return entry + directory_entry;
			}
			entry += (*it)->DisplayedFiles();
		}
	}
	return -1;
}


PGDirectory::PGDirectory(std::string path, bool respect_gitignore) :
	path(path), last_modified_time(-1), loaded_files(false), expanded(false) {
	this->Update(respect_gitignore);
}

PGDirectory::~PGDirectory() {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		delete *it;
	}
	directories.clear();
}

void PGDirectory::CollapseAll() {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->CollapseAll();
	}
	this->expanded = false;
}

void PGDirectory::Update(bool respect_gitignore) {
	files.clear();

	std::vector<PGFile> dirs;
	if (respect_gitignore) {
		std::vector<std::pair<bool, std::string>> _files;
		PGListFiles(this->path.c_str(), [](void* data, const char* path, bool is_directory) {
			auto files = (std::vector<std::pair<bool, std::string>>*)data;
			files->push_back(std::pair<bool, std::string>(is_directory, path));
		}, &_files, true);

		for (auto it = _files.begin(); it != _files.end(); it++) {
			if (it->first) {
				dirs.push_back(PGFile(it->second));
			} else {
				files.push_back(PGFile(it->second));
			}
		}
	} else {
		if (PGGetDirectoryFiles(path, dirs, files) != PGDirectorySuccess) {
			loaded_files = false;
			return;
		}
	}

	// for each directory, check if it is already present
	// if it is not we add it
	std::vector<PGDirectory*> current_directores = std::vector<PGDirectory*>(directories.begin(), directories.end());
	for (auto it = dirs.begin(); it != dirs.end(); it++) {
		std::string path = PGPathJoin(this->path, it->path);
		bool found = false;
		for (auto it2 = current_directores.begin(); it2 != current_directores.end(); it2++) {
			if ((*it2)->path == path) {
				// if the directory is already known here, update it
				(*it2)->Update(respect_gitignore);
				current_directores.erase(it2);
				found = true;
				break;
			}
		}
		if (!found) {
			// if the directory is not known add it and scan it
			directories.push_back(new PGDirectory(path, respect_gitignore));
		}
	}
	// remove any directories we did not find (because they have been deleted or removed)
	for (auto it2 = current_directores.begin(); it2 != current_directores.end(); it2++) {
		directories.erase(std::find(directories.begin(), directories.end(), *it2));
		delete *it2;
	}
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
	std::vector<std::string> files;
	PGListFiles(this->path.c_str(), [](void* data, const char* path, bool is_directory) {
		auto files = (std::vector<std::string>*)data;
		files->push_back(path);
	}, &files, false);

	for (auto it = files.begin(); it != files.end(); it++) {
		if (whitelist && !PGGlobSetMatches(whitelist, it->c_str())) {
			// file does not match whitelist, ignore it
			continue;
		}
		result_files.push_back(PGFile(*it));
	}
}
