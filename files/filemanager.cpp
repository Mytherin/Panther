
#include "filemanager.h"

FileManager::FileManager() {
	this->lock = std::unique_ptr<PGMutex>(CreateMutex());
}

FileManager::~FileManager() {
}

std::shared_ptr<TextFile> FileManager::_OpenFile() {
	TextFile* textfile = new TextFile();
	return _OpenFile(textfile);
}

std::shared_ptr<TextFile> FileManager::_OpenFile(std::string path, PGFileError& error) {
	return _OpenFile(TextFile::OpenTextFile(path, error, false));
}

std::shared_ptr<TextFile> FileManager::_OpenFile(TextFile* textfile) {
	if (!textfile) return nullptr;
	auto ptr = std::shared_ptr<TextFile>(textfile);
	LockMutex(lock.get());
	open_files.push_back(ptr);
	UnlockMutex(lock.get());
	return ptr;
}

std::shared_ptr<TextFile> FileManager::_OpenFile(std::shared_ptr<TextFile> textfile) {
	if (!textfile) return nullptr;
	assert(std::find(open_files.begin(), open_files.end(), textfile) == open_files.end());
	LockMutex(lock.get());
	open_files.push_back(textfile);
	UnlockMutex(lock.get());
	return textfile;
} 

void FileManager::_CloseFile(std::shared_ptr<TextFile> file) {
	LockMutex(lock.get());
	if (file->find_task) {
		file->find_task->active = false;
		file->find_task = nullptr;
	}
	file->current_task = nullptr;
	assert(std::find(open_files.begin(), open_files.end(), file) != open_files.end());
	open_files.erase(std::find(open_files.begin(), open_files.end(), file));
	UnlockMutex(lock.get());
}

std::shared_ptr<TextFile> FileManager::_FindFile(std::string path) {
	LockMutex(lock.get());
	for (auto it = open_files.begin(); it != open_files.end(); it++) {
		if ((*it)->GetFullPath() == path) {
			UnlockMutex(lock.get());
			return *it;
		}
	}
	UnlockMutex(lock.get());
	return nullptr;
}

void FileManager::_LoadWorkspace(nlohmann::json& j) {
	assert(j.is_array());
	open_files.clear();
	for (auto it = j.begin(); it != j.end(); it++) {
		std::shared_ptr<TextFile> file = nullptr;
		std::string path = "";

		if (it->count("file") > 0) {
			path = (*it)["file"].get<std::string>();
		}
		if (it->count("buffer") > 0) {
			// if we have the text stored in a buffer
			// we load the text from the buffer, rather than from the file
			std::string buffer = (*it)["buffer"];
			file = TextFile::OpenTextFile(PGEncodingUTF8, path, (char*)buffer.c_str(), buffer.size(), true);
			//file = std::shared_ptr<TextFile>(new TextFile(PGEncodingUTF8, path.c_str(), (char*)buffer.c_str(), buffer.size(), true, false));
			if (path.size() == 0) {
				file->name = "untitled";
			}
			file->SetUnsavedChanges(true);
			FileManager::OpenFile(file);
		} else if (path.size() > 0) {
			// otherwise, if there is a file speciifed
			// we load the text from the file instead
			PGFileError error;
			file = FileManager::OpenFile(path, error);
		} else {
			continue;
		}
		if (!file) continue;

		// load additional text file settings from the workspace
		// note that we do not apply them to the textfile immediately because of concurrency concerns
		PGTextFileSettings settings;
		if (it->count("name") > 0) {
			std::string name = (*it)["name"];
			settings.name = name;
		}
		if (it->count("lineending") > 0) {
			std::string ending = (*it)["lineending"];
			if (ending == "windows") {
				settings.line_ending = PGLineEndingWindows;
			} else if (ending == "unix") {
				settings.line_ending = PGLineEndingUnix;
			} else if (ending == "macos") {
				settings.line_ending = PGLineEndingMacOS;
			}
		}
		if (it->count("encoding") > 0) {
			std::string encoding_name = (*it)["encoding"];
			settings.encoding = PGEncodingFromString(encoding_name);
		}
		if (it->count("language") > 0) {
			std::string language_name = (*it)["language"];
			settings.language = PGLanguageManager::GetLanguageFromName(language_name);
		}
		// apply the settings, either immediately if the file has been loaded
		// or wait until after it has been loaded otherwise
		file->SetSettings(settings);
	}

}

void FileManager::_WriteWorkspace(nlohmann::json& j) {
	assert(j.is_array());
	for (auto it = open_files.begin(); it != open_files.end(); it++) {
		j.push_back(nlohmann::json::object());
		auto& cur = j.back();
		TextFile* file = it->get();

		if (file->HasUnsavedChanges()) {
			// the file has unsaved changes
			// so we have to write the file
			auto store_file_type = file->WorkspaceFileStorage();
			if (store_file_type == TextFile::PGStoreFileBuffer) {
				// write the buffer
				cur["buffer"] = file->GetText();
			} else if (store_file_type == TextFile::PGStoreFileDeltas) {
				// delta storage not supported yet
				assert(0);
			}
		}

		std::string path = file->GetFullPath();
		if (path.size() != 0) {
			cur["file"] = path;
		} else {
			cur["name"] = file->GetName();
		}
		auto line_ending = file->GetLineEnding();
		if (line_ending == PGLineEndingMixed || line_ending == PGLineEndingUnknown) {
			line_ending = GetSystemLineEnding();
		}
		std::string endings;
		switch (line_ending) {
			case PGLineEndingWindows:
				endings = "Windows";
				break;
			case PGLineEndingUnix:
				endings = "Unix";
				break;
			case PGLineEndingMacOS:
				endings = "MacOS";
				break;
		}
		auto language = file->GetLanguage();
		if (language) {
			cur["language"] = language->GetName();
		}
		cur["lineending"] = endings;
		cur["encoding"] = PGEncodingToString(file->GetFileEncoding());
	}
}

std::shared_ptr<TextFile> FileManager::_GetFileByIndex(size_t index) {
	if (index < open_files.size())
		return open_files[index];
	return nullptr;
}

size_t FileManager::_GetFileIndex(std::shared_ptr<TextFile> file) {
	auto entry = std::find(open_files.begin(), open_files.end(), file);
	assert(entry != open_files.end());
	return entry - open_files.begin();
}
