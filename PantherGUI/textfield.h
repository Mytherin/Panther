#pragma once

#include "control.h"
#include "cursor.h"
#include "textfile.h"
#include "time.h"

struct TextSelection {
	int line_start;
	int character_start;
	int line_end;
	int character_end;
};

class TextField : public Control {
	friend class Cursor;
public:
	TextField(PGWindowHandle, std::string filename);

	void PeriodicRender(void);
	void Draw(PGRendererHandle, PGRect*);
	void MouseWheel(int x, int y, int distance, PGModifier modifier);
	void MouseClick(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	void KeyboardButton(PGButton button, PGModifier modifier);
	void KeyboardCharacter(char character, PGModifier modifier);
	void KeyboardUnicode(char *character, PGModifier modifier);
	
	void InvalidateLine(int line);
	void InvalidateBeforeLine(int line);
	void InvalidateAfterLine(int line);
	void InvalidateBetweenLines(int start, int end);

	int GetLineOffset() { return lineoffset_y; }
	void SetLineOffset(ssize_t offset) { lineoffset_y = offset; }
	int GetLineHeight();
	void DisplayCarets();

	void ClearCursors(std::vector<Cursor*>&);

	Cursor*& GetActiveCursor() { return active_cursor; }
	TextFile& GetTextFile() { return textfile; }
	std::vector<Cursor*>& GetCursors() { return cursors; }
private:
	ssize_t text_offset;
	int offset_x;
	ssize_t lineoffset_y;
	std::vector<Cursor*> cursors;
	std::vector<std::vector<short>> offsets;
	int line_height;
	bool drag_selection;
	bool drag_selection_cursors;
	bool display_carets;
	int display_carets_count;

	Cursor* active_cursor;
	TextFile textfile;

	MouseClickInstance last_click;

	void ClearExtraCursors();
	void GetLineCharacterFromPosition(int x, int y, ssize_t& line, ssize_t& character);
	bool SetScrollOffset(ssize_t offset);
};