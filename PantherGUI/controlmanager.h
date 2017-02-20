#pragma once

#include "container.h"
#include "control.h"
#include "projectexplorer.h"
#include "tabcontrol.h"
#include "textfield.h"

typedef void(*PGMouseCallback)(Control* control, bool, void*);

struct PGMouseRegion {
	bool mouse_inside = false;
	PGIRect* rect = nullptr;
	void* data = nullptr;
	Control* control = nullptr;
	PGMouseCallback mouse_event;

	PGMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data = nullptr) : rect(rect), control(control), mouse_event(mouse_event), data(data) { }
};

class TextField;
class StatusBar;
class PGFindText;

class ControlManager : public PGContainer {
public:
	ControlManager(PGWindowHandle window);
	~ControlManager() { is_destroyed = true; }

	void PeriodicRender(void);

	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);
	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	void Draw(PGRendererHandle, PGIRect*);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);

	void RefreshWindow(bool redraw_now = false);
	void RefreshWindow(PGIRect rectangle, bool redraw_now = false);

	bool KeyboardCharacter(char character, PGModifier modifier);

	Control* GetActiveControl() { return focused_control; }
	void RegisterControlForMouseEvents(Control* control);
	void UnregisterControlForMouseEvents(Control* control);

	void RegisterMouseRegion(PGIRect* rect, Control* control, PGMouseCallback mouse_event, void* data = nullptr);
	void UnregisterMouseRegion(PGIRect* rect);

	void DropFile(std::string filename);

	TextField* active_textfield;
	ProjectExplorer* active_projectexplorer;
	PGFindText* active_findtext;
	StatusBar* statusbar;

	bool CloseControlManager();

	void ShowFindReplace(PGFindTextType type);
	void CreateNewWindow();

	PG_CONTROL_KEYBINDINGS;
private:
	PGIRect invalidated_area;
	bool invalidated;
	bool is_destroyed = false;

	void EnterManager();
	void LeaveManager();
#ifdef PANTHER_DEBUG
	int entrance_count = 0;
#endif

	MouseClickInstance last_click;

	std::vector<PGMouseRegion> regions;
};

ControlManager* GetControlManager(Control* c);
