
#include "filemanager.h"

FileManager::FileManager() {
}

TextFile* FileManager::_OpenFile() {
	TextFile* textfile = new TextFile(nullptr);
	open_files.push_back(textfile);
	return textfile;
}

TextFile* FileManager::_OpenFile(std::string path) {
	TextFile* textfile = TextFile::OpenTextFile(nullptr, path, false);
	if (!textfile) return nullptr;
	open_files.push_back(textfile);
	return textfile;
}

void FileManager::_CloseFile(TextFile* textfile) {
	open_files.erase(std::find(open_files.begin(), open_files.end(), textfile));
	Scheduler::RegisterTask(new Task([](Task* task, void* textfile) {
		TextFile* tf = (TextFile*)textfile;
		delete tf;
	}, textfile), PGTaskNotUrgent);
}


