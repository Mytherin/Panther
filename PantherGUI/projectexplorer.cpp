
#include "controlmanager.h"
#include "projectexplorer.h"
#include "style.h"
#include "togglebutton.h"

#include "toolbar.h"


PG_CONTROL_INITIALIZE_KEYBINDINGS(ProjectExplorer);

#define FOLDER_IDENT 8
#define PROJECT_EXPLORER_PADDING 24

ProjectExplorer::ProjectExplorer(PGWindowHandle window) :
	PGContainer(window), dragging_scrollbar(false), renaming_file(-1), scrollbar_offset(0), file_render_height(0), show_all_files(true) {
#ifdef WIN32
	this->directories.push_back(new PGDirectory("C:\\Users\\wieis\\Documents\\Visual Studio 2015\\Projects\\Panther\\PantherGUI", !show_all_files));
#else
	this->directories.push_back(new PGDirectory("/Users/myth/Programs/re2", !show_all_files));
#endif
	this->directories.back()->expanded = true;
	font = PGCreateFont("segoe ui", false, true);
	SetTextFontSize(font, 12);

	scrollbar = new Scrollbar(this, window, false, false);
	scrollbar->padding.bottom = SCROLLBAR_PADDING;
	scrollbar->padding.top = SCROLLBAR_PADDING;
	scrollbar->margin.top = TOOLBAR_HEIGHT;
	scrollbar->SetAnchor(PGAnchorRight | PGAnchorTop);
	scrollbar->fixed_width = SCROLLBAR_SIZE;
	scrollbar->percentage_height = 1;
	//scrollbar->SetPosition(PGPoint(this->width - SCROLLBAR_SIZE, 0));
	scrollbar->OnScrollChanged([](Scrollbar* scroll, lng value) {
		((ProjectExplorer*)scroll->parent)->SetScrollbarOffset(value);
	});
	this->AddControl(scrollbar);

	file_render_height = std::max(GetTextHeight(font) + 2.0f, 16.0f);

	//RenderImage(renderer, bitmap, x, y);
	//DeleteImage(bitmap);

	ToggleButton* button = new ToggleButton(window, this, show_all_files);
	button->SetImage(PGStyleManager::GetImage("data/icons/showallfiles.png"));
	button->padding = PGPadding(4, 4, 4, 4);
	button->fixed_width = TOOLBAR_HEIGHT - button->padding.left - button->padding.right;
	button->fixed_height = TOOLBAR_HEIGHT - button->padding.top - button->padding.bottom;
	button->SetAnchor(PGAnchorLeft | PGAnchorTop);
	button->margin.left = 2;
	button->margin.right = 2;
	button->background_color = PGColor(0, 0, 0, 0);
	button->background_stroke_color = PGColor(0, 0, 0, 0);
	button->background_color_hover = PGStyleManager::GetColor(PGColorTextFieldSelection);
	button->tooltip = "Show All Files";
	button->OnToggle([](Button* button, bool toggled, void* data) {
		ProjectExplorer* p = (ProjectExplorer*)data;
		p->SetShowAllFiles(toggled);
	}, this);
	this->AddControl(button);

	Button* b = new Button(window, this);
	b->SetImage(PGStyleManager::GetImage("data/icons/collapseall.png"));
	b->padding = PGPadding(4, 4, 4, 4);
	b->fixed_width = TOOLBAR_HEIGHT - b->padding.left - b->padding.right;
	b->fixed_height = TOOLBAR_HEIGHT - b->padding.top - b->padding.bottom;
	b->SetAnchor(PGAnchorLeft | PGAnchorTop);
	b->margin.left = 2;
	b->margin.right = 2;
	b->left_anchor = button;
	b->background_color = PGColor(0, 0, 0, 0);
	b->background_stroke_color = PGColor(0, 0, 0, 0);
	b->background_color_hover = PGStyleManager::GetColor(PGColorTextFieldSelection);
	b->tooltip = "Collapse All";
	b->OnPressed([](Button* b, void* data) {
		ProjectExplorer* p = (ProjectExplorer*)data;
		p->CollapseAll();
	}, this);
	this->AddControl(b);
}

ProjectExplorer::~ProjectExplorer() {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		delete *it;
	}
}

void ProjectExplorer::PeriodicRender(void) {
	bool invalidated = false;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		auto flags = PGGetFileFlags((*it)->path);
		if (flags.modification_time != (*it)->last_modified_time) {
			(*it)->last_modified_time = flags.modification_time;
			(*it)->Update(!this->show_all_files);
			invalidated = true;
		}
	}
	if (invalidated) {
		this->Invalidate();
	}
}

void ProjectExplorer::SetShowAllFiles(bool show_all_files) {
	this->show_all_files = show_all_files;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->Update(!this->show_all_files);
	}
	this->Invalidate();
}

void ProjectExplorer::CollapseAll() {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->CollapseAll();
	}
	this->Invalidate();
}

void ProjectExplorer::DrawFile(PGRendererHandle renderer, PGBitmapHandle file_image, PGFile file, PGScalar x, PGScalar& y, bool selected, bool highlighted) {
	if (selected || highlighted) {
		PGColor color = selected ? PGStyleManager::GetColor(PGColorTextFieldSelection) : PGColor(45, 45, 45);
		RenderRectangle(renderer, PGRect(x, y, this->width, file_render_height), color, PGDrawStyleFill);
	}
	if (!file_image) {
		std::string extension = file.Extension();
		auto language = PGLanguageManager::GetLanguage(extension);
		PGColor language_color = language ? language->GetColor() : PGColor(255, 255, 255);
		if (extension == "cpp" || extension == "cc") {
			extension = "c++";
		}
		if (extension == "c++") {
			file_image = PGStyleManager::GetImage("icon_cpp.png");
		}
		if (extension == "h") {
			file_image = PGStyleManager::GetImage("icon_h.png");
		}
		if (extension == "json") {
			file_image = PGStyleManager::GetImage("icon_json.png");
		}
		if (extension == "xml") {
			file_image = PGStyleManager::GetImage("icon_xml.png");
		}
		if (extension == "mm") {
			file_image = PGStyleManager::GetImage("icon_objc.png");
		}
		if (extension == "js") {
			file_image = PGStyleManager::GetImage("icon_js.png");
		}
		if (extension == "py") {
			file_image = PGStyleManager::GetImage("icon_py.png");
		}
		if (extension == "rs") {
			file_image = PGStyleManager::GetImage("icon_rs.png");
		}
	}

	if (file_image) {
		RenderImage(renderer, file_image, x, y);
	} else {
		RenderFileIcon(renderer, font, "", x + (20 - 8) / 2.0, y + (file_render_height - 12) / 2.0, 8, 12,
			PGColor(255, 255, 255),
			PGStyleManager::GetColor(PGColorTabControlBackground),
			PGStyleManager::GetColor(PGColorTabControlBorder));
	}
	x += 24;
	if (!(selected && renaming_file >= 0)) {
		RenderText(renderer, font, file.path.c_str(), file.path.size(), x, y);
	} else {
		textfield->x = x;
		textfield->width = this->width - textfield->x - scrollbar->width - 16;
	}
	y += file_render_height;
}

void ProjectExplorer::SetScrollbarOffset(double offset) {
	scrollbar_offset = std::min((double)MaximumScrollOffset(), std::max(0.0, offset));
}

void ProjectExplorer::ScrollToFile(lng file_number) {
	if (scrollbar_offset > file_number) {
		scrollbar_offset = file_number;
	} else if (scrollbar_offset + RenderedFiles() <= file_number) {
		scrollbar_offset = file_number - RenderedFiles() + 1;
	}
	SetScrollbarOffset(scrollbar_offset);
}

void ProjectExplorer::FinishRename(bool success, bool update_selection) {
	if (renaming_file < 0) return;
	renaming_file = -1;
	if (success) {
		std::string new_name = textfield->GetTextFile().GetText();
		std::string full_name = PGPathJoin(PGFile(renaming_path).Directory(), new_name);
		rename(renaming_path.c_str(), full_name.c_str());
		directories[0]->Update(!this->show_all_files);
		if (update_selection) {
			PGDirectory* directory; PGFile file;
			lng filenr = this->FindFile(full_name, &directory, &file);
			if (filenr >= 0) {
				this->SelectFile(filenr, PGSelectSingleFile, false, false);
				ScrollToFile(filenr);
			}
		}
		this->Invalidate();
	}
	this->RemoveControl(textfield);
	textfield = nullptr;
}

void ProjectExplorer::RenameFile() {
	if (selected_files.size() != 1) return;
	assert(renaming_file < 0);
	ScrollToFile(selected_files[0]);
	// find the selected file
	PGDirectory *directory;
	PGFile file;
	FindFile(selected_files[0], &directory, &file);
	std::string initial_text;
	if (directory) {
		file = PGFile(directory->path);
	}
	initial_text = file.Filename();
	this->renaming_path = file.path;
	this->renaming_file = selected_files[0];
	this->textfield = new SimpleTextField(window);
	this->textfield->SetText(initial_text);
	lng pos = initial_text.find('.');
	this->textfield->GetTextFile().SetCursorLocation(0, 0, 0, pos == std::string::npos ? initial_text.size() : pos);
	textfield->GetTextFile().SetXOffset(0);
	textfield->SetRenderBackground(false);
	textfield->x = 0; // FIXME
	textfield->width = this->width - textfield->x - scrollbar->width - 16;
	textfield->height = file_render_height;
	textfield->SetFont(font);

	this->AddControl(this->textfield);
}

void ProjectExplorer::DrawDirectory(PGRendererHandle renderer, PGDirectory& directory, PGScalar x, PGScalar& y, PGScalar max_y, lng& current_offset, lng offset, lng& selection, lng highlighted_entry) {
	if (current_offset >= offset) {
		bool selected = selected_files.size() > selection ? selected_files[selection] == current_offset : false;
		if (selected) selection++;
		SetTextStyle(font, PGTextStyleBold);

		if (directory.expanded) {
			RenderTriangle(renderer, PGPoint(x - 8, y + 3 + file_render_height / 2.0f), PGPoint(x - 2, y + 3 + file_render_height / 2.0f), PGPoint(x - 2, y - 3 + file_render_height / 2.0f), PGColor(255, 255, 255), PGDrawStyleFill);
		} else {
			RenderTriangle(renderer, PGPoint(x - 8, y - 4 + file_render_height / 2.0f), PGPoint(x - 8, y + 4 + file_render_height / 2.0f), PGPoint(x - 2, y + file_render_height / 2.0f), PGColor(255, 255, 255), PGDrawStyleFill);
		}
		PGBitmapHandle image = directory.expanded ? PGStyleManager::GetImage("folder_open.png") : PGStyleManager::GetImage("folder_closed.png");
		DrawFile(renderer, image, PGFile(directory.path).Filename(), x, y, selected, highlighted_entry == current_offset);
		SetTextStyle(font, PGTextStyleNormal);
	}
	current_offset++;
	if (directory.expanded) {
		PGScalar start_x = x + FOLDER_IDENT / 2.0f;
		PGScalar start_y = y;
		x += FOLDER_IDENT;
		for (auto it = directory.directories.begin(); it != directory.directories.end(); it++) {
			if (y > max_y) return;
			DrawDirectory(renderer, **it, x, y, max_y, current_offset, offset, selection, highlighted_entry);
		}
		for (auto it = directory.files.begin(); it != directory.files.end(); it++) {
			if (y > max_y) return;
			if (current_offset >= offset) {
				bool selected = selected_files.size() > selection ? selected_files[selection] == current_offset : false;
				if (selected) selection++;
				DrawFile(renderer, nullptr, *it, x, y, selected, highlighted_entry == current_offset);
			}
			current_offset++;
		}
	}
}

void ProjectExplorer::Draw(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();

	PGPoint mouse = GetMousePosition(window);
	mouse.x -= X();
	mouse.y -= Y();

	lng highlighted_entry = scrollbar_offset + (mouse.y / file_render_height);
	if (mouse.x < 0 || mouse.x > this->width) {
		highlighted_entry = -1;
	}

	y += PROJECT_EXPLORER_PADDING;
	SetRenderBounds(renderer, PGRect(x, y, this->width, this->height - PROJECT_EXPLORER_PADDING));

	// render the background
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height - PROJECT_EXPLORER_PADDING), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);

	SetTextColor(font, PGStyleManager::GetColor(PGColorProjectExplorerText));
	lng offset = scrollbar_offset;
	lng current_offset = 0;
	lng current_selection = 0;

	// render the files in the directory
	for (auto it = directories.begin(); it != directories.end(); it++) {
		DrawDirectory(renderer, **it, x + FOLDER_IDENT + 2, y, y + this->height - PROJECT_EXPLORER_PADDING, current_offset, offset, current_selection, highlighted_entry);
	}

	scrollbar->UpdateValues(0, MaximumScrollOffset(), RenderedFiles(), scrollbar_offset);

	if (textfield) {
		textfield->y = (renaming_file - offset) * file_render_height;
	}

	ClearRenderBounds(renderer);
	y = Y();

	RenderRectangle(renderer, PGRect(x, y, this->width, PROJECT_EXPLORER_PADDING), PGStyleManager::GetColor(PGColorTabControlBackground), PGDrawStyleFill);

	PGContainer::Draw(renderer);
}

bool ProjectExplorer::KeyboardButton(PGButton button, PGModifier modifier) {
	if (button == PGButtonF2) {
		this->RenameFile();
		return true;
	}
	if (button == PGButtonEscape) {
		// cancel rename
		FinishRename(false, false);
		return true;
	}
	if (button == PGButtonEnter) {
		// succeed in rename
		FinishRename(true, true);
		return true;
	}
	if (this->PressKey(ProjectExplorer::keybindings, button, modifier)) {
		return true;
	}
	return PGContainer::KeyboardButton(button, modifier);
}


bool ProjectExplorer::KeyboardCharacter(char character, PGModifier modifier) {
	if (this->PressCharacter(ProjectExplorer::keybindings, character, modifier)) {
		return true;
	}
	return PGContainer::KeyboardCharacter(character, modifier);
}


void ProjectExplorer::MouseWheel(int x, int y, double hdistance, double distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		SetScrollbarOffset(scrollbar_offset - distance / 120.0f);
		scrollbar->UpdateValues(0, MaximumScrollOffset(), RenderedFiles(), scrollbar_offset);
		this->Invalidate();
	}
}

void ProjectExplorer::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.x < this->width - SCROLLBAR_SIZE && mouse.y > PROJECT_EXPLORER_PADDING) {
		lng selected_file = scrollbar_offset + ((mouse.y - PROJECT_EXPLORER_PADDING) / file_render_height);
		if (selected_file < 0 || selected_file >= TotalFiles()) return;

		if (button == PGLeftMouseButton) {
			// FIXME: click on file opens it
			// FIXME: dragging files
			// FIXME: right click on files
			PGSelectFileType select_type;
			if (modifier == PGModifierCtrl) {
				select_type = PGSelectAddSingleFile;
			} else if (modifier == PGModifierShift) {
				select_type = PGSelectAddRangeFile;
			} else if (modifier == PGModifierNone) {
				select_type = PGSelectSingleFile;
			} else {
				return;
			}
			SelectFile(selected_file, select_type, click_count > 0 && modifier == PGModifierNone, true);
		} else if (button == PGRightMouseButton) {
			PGDirectory *directory;
			PGFile file;
			FindFile(selected_file, &directory, &file);

			SelectFile(selected_file, PGSelectSingleFile, false, false);

			PGPopupMenuHandle menu = PGCreatePopupMenu(this->window, this);
			if (directory) {
				PGPopupMenuInsertEntry(menu, "Add New File", [](Control* control, PGPopupInformation* info) {
				});
				PGPopupMenuInsertEntry(menu, "Add New Folder", [](Control* control, PGPopupInformation* info) {
				});
				PGPopupMenuInsertEntry(menu, "Add Existing File", [](Control* control, PGPopupInformation* info) {
				});
				PGPopupMenuInsertSeparator(menu);
			}
			PGPopupMenuInsertEntry(menu, "Rename", [](Control* control, PGPopupInformation* info) {
				auto explorer = dynamic_cast<ProjectExplorer*>(control);
				explorer->RenameFile();
			});
			PGPopupMenuInsertSeparator(menu);
			PGPopupMenuInsertEntry(menu, "Cut", [](Control* control, PGPopupInformation* info) {
			});
			PGPopupMenuInsertEntry(menu, "Copy", [](Control* control, PGPopupInformation* info) {
			});
			PGPopupMenuInsertEntry(menu, "Paste", [](Control* control, PGPopupInformation* info) {
			});
			PGPopupMenuInsertSeparator(menu);
			PGPopupInformation info(menu, directory);
			info.data = directory ? directory->path : file.path;
			info.text = directory ? "Open Folder in File Explorer" : "Open File in File Explorer";
			PGPopupMenuInsertEntry(menu, info, [](Control* control, PGPopupInformation* info) {
				OpenFolderInExplorer(info->data);
			});
			info = PGPopupInformation(menu, directory);
			info.data = directory ? directory->path : file.path;
			info.text = directory ? "Open Folder in Terminal" : "Open File in Terminal";
			PGPopupMenuInsertEntry(menu, info, [](Control* control, PGPopupInformation* info) {
				OpenFolderInTerminal(info->data);
			});
			PGPopupMenuInsertSeparator(menu);
			PGPopupMenuInsertEntry(menu, "Delete", [](Control* control, PGPopupInformation* info) {
			});

			PGDisplayPopupMenu(menu, PGTextAlignLeft | PGTextAlignTop);
		}
		return;
	}
	PGContainer::MouseDown(x, y, button, modifier, click_count);
}

void ProjectExplorer::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGContainer::MouseUp(x, y, button, modifier);
}

void ProjectExplorer::MouseMove(int x, int y, PGMouseButton buttons) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.x < 0 || mouse.x > this->width) return;
	this->Invalidate();
	PGContainer::MouseMove(x, y, buttons);
}

lng ProjectExplorer::FindFile(std::string full_name, PGDirectory** directory, PGFile* file) {
	lng index = 0;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		lng entry = (*it)->FindFile(full_name, directory, file);
		if (entry >= 0) {
			return index + entry;
		}
		index += (*it)->DisplayedFiles();
	}
	return -1;
}

bool ProjectExplorer::RevealFile(std::string full_name, bool search_only_expanded) {
	PGDirectory* directory = nullptr;
	PGFile file;
	lng index = 0;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		lng entry = (*it)->FindFile(full_name, &directory, &file, search_only_expanded);
		if (entry >= 0) {
			this->selected_files.clear();
			this->selected_files.push_back(index + entry);
			ScrollToFile(index + entry);
			(*it)->expanded = true;
			this->Invalidate();
			return true;
		}
		index += (*it)->DisplayedFiles();
	}
	return false;
}

void ProjectExplorer::FindFile(lng file_number, PGDirectory** directory, PGFile* file) {
	*directory = nullptr;
	lng file_count = 0;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		lng files = (*it)->DisplayedFiles();
		if (file_number >= file_count && file_number < file_count + files) {
			(*it)->FindFile(file_number - file_count, directory, file);
			return;
		}
		file_count += files;
	}
}

std::vector<PGFile> ProjectExplorer::GetFiles() {
	std::vector<PGFile> files;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->GetFiles(files);
	}
	return files;
}

void ProjectExplorer::SelectFile(lng selected_file, PGSelectFileType type, bool open_file, bool click) {
	FinishRename(true, false);
	if (type == PGSelectSingleFile) {
		PGFile file;
		PGDirectory* directory;
		this->FindFile(selected_file, &directory, &file);

		if (directory && click) {
			// (un)expand the selected directory
			lng current_count = directory->DisplayedFiles();
			directory->expanded = !directory->expanded;
			lng new_count = directory->DisplayedFiles();
			// update any subsequent selections because they now point to a different location
			for (lng i = 0; i < selected_files.size(); i++) {
				if (selected_files[i] > selected_file) {
					selected_files[i] += new_count - current_count;
				}
			}
		} else {
			// simply select the file
			selected_files.clear();
			selected_files.push_back(selected_file);
			if (open_file || click) {
				TabControl* t = GetControlManager(this)->active_textfield->GetTabControl();
				if (open_file) {
					t->OpenFile(file.path);
				} else {
					PGFileError error;
					if (!(t->SwitchToTab(file.path))) {
						auto ptr = std::shared_ptr<TextFile>(TextFile::OpenTextFile(t->GetTextField(), file.path, error));
						if (error == PGFileSuccess) {
							assert(ptr);
							t->OpenTemporaryFile(ptr);
						}
					}
				}
			}
		}
	} else if (type == PGSelectAddRangeFile) {
		if (selected_files.size() == 0) {
			SelectFile(selected_file, type, open_file, click);
			return;
		}
		lng start = selected_files.back();
		lng end = selected_file;
		if (start > end) {
			std::swap(start, end);
		}
		selected_files.clear();
		for (; start <= end; start++) {
			selected_files.push_back(start);
		}
	} else if (type == PGSelectAddSingleFile) {
		auto entry = std::find(selected_files.begin(), selected_files.end(), selected_file);
		if (entry == selected_files.end()) {
			selected_files.push_back(selected_file);
			std::sort(selected_files.begin(), selected_files.end());
		} else {
			selected_files.erase(entry);
		}
	}
	this->Invalidate();
}

lng ProjectExplorer::TotalFiles() {
	lng total_files = 0;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		total_files += (*it)->DisplayedFiles();
	}
	return total_files;
}

lng ProjectExplorer::MaximumScrollOffset() {
	return std::max(0LL, TotalFiles() - RenderedFiles());
}

lng ProjectExplorer::RenderedFiles() {
	if (file_render_height <= 0) {
		return 0;
	}
	return (this->height - PROJECT_EXPLORER_PADDING) / file_render_height;
}

void ProjectExplorer::LosesFocus(void) {
	FinishRename(false, false);
}

