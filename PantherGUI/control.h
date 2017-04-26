#pragma once

#include "keybindings.h"
#include "utils.h"
#include "windowfunctions.h"

typedef int PGAnchor;

extern const PGAnchor PGAnchorNone;
extern const PGAnchor PGAnchorLeft;
extern const PGAnchor PGAnchorRight;
extern const PGAnchor PGAnchorTop;
extern const PGAnchor PGAnchorBottom;

enum PGControlType {
	PGControlTypeBasicTextField,
	PGControlTypeButton,
	PGControlTypeContainer,
	PGControlTypeControlManager,
	PGControlTypeFindText,
	PGControlTypeGotoAnything,
	PGControlTypeProjectExplorer,
	PGControlTypeScrollbar,
	PGControlTypeSearchBox,
	PGControlTypeSimpleTextField,
	PGControlTypeSplitter,
	PGControlTypeStatusBar,
	PGControlTypeStatusNotification,
	PGControlTypeTabControl,
	PGControlTypeTextField,
	PGControlTypeTextFieldContainer,
	PGControlTypeLabel
};

class Control {
public:
	Control(PGWindowHandle window);
	virtual ~Control();

	virtual void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);
	// Returns true if the button has been consumed by the control, false if it has been ignored
	virtual bool KeyboardButton(PGButton button, PGModifier modifier);
	virtual bool KeyboardCharacter(char character, PGModifier modifier);
	virtual bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	virtual void Update(void);
	virtual void Draw(PGRendererHandle);

	virtual bool AcceptsDragDrop(PGDragDropType type);
	virtual void DragDrop(PGDragDropType type, int x, int y, void* data);
	virtual void PerformDragDrop(PGDragDropType type, int x, int y, void* data);
	virtual void ClearDragDrop(PGDragDropType type);

	virtual void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	virtual void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	virtual void MouseMove(int x, int y, PGMouseButton buttons);

	
	// In order for a control to get MouseEnter(), MouseLeave() and ShowTooltip() events
	// it must register with the ControlManager
	virtual void MouseEnter();
	virtual void MouseLeave();

	virtual void LosesFocus(void);
	virtual void GainsFocus(void);

	virtual void Invalidate(bool initial_invalidate = true);

	virtual void LoadWorkspace(nlohmann::json& j);
	virtual void WriteWorkspace(nlohmann::json& j);

	virtual Control* GetActiveControl() { return nullptr; }
	virtual bool ControlTakesFocus() { return false; }
	virtual bool ControlHasFocus() { return !parent ? true : (parent->ControlHasFocus() && parent->GetActiveControl() == this); }

	void SetSize(PGSize size);
	void SetPosition(PGPoint point) { this->x = point.x; this->y = point.y; }
	void SetAnchor(PGAnchor anchor) { this->anchor = anchor; }
	PGSize GetSize() { return PGSize(this->width, this->height); }
	PGWindowHandle GetWindow() { return window; }

	PGRect GetRectangle() { 
		return PGRect(this->x, this->y, width, height); 
	}

	virtual void OnResize(PGSize old_size, PGSize new_size);
	virtual void TriggerResize();

	virtual void ResolveSize(PGSize new_size);


	virtual bool IsDragging();

	virtual PGCursorType GetCursor(PGPoint mouse) { return PGCursorStandard; }
	virtual PGCursorType GetDraggingCursor() { return PGCursorStandard; }

	virtual PGControlType GetControlType() = 0;
	
	void OnDestroy(PGControlDataCallback callback, void* data) { destroy_data.function = callback; destroy_data.data = data; }
//protected:
	bool dirty;
	bool visible;

	PGScalar X();
	PGScalar Y();
	PGPoint Position();

	Control* parent = nullptr;

	bool size_resolved = false;
	PGScalar x = 0, y = 0;
	PGScalar width, height;
	PGScalar minimum_width = 0, minimum_height = 0;
	PGScalar maximum_width = FLT_MAX, maximum_height = FLT_MAX;
	PGScalar fixed_width = -1, fixed_height = -1;
	PGScalar percentage_width = -1, percentage_height = -1;
	PGAnchor anchor;
	Control* bottom_anchor = nullptr;
	Control* top_anchor = nullptr;
	Control* left_anchor = nullptr;
	Control* right_anchor = nullptr;

	PGPadding margin;
	PGPadding padding;

	PGWindowHandle window;
	bool HasFocus();

	void SetTooltip(std::string tooltip);
protected:
	bool PressKey(std::map<PGKeyPress, PGKeyFunctionCall>& keybindings, PGButton button, PGModifier modifier);
	bool PressCharacter(std::map<PGKeyPress, PGKeyFunctionCall>& keybindings, char character, PGModifier modifier);
	bool PressMouseButton(std::map<PGMousePress, PGMouseFunctionCall>& mousebindings, PGMouseButton button, PGPoint mouse, PGModifier modifier, int clicks, lng line, lng character);
private:
	PGTooltipHandle tooltip = nullptr;

	PGFunctionData destroy_data;
};
