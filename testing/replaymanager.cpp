
#include "replaymanager.h"

PGReplayEvent PGReplayEventKeyboardButton = 1;
PGReplayEvent PGReplayEventKeyboardCharacter = 2;
PGReplayEvent PGReplayEventKeyboardUnicode = 3;
PGReplayEvent PGReplayEventUpdate = 4;
PGReplayEvent PGReplayEventDraw = 5;
PGReplayEvent PGReplayEventMouseWheel = 6;
PGReplayEvent PGReplayEventMouseDown = 7;
PGReplayEvent PGReplayEventMouseUp = 8;
PGReplayEvent PGReplayEventMouseMove = 9;
PGReplayEvent PGReplayEventLosesFocus = 10;
PGReplayEvent PGReplayEventGainsFocus = 11;
PGReplayEvent PGReplayEventAcceptsDragDrop = 12;
PGReplayEvent PGReplayEventDragDrop = 13;
PGReplayEvent PGReplayEventPerformDragDrop = 14;
PGReplayEvent PGReplayEventClearDragDrop = 15;
PGReplayEvent PGReplayEventSetSize = 16;
PGReplayEvent PGReplayEventCloseControlManager = 17;
PGReplayEvent PGReplayEventRefreshWindow = 18;
PGReplayEvent PGReplayEventRefreshWindowRectangle = 19;
PGReplayEvent PGReplayEventDropFile = 20;
PGReplayEvent PGReplayEventGetClipboardText = 21;
PGReplayEvent PGReplayEventGetTime = 22;
PGReplayEvent PGReplayEventGetReadFile = 23;
PGReplayEvent PGReplayEventGetDirectoryFiles = 24;

bool PGGlobalReplayManager::recording_replay = false;
bool PGGlobalReplayManager::running_replay = false;
PGMutexHandle PGGlobalReplayManager::lock = nullptr;

PGGlobalReplayManager::PGGlobalReplayManager(std::string path, bool record) :
	path(path) {
	assert(path.size() > 0);
	lock = CreateMutex();
	PGFileError error;
	if (record) {
		PGGlobalReplayManager::recording_replay = true;
		this->file = panther::OpenFile(path, PGFileReadWrite, error);
	} else {
		this->file = panther::OpenFile(path, PGFileReadOnly, error);
		PGFileError error;
		// read the replay into memory
		data = (char*)panther::ReadFile(file, size, error);
		if (!data || error != PGFileSuccess) {
			assert(0);
			return;
		}
		ReadStoredFiles();
		PGGlobalReplayManager::running_replay = true;
	}
	assert(file);
	assert(error == PGFileSuccess);
}

void PGGlobalReplayManager::_RunReplay() {
	assert(running_replay);
	PGInitialize();
	size_t position = 0;
	while (ExecuteCommand(data, position, size));
}

#define ReadEntry(result, load_type, target_type, data, position, size) {\
	if (position + sizeof(load_type) > size) goto cleanup; \
	load_type res = *((load_type*)(data + position)); \
	position += sizeof(load_type); \
	result = (target_type)res; \
}

void PGGlobalReplayManager::ReadFileEvent(char* data, size_t& position, size_t size) {
	PGGlobalReplayManager::Lock();
	auto fname = ReadString(data, position, size);
	PGFileError error;
	std::string text;
	ReadEntry(error, int, PGFileError, data, position, size);
	text = ReadString(data, position, size);
	stored_files[fname] = FileData();
	stored_files[fname].data = text;
	stored_files[fname].error = error;
	PGGlobalReplayManager::Unlock();
	return;
cleanup:
	assert(0);
	PGGlobalReplayManager::Unlock();
}

void PGGlobalReplayManager::ReadDirectoryEvent(char* data, size_t& position, size_t size) {
	auto dirname = ReadString(data, position, size);
	PGGlobalReplayManager::Lock();
	stored_directories[dirname] = DirectoryData();
	auto& dirdata = stored_directories[dirname];
	lng directory_count;
	ReadEntry(directory_count, lng, lng, data, position, size);
	for (lng i = 0; i < directory_count; i++) {
		dirdata.directories.push_back(ReadString(data, position, size));
	}
	lng file_count;
	ReadEntry(file_count, lng, lng, data, position, size);
	for (lng i = 0; i < file_count; i++) {
		dirdata.files.push_back(ReadString(data, position, size));
	}
	ReadEntry(dirdata.flags, int, PGDirectoryFlags, data, position, size);
	PGGlobalReplayManager::Unlock();
	return;
cleanup:
	assert(0);
	PGGlobalReplayManager::Unlock();
}

void PGGlobalReplayManager::ReadStoredFiles() {
	size_t position = 0;
	std::vector<PGReplayEvent> events;
	while (position < size) {
		PGReplayEvent event = data[position++];
		if (event == PGReplayEventGetReadFile) {
			ReadFileEvent(data, position, size);
		} else if (event == PGReplayEventGetDirectoryFiles) {
			ReadDirectoryEvent(data, position, size);
			continue;
cleanup:
			assert(0);
			return;
		} else if (!NextEvent(event, data, position, size)) {
			break;
		}
		events.insert(events.begin(), event);
	}
}

std::string PGGlobalReplayManager::ReadString(char* data, size_t& position, size_t size) {
	size_t string_size;
	ReadEntry(string_size, size_t, size_t, data, position, size);
	if (string_size < size && position + string_size < size) {
		std::string val = std::string(data + position, string_size);
		position += string_size;
		return val;
	}
cleanup:
	assert(0);
	return "";
}

bool PGGlobalReplayManager::NextEvent(PGReplayEvent event, char* data, size_t& position, size_t size) {
	if (event == PGReplayEventKeyboardButton) {
		position += sizeof(byte) + sizeof(int) + sizeof(PGManagerID);
	} else if (event == PGReplayEventKeyboardCharacter) {
		position += sizeof(byte) * 2 + sizeof(PGManagerID);
	} else if (event == PGReplayEventKeyboardUnicode) {
		position += sizeof(byte) * 6 + sizeof(PGManagerID);
	} else if (event == PGReplayEventUpdate) {
		position += sizeof(PGManagerID);
	} else if (event == PGReplayEventDraw) {
		position += sizeof(PGManagerID);
	} else if (event == PGReplayEventMouseWheel) {
		position += sizeof(int) * 2 + sizeof(double) * 2 + sizeof(byte) + sizeof(PGManagerID);
	} else if (event == PGReplayEventMouseDown) {
		position += sizeof(int) * 4 + sizeof(byte) + sizeof(PGManagerID);
	} else if (event == PGReplayEventMouseUp) {
		position += sizeof(int) * 3 + sizeof(byte) + sizeof(PGManagerID);
	} else if (event == PGReplayEventMouseMove) {
		position += sizeof(int) * 2 + sizeof(byte) + sizeof(PGManagerID);
	} else if (event == PGReplayEventLosesFocus) {
		position += sizeof(PGManagerID);
	} else if (event == PGReplayEventGainsFocus) {
		position += sizeof(PGManagerID);
	} else if (event == PGReplayEventAcceptsDragDrop) {
		position += sizeof(int) + sizeof(PGManagerID);
	} else if (event == PGReplayEventPerformDragDrop) {
		position += sizeof(int) * 3 + sizeof(PGManagerID);
	} else if (event == PGReplayEventClearDragDrop) {
		position += sizeof(int) + sizeof(PGManagerID);
	} else if (event == PGReplayEventSetSize) {
		position += sizeof(double) * 2 + sizeof(PGManagerID);
	} else if (event == PGReplayEventCloseControlManager) {
		position += sizeof(PGManagerID);
	} else if (event == PGReplayEventRefreshWindow) {
		position += sizeof(byte) + sizeof(PGManagerID);
	} else if (event == PGReplayEventRefreshWindowRectangle) {
		position += sizeof(int) * 4 + sizeof(byte) + sizeof(PGManagerID);
	} else if (event == PGReplayEventDropFile) {
		position += sizeof(PGManagerID);
		ReadString(data, position, size);
	} else if (event == PGReplayEventGetClipboardText) {
		ReadString(data, position, size);
	} else if (event == PGReplayEventGetTime) {
		position += sizeof(lng);
	} else if (event == PGReplayEventGetReadFile) {
		auto fname = ReadString(data, position, size);
		position += sizeof(int);
		auto text = ReadString(data, position, size);
	} else if (event == PGReplayEventGetDirectoryFiles) {
		auto fname = ReadString(data, position, size);
		lng dircount;
		ReadEntry(dircount, lng, lng, data, position, size);
		for (lng i = 0; i < dircount; i++) {
			ReadString(data, position, size);
		}
		lng filecount;
		ReadEntry(filecount, lng, lng, data, position, size);
		for (lng i = 0; i < filecount; i++) {
			ReadString(data, position, size);
		}
		position += sizeof(int);
	} else {
		// unrecognized event
cleanup:
		assert(0);
		return false;
	}
	return position < size;
}

void PGGlobalReplayManager::PeekEvent(PGReplayEvent event, char* data, size_t position, size_t size, bool recursive) {
	NextEvent(event, data, position, size);
	while (position < size) {
		PGReplayEvent event = data[position++];

		if (event == PGReplayEventGetClipboardText) {
			this->current_clipboard = ReadString(data, position, size);
		} else if (event == PGReplayEventGetTime) {
			ReadEntry(this->current_time, lng, lng, data, position, size);
		} else if (event == PGReplayEventGetReadFile) {
			ReadFileEvent(data, position, size);
		} else if (event == PGReplayEventGetDirectoryFiles) {
			ReadDirectoryEvent(data, position, size);
		} else {
			if (!recursive) {
				// we have found an actual event that we have to execute
				// however, first we read any subsequent GetClipboardText/GetTime/etc messages
				size_t current_position = position;
				PeekEvent(event, data, current_position, size, true);
			}
			return;
		}
		//NextEvent(event, data, position, size);
	}
cleanup:
	return;
}

PGReplayEvent prev_event;
bool PGGlobalReplayManager::ExecuteCommand(char* data, size_t& position, size_t size) {
	if (position >= size) {
		return false;
	}
	PGReplayEvent event = data[position++];
	PGManagerID manager_id;
	PeekEvent(event, data, position, size);
	if (event == PGReplayEventKeyboardButton) {
		PGButton button;
		PGModifier modifier;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(button, int, PGButton, data, position, size);
		ReadEntry(modifier, byte, PGModifier, data, position, size);
		managers[manager_id]->KeyboardButton(button, modifier);
	} else if (event == PGReplayEventKeyboardCharacter) {
		char character;
		PGModifier modifier;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(character, char, char, data, position, size);
		ReadEntry(modifier, byte, PGModifier, data, position, size);
		managers[manager_id]->KeyboardCharacter(character, modifier);
	} else if (event == PGReplayEventKeyboardUnicode) {
		PGUTF8Character character;
		PGModifier modifier;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(character.length, byte, byte, data, position, size);
		ReadEntry(character.character[0], byte, byte, data, position, size);
		ReadEntry(character.character[1], byte, byte, data, position, size);
		ReadEntry(character.character[2], byte, byte, data, position, size);
		ReadEntry(character.character[3], byte, byte, data, position, size);
		ReadEntry(modifier, byte, PGModifier, data, position, size);
		managers[manager_id]->KeyboardUnicode(character, modifier);
		return NextEvent(event, data, position, size);
	} else if (event == PGReplayEventUpdate) {
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		managers[manager_id]->Update();
	} else if (event == PGReplayEventDraw) {
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		managers[manager_id]->Draw(nullptr);
	} else if (event == PGReplayEventMouseWheel) {
		int x, y;
		double hdistance, distance;
		PGModifier modifier;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(x, int, int, data, position, size);
		ReadEntry(y, int, int, data, position, size);
		ReadEntry(hdistance, double, double, data, position, size);
		ReadEntry(distance, double, double, data, position, size);
		ReadEntry(modifier, byte, PGModifier, data, position, size);
		managers[manager_id]->MouseWheel(x, y, hdistance, distance, modifier);
	} else if (event == PGReplayEventMouseDown) {
		int x, y;
		PGMouseButton button;
		PGModifier modifier;
		int click_count;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(x, int, int, data, position, size);
		ReadEntry(y, int, int, data, position, size);
		ReadEntry(button, int, PGMouseButton, data, position, size);
		ReadEntry(modifier, byte, PGModifier, data, position, size);
		ReadEntry(click_count, int, int, data, position, size);
		managers[manager_id]->MouseDown(x, y, button, modifier, click_count);
	} else if (event == PGReplayEventMouseUp) {
		int x, y;
		PGMouseButton button;
		PGModifier modifier;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(x, int, int, data, position, size);
		ReadEntry(y, int, int, data, position, size);
		ReadEntry(button, int, PGMouseButton, data, position, size);
		ReadEntry(modifier, byte, PGModifier, data, position, size);
		managers[manager_id]->MouseUp(x, y, button, modifier);
	} else if (event == PGReplayEventMouseMove) {
		int x, y;
		PGMouseButton button;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(x, int, int, data, position, size);
		ReadEntry(y, int, int, data, position, size);
		ReadEntry(button, int, PGMouseButton, data, position, size);
		managers[manager_id]->MouseMove(x, y, button);
	} else if (event == PGReplayEventLosesFocus) {
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		managers[manager_id]->LosesFocus();
	} else if (event == PGReplayEventGainsFocus) {
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		managers[manager_id]->GainsFocus();
	} else if (event == PGReplayEventAcceptsDragDrop) {
		PGDragDropType type;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(type, int, PGDragDropType, data, position, size);
		managers[manager_id]->AcceptsDragDrop(type);
	} else if (event == PGReplayEventPerformDragDrop) {
		int x, y;
		PGDragDropType type;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(type, int, PGDragDropType, data, position, size);
		ReadEntry(x, int, int, data, position, size);
		ReadEntry(y, int, int, data, position, size);
		managers[manager_id]->PerformDragDrop(type, x, y, nullptr);
	} else if (event == PGReplayEventClearDragDrop) {
		PGDragDropType type;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(type, int, PGDragDropType, data, position, size);
		managers[manager_id]->ClearDragDrop(type);
	} else if (event == PGReplayEventSetSize) {
		PGSize sz;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(sz.width, double, double, data, position, size);
		ReadEntry(sz.height, double, double, data, position, size);
		managers[manager_id]->SetSize(sz);
	} else if (event == PGReplayEventCloseControlManager) {
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		managers[manager_id]->CloseControlManager();
	} else if (event == PGReplayEventRefreshWindow) {
		bool redraw_now;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(redraw_now, byte, bool, data, position, size);
		managers[manager_id]->RefreshWindow(redraw_now);
	} else if (event == PGReplayEventRefreshWindowRectangle) {
		bool redraw_now;
		PGIRect rect;
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		ReadEntry(rect.x, int, int, data, position, size);
		ReadEntry(rect.y, int, int, data, position, size);
		ReadEntry(rect.width, int, int, data, position, size);
		ReadEntry(rect.height, int, int, data, position, size);
		ReadEntry(redraw_now, byte, bool, data, position, size);
		managers[manager_id]->RefreshWindow(rect, redraw_now);
	} else if (event == PGReplayEventDropFile) {
		ReadEntry(manager_id, PGManagerID, PGManagerID, data, position, size);
		std::string fname = ReadString(data, position, size);
		managers[manager_id]->DropFile(fname);
	} else if (event == PGReplayEventGetClipboardText) {
		return NextEvent(event, data, position, size);
	} else if (event == PGReplayEventGetTime) {
		return NextEvent(event, data, position, size);
	} else if (event == PGReplayEventGetReadFile) {
		return NextEvent(event, data, position, size);
	} else if (event == PGReplayEventGetDirectoryFiles) {
		return NextEvent(event, data, position, size);
	} else {
		// unrecognized event
		assert(0);
		return false;
	}
	prev_event = event;
	return true;
cleanup:
	// invalid or corrupted replay file
	assert(0);
	return false;
}

void PGGlobalReplayManager::Lock() {
	LockMutex(lock);
}

void PGGlobalReplayManager::Unlock() {
	if (PGGlobalReplayManager::recording_replay) {
		panther::Flush(GetInstance()->file);
	}
	UnlockMutex(lock);
}

void PGGlobalReplayManager::_WriteEvent(PGManagerID manager_id, PGReplayEvent event) {
	panther::WriteToFile(file, (char*) &event, sizeof(PGReplayEvent));
	panther::WriteToFile(file, (char*)&manager_id, sizeof(PGManagerID));
}

void PGGlobalReplayManager::_WriteByte(char value) {
	panther::WriteToFile(file, &value, sizeof(char));
}

void PGGlobalReplayManager::_WriteLong(lng value) {
	panther::WriteToFile(file, (char*)&value, sizeof(lng));
}

void PGGlobalReplayManager::_WriteInt(int value) {
	panther::WriteToFile(file, (char*)&value, sizeof(int));
}

void PGGlobalReplayManager::_WriteDouble(double value) {
	panther::WriteToFile(file, (char*)&value, sizeof(double));
}

void PGGlobalReplayManager::_WriteString(std::string value) {
	size_t size = value.size();
	panther::WriteToFile(file, (char*)&size, sizeof(size_t));
	panther::WriteToFile(file, value.c_str(), value.size());
}

int PGGlobalReplayManager::RegisterManager(ControlManager* manager) {
	PGGlobalReplayManager* m = GetInstance();
	int entry = m->managers.size();
	assert(entry <= 255);
	m->managers.push_back(manager);
	return entry;
}

std::string PGGlobalReplayManager::_GetClipboardText() {
	return GetInstance()->current_clipboard;
}

PGTime PGGlobalReplayManager::_GetTime() {
	return GetInstance()->current_time;
}

void PGGlobalReplayManager::RecordGetClipboardText(std::string text) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(PGReplayEventGetClipboardText);
	PGGlobalReplayManager::WriteString(text);
	PGGlobalReplayManager::Unlock();
}

void PGGlobalReplayManager::RecordGetTime(PGTime time) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(PGReplayEventGetTime);
	PGGlobalReplayManager::WriteLong(time);
	PGGlobalReplayManager::Unlock();
}

void PGGlobalReplayManager::RecordReadFile(std::string fname, void* result,
	lng result_size, PGFileError error) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(PGReplayEventGetReadFile);
	PGGlobalReplayManager::WriteString(fname);
	PGGlobalReplayManager::WriteInt(error);
	PGGlobalReplayManager::WriteString(std::string((char*)result, result_size));
	PGGlobalReplayManager::Unlock();
}

void* PGGlobalReplayManager::ReadFile(std::string filename,
	lng& result_size, PGFileError& error) {
	assert(PGGlobalReplayManager::running_replay);
	auto instance = GetInstance();
	if (instance->stored_files.count(filename) == 0) {
		assert(0);
		return nullptr;
	}
	FileData& d = instance->stored_files[filename];
	error = d.error;
	char* ptr = nullptr;
	result_size = 0;
	if (d.error == PGFileSuccess) {
		ptr = (char*)malloc(d.data.size());
		memcpy(ptr, d.data.c_str(), d.data.size());
		result_size = d.data.size();
	}
	return ptr;
}

PGDirectoryFlags PGGlobalReplayManager::GetDirectoryFiles(std::string directory, std::vector<PGFile>& directories, std::vector<PGFile>& files) {
	assert(PGGlobalReplayManager::running_replay);
	auto instance = GetInstance();
	if (instance->stored_directories.count(directory) == 0) {
		assert(0);
		return PGDirectoryNotFound;
	}
	PGGlobalReplayManager::Lock();
	DirectoryData& d = instance->stored_directories[directory];
	for (auto it = d.directories.begin(); it != d.directories.end(); it++) {
		directories.push_back(PGFile(*it));
	}
	for (auto it = d.files.begin(); it != d.files.end(); it++) {
		files.push_back(PGFile(*it));
	}
	PGDirectoryFlags flags = d.flags;
	PGGlobalReplayManager::Unlock();
	return flags;
}


ReplayManager::ReplayManager(PGWindowHandle window) :
	ControlManager(window) {
}

bool ReplayManager::KeyboardButton(PGButton button, PGModifier modifier) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventKeyboardButton);
	PGGlobalReplayManager::WriteInt(button);
	PGGlobalReplayManager::WriteByte(modifier);
	PGGlobalReplayManager::Unlock();
	return ControlManager::KeyboardButton(button, modifier);
}

bool ReplayManager::KeyboardCharacter(char character, PGModifier modifier) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventKeyboardCharacter);
	PGGlobalReplayManager::WriteByte(character);
	PGGlobalReplayManager::WriteByte(modifier);
	PGGlobalReplayManager::Unlock();
	return ControlManager::KeyboardCharacter(character, modifier);
}

bool ReplayManager::KeyboardUnicode(PGUTF8Character character, PGModifier modifier) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventKeyboardUnicode);
	PGGlobalReplayManager::WriteByte(character.length);
	PGGlobalReplayManager::WriteByte(character.character[0]);
	PGGlobalReplayManager::WriteByte(character.character[1]);
	PGGlobalReplayManager::WriteByte(character.character[2]);
	PGGlobalReplayManager::WriteByte(character.character[3]);
	PGGlobalReplayManager::WriteByte(modifier);
	PGGlobalReplayManager::Unlock();
	return ControlManager::KeyboardUnicode(character, modifier);
}

void ReplayManager::Update() {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventUpdate);
	PGGlobalReplayManager::Unlock();
	ControlManager::Update();
}

void ReplayManager::Draw(PGRendererHandle renderer) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventDraw);
	PGGlobalReplayManager::Unlock();
	ControlManager::Draw(renderer);
}

void ReplayManager::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventMouseWheel);
	PGGlobalReplayManager::WriteInt(x);
	PGGlobalReplayManager::WriteInt(y);
	PGGlobalReplayManager::WriteDouble(distance);
	PGGlobalReplayManager::WriteDouble(hdistance);
	PGGlobalReplayManager::WriteByte(modifier);
	PGGlobalReplayManager::Unlock();
	ControlManager::MouseWheel(x, y, hdistance, distance, modifier);
}

void ReplayManager::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventMouseDown);
	PGGlobalReplayManager::WriteInt(x);
	PGGlobalReplayManager::WriteInt(y);
	PGGlobalReplayManager::WriteInt(button);
	PGGlobalReplayManager::WriteByte(modifier);
	PGGlobalReplayManager::WriteInt(click_count);
	PGGlobalReplayManager::Unlock();
	ControlManager::MouseDown(x, y, button, modifier, click_count);
}

void ReplayManager::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventMouseUp);
	PGGlobalReplayManager::WriteInt(x);
	PGGlobalReplayManager::WriteInt(y);
	PGGlobalReplayManager::WriteInt(button);
	PGGlobalReplayManager::WriteByte(modifier);
	PGGlobalReplayManager::Unlock();
	ControlManager::MouseUp(x, y, button, modifier);
}

void ReplayManager::MouseMove(int x, int y, PGMouseButton buttons) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventMouseMove);
	PGGlobalReplayManager::WriteInt(x);
	PGGlobalReplayManager::WriteInt(y);
	PGGlobalReplayManager::WriteByte(buttons);
	PGGlobalReplayManager::Unlock();
	ControlManager::MouseMove(x, y, buttons);
}

void ReplayManager::LosesFocus(void) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventLosesFocus);
	PGGlobalReplayManager::Unlock();
	ControlManager::LosesFocus();
}

void ReplayManager::GainsFocus(void) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventGainsFocus);
	PGGlobalReplayManager::Unlock();
	ControlManager::GainsFocus();
}

bool ReplayManager::AcceptsDragDrop(PGDragDropType type) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventAcceptsDragDrop);
	PGGlobalReplayManager::WriteInt(type);
	PGGlobalReplayManager::Unlock();
	return ControlManager::AcceptsDragDrop(type);
}

void ReplayManager::DragDrop(PGDragDropType type, int x, int y, void* data) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventDragDrop);
	PGGlobalReplayManager::WriteInt(type);
	PGGlobalReplayManager::WriteInt(x);
	PGGlobalReplayManager::WriteInt(y);
	assert(0); // FIXME: data should be copied as well
	PGGlobalReplayManager::Unlock();
	ControlManager::DragDrop(type, x, y, data);
}

void ReplayManager::PerformDragDrop(PGDragDropType type, int x, int y, void* data) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventPerformDragDrop);
	PGGlobalReplayManager::WriteInt(type);
	PGGlobalReplayManager::WriteInt(x);
	PGGlobalReplayManager::WriteInt(y);
	assert(0); // FIXME: data should be copied as well
	PGGlobalReplayManager::Unlock();
	ControlManager::PerformDragDrop(type, x, y, data);
}

void ReplayManager::ClearDragDrop(PGDragDropType type) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventClearDragDrop);
	PGGlobalReplayManager::WriteInt(type);
	PGGlobalReplayManager::Unlock();
	ControlManager::ClearDragDrop(type);
}

void ReplayManager::SetSize(PGSize size) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventSetSize);
	PGGlobalReplayManager::WriteDouble(size.width);
	PGGlobalReplayManager::WriteDouble(size.height);
	PGGlobalReplayManager::Unlock();
	ControlManager::SetSize(size);
}

bool ReplayManager::CloseControlManager() {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventCloseControlManager);
	PGGlobalReplayManager::Unlock();
	return ControlManager::CloseControlManager();
}

void ReplayManager::RefreshWindow(bool redraw_now) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventRefreshWindow);
	PGGlobalReplayManager::WriteByte(redraw_now);
	PGGlobalReplayManager::Unlock();
	ControlManager::RefreshWindow(redraw_now);
}

void ReplayManager::RefreshWindow(PGIRect rectangle, bool redraw_now) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventRefreshWindowRectangle);
	PGGlobalReplayManager::WriteInt(rectangle.x);
	PGGlobalReplayManager::WriteInt(rectangle.y);
	PGGlobalReplayManager::WriteInt(rectangle.width);
	PGGlobalReplayManager::WriteInt(rectangle.height);
	PGGlobalReplayManager::WriteByte(redraw_now);
	PGGlobalReplayManager::Unlock();
	ControlManager::RefreshWindow(rectangle, redraw_now);
}

void ReplayManager::DropFile(std::string filename) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(manager_id, PGReplayEventDropFile);
	PGGlobalReplayManager::WriteString(filename);
	PGGlobalReplayManager::Unlock();

	ControlManager::DropFile(filename);
}

void PGGlobalReplayManager::RecordGetDirectoryFiles(std::string directory, std::vector<PGFile>& directories, std::vector<PGFile>& files, PGDirectoryFlags flags) {
	PGGlobalReplayManager::Lock();
	PGGlobalReplayManager::WriteEvent(PGReplayEventGetDirectoryFiles);
	PGGlobalReplayManager::WriteString(directory);
	PGGlobalReplayManager::WriteLong(directories.size());
	for (auto it = directories.begin(); it != directories.end(); it++) {
		PGGlobalReplayManager::WriteString(it->path);
	}
	PGGlobalReplayManager::WriteLong(files.size());
	for (auto it = files.begin(); it != files.end(); it++) {
		PGGlobalReplayManager::WriteString(it->path);
	}
	PGGlobalReplayManager::WriteInt(flags);
	PGGlobalReplayManager::Unlock();
}
