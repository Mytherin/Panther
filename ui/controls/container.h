#pragma once

// A composite control that contains both a TextField and a TabControl
// This control exists because both the TabControl and TextField can be controlled at
// the same time using keys (e.g. Ctrl+Tab to move tabs while typing in a textfield)

#include "control.h"
#include "windowfunctions.h"

#define TEXT_TAB_HEIGHT 30

class PGContainer : public Control {
public:
	PGContainer(PGWindowHandle window);
	virtual ~PGContainer();

	virtual void Initialize();

	virtual void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);
	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual bool KeyboardCharacter(char character, PGModifier modifier);
	virtual bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	virtual void Update(void);
	virtual void Draw(PGRendererHandle);

	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);

	void SetFocus();

	virtual void LosesFocus(void);
	virtual void GainsFocus(void);

	virtual bool AcceptsDragDrop(PGDragDropType type);
	virtual void DragDrop(PGDragDropType type, int x, int y, void* data);
	virtual void PerformDragDrop(PGDragDropType type, int x, int y, void* data);
	virtual void ClearDragDrop(PGDragDropType type);

	virtual void OnResize(PGSize old_size, PGSize new_size);
	virtual void TriggerResize() { pending_resize = true; }

	virtual void LoadWorkspace(nlohmann::json& j);
	virtual void WriteWorkspace(nlohmann::json& j);

	virtual bool ControlTakesFocus() { return true; }

	Control* GetMouseOverControl(int x, int y);
	Control* GetActiveControl() { return focused_control; }

	PGCursorType GetCursor(PGPoint mouse);
	PGCursorType GetDraggingCursor();
	virtual bool IsDragging();

	void AddControl(std::shared_ptr<Control> control);
	void RemoveControl(std::shared_ptr<Control>& control);
	void RemoveControl(Control* control);

	void Invalidate(bool initial_invalidate = true);

	virtual PGControlType GetControlType() { return PGControlTypeContainer; }
protected:
	bool everything_dirty = false;

	Control* focused_control = nullptr;

	virtual void SetFocusedControl(Control* c);
	void FlushRemoves();

	std::vector<std::shared_ptr<Control>> controls;

private:
	bool pending_resize;
	bool pending_removal;
	std::vector<std::shared_ptr<Control>> pending_additions;

	void ActuallyAddControl(std::shared_ptr<Control> control);
	void ActuallyRemoveControl(size_t index);
};