
#pragma once

#include "control.h"
#include "textfield.h"
#include "filemanager.h"

struct Tab {
	lng id = 0;
	TextFile* file;
	PGScalar width;
	PGScalar x = -1;
	PGScalar target_x = -1;

	Tab(TextFile* file, lng id) : file(file), x(-1), target_x(-1), id(id) { }
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
public:
	TabControl(PGWindowHandle window, TextField* textfield, std::vector<TextFile*> files);

	void PeriodicRender(void);

	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);

	void MouseMove(int x, int y, PGMouseButton buttons);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);

	bool AcceptsDragDrop(PGDragDropType type);
	void DragDrop(PGDragDropType type, int x, int y, void* data);
	void ClearDragDrop(PGDragDropType type);
	void PerformDragDrop(PGDragDropType type, int x, int y, void* data);


	void Draw(PGRendererHandle, PGIRect*);
	void RenderTab(PGRendererHandle renderer, Tab& tab, PGScalar& position_x, PGScalar x, PGScalar y, bool selected_tab);

	void OpenFile(std::string path);
	void OpenFile(TextFile* textfile);
	void OpenFile(TextFile* textfile, lng index);
	void PrevTab();
	void NextTab();
	void CloseTab(int tab);
	void CloseTab(TextFile* textfile);
	bool CloseAllTabs();
	void AddTab(TextFile* textfile);
	void AddTab(TextFile* file, lng index);
	void NewTab();
	void SwitchToTab(TextFile* file);
	void ReopenLastFile();


	bool IsDragging() {
		return drag_tab;
	}

	int currently_selected_tab = 0;

	PG_CONTROL_KEYBINDINGS;
protected:
	void ReopenFile(PGClosedTab tab);

	void AddTab(TextFile* file, lng id, lng neighborid);

	bool CloseTabConfirmation(int tab);
	void ActuallyCloseTab(int tab);

	Tab OpenTab(TextFile* textfile);

	PGScalar MeasureTabWidth(Tab& tab);

	FileManager file_manager;

	PGFontHandle font;

	int GetSelectedTab(int x);

	std::vector<PGClosedTab> closed_tabs;
	std::vector<Tab> tabs;
	Tab dragging_tab;
	bool active_tab_hidden = false;

	TextField* textfield;

	void SwitchToFile(TextFile* file);

	PGScalar tab_offset;
	int active_tab;

	PGScalar drag_offset;
	bool drag_tab;

	PGScalar tab_padding;
	PGScalar file_icon_height;
	PGScalar file_icon_width;

	lng current_id = 0;
};
