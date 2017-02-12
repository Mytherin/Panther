
#include "controlmanager.h"
#include "projectexplorer.h"
#include "style.h"

#define FOLDER_IDENT 16

ProjectExplorer::ProjectExplorer(PGWindowHandle window) :
	PGContainer(window), dragging_scrollbar(false) {
	this->directories.push_back(new PGDirectory("C:\\Users\\wieis\\Documents\\Visual Studio 2015\\Projects\\Panther\\PantherGUI"));
	this->directories.back()->expanded = true;
	font = PGCreateFont("myriad", false, true);
	SetTextFontSize(font, 12);

	scrollbar = new Scrollbar(this, window, false, false);
	scrollbar->bottom_padding = SCROLLBAR_PADDING;
	scrollbar->top_padding = SCROLLBAR_PADDING + SCROLLBAR_SIZE;
	scrollbar->SetPosition(PGPoint(this->width - SCROLLBAR_SIZE, 0));
	scrollbar->OnScrollChanged([](Scrollbar* scroll, lng value) {
		((ProjectExplorer*)scroll->parent)->scrollbar_offset = value;
	});
	this->AddControl(scrollbar);

	//RenderImage(renderer, bitmap, x, y);
	//DeleteImage(bitmap);
}

ProjectExplorer::~ProjectExplorer() {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		delete *it;
	}
}

void ProjectExplorer::PeriodicRender(void) {
	// FIXME: check if files in directory were actually changed
	for (auto it = directories.begin(); it != directories.end(); it++) {
		(*it)->Update();
	}
	this->Invalidate();
}

void ProjectExplorer::DrawFile(PGRendererHandle renderer, PGBitmapHandle file_image, PGFile file, PGScalar x, PGScalar& y, bool selected) {
	if (selected) {
		RenderRectangle(renderer, PGRect(x, y, this->width, GetTextHeight(font)), PGStyleManager::GetColor(PGColorTextFieldSelection), PGDrawStyleFill);
	}

	if (file_image) {
		RenderImage(renderer, file_image, x, y);
	} else {
		std::string extension = file.Extension();
		auto language = PGLanguageManager::GetLanguage(extension);
		PGColor language_color = language ? language->GetColor() : PGColor(255, 255, 255);
		if (extension == "cpp" || extension == "cc") {
			extension = "c++";
			language_color = PGColor(185, 117, 181);
		}
		PGScalar text_height = GetTextHeight(font);
		if (!(language_color.r == 255 && language_color.g == 255 && language_color.b == 255)) {
			PGScalar font_size = GetTextFontSize(font);
			SetTextFontSize(font, 10);
			SetTextColor(font, language_color);
			PGScalar size = MeasureTextWidth(font, extension.c_str(), extension.size());
			RenderText(renderer, font, extension.c_str(), extension.size(), x + (20 - size) / 2, y + (text_height - GetTextHeight(font)) / 2.0);

			SetTextFontSize(font, font_size);
			SetTextColor(font, PGStyleManager::GetColor(PGColorProjectExplorerText));
		} else {
			RenderFileIcon(renderer, font, "", x + (20 - 8) / 2.0, y + (text_height - 12) / 2.0, 8, 12,
				PGColor(255, 255, 255),
				PGStyleManager::GetColor(PGColorTabControlBackground),
				PGStyleManager::GetColor(PGColorTabControlBorder));
		}
	}
	x += 24;
	RenderText(renderer, font, file.path.c_str(), file.path.size(), x, y);
	y += GetTextHeight(font);
}

void ProjectExplorer::DrawDirectory(PGRendererHandle renderer, PGDirectory& directory, PGScalar x, PGScalar& y, lng& current_offset, lng offset, lng& selection) {
	if (current_offset >= offset) {
		bool selected = selected_files.size() > selection ? selected_files[selection] == current_offset : false;
		if (selected) selection++;
		SetTextStyle(font, PGTextStyleBold);
		PGBitmapHandle image = directory.expanded ? PGStyleManager::GetImage("folder_open.png") : PGStyleManager::GetImage("folder_closed.png");
		DrawFile(renderer, image, PGFile(directory.path).Filename(), x, y, selected);
		SetTextStyle(font, PGTextStyleNormal);
	}
	current_offset++;
	if (directory.expanded) {
		PGScalar start_x = x + FOLDER_IDENT / 2.0f;
		PGScalar start_y = y;
		x += FOLDER_IDENT;
		for (auto it = directory.directories.begin(); it != directory.directories.end(); it++) {
			DrawDirectory(renderer, **it, x, y, current_offset, offset, selection);
		}
		for (auto it = directory.files.begin(); it != directory.files.end(); it++) {
			if (current_offset >= offset) {
				bool selected = selected_files.size() > selection ? selected_files[selection] == current_offset : false;
				if (selected) selection++;
				DrawFile(renderer, nullptr, *it, x, y, selected);
			}
			current_offset++;
		}
		RenderDashedLine(renderer, PGLine(start_x, start_y, start_x, y), PGColor(255, 255, 255), 1.0f, 1.0f, 1);
	}
}

void ProjectExplorer::Draw(PGRendererHandle renderer, PGIRect *rect) {
	PGScalar x = X() - rect->x;
	PGScalar y = Y() - rect->y;
	SetRenderBounds(renderer, PGRect(x, y, this->width, this->height));

	// render the background
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height), PGStyleManager::GetColor(PGColorTextFieldBackground), PGDrawStyleFill);

	SetTextColor(font, PGStyleManager::GetColor(PGColorProjectExplorerText));
	lng offset = scrollbar_offset;
	lng current_offset = 0;
	lng current_selection = 0;

	// render the files in the directory
	for (auto it = directories.begin(); it != directories.end(); it++) {
		DrawDirectory(renderer, **it, x, y, current_offset, offset, current_selection);
	}

	scrollbar->UpdateValues(0, MaximumScrollOffset(), RenderedFiles(), scrollbar_offset);
	PGContainer::Draw(renderer, rect);
	ClearRenderBounds(renderer);
}

void ProjectExplorer::MouseWheel(int x, int y, double distance, PGModifier modifier) {
	if (modifier == PGModifierNone) {
		scrollbar_offset = std::min((double)MaximumScrollOffset(), std::max(0.0, scrollbar_offset - distance));
		scrollbar->UpdateValues(0, MaximumScrollOffset(), RenderedFiles(), scrollbar_offset);
		this->Invalidate();
	}
}

void ProjectExplorer::MouseDown(int x, int y, PGMouseButton button, PGModifier modifier, int click_count) {
	PGPoint mouse(x - this->x, y - this->y);
	if (mouse.x < this->width - SCROLLBAR_SIZE) {
		lng selected_file = scrollbar_offset + (mouse.y / GetTextHeight(font));
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
			SelectFile(selected_file, select_type, click_count > 0 && modifier == PGModifierNone);
		} else if (button == PGRightMouseButton) {

		}
		return;
	}
	PGContainer::MouseDown(x, y, button, modifier, click_count);
}

void ProjectExplorer::MouseUp(int x, int y, PGMouseButton button, PGModifier modifier) {
	PGContainer::MouseUp(x, y, button, modifier);
}

void ProjectExplorer::MouseMove(int x, int y, PGMouseButton buttons) {
	PGContainer::MouseMove(x, y, buttons);
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

void PGDirectory::FindFile(lng file_number, PGDirectory** directory, PGFile* file) {
	if (file_number == 0) {
		*directory = this;
		return;
	}
	lng file_count = 1;
	for (auto it = directories.begin(); it != directories.end(); it++) {
		lng files = (*it)->DisplayedFiles();
		if (file_number >= file_count && file_number < file_count + files) {
			(*it)->FindFile(file_number - file_count, directory, file);
			return;
		}
		file_count += files;
	}
	assert(this->expanded);
	lng entry = file_number - file_count;
	assert(entry >= 0 && entry < files.size());
	*file = files[entry];
	file->path = PGPathJoin(this->path, file->path);
}

void ProjectExplorer::SelectFile(lng selected_file, PGSelectFileType type, bool open_file) {
	if (type == PGSelectSingleFile) {
		PGFile file;
		PGDirectory* directory;
		this->FindFile(selected_file, &directory, &file);

		if (directory) {
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
			if (open_file) {
				TabControl* t = GetControlManager(this)->active_tabcontrol;
				t->OpenFile(file.path);
			}
		}
	} else if (type == PGSelectAddRangeFile) {
		if (selected_files.size() == 0) {
			SelectFile(selected_file, type, open_file);
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
	return TotalFiles() - RenderedFiles();
}

lng ProjectExplorer::RenderedFiles() {
	return this->height / GetTextHeight(font);
}

void ProjectExplorer::OnResize(PGSize old_size, PGSize new_size) {
	scrollbar->SetPosition(PGPoint(this->width - scrollbar->width, SCROLLBAR_PADDING));
	scrollbar->SetSize(PGSize(SCROLLBAR_SIZE, this->height - 2 * SCROLLBAR_PADDING));
	PGContainer::OnResize(old_size, new_size);
}

PGDirectory::PGDirectory(std::string path) :
	path(path), last_modified_time(-1), loaded_files(false), expanded(false) {
	this->Update();
}

PGDirectory::~PGDirectory() {
	for (auto it = directories.begin(); it != directories.end(); it++) {
		delete *it;
	}
	directories.clear();
}


void PGDirectory::Update() {
	files.clear();
	std::vector<PGFile> dirs;
	if (PGGetDirectoryFiles(path, dirs, files) == PGDirectorySuccess) {
		// for each directory, check if it is already present
		// if it is not we add it
		for (auto it = dirs.begin(); it != dirs.end(); it++) {
			std::string path = PGPathJoin(this->path, it->path);
			bool found = false;
			for (auto it2 = directories.begin(); it2 != directories.end(); it2++) {
				if ((*it2)->path == path) {
					found = true;
					break;
				}
			}
			if (!found) {
				directories.push_back(new PGDirectory(path));
			}
		}
		// FIXME: if directory not found, delete it
		loaded_files = true;
	} else {
		loaded_files = false;
	}
}

lng PGDirectory::DisplayedFiles() {
	if (expanded) {
		lng recursive_files = 1;
		for (auto it = directories.begin(); it != directories.end(); it++) {
			recursive_files += (*it)->DisplayedFiles();
		}
		return recursive_files + files.size();
	} else {
		return 1;
	}
}