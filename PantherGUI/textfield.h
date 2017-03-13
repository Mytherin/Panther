#pragma once

#include "basictextfield.h"
#include "control.h"
#include "cursor.h"
#include "textfile.h"
#include "time.h"

#include "notification.h"
#include "scrollbar.h"

#include <map>

class TabControl;

struct TextSelection {
	int line_start;
	int character_start;
	int line_end;
	int character_end;
};

enum PGGotoType {
	PGGotoNone,
	PGGotoCommand,
	PGGotoLine,
	PGGotoFile,
	PGGotoDefinition
};

#define MAX_MINIMAP_LINE_CACHE 10000

struct RenderedLine {
	TextLine tline;
	lng line;
	lng position;
	RenderedLine(TextLine tline, lng line, lng position) : tline(tline), line(line), position(position) {
	}
};

class TextField : public BasicTextField {
public:
	TextField(PGWindowHandle, std::shared_ptr<TextFile> file);
	~TextField();

	void PeriodicRender(void);
	void Draw(PGRendererHandle, PGIRect*);
	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);
	
	void InvalidateLine(lng line);
	void InvalidateBeforeLine(lng line);
	void InvalidateAfterLine(lng line);
	void InvalidateBetweenLines(lng start, lng end);
	void InvalidateMinimap();

	void SelectionChanged();

	bool IsDragging();

	void SetTextFile(std::shared_ptr<TextFile> textfile);

	void SetTabControl(TabControl* tabs) { tabcontrol = tabs; }
	TabControl* GetTabControl() { return tabcontrol; }

	void OnResize(PGSize old_size, PGSize new_size);

	PGCursorType GetCursor(PGPoint mouse);

	void MinimapMouseEvent(bool mouse_enter);
	
	PGScalar GetTextfieldWidth();
	PGScalar GetTextfieldHeight();
	PGScalar GetMaxXOffset() { return max_xoffset; }

	void TextChanged();

	void IncreaseFontSize(int modifier);

	void DisplayNotification(PGFileError error);
	void DisplayGotoDialog(PGGotoType goto_type);
	void DisplaySearchBox(std::vector<SearchEntry>& entries);

	void ClearSearchBox(Control* searchbox);

	PG_CONTROL_KEYBINDINGS;
	virtual PGControlType GetControlType() { return PGControlTypeTextField; }
protected:
	void GetLineCharacterFromPosition(PGScalar x, PGScalar y, lng& line, lng& character);
	void GetLineFromPosition(PGScalar y, lng& line);

	void GetPositionFromLineCharacter(lng line, lng character, PGScalar& x, PGScalar& y);
	void GetPositionFromLine(lng line, PGScalar& y);
private:
	TabControl* tabcontrol;

	double vscroll_left = 0;
	double vscroll_speed = 0;
	double hscroll_left = 0;

	std::unique_ptr<Scrollbar> scrollbar;
	std::unique_ptr<Scrollbar> horizontal_scrollbar;

	PGFontHandle minimap_font;

	PGIRect minimap_region;
	PGRect textfield_region;

	PGScalar line_height;
	PGScalar minimap_line_height;

	bool display_linenumbers;
	PGScalar margin_width;

	bool display_scrollbar;
	bool display_horizontal_scrollbar = false;
	PGScalar drag_offset;

	std::vector<RenderedLine> rendered_lines;

	PGScalar max_xoffset;

	bool mouse_in_minimap = false;
	bool display_minimap;
	PGScalar GetMinimapWidth();
	PGScalar GetMinimapHeight();
	PGScalar GetMinimapOffset();
	void GetMinimapLinesRendered(lng& lines_rendered, double& percentage);
	PGVerticalScroll GetMinimapStartLine();
	void SetMinimapOffset(PGScalar offset);

	void DrawTextField(PGRendererHandle, PGFontHandle, PGIRect*, bool minimap, PGScalar position_x, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay);

	void CreateNotification(PGNotificationType type, std::string text);
	void ShowNotification();

	void ClearNotification();

	PGNotification* notification = nullptr;
	Control* active_searchbox = nullptr;
};
