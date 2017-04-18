
#include "filemanager.h"

FileManager::FileManager() {
	this->lock = std::unique_ptr<PGMutex>(CreateMutex());
}

FileManager::~FileManager() {
}

std::shared_ptr<TextFile> FileManager::_OpenFile() {
	TextFile* textfile = new TextFile(nullptr);
	return _OpenFile(textfile);
}

std::shared_ptr<TextFile> FileManager::_OpenFile(std::string path, PGFileError& error) {
	TextFile* textfile = TextFile::OpenTextFile(nullptr, path, error, false);
	return _OpenFile(textfile);
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
	LockMutex(lock.get());
	open_files.push_back(textfile);
	UnlockMutex(lock.get());
	return textfile;
}

void FileManager::_CloseFile(std::shared_ptr<TextFile> textfile) {
	LockMutex(lock.get());
	assert(std::find(open_files.begin(), open_files.end(), textfile) != open_files.end());
	open_files.erase(std::find(open_files.begin(), open_files.end(), textfile));
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

