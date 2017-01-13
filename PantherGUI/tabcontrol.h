
#pragma once

#include "control.h"
#include "textfield.h"
#include "filemanager.h"

struct Tab {
	TextFile* file;
	PGScalar width;
	PGScalar x;
	PGScalar target_x;

	Tab(TextFile* file) : file(file), x(-1), target_x(-1) { }
};

class TabControl : public Control {
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
	void PrevTab();
	void NextTab();
	void CloseTab(int tab);
	void CloseTab(TextFile* textfile);
	void AddTab(TextFile* textfile);
	void NewTab();

	bool IsDragging() {
		return drag_tab;
	}
	int currently_selected_tab = 0;
protected:
	PGScalar MeasureTabWidth(Tab& tab);

	FileManager file_manager;

	PGFontHandle font;

	int GetSelectedTab(int x);

	std::vector<Tab> tabs;
	Tab dragging_tab;
	bool active_tab_hidden = false;

	TextField* textfield;

	void SwitchToFile(TextFile* file);

	PGScalar tab_offset;
	int active_tab;

	bool drag_tab = false;
	PGScalar drag_offset;
};
