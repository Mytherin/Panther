#pragma once

#pragma once

#include "button.h"
#include "container.h"
#include "directory.h"
#include "simpletextfield.h"
#include "togglebutton.h"
#include "scrollbar.h"

typedef enum {
	PGFileOperationRename
} PGFileOperation;

class FileOperationDelta {
public:
	FileOperationDelta(PGFileOperation type) : type(type) {}

	PGFileOperation GetType() { return type; }
private:
	PGFileOperation type;	
};

class FileOperationRename : public FileOperationDelta {
public:
	std::string old_name;
	std::string new_name;

	FileOperationRename(std::string old_name, std::string new_name) :
		FileOperationDelta(PGFileOperationRename), old_name(old_name), new_name(new_name) { }
};

class ProjectExplorer : public PGContainer {
public:
	ProjectExplorer(PGWindowHandle window);
	~ProjectExplorer();

	void Update(void);

	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);

	void LosesFocus(void);

	void LoadWorkspace(nlohmann::json& j);
	void WriteWorkspace(nlohmann::json& j);

	bool RevealFile(std::string file, bool search_only_expanded);
	
	PGCursorType GetCursor(PGPoint mouse) { return PGCursorStandard; }
	void SetScrollbarOffset(double scrollbar_offset);

	void Draw(PGRendererHandle renderer);

	void AddDirectory(std::string directory);
	void RemoveDirectory(lng index);
	void RemoveDirectory(PGDirectory* directory);

	PG_CONTROL_KEYBINDINGS;

	void Undo();
	void Redo();
	void CollapseAll();
	void ToggleShowAllFiles();
	void SetShowAllFiles(bool show_all_files);

	void IterateOverFiles(PGDirectoryIterCallback callback, void* data);

	virtual PGControlType GetControlType() { return PGControlTypeProjectExplorer; }
private:
	void UpdateDirectories(bool force);

	std::shared_ptr<Task> update_task;

	void Undo(FileOperationDelta*);
	void Redo(FileOperationDelta*);

	std::vector<std::unique_ptr<FileOperationDelta>> undos;
	std::vector<std::unique_ptr<FileOperationDelta>> redos;

	PGScalar file_render_height = 0;

	PGFontHandle font;

	std::string renaming_path;
	lng renaming_file = -1;
	SimpleTextField* textfield = nullptr;

	lng selected_entry = 0;
	bool show_all_files = false;

	void DeleteSelectedFiles();
	void ActuallyDeleteSelectedFiles();
	void RenameFile();
	void FinishRename(bool success, bool update_selection);
	void ActuallyPerformRename(std::string old_name, std::string new_name, bool update_selection);

	std::vector<std::shared_ptr<PGDirectory>> directories;

	bool dragging_scrollbar;
	double scrollbar_offset = 0;

	Scrollbar* scrollbar;

	Button *undo_button, *redo_button;
	ToggleButton* show_all_files_toggle;

	std::vector<lng> selected_files;

	std::unique_ptr<PGMutex> lock;
	
	enum PGSelectFileType {
		PGSelectSingleFile,
		PGSelectAddSingleFile,
		PGSelectSelectRangeFile,
		PGSelectAddRangeFile
	};

	void ScrollToFile(lng file_number);

	void FindFile(lng file_number, PGDirectory** directory, PGFile* file);
	lng FindFile(std::string full_name, PGDirectory** directory, PGFile* file);
	void SelectFile(lng selected_file, PGSelectFileType type, bool open_file, bool click);

	lng TotalFiles();
	lng MaximumScrollOffset();
	lng RenderedFiles();

	void DrawFile(PGRendererHandle renderer, PGBitmapHandle file_image, PGFile file, PGScalar x, PGScalar& y, bool selected, bool highlighted);
	void DrawDirectory(PGRendererHandle renderer, PGDirectory& directory, PGScalar x, PGScalar& y, PGScalar max_y, lng& current_offset, lng offset, lng& selection, lng highlighted_entry);
};