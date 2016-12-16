#pragma once

#include "textfile.h"
#include "windowfunctions.h"

class FileManager;
class FileManager {
public:
	static FileManager* GetInstance() {
		static FileManager filemanager;
		return &filemanager;
	}

	static TextFile* OpenFile() { return FileManager::GetInstance()->_OpenFile(); }
	static TextFile* OpenFile(std::string path) { return FileManager::GetInstance()->_OpenFile(path); }
	static void CloseFile(TextFile* file) { FileManager::GetInstance()->_CloseFile(file); }

	static std::vector<TextFile*>& GetFiles() { return GetInstance()->open_files; }
private:
	TextFile* _OpenFile();
	TextFile* _OpenFile(std::string path);
	void _CloseFile(TextFile*);
	FileManager();

	TextField* textfield;
	std::vector<TextFile*> open_files;
};