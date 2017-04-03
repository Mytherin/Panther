#pragma once

#pragma once

#include "button.h"
#include "container.h"
#include "directory.h"
#include "simpletextfield.h"
#include "togglebutton.h"
#include "scrollbar.h"

class ProjectExplorer : public PGContainer {
public:
	ProjectExplorer(PGWindowHandle window);
	~ProjectExplorer();

	void PeriodicRender(void);

	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);

	void LosesFocus(void);

	void OnResize(PGSize old_size, PGSize new_size);

	PGCursorType GetCursor(PGPoint mouse) { return PGCursorStandard; }

	void Draw(PGRendererHandle renderer);

	std::vector<PGFile> GetFiles();
	std::vector<PGDirectory*>& GetDirectories() { return directories; }

	PG_CONTROL_KEYBINDINGS;

	virtual PGControlType GetControlType() { return PGControlTypeProjectExplorer; }
private:
	PGScalar file_render_height;

	PGFontHandle font;

	std::string renaming_path;
	lng renaming_file = -1;
	SimpleTextField* textfield = nullptr;

	void RenameFile();
	void FinishRename(bool success, bool update_selection);

	std::vector<PGDirectory*> directories;

	bool dragging_scrollbar;
	double scrollbar_offset;

	Scrollbar* scrollbar;

	std::vector<lng> selected_files;
	
	enum PGSelectFileType {
		PGSelectSingleFile,
		PGSelectAddSingleFile,
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