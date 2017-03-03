
#pragma once

#include "control.h"
#include "textfield.h"
#include "filemanager.h"

struct Tab {
	lng id = 0;
	std::shared_ptr<TextFile> file;
	PGScalar width;
	PGScalar x = -1;
	PGScalar target_x = -1;
	bool hover = false;

	Tab(std::shared_ptr<TextFile> file, lng id) : file(file), x(-1), target_x(-1), id(id), hover(false) { }
};

struct PGClosedTab {
	lng id;
	lng neighborid;
	std::string filepath;

	PGClosedTab(Tab tab, lng neighborid) {
		id = tab.id;
		this->neighborid = neighborid;
		filepath = tab.file->GetFullPath();
	}
};

class TabControl : public Control {
	friend class TextField; //FIXME
	friend class PGGotoAnything;
public:
	TabControl(PGWindowHandle window, TextField* textfield, std::vector<std::shared_ptr<TextFile>> files);

	void PeriodicRender(void);

	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);

	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);

	bool AcceptsDragDrop(PGDragDropType type);
	void DragDrop(PGDragDropType type, int x, int y, void* data);
	void ClearDragDrop(PGDragDropType type);
	void PerformDragDrop(PGDragDropType type, int x, int y, void* data);

	void LoadWorkspace(nlohmann::json& j);
	void WriteWorkspace(nlohmann::json& j);

	enum PGTabType {
		PGTabTypeNone,
		PGTabTypeSelected,
		PGTabTypeTemporary
	};

	void Draw(PGRendererHandle, PGIRect*);
	void RenderTab(PGRendererHandle renderer, Tab& tab, PGScalar& position_x, PGScalar x, PGScalar y, PGTabType type);

	void OpenFile(std::string path);
	void OpenFile(std::shared_ptr<TextFile> textfile);
	void OpenFile(std::shared_ptr<TextFile> textfile, lng index);
	void PrevTab();
	void NextTab();
	void CloseTab(int tab);
	void CloseTab(std::shared_ptr<TextFile> textfile);
	bool CloseAllTabs();
	bool CloseAllTabs(PGDirection direction);

	void AddTab(std::shared_ptr<TextFile> textfile);
	void AddTab(std::shared_ptr<TextFile> file, lng index);
	void NewTab();
	void SwitchToTab(std::shared_ptr<TextFile> file);
	bool SwitchToTab(std::string path);
	void ReopenLastFile();

	void OpenTemporaryFile(std::shared_ptr<TextFile> textfile);
	void CloseTemporaryFile();

	TextField* GetTextField() { return textfield; }

	bool IsDragging() {
		return drag_tab || dragging_tab.file != nullptr;
	}

	int currently_selected_tab = 0;

	PG_CONTROL_KEYBINDINGS;
protected:
	void ReopenFile(PGClosedTab tab);

	void AddTab(std::shared_ptr<TextFile> file, lng id, lng neighborid);

	bool CloseTabConfirmation(int tab, bool respect_hot_exit = true);
	void ActuallyCloseTab(int tab);
	void ActuallyCloseTab(std::shared_ptr<TextFile> textfile);

	Tab OpenTab(std::shared_ptr<TextFile> textfile);

	PGScalar MeasureTabWidth(Tab& tab);

	FileManager file_manager;

	PGFontHandle font;

	int GetSelectedTab(int x);

	std::vector<PGClosedTab> closed_tabs;
	std::vector<Tab> tabs;
	Tab dragging_tab;
	bool active_tab_hidden = false;

	lng current_selection = -1;

	PGScalar scroll_position = 0;
	PGScalar max_scroll = 0;

	TextField* textfield;

	void SwitchToFile(std::shared_ptr<TextFile> file);

	PGScalar tab_offset;
	int active_tab;
	
	PGScalar GetTabPosition(int tabnr);
	void SetActiveTab(int active_tab);

	PGScalar drag_offset;
	bool drag_tab;

	PGScalar tab_padding = 0;
	PGScalar file_icon_height = 0;
	PGScalar file_icon_width = 0;

	lng current_id = 0;

	std::shared_ptr<TextFile> temporary_textfile = nullptr;
};
