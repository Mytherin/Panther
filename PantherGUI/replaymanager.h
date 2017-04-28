#pragma once

#include "controlmanager.h"
#include "mmap.h"

class ReplayManager;

typedef byte PGReplayEvent;
typedef byte PGManagerID;

extern PGReplayEvent PGReplayEventKeyboardButton;
extern PGReplayEvent PGReplayEventKeyboardCharacter;
extern PGReplayEvent PGReplayEventKeyboardUnicode;
extern PGReplayEvent PGReplayEventUpdate;
extern PGReplayEvent PGReplayEventDraw;
extern PGReplayEvent PGReplayEventMouseWheel;
extern PGReplayEvent PGReplayEventMouseDown;
extern PGReplayEvent PGReplayEventMouseUp;
extern PGReplayEvent PGReplayEventMouseMove;
extern PGReplayEvent PGReplayEventLosesFocus;
extern PGReplayEvent PGReplayEventGainsFocus;
extern PGReplayEvent PGReplayEventAcceptsDragDrop;
extern PGReplayEvent PGReplayEventDragDrop;
extern PGReplayEvent PGReplayEventPerformDragDrop;
extern PGReplayEvent PGReplayEventClearDragDrop;
extern PGReplayEvent PGReplayEventSetSize;
extern PGReplayEvent PGReplayEventCloseControlManager;
extern PGReplayEvent PGReplayEventRefreshWindow;
extern PGReplayEvent PGReplayEventRefreshWindowRectangle;
extern PGReplayEvent PGReplayEventDropFile;
extern PGReplayEvent PGReplayEventGetClipboardText;
extern PGReplayEvent PGReplayEventGetTime;
extern PGReplayEvent PGReplayEventGetReadFile;

enum PGReplayAction {
	PGReplayRecord,
	PGReplayPlay
};

class PGGlobalReplayManager {
public:
	static bool recording_replay;
	static bool running_replay;

	static PGGlobalReplayManager* GetInstance(std::string path = "", bool record = true) {
		static PGGlobalReplayManager settingsmanager = PGGlobalReplayManager(path, record);
		return &settingsmanager;
	}
	
	static int RegisterManager(ControlManager*);

	static void RecordGetClipboardText(std::string text);
	static void RecordGetTime(PGTime time);
	static void RecordReadFile(std::string fname, void* result, lng result_size, PGFileError error);

	static PGTime GetTime() { return GetInstance()->_GetTime(); }
	static std::string GetClipboardText() { return GetInstance()->_GetClipboardText(); }

	static void* ReadFile(std::string filename, lng& result_size, PGFileError& error);

	static void RunReplay() { GetInstance()->_RunReplay(); }

	static void Initialize(std::string path, PGReplayAction action) { (void)GetInstance(path, action == PGReplayRecord); }
	
	static void WriteEvent(PGReplayEvent event) { return GetInstance()->_WriteByte(event); }
	static void WriteEvent(PGManagerID manager_id, PGReplayEvent event) { return GetInstance()->_WriteEvent(manager_id, event); }
	static void WriteByte(char value) { return GetInstance()->_WriteByte(value); }
	static void WriteLong(lng value) { return GetInstance()->_WriteLong(value); }

	static void WriteInt(int value) { return GetInstance()->_WriteInt(value); }
	static void WriteDouble(double value) { return GetInstance()->_WriteDouble(value); }
	static void WriteString(std::string value) { return GetInstance()->_WriteString(value); }
private:
	PGGlobalReplayManager(std::string path, bool record);

	static PGTime _GetTime();
	static std::string _GetClipboardText();

	void _WriteEvent(PGManagerID manager_id, PGReplayEvent event);
	void _WriteByte(char value);
	void _WriteLong(lng value);
	void _WriteInt(int value);
	void _WriteDouble(double value);
	void _WriteString(std::string value);

	struct FileData {
		std::string data;
		PGFileError error;
	};
	std::map<std::string, FileData> stored_files;
	lng current_time;
	std::string current_clipboard;

	std::vector<ControlManager*> managers;
	std::string path;
	PGFileHandle file;

	void ReadStoredFiles();
	void _RunReplay();
	std::string ReadString(char* data, size_t& position, size_t size);
	void PeekEvent(PGReplayEvent event, char* data, size_t position, size_t size);
	bool NextEvent(PGReplayEvent event, char* data, size_t& position, size_t size);
	bool ExecuteCommand(char* data, size_t& position, size_t size);

	char* data;
	lng size;
};

class ReplayManager : public ControlManager {
public:
	ReplayManager(PGWindowHandle window);

	void MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier);
	bool KeyboardButton(PGButton button, PGModifier modifier);
	bool KeyboardCharacter(char character, PGModifier modifier);
	bool KeyboardUnicode(PGUTF8Character character, PGModifier modifier);
	void Update(void);
	void Draw(PGRendererHandle);

	void MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count);
	void MouseUp(int x, int y, PGMouseButton button, PGModifier modifier);
	void MouseMove(int x, int y, PGMouseButton buttons);
	
	void LosesFocus(void);
	void GainsFocus(void);

	bool AcceptsDragDrop(PGDragDropType type);
	void DragDrop(PGDragDropType type, int x, int y, void* data);
	void PerformDragDrop(PGDragDropType type, int x, int y, void* data);
	void ClearDragDrop(PGDragDropType type);

	void SetSize(PGSize size);

	bool CloseControlManager();

	void RefreshWindow(bool redraw_now = false);
	void RefreshWindow(PGIRect rectangle, bool redraw_now = false);

	void DropFile(std::string filename);

	PGManagerID manager_id;
};