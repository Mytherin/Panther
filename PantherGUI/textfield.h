#pragma once

#include "basictextfield.h"
#include "control.h"
#include "cursor.h"
#include "textfile.h"
#include "time.h"

#include "scrollbar.h"

#include <map>

struct TextSelection {
	int line_start;
	int character_start;
	int line_end;
	int character_end;
};

#define FLICKER_CARET_INTERVAL 15
#define MAX_MINIMAP_LINE_CACHE 10000

class TextField : public BasicTextField {
public:
	TextField(PGWindowHandle, TextFile* file);
	~TextField();

	void Draw(PGRendererHandle, PGIRect*);
	void MouseWheel(int x, int y, double distance, PGModifier modifier);
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
	void InvalidateMinimap();

	bool IsDragging();

	void SetTextFile(TextFile* textfile);

	void OnResize(PGSize old_size, PGSize new_size);

	PGCursorType GetCursor(PGPoint mouse);

	void MinimapMouseEvent(bool mouse_enter);
	
	PGScalar GetTextfieldWidth();
	PGScalar GetTextfieldHeight();
	PGScalar GetMaxXOffset() { return max_xoffset; }

	void TextChanged();
	void TextChanged(std::vector<lng> lines);
private:
	Scrollbar* scrollbar;
	Scrollbar* horizontal_scrollbar;

	PGFontHandle minimap_font;

	PGIRect minimap_region;
	PGRect textfield_region;

	PGScalar line_height;
	PGScalar minimap_line_height;

	bool display_linenumbers;

	bool display_scrollbar;
	bool display_horizontal_scrollbar = false;
	PGScalar drag_offset;
	std::map<Cursor*, CursorSelection> minimal_selections;

	void ClearDragging();

	PGScalar max_xoffset;

	bool mouse_in_minimap = false;
	bool display_minimap;
	PGScalar GetMinimapWidth();
	PGScalar GetMinimapHeight();
	PGScalar GetMinimapOffset();
	lng GetMinimapStartLine();
	void SetMinimapOffset(PGScalar offset);

	void DrawTextField(PGRendererHandle, PGFontHandle, PGIRect*, bool minimap, PGScalar position_x_text, PGScalar position_y, PGScalar width, bool render_overlay);

	std::map<lng, PGBitmapHandle> minimap_line_cache;
};