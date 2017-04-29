#pragma once

#include "textfile.h"
#include "windowfunctions.h"

class FileManager {
public:
	static FileManager* GetInstance() {
		static FileManager filemanager;
		return &filemanager;
	}

	static void Initialize() { GetInstance(); }

	static std::shared_ptr<TextFile> OpenFile() { return GetInstance()->_OpenFile(); }
	static std::shared_ptr<TextFile> OpenFile(std::string path, PGFileError& error) { return GetInstance()->_OpenFile(path, error); }
	static std::shared_ptr<TextFile> OpenFile(std::shared_ptr<TextFile> textfile) { return GetInstance()->_OpenFile(textfile); }
	static void CloseFile(std::shared_ptr<TextFile> ptr) { GetInstance()->_CloseFile(ptr); }

	static std::shared_ptr<TextFile> FindFile(std::string path) { return GetInstance()->_FindFile(path); }

private:
	FileManager();
	~FileManager();

	std::shared_ptr<TextFile> _OpenFile();
	std::shared_ptr<TextFile> _OpenFile(std::string path, PGFileError& error);
	std::shared_ptr<TextFile> _OpenFile(std::shared_ptr<TextFile> textfile);
	void _CloseFile(std::shared_ptr<TextFile>);

	std::shared_ptr<TextFile> _FindFile(std::string path);
	
	std::vector<std::shared_ptr<TextFile>> open_files;

	std::shared_ptr<TextFile> _OpenFile(TextFile* textfile);

	std::unique_ptr<PGMutex> lock;
};