#pragma once

#include "textfile.h"
#include "windowfunctions.h"

class FileManager;
class FileManager {
public:
	FileManager();
	~FileManager();

	std::shared_ptr<TextFile> OpenFile();
	std::shared_ptr<TextFile> OpenFile(std::string path, PGFileError& error);
	std::shared_ptr<TextFile> OpenFile(std::shared_ptr<TextFile> textfile);
	void CloseFile(std::shared_ptr<TextFile>);
	void ClearFiles();

	std::vector<std::shared_ptr<TextFile>>& GetFiles() { return open_files; }
private:
	TextField* textfield;
	std::vector<std::shared_ptr<TextFile>> open_files;

	std::shared_ptr<TextFile> OpenFile(TextFile* textfile);
};