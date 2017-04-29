#pragma once

#include "container.h"
#include "control.h"
#include "projectexplorer.h"
#include "splitter.h"
#include "tabcontrol.h"
#include "textfield.h"
#include "textfieldcontainer.h"

class TextField;
class StatusBar;
class PGFindText;
class PGToolbar;

class ControlManager : public PGContainer {
public:
	ControlManager(PGWindowHandle window);
	~ControlManager() { is_destroyed = true; }

	virtual void Update(void);

	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual bool KeyboardCharacter(char character, PGModifier modifier);
	virtual bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	virtual void Draw(PGRendererHandle);

	virtual void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);
	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);

	virtual void RefreshWindow(bool redraw_now = false);
	virtual void RefreshWindow(PGIRect rectangle, bool redraw_now = false);

	Control* GetActiveControl() { return focused_control; }
	void RegisterControlForMouseEvents(Control* control);
	void UnregisterControlForMouseEvents(Control* control);

	void RegisterMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data = nullptr);
	void UnregisterMouseRegion(PGIRect* rect);

	void RegisterMouseRegionContainer(MouseRegionSet* regions, Control* control);
	void UnregisterMouseRegionContainer(MouseRegionSet* regions);

	void RegisterGenericMouseRegion(PGMouseRegion* region);

	void LoadWorkspace(nlohmann::json& j);
	void WriteWorkspace(nlohmann::json& j);

	void ShowProjectExplorer(bool visible);

	virtual void DropFile(std::string filename);

	TextField* active_textfield;
	ProjectExplorer* active_projectexplorer;
	PGFindText* active_findtext;
	PGToolbar* toolbar;
	StatusBar* statusbar;
	Control* splitter;

	PGScalar projectexplorer_width = 200;

	virtual bool CloseControlManager();
	bool IsDragging() { return is_dragging; }

	void ShowFindReplace(PGFindTextType type);
	void CreateNewWindow();

	virtual void LosesFocus(void);
	virtual void GainsFocus(void);

	bool ControlHasFocus() { return is_focused; }

	void SetTextFieldLayout(int columns, int rows);
	void SetTextFieldLayout(int columns, int rows, std::vector<std::shared_ptr<TextFile>> initial_files);

	void OnSelectionChanged(PGControlDataCallback callback, void* data);
	void UnregisterOnSelectionChanged(PGControlDataCallback callback, void* data);

	void OnTextChanged(PGControlDataCallback callback, void* data);
	void UnregisterOnTextChanged(PGControlDataCallback callback, void* data);

	void OnActiveTextFieldChanged(PGControlDataCallback callback, void* data);
	void UnregisterOnActiveTextFieldChanged(PGControlDataCallback callback, void* data);

	void OnActiveFileChanged(PGControlDataCallback callback, void* data);
	void UnregisterOnActiveFileChanged(PGControlDataCallback callback, void* data);

	void ActiveFileChanged(Control *control);
	void TextChanged(Control *control);
	void SelectionChanged(Control *control);
	void ActiveTextFieldChanged(Control *control);
	
	void InvalidateWorkspace();

	PG_CONTROL_KEYBINDINGS;
protected:
	virtual void SetFocusedControl(Control* c);
private:
	int rows = 0, columns = 0;
	std::vector<TextFieldContainer*> textfields;
	std::vector<Splitter*> splitters;

	PGIRect invalidated_area;
	bool invalidated;
	bool is_destroyed = false;
	bool is_dragging = false;
	bool is_focused = true;

	void EnterManager();
	void LeaveManager();
#ifdef PANTHER_DEBUG
	int entrance_count = 0;
#endif

	int write_workspace_counter = 0;

	MouseClickInstance last_click;

	std::vector<std::unique_ptr<PGMouseRegion>> regions;

	std::vector<std::pair<PGControlDataCallback, void*>> selection_changed_callbacks;
	std::vector<std::pair<PGControlDataCallback, void*>> text_changed_callbacks;
	std::vector<std::pair<PGControlDataCallback, void*>> active_textfield_callbacks;
	std::vector<std::pair<PGControlDataCallback, void*>> active_file_callbacks;
};

ControlManager* GetControlManager(Control* c);
