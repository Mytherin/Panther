#pragma once

#include "basictextfield.h"
#include "control.h"
#include "cursor.h"
#include "textfile.h"
#include "time.h"

#include <map>

struct TextSelection {
	int line_start;
	int character_start;
	int line_end;
	int character_end;
};

#define FLICKER_CARET_INTERVAL 15
#define SCROLLBAR_BASE_OFFSET 16
#define SCROLLBAR_WIDTH 16
#define MAX_MINIMAP_LINE_CACHE 10000

class StatusBar;

class TextField : public BasicTextField {
public:
	TextField(PGWindowHandle, TextFile* file);
	~TextField();

	void PeriodicRender(void);
	void Draw(PGRendererHandle, PGIRect*);
	void MouseWheel(int x, int y, int distance, PGModifier modifier);
	void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);
	
	void InvalidateLine(lng line);
	void InvalidateBeforeLine(lng line);
	void InvalidateAfterLine(lng line);
	void InvalidateBetweenLines(lng start, lng end);
	void InvalidateScrollbar();
	void InvalidateHScrollbar();
	void InvalidateMinimap();

	void SetTextFile(TextFile* textfile);

	void OnResize(PGSize old_size, PGSize new_size);

	PGCursorType GetCursor(PGPoint mouse);

	void MinimapMouseEvent(bool mouse_enter);
	void ScrollbarMouseEvent(bool mouse_enter);
	void HScrollbarMouseEvent(bool mouse_enter);
	
	PGScalar GetTextfieldWidth();
	PGScalar GetTextfieldHeight();
	PGScalar GetMaxXOffset() { return max_xoffset; }

	void SelectionChanged();
	void TextChanged();
	void TextChanged(std::vector<TextLine*> lines);

	void SetStatusBar(StatusBar* bar) { statusbar = bar; }

private:
	StatusBar* statusbar = nullptr;

	PGFontHandle minimap_font;

	PGIRect scrollbar_region;
	PGIRect hscrollbar_region;
	PGIRect minimap_region;
	PGIRect arrow_regions[4];

	PGScalar line_height;
	PGScalar minimap_line_height;

	bool display_linenumbers;

	enum PGDragRegion {
		PGDragRegionScrollbarArrowUp,
		PGDragRegionScrollbarArrowDown,
		PGDragRegionScrollbarArrowLeft,
		PGDragRegionScrollbarArrowRight,
		PGDragRegionAboveScrollbar,
		PGDragRegionBelowScrollbar,
	};
	PGDragRegion drag_region;
	time_t drag_start;

	bool display_scrollbar;
	PGScalar drag_offset;

	bool display_horizontal_scrollbar = false;
	PGScalar max_xoffset;
	PGScalar max_textsize;

	PGScalar GetScrollbarHeight();
	PGScalar GetScrollbarOffset();
	void SetScrollbarOffset(PGScalar offset);

	PGScalar GetHScrollbarHeight();
	PGScalar GetHScrollbarOffset();
	void SetHScrollbarOffset(PGScalar offset);

	bool display_minimap;
	PGScalar GetMinimapWidth();
	PGScalar GetMinimapHeight();
	PGScalar GetMinimapOffset();
	lng GetMinimapStartLine();
	void SetMinimapOffset(PGScalar offset);

	void DrawTextField(PGRendererHandle, PGFontHandle, PGIRect*, bool minimap, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay);

	bool SetScrollOffset(lng offset);

	std::map<TextLine*, PGBitmapHandle> minimap_line_cache;
};