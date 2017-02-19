
#include "directory.h"
#include "textfile.h"

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

PGDirectory::PGDirectory(std::string path) :
	path(path), last_modified_time(-1), loaded_files(false), expanded(false) {
	this->Update();
}

PGDirectory::~PGDirectory() {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		delete *it;
	}
	directories.clear();
}


void PGDirectory::Update() {
	files.clear();
	std::vector<PGFile> dirs;
	if (PGGetDirectoryFiles(path, dirs, files) == PGDirectorySuccess) {
		// for each directory, check if it is already present
		// if it is not we add it
		for (auto it = dirs.begin(); it != dirs.end(); it++) {
			std::string path = PGPathJoin(this->path, it->path);
			bool found = false;
			for (auto it2 = directories.begin(); it2 != directories.end(); it2++) {
				if ((*it2)->path == path) {
					found = true;
					break;
				}
			}
			if (!found) {
				directories.push_back(new PGDirectory(path));
			}
		}
		// FIXME: if directory not found, delete it
		loaded_files = true;
	} else {
		loaded_files = false;
	}
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

void PGDirectory::FindInDirectory(PGRegexHandle regex, PGGlobSet globset, int context_lines, PGMatchCallback callback, void* data) {
	for (auto it = files.begin(); it != files.end(); it++) {
		auto file = (*it);
		std::string path = PGPathJoin(this->path, file.path);
		if (globset) {
			if (!PGGlobSetMatches(globset, path.c_str())) {
				// FIXME: if black list, then ignore files that match instead of files that do not
				// ignore this file
				continue;
			}
		}
		lng size;
		PGFileError error = PGFileSuccess;
		// FIXME: use streaming textfile instead
		TextFile* textfile = TextFile::OpenTextFile(nullptr, path, error, true);
		if (error != PGFileSuccess || !textfile) {
			continue;
		}
		textfile->FindAllMatches(regex, context_lines, callback, data);
		delete textfile;
	}
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->FindInDirectory(regex, globset, context_lines, callback, data);
	}
}
