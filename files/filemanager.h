#pragma once

#include "textfile.h"
#include "windowfunctions.h"

// FIXME: FileManager should not be static -> one filemanager per workspace
class FileManager {
public:
	static FileManager* GetInstance() {
		static FileManager filemanager;
		return &filemanager;
	}

	static void LoadWorkspace(nlohmann::json& j) { GetInstance()->_LoadWorkspace(j); }
	static void WriteWorkspace(nlohmann::json& j) { return GetInstance()->_WriteWorkspace(j); }

	static void Initialize() { GetInstance(); }

	static std::shared_ptr<TextFile> GetFileByIndex(size_t index) { return GetInstance()->_GetFileByIndex(index); }
	static size_t GetFileIndex(std::shared_ptr<TextFile> file) { return GetInstance()->_GetFileIndex(file); }
	static std::shared_ptr<TextFile> OpenFile() { return GetInstance()->_OpenFile(); }
	static std::shared_ptr<TextFile> OpenFile(std::string path, PGFileError& error) { return GetInstance()->_OpenFile(path, error); }
	static std::shared_ptr<TextFile> OpenFile(std::shared_ptr<TextFile> textfile) { return GetInstance()->_OpenFile(textfile); }
	static void CloseFile(std::shared_ptr<TextFile> ptr) { GetInstance()->_CloseFile(ptr); }

	static std::shared_ptr<TextFile> FindFile(std::string path) { return GetInstance()->_FindFile(path); }

private:
	FileManager();
	~FileManager();

	void _LoadWorkspace(nlohmann::json& j);
	void _WriteWorkspace(nlohmann::json& j);

	std::shared_ptr<TextFile> _OpenFile();
	std::shared_ptr<TextFile> _OpenFile(std::string path, PGFileError& error);
	std::shared_ptr<TextFile> _OpenFile(std::shared_ptr<TextFile> textfile);
	void _CloseFile(std::shared_ptr<TextFile>);

	std::shared_ptr<TextFile> _GetFileByIndex(size_t index);
	size_t _GetFileIndex(std::shared_ptr<TextFile> file);

	std::shared_ptr<TextFile> _FindFile(std::string path);
	
	std::vector<std::shared_ptr<TextFile>> open_files;

	std::shared_ptr<TextFile> _OpenFile(TextFile* textfile);

	std::unique_ptr<PGMutex> lock;
};