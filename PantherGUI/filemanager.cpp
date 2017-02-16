
#include "filemanager.h"

FileManager::FileManager() {
}

FileManager::~FileManager() {
	/*for (auto it = open_files.begin(); it != open_files.end(); it++) {
		if ((*it)->DecRef()) {
			delete *it;
		}
	}*/
}

std::shared_ptr<TextFile> FileManager::OpenFile() {
	TextFile* textfile = new TextFile(nullptr);
	return OpenFile(textfile);
}

std::shared_ptr<TextFile> FileManager::OpenFile(std::string path, PGFileError& error) {
	TextFile* textfile = TextFile::OpenTextFile(nullptr, path, error, false);
	return OpenFile(textfile);
}

void FileManager::CloseFile(std::shared_ptr<TextFile> textfile) {
	open_files.erase(std::find(open_files.begin(), open_files.end(), textfile));
	/*if (textfile->DecRef()) {
		Scheduler::RegisterTask(new Task([](Task* task, void* textfile) {
			TextFile* tf = (TextFile*)textfile;
			delete tf;
		}, textfile), PGTaskNotUrgent);
	}*/
}

void FileManager::ClearFiles() {
	open_files.clear();
}

std::shared_ptr<TextFile> FileManager::OpenFile(TextFile* textfile) {
	if (!textfile) return nullptr;
	open_files.push_back(std::shared_ptr<TextFile>(textfile));
	return open_files.back();
}

std::shared_ptr<TextFile> FileManager::OpenFile(std::shared_ptr<TextFile> textfile) {
	if (!textfile) return nullptr;
	open_files.push_back(textfile);
	return open_files.back();
}
