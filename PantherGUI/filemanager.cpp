
#include "filemanager.h"

FileManager::FileManager() {
}

FileManager::~FileManager() {
	for (auto it = open_files.begin(); it != open_files.end(); it++) {
		if ((*it)->DecRef()) {
			delete *it;
		}
	}
}

TextFile* FileManager::OpenFile() {
	TextFile* textfile = new TextFile(nullptr);
	return OpenFile(textfile);
}

TextFile* FileManager::OpenFile(std::string path, PGFileError& error) {
	TextFile* textfile = TextFile::OpenTextFile(nullptr, path, error, false);
	return OpenFile(textfile);
}

void FileManager::CloseFile(TextFile* textfile) {
	open_files.erase(std::find(open_files.begin(), open_files.end(), textfile));
	if (textfile->DecRef()) {
		Scheduler::RegisterTask(new Task([](Task* task, void* textfile) {
			TextFile* tf = (TextFile*)textfile;
			delete tf;
		}, textfile), PGTaskNotUrgent);
	}
}

void FileManager::ClearFiles() {
	for (auto it = open_files.begin(); it != open_files.end(); it++) {
		if ((*it)->DecRef()) {
			delete *it;
		}
	}
	open_files.clear();
}

TextFile* FileManager::OpenFile(TextFile* textfile) {
	if (!textfile) return nullptr;
	textfile->AddRef();
	open_files.push_back(textfile);
	return textfile;
}
