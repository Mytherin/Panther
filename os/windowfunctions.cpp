
#include "control.h"
#include "windowfunctions.h"
#include "controlmanager.h"

#include "workspace.h"

#include "c.h"
#include "findresults.h"
#include "xml.h"

#include "keybindings.h"
#include "settings.h"
#include "globalsettings.h"

#include "projectexplorer.h"
#include "replaymanager.h"
#include "splitter.h"
#include "statusbar.h"
#include "toolbar.h"

PGScalar PGMarginAuto = INFINITY;

PGTextAlign PGTextAlignBottom = 0x01;
PGTextAlign PGTextAlignTop = 0x02;
PGTextAlign PGTextAlignLeft = 0x04;
PGTextAlign PGTextAlignRight = 0x08;
PGTextAlign PGTextAlignHorizontalCenter = 0x10;
PGTextAlign PGTextAlignVerticalCenter = 0x20;

const PGModifier PGModifierNone = 0x00;
const PGModifier PGModifierCmd = 0x01;
const PGModifier PGModifierCtrl = 0x02;
const PGModifier PGModifierShift = 0x04;
const PGModifier PGModifierAlt = 0x08;
const PGModifier PGModifierCtrlShift = PGModifierCtrl | PGModifierShift;
const PGModifier PGModifierCtrlAlt = PGModifierCtrl | PGModifierAlt;
const PGModifier PGModifierShiftAlt = PGModifierShift | PGModifierAlt;
const PGModifier PGModifierCtrlShiftAlt = PGModifierCtrl | PGModifierShift | PGModifierAlt;
const PGModifier PGModifierCmdCtrl = PGModifierCmd | PGModifierCtrl;
const PGModifier PGModifierCmdShift = PGModifierCmd | PGModifierShift;
const PGModifier PGModifierCmdAlt = PGModifierCmd | PGModifierAlt;
const PGModifier PGModifierCmdCtrlShift = PGModifierCmd | PGModifierCtrl | PGModifierShift;
const PGModifier PGModifierCmdShiftAlt = PGModifierCmd | PGModifierShift | PGModifierAlt;
const PGModifier PGModifierCmdCtrlAlt = PGModifierCmd | PGModifierCtrl | PGModifierAlt;
const PGModifier PGModifierCmdCtrlShiftAlt = PGModifierCmd | PGModifierCtrl | PGModifierShift | PGModifierAlt;

const PGMouseButton PGMouseButtonNone = 0x00;
const PGMouseButton PGLeftMouseButton = 0x01;
const PGMouseButton PGRightMouseButton = 0x02;
const PGMouseButton PGMiddleMouseButton = 0x04;
const PGMouseButton PGXButton1 = 0x08;
const PGMouseButton PGXButton2 = 0x10;

const PGPopupMenuFlags PGPopupMenuFlagsNone = 0x00;
const PGPopupMenuFlags PGPopupMenuChecked = 0x01;
const PGPopupMenuFlags PGPopupMenuGrayed = 0x02;
const PGPopupMenuFlags PGPopupMenuSelected = 0x04;
const PGPopupMenuFlags PGPopupMenuSubmenu = 0x08;
const PGPopupMenuFlags PGPopupMenuHighlighted = 0x10;

std::string GetButtonName(PGButton button) {
	switch (button) {
		case PGButtonNone: return std::string("PGButtonNone");
		case PGButtonInsert: return std::string("PGButtonInsert");
		case PGButtonHome: return std::string("PGButtonHome");
		case PGButtonPageUp: return std::string("PGButtonPageUp");
		case PGButtonPageDown: return std::string("PGButtonPageDown");
		case PGButtonDelete: return std::string("PGButtonDelete");
		case PGButtonEnd: return std::string("PGButtonEnd");
		case PGButtonPrintScreen: return std::string("PGButtonPrintScreen");
		case PGButtonScrollLock: return std::string("PGButtonScrollLock");
		case PGButtonPauseBreak: return std::string("PGButtonPauseBreak");
		case PGButtonEscape: return std::string("PGButtonEscape");
		case PGButtonTab: return std::string("PGButtonTab");
		case PGButtonCapsLock: return std::string("PGButtonCapsLock");
		case PGButtonF1: return std::string("PGButtonF1");
		case PGButtonF2: return std::string("PGButtonF2");
		case PGButtonF3: return std::string("PGButtonF3");
		case PGButtonF4: return std::string("PGButtonF4");
		case PGButtonF5: return std::string("PGButtonF5");
		case PGButtonF6: return std::string("PGButtonF6");
		case PGButtonF7: return std::string("PGButtonF7");
		case PGButtonF8: return std::string("PGButtonF8");
		case PGButtonF9: return std::string("PGButtonF9");
		case PGButtonF10: return std::string("PGButtonF10");
		case PGButtonF11: return std::string("PGButtonF11");
		case PGButtonF12: return std::string("PGButtonF12");
		case PGButtonNumLock: return std::string("PGButtonNumLock");
		case PGButtonDown: return std::string("PGButtonDown");
		case PGButtonUp: return std::string("PGButtonUp");
		case PGButtonLeft: return std::string("PGButtonLeft");
		case PGButtonRight: return std::string("PGButtonRight");
		case PGButtonBackspace: return std::string("PGButtonBackspace");
		case PGButtonEnter: return std::string("PGButtonEnter");
	}
	return std::string("UnknownButton");
}

std::string GetModifierName(PGModifier modifier) {
	if (modifier == PGModifierNone) return std::string("PGModifierNone");
	else if (modifier == PGModifierCmd) return std::string("PGModifierCmd");
	else if (modifier == PGModifierCtrl) return std::string("PGModifierCtrl");
	else if (modifier == PGModifierShift) return std::string("PGModifierShift");
	else if (modifier == PGModifierAlt) return std::string("PGModifierAlt");
	else if (modifier == PGModifierCtrlShift) return std::string("PGModifierCtrlShift");
	else if (modifier == PGModifierCtrlAlt) return std::string("PGModifierCtrlAlt");
	else if (modifier == PGModifierShiftAlt) return std::string("PGModifierShiftAlt");
	else if (modifier == PGModifierCtrlShiftAlt) return std::string("PGModifierCtrlShiftAlt");
	else if (modifier == PGModifierCmdCtrl) return std::string("PGModifierCmdCtrl");
	else if (modifier == PGModifierCmdShift) return std::string("PGModifierCmdShift");
	else if (modifier == PGModifierCmdAlt) return std::string("PGModifierCmdAlt");
	else if (modifier == PGModifierCmdCtrlShift) return std::string("PGModifierCmdCtrlShift");
	else if (modifier == PGModifierCmdShiftAlt) return std::string("PGModifierCmdShiftAlt");
	else if (modifier == PGModifierCmdCtrlAlt) return std::string("PGModifierCmdCtrlAlt");
	else if (modifier == PGModifierCmdCtrlShiftAlt) return std::string("PGModifierCmdCtrlShiftAlt");
	return std::string("UnknownModifier");
}

std::string GetMouseButtonName(PGMouseButton modifier) {
	if (modifier == PGMouseButtonNone) return std::string("PGMouseButtonNone");
	else if (modifier == PGLeftMouseButton) return std::string("PGLeftMouseButton");
	else if (modifier == PGRightMouseButton) return std::string("PGRightMouseButton");
	else if (modifier == PGMiddleMouseButton) return std::string("PGMiddleMouseButton");
	else if (modifier == PGXButton1) return std::string("PGXButton1");
	else if (modifier == PGXButton2) return std::string("PGXButton2");
	else if (modifier == (PGLeftMouseButton | PGRightMouseButton)) return std::string("PGLeftMouseButton | PGRightMouseButton");
	else if (modifier == (PGLeftMouseButton | PGMiddleMouseButton)) return std::string("PGLeftMouseButton | PGMiddleMouseButton");
	else if (modifier == (PGRightMouseButton | PGMiddleMouseButton)) return std::string("PGRightMouseButton | PGMiddleMouseButton");
	return std::string("UnknownModifier");
}

PGWorkspace* PGInitializeFirstWorkspace() {
	PGLanguageManager::AddLanguage(new CLanguage());
	PGLanguageManager::AddLanguage(new XMLLanguage());
	PGLanguageManager::AddLanguage(new FindResultsLanguage());

	PGSettingsManager::Initialize();
	PGKeyBindingsManager::Initialize();
	PGGlobalSettings::Initialize("globalsettings.json");

	Scheduler::Initialize();
	Scheduler::SetThreadCount(2);

	// load a workspace
	nlohmann::json& settings = PGGlobalSettings::GetSettings();
	if (settings.count("workspaces") == 0 || !settings["workspaces"].is_array()) {
		// no known workspaces in the settings, initialize the settings
		settings["workspaces"] = nlohmann::json::array();
	}
	lng active_workspace = 0;
	if (settings.count("active_workspace") != 0 && settings["active_workspace"].is_number()) {
		active_workspace = panther::min(panther::max(0LL, settings["active_workspace"].get<lng>()), (lng) settings["workspaces"].array().size() - 1);
	} else {
		settings["active_workspace"] = 0;
		active_workspace = 0;
	}
	if (settings["workspaces"].array().size() == 0) {
		// no known workspaces, add one
		settings["workspaces"][0] = "workspace.json";
		settings["active_workspace"] = 0;
		active_workspace = 0;
	}

	std::string workspace_path = settings["workspaces"][active_workspace];
	// load the active workspace
	PGWorkspace* workspace = new PGWorkspace();
	workspace->LoadWorkspace(workspace_path);
	return workspace;
}

PGPoint GetMousePosition(PGWindowHandle window, Control* c) {
	return GetMousePosition(window) - c->Position();
}

void DropFile(PGWindowHandle handle, std::string filename) {
	(GetWindowManager(handle))->DropFile(filename);
}

#define CLIPBOARD_HISTORY_MAX 5
static std::vector<std::string> clipboard_history;

void SetClipboardText(PGWindowHandle window, std::string text) {
	if (text.size() == 0) return;
	if (!PGGlobalReplayManager::running_replay) {
		SetClipboardTextOS(window, text);
	}
	if (clipboard_history.size() > CLIPBOARD_HISTORY_MAX) {
		clipboard_history.erase(clipboard_history.begin());
	}
	clipboard_history.push_back(text);
}

std::string GetClipboardText(PGWindowHandle window) {
	if (PGGlobalReplayManager::running_replay) {
		return PGGlobalReplayManager::GetClipboardText();
	}
	std::string text = GetClipboardTextOS(window);
	if (PGGlobalReplayManager::recording_replay) {
		PGGlobalReplayManager::RecordGetClipboardText(text);
	}
	return text;
}

PGTime PGGetTime() {
	if (PGGlobalReplayManager::running_replay) {
		return PGGlobalReplayManager::GetTime();
	}
	PGTime time = PGGetTimeOS();
	if (PGGlobalReplayManager::recording_replay) {
		PGGlobalReplayManager::RecordGetTime(time);
	}
	return time;
}

std::vector<std::string> GetClipboardTextHistory() {
	return clipboard_history;
}

std::string PGFile::Filename() {
	size_t pos = path.find_last_of(GetSystemPathSeparator());
	if (pos == std::string::npos) return "";
	return path.substr(pos + 1);
}

std::string PGFile::Extension() {
	size_t pos = path.find_last_of('.');
	if (pos == std::string::npos) return "";
	return path.substr(pos + 1);
}

std::string PGFile::Directory() {
	size_t pos = path.find_last_of(GetSystemPathSeparator());
	if (pos == std::string::npos || pos <= 0) return "";
	return path.substr(0, pos);
}

std::string PGPathJoin(std::string path_one, std::string path_two) {
	// FIXME: safe path join
	return path_one + GetSystemPathSeparator() + path_two;
}

void PGPopupMenuInsertEntry(PGPopupMenuHandle handle, std::string text, PGPopupCallback callback, PGPopupMenuFlags flags) {
	PGPopupInformation info(handle);
	info.text = text;
	info.hotkey = "";
	PGPopupMenuInsertEntry(handle, info, callback, flags);
}

std::shared_ptr<ControlManager> PGCreateControlManager(PGWindowHandle handle, std::vector<std::shared_ptr<TextFile>> initial_files) {
	std::shared_ptr<ControlManager> manager;
	if (PGGlobalReplayManager::recording_replay) {
		auto m = std::make_shared<ReplayManager>(handle);
		manager = m;
		m->manager_id = PGGlobalReplayManager::RegisterManager(manager.get());
	} else {
		manager = std::make_shared<ControlManager>(handle);
		if (PGGlobalReplayManager::running_replay) {
			PGGlobalReplayManager::RegisterManager(manager.get());
		}
	}
	SetWindowManager(handle, manager);
	manager->SetPosition(PGPoint(0, 0));
	manager->SetSize(PGSize(1000, 700));
	manager->percentage_height = 1;
	manager->percentage_width = 1;

	auto explorer = make_shared_control<ProjectExplorer>(handle);
	explorer->SetAnchor(PGAnchorBottom | PGAnchorLeft);
	explorer->fixed_width = 200;
	explorer->percentage_height = 1;
	explorer->minimum_width = 50;

	auto toolbar = make_shared_control<PGToolbar>(handle);
	toolbar->SetAnchor(PGAnchorLeft | PGAnchorTop);
	toolbar->percentage_width = 1;
	toolbar->fixed_height = TOOLBAR_HEIGHT;

	auto bar = make_shared_control<StatusBar>(handle);
	bar->SetAnchor(PGAnchorLeft | PGAnchorBottom);
	bar->percentage_width = 1;
	bar->fixed_height = STATUSBAR_HEIGHT;
	explorer->bottom_anchor = bar.get();
	explorer->top_anchor = toolbar.get();

	auto splitter = make_shared_control<Splitter>(handle, true);
	splitter->SetAnchor(PGAnchorBottom | PGAnchorLeft);
	splitter->left_anchor = explorer.get();
	splitter->bottom_anchor = bar.get();
	splitter->top_anchor = toolbar.get();
	splitter->fixed_width = 4;
	splitter->percentage_height = 1;

	manager->AddControl(bar);
	manager->AddControl(explorer);
	manager->AddControl(splitter);
	manager->AddControl(toolbar);

	manager->toolbar = toolbar.get();
	manager->statusbar = bar.get();
	manager->active_projectexplorer = explorer.get();
	manager->splitter = splitter.get();

	manager->SetTextFieldLayout(1, 1, initial_files);

	auto menu = PGCreateMenu(handle, manager.get());
	auto file = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertCommand(file, "New File", "new_tab", TabControl::keybindings_noargs, TabControl::keybindings, TabControl::keybindings_images, PGPopupMenuFlagsNone,
		[](Control* c, PGPopupInformation* info) {
		((PGKeyFunction)info->pdata)(((ControlManager*)c)->active_textfield->GetTabControl());
	}, menu);
	PGPopupMenuInsertCommand(file, "Open File", "open_file", TabControl::keybindings_noargs, TabControl::keybindings, TabControl::keybindings_images, PGPopupMenuFlagsNone,
		[](Control* c, PGPopupInformation* info) {
		((PGKeyFunction)info->pdata)(((ControlManager*)c)->active_textfield->GetTabControl());
	}, menu);
	PGPopupMenuInsertEntry(file, PGPopupInformation(file, "Open Folder", "Ctrl+Shift+O"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, "Open Recent", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, "Reopen with Encoding", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertEntry(file, "New View into File", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertCommand(file, "Save", "save", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images, PGPopupMenuFlagsNone,
		[](Control* c, PGPopupInformation* info) {
		((PGKeyFunction)info->pdata)(((ControlManager*)c)->active_textfield);
	}, menu);
	PGPopupMenuInsertCommand(file, "Save As", "save_as", TextField::keybindings_noargs, TextField::keybindings, TextField::keybindings_images, PGPopupMenuFlagsNone,
		[](Control* c, PGPopupInformation* info) {
		((PGKeyFunction)info->pdata)(((ControlManager*)c)->active_textfield);
	}, menu);
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertCommand(file, "New Window", "new_window", ControlManager::keybindings_noargs, ControlManager::keybindings, ControlManager::keybindings_images, PGPopupMenuFlagsNone, nullptr, menu);
	PGPopupMenuInsertCommand(file, "Close Window", "close_window", ControlManager::keybindings_noargs, ControlManager::keybindings, ControlManager::keybindings_images, PGPopupMenuFlagsNone, nullptr, menu);
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertCommand(file, "Close File", "close_tab", TabControl::keybindings_noargs, TabControl::keybindings, TabControl::keybindings_images, PGPopupMenuFlagsNone,
		[](Control* c, PGPopupInformation* info) {
		((PGKeyFunction)info->pdata)(((ControlManager*)c)->active_textfield->GetTabControl());
	}, menu);
	PGPopupMenuInsertEntry(file, "Close All Files", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSeparator(file);
	PGPopupMenuInsertEntry(file, "Exit", [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, file, "File");

	auto edit = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertEntry(edit, PGPopupInformation(edit, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, edit, "Edit");

	auto view = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertEntry(view, PGPopupInformation(view, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, view, "View");

	auto project = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertEntry(project, PGPopupInformation(project, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, project, "Project");

	auto build = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertEntry(build, PGPopupInformation(build, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, build, "Build");

	auto tools = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertEntry(tools, PGPopupInformation(tools, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, tools, "Tools");

	auto preferences = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertEntry(preferences, PGPopupInformation(preferences, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, preferences, "Preferences");

	auto help = PGCreatePopupMenu(handle, manager.get());
	PGPopupMenuInsertEntry(help, PGPopupInformation(help, "New File", "Ctrl+N"), [](Control* c, PGPopupInformation* info) {
		assert(0);
	});
	PGPopupMenuInsertSubmenu(menu, help, "Help");

	PGSetWindowMenu(handle, menu);
	return manager;
}
