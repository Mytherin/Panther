
#include "goto.h"
#include "style.h"
#include "controlmanager.h"
#include "textfield.h"
#include "togglebutton.h"
#include "projectexplorer.h"

#include <unordered_set>

#define HPADDING 35
#define HPADDING_SMALL 5
#define VPADDING 5

PG_CONTROL_INITIALIZE_KEYBINDINGS(PGGotoAnything);

struct ScrollData {
	PGVerticalScroll offset;
	TextField* tf;
	std::vector<PGCursorRange> backup_cursors;
};

PGGotoAnything::PGGotoAnything(TextField* textfield, PGWindowHandle window, PGGotoType type) :
	PGContainer(window), textfield(textfield), box(nullptr), field(nullptr), 
	preview(nullptr), scroll_data(nullptr), type(type) {
	font = PGCreateFont(PGFontTypeUI);
	SetTextFontSize(font, 12);
	SetTextColor(font, PGStyleManager::GetColor(PGColorStatusBarText));
}

PGGotoAnything::~PGGotoAnything() {
	if (scroll_data) delete scroll_data;
	if (preview) {
		this->textfield->GetTabControl()->CloseTemporaryFile();
	}
}

void PGGotoAnything::Initialize() {
	PGScalar button_width = MeasureTextWidth(font, "Definition") + 2 * HPADDING;
	PGScalar button_height = GetTextHeight(font) + VPADDING * 2;
	ToggleButton* buttons[4];
	for (int i = 0; i < 4; i++) {
		buttons[i] = new ToggleButton(window, this, false);
		buttons[i]->SetSize(PGSize(button_width, button_height));
		buttons[i]->x = i * button_width;
		buttons[i]->y = VPADDING;
		this->AddControl(std::shared_ptr<Control>(buttons[i]));
	}
	goto_command = buttons[0];
	goto_line = buttons[1];
	goto_file = buttons[2];
	goto_definition = buttons[3];

	goto_command->SetText(std::string("Command"), font);
	goto_line->SetText(std::string("Line"), font);
	goto_file->SetText(std::string("File"), font);
	goto_definition->SetText(std::string("Definition"), font);

	goto_command->OnPressed([](Button* b, void* data) {
		((PGGotoAnything*)data)->SetType(PGGotoCommand);
	}, this);
	goto_line->OnPressed([](Button* b, void* data) {
		((PGGotoAnything*)data)->SetType(PGGotoLine);
	}, this);
	goto_file->OnPressed([](Button* b, void* data) {
		((PGGotoAnything*)data)->SetType(PGGotoFile);
	}, this);
	goto_definition->OnPressed([](Button* b, void* data) {
		((PGGotoAnything*)data)->SetType(PGGotoDefinition);
	}, this);

	this->width = button_width * 4;
	auto new_type = this->type;
	this->type = PGGotoNone;
	SetType(new_type);
}

void PGGotoAnything::Draw(PGRendererHandle renderer) {
	SetRenderBounds(renderer, PGRect(X(), Y(), this->width, this->height));
	PGContainer::Draw(renderer);
	ClearRenderBounds(renderer);
}


bool PGGotoAnything::KeyboardButton(PGButton button, PGModifier modifier) {
	if (this->PressKey(PGGotoAnything::keybindings, button, modifier)) {
		return true;
	}
	return PGContainer::KeyboardButton(button, modifier);
}

bool PGGotoAnything::KeyboardCharacter(char character, PGModifier modifier) {
	if (this->PressCharacter(PGGotoAnything::keybindings, character, modifier)) {
		return true;
	}
	return PGContainer::KeyboardCharacter(character, modifier);
}

void PGGotoAnything::OnResize(PGSize old_size, PGSize new_size) {
	if (box) {
		box->SetSize(PGSize(this->width, this->height - box->y));
	}
	if (field) {
		field->SetSize(PGSize(this->width, field->height));
	}
}

void PGGotoAnything::SetType(PGGotoType type) {
	this->Cancel(false);
	this->type = type;

	if (field) {
		this->RemoveControl(field);
		field = nullptr;
	}
	if (box) {
		this->RemoveControl(box);
		box = nullptr;
	}
	if (scroll_data) {
		delete scroll_data;
		scroll_data = nullptr;
	}
	current_textfile = nullptr;

	goto_line->SetToggled(false);
	goto_file->SetToggled(false);
	goto_definition->SetToggled(false);
	goto_command->SetToggled(false);

	switch (type) {
		case PGGotoLine:
		{
			goto_line->SetToggled(true);
			field = new SimpleTextField(this->window);
			field->SetSize(PGSize(this->width, GetTextHeight(font) + 6));
			field->SetPosition(PGPoint(0, goto_line->y + goto_line->height));

			this->scroll_data = new ScrollData();
			std::shared_ptr<TextView> view = textfield->GetTextView();
			assert(view.get());
			scroll_data->offset = view->GetLineOffset();
			scroll_data->tf = textfield;
			scroll_data->backup_cursors = Cursor::BackupCursors(view->cursors);

			field->OnTextChanged([](Control* c, void* data) {
				SimpleTextField* input = (SimpleTextField*)c;
				TextField* tf = (TextField*)data;
				TextLine textline = input->GetTextView()->file->GetLine(0);
				std::string str = std::string(textline.GetLine(), textline.GetLength());
				const char* line = str.c_str();
				char* p = nullptr;
				// attempt to convert the text to a number
				// FIXME: strtoll (long = 32-bit on windows)
				long converted = panther::strtolng(line, &p);
				errno = 0;
				if (p != line) { // if p == line, then line is empty so we do nothing
					if (*p == '\0') { // if *p == '\0' the entire string was converted
						bool valid = true;
						// bounds checking
						if (converted <= 0) {
							converted = 1;
							valid = false;
						} else if (converted > tf->GetTextView()->file->GetLineCount()) {
							converted = tf->GetTextView()->file->GetLineCount();
							valid = false;
						}
						converted--;
						// move the cursor and offset of the currently active file
						tf->GetTextView()->SetLineOffset(std::max(converted - tf->GetLineHeight() / 2, (long)0));
						tf->GetTextView()->SetCursorLocation(converted, 0);
						tf->Invalidate();
						input->SetValidInput(valid);
						input->Invalidate();
					} else {
						// invalid input, notify the user
						input->SetValidInput(false);
						input->Invalidate();
					}
				}
			}, (void*) textfield);
			this->AddControl(std::shared_ptr<Control>(field));
			break;
		}
		case PGGotoFile:
		{
			goto_file->SetToggled(true);
			ControlManager* manager = GetControlManager(this);
			ProjectExplorer* explorer = manager->active_projectexplorer;
			TabControl* tabs = textfield->GetTabControl();
			std::vector<SearchEntry> entries;
			for (auto it = tabs->tabs.begin(); it != tabs->tabs.end(); it++) {
				SearchEntry entry;
				entry.display_name = it->view->file->GetName();
				entry.display_subtitle = it->view->file->GetFullPath();
				entry.text = it->view->file->GetFullPath();
				entry.data = it->view;
				entry.basescore = 10;
				entry.multiplier = 1.5;
				entries.push_back(entry);
			}

			auto box = make_shared_control<SearchBox>(this->window, entries, &explorer->index);
			this->box = box.get();
			box->SetSize(PGSize(this->width, this->height - (goto_command->y + goto_command->height)));
			box->SetPosition(PGPoint(0, goto_command->y + goto_command->height));
			box->OnRender(
				[](PGRendererHandle renderer, PGFontHandle font, SearchEntry& entry, PGScalar& x, PGScalar& y, PGScalar button_height) {
				// render the text file icon next to each open file
				PGFile file = PGFile(entry.text);
				std::string filename = file.Filename();
				std::string ext = file.Extension();

				PGScalar file_icon_height = button_height * 0.6;
				PGScalar file_icon_width = file_icon_height * 0.8;

				PGLanguage* language = PGLanguageManager::GetLanguage(ext);

				PGColor color = GetTextColor(font);
				x += 2.5f;
				std::transform(ext.begin(), ext.end(), ext.begin(), ::toupper);
				RenderFileIcon(renderer, font, ext.c_str(), x, y + (button_height - file_icon_height) / 2, file_icon_width, file_icon_height,
					language ? language->GetColor() : PGColor(255, 255, 255), PGColor(30, 30, 30), PGColor(91, 91, 91));
				x += file_icon_width + 2.5f;
				SetTextColor(font, color);
			});
			current_textfile = textfield->GetTextView();
			box->OnSelectionChanged([](SearchBox* searchbox, SearchEntry& entry, void* data) {
				PGGotoAnything* g = (PGGotoAnything*)data;
				TabControl* tabs = g->textfield->GetTabControl();
				if (g->preview) {
					g->preview = nullptr;
					tabs->CloseTemporaryFile();
				}
				if (entry.data != nullptr) {
					tabs->SwitchToTab(entry.data);
				} else {
					PGFileError error;
					auto file = std::shared_ptr<TextFile>(TextFile::OpenTextFile(entry.text, error));
					if (file) {
						g->preview = make_shared_control<TextView>(g->textfield, file);
						tabs->OpenTemporaryFile(g->preview);
					}
				}
			}, (void*)this);

			this->AddControl(box);
			break;
		}
		case PGGotoDefinition:
		{
			goto_definition->SetToggled(true);
			break;
		}
		case PGGotoCommand:
		{
			goto_command->SetToggled(true);
			break;
		}
		default:
			assert(0);
			break;
	}
}

void PGGotoAnything::Cancel(bool success) {
	switch (this->type) {
		case PGGotoLine:
		{
			assert(scroll_data);
			if (!success) {
				textfield->GetTextView()->RestoreCursors(scroll_data->backup_cursors);
				textfield->GetTextView()->SetLineOffset(scroll_data->offset);
			}
			break;
		}
		case PGGotoFile:
		{
			assert(box);
			assert(current_textfile);
			TabControl* tabs = this->textfield->GetTabControl();
			if (!success) {
				tabs->SwitchToTab(current_textfile);
			} else {
				if (preview) {
					tabs->OpenFile(preview);
					tabs->CloseTemporaryFile();
					preview = nullptr;
				}
			}
			current_textfile = nullptr;
		}
		default:
			break;
	}
	this->type = PGGotoNone;
}

void PGGotoAnything::Close(bool success) {
	this->Cancel(success);
	auto parent = this->parent.lock();
	dynamic_cast<PGContainer*>(parent.get())->RemoveControl(this);
}

void PGGotoAnything::InitializeKeybindings() {
	std::map<std::string, PGKeyFunction>& noargs = PGGotoAnything::keybindings_noargs;
	noargs["confirm"] = [](Control* c) {
		((PGGotoAnything*)c)->Close(true);
	};
	noargs["cancel"] = [](Control* c) {
		((PGGotoAnything*)c)->Close(false);
	};
}
