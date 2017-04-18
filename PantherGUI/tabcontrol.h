
#pragma once

#include "control.h"
#include "textfield.h"
#include "filemanager.h"

class TabControl;

struct Tab {
	lng id = 0;
	std::shared_ptr<TextFile> file;
	PGScalar width;
	PGScalar x = -1;
	PGScalar target_x = -1;
	bool hover = false;
	bool button_hover = false;

	Tab(std::shared_ptr<TextFile> file, lng id) : file(file), x(-1), target_x(-1), id(id), hover(false), button_hover(false) { }
};

struct PGTabMouseRegion : public PGMouseRegion {
	TabControl* tabs = nullptr;
	bool currently_contains = false;

	void MouseMove(PGPoint mouse);

	PGTabMouseRegion(TabControl* control) : PGMouseRegion((Control*)control), tabs(control), currently_contains(false) {}
};


struct PGClosedTab {
	lng id;
	lng neighborid;
	std::string filepath;
	PGTextFileSettings settings;

	PGClosedTab(Tab tab, lng neighborid, PGTextFileSettings settings) {
		id = tab.id;
		this->neighborid = neighborid;
		this->settings = settings;
		filepath = tab.file->GetFullPath();
	}
};

struct TabDragDropStruct {
	TabDragDropStruct(std::shared_ptr<TextFile> file, TabControl* tabs, PGScalar drag_offset) : file(file), tabs(tabs), accepted(nullptr), drag_offset(drag_offset) {}
	std::shared_ptr<TextFile> file;
	TabControl* tabs;
	TabControl* accepted = nullptr;
	PGScalar drag_offset;
};

class TabControl : public Control {
	friend class TextField; //FIXME
	friend class PGGotoAnything;
	friend class PGTabMouseRegion;
public:
	TabControl(PGWindowHandle window, TextField* textfield, std::vector<std::shared_ptr<TextFile>> files);
	~TabControl();

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

	void Draw(PGRendererHandle);
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
	void OpenTemporaryFileAsActualFile();
	void CloseTemporaryFile();

	TextField* GetTextField() { return textfield; }

	bool IsDragging() {
		return drag_tab || dragging_tab.file != nullptr;
	}

	int currently_selected_tab = 0;

	PG_CONTROL_KEYBINDINGS;

	virtual PGControlType GetControlType() { return PGControlTypeTabControl; }
protected:
	static void ClearEmptyFlag(Control* c, void* data);

	void ReopenFile(PGClosedTab tab);

	void AddTab(std::shared_ptr<TextFile> file, lng id, lng neighborid);

	bool CloseTabConfirmation(int tab, bool respect_hot_exit = true);
	void ActuallyCloseTab(int tab);
	void ActuallyCloseTab(std::shared_ptr<TextFile> textfile);

	Tab OpenTab(std::shared_ptr<TextFile> textfile);

	PGScalar MeasureTabWidth(Tab& tab);
	
	PGFontHandle font;

	int GetSelectedTab(PGScalar x);

	std::vector<PGClosedTab> closed_tabs;
	std::vector<Tab> tabs;
	Tab dragging_tab;
	bool active_tab_hidden = false;

	lng current_selection = -1;

	PGScalar rendered_scroll = 0;
	PGScalar scroll_position = 0;
	PGScalar target_scroll = 0;
	PGScalar max_scroll = 0;

	void SetScrollPosition(PGScalar position);

	TextField* textfield;

	void SwitchToFile(std::shared_ptr<TextFile> file);

	PGScalar tab_offset;
	int active_tab;
	
	PGScalar GetTabPosition(int tabnr);
	void SetActiveTab(int active_tab);

	TabDragDropStruct* drag_data = nullptr;
	PGScalar drag_offset;
	bool drag_tab;

	PGScalar tab_padding = 0;
	PGScalar file_icon_height = 0;
	PGScalar file_icon_width = 0;

	lng current_id = 0;
	
	std::shared_ptr<TextFile> temporary_textfile = nullptr;
	PGScalar temporary_tab_width = 0;

	bool is_empty = false;
};
