#pragma once

const char* PANTHER_DEFAULT_KEYBINDINGS = R"DEFAULTSETTINGS(
{
	"global": [
		{ "key": "cmd+shift+n", "command": "new_window" },
		{ "key": "cmd+shift+w", "command": "close_window" },

		{ "key": "cmd+f", "command": "show_find", "args": {"type": "find"} },
		{ "key": "cmd+alt+f", "command": "show_find", "args": {"type": "findreplace"} },
		{ "key": "cmd+shift+f", "command": "show_find", "args": {"type": "findinfiles"} }
	],
	"basictextfield": [
		{ "key": "backspace", "command": "left_delete" },
		{ "key": "shift+backspace", "command": "left_delete" },
		{ "key": "cmd+backspace", "command": "left_delete_word" },
		{ "key": "cmd+shift+backspace", "command": "left_delete_line" },
		{ "key": "delete", "command": "right_delete" },
		{ "key": "cmd+delete", "command": "right_delete_word" },
		{ "key": "cmd+shift+delete", "command": "right_delete_line" },
		{ "key": "shift+delete", "command": "delete_selected_lines" },

		{ "key": "cmd+z", "command": "undo" },
		{ "key": "cmd+shift+z", "command": "redo" },
		{ "key": "cmd+y", "command": "redo" },

		{ "key": "cmd+[", "command": "undo_selection" },
		{ "key": "cmd+]", "command": "redo_selection" },

		{ "key": "cmd+x", "command": "cut" },
		{ "key": "cmd+c", "command": "copy" },
		{ "key": "cmd+v", "command": "paste" },
		{ "key": "cmd+shift+v", "command": "paste_from_history" },

		{ "key": "cmd+a", "command": "select_all" },

		{ "key": "insert", "command": "toggle_overwrite" },

		{ "key": "cmd+backspace", "command": "delete_word", "args": { "forward": false } },
		{ "key": "cmd+shift+backspace", "command": "delete_line", "args": { "forward": false } },
		{ "key": "cmd+delete", "command": "delete_word", "args": { "forward": true } },
		{ "key": "cmd+shift+delete", "command": "delete_line", "args": { "forward": true } },

		{ "key": "left", "command": "offset_character", "args": {"direction": "left"}},
		{ "key": "right", "command": "offset_character", "args": {"direction": "right"}},
		{ "key": "shift+left", "command": "offset_character", "args": {"direction": "left", "selection": true}},
		{ "key": "shift+right", "command": "offset_character", "args": {"direction": "right", "selection": true}},

		{ "key": "cmd+left", "command": "offset_character", "args": {"direction": "left", "word": true}},
		{ "key": "cmd+right", "command": "offset_character", "args": {"direction": "right", "word": true}},
		{ "key": "cmd+shift+left", "command": "offset_character", "args": {"direction": "left", "selection": true, "word": true}},
		{ "key": "cmd+shift+right", "command": "offset_character", "args": {"direction": "right", "selection": true, "word": true}},

		{ "key": "ctrl+a", "command": "offset_start_of_line"},
		{ "key": "ctrl+shift+a", "command": "select_start_of_line"},

		{ "key": "ctrl+e", "command": "offset_end_of_line"},
		{ "key": "ctrl+shift+e", "command": "select_end_of_line"},

		{ "mouse": "cmd+left", "command": "select_word" },
		{ "mouse": "shift+left", "command": "set_cursor_selection" },
		{ "mouse": "left", "clicks": 1, "command": "set_cursor_location" },
		{ "mouse": "left", "clicks": 2, "command": "select_word" },
		{ "mouse": "left", "clicks": 3, "command": "select_line" }

	],
	"simpletextfield": [
		{ "key": "up", "command": "prev_entry"},
		{ "key": "down", "command": "next_entry"}
	],
	"textfield": [
		{ "key": "cmd+s", "command": "save" },
		{ "key": "cmd+shift+s", "command": "save_as" },

		{ "key": "ctrl+g", "command": "show_goto", "args": {"type": "line"} },
		{ "key": "cmd+p", "command": "show_goto", "args": {"type": "file"} },

		{ "key": "enter", "command": "insert", "args": { "characters": "\n" } },
		{ "key": "shift+enter", "command": "insert", "args": { "characters": "\n" } },
		{ "key": "ctrl+alt+up", "command": "swap_line_up" },
		{ "key": "ctrl+alt+down", "command": "swap_line_down" },
		{ "key": "cmd+/", "command": "toggle_comment", "args": { "block": false } },
		{ "key": "cmd+alt+/", "command": "toggle_comment", "args": { "block": true } },
		{ "key": "cmd++", "command": "increase_font_size" },
		{ "key": "cmd+=", "command": "increase_font_size" },
		{ "key": "cmd+keypad_plus", "command": "increase_font_size" },
		{ "key": "cmd+-", "command": "decrease_font_size" },
		{ "key": "cmd+keypad_minus", "command": "decrease_font_size" },
		{ "key": "cmd+equals", "command": "increase_font_size" },
		{ "key": "cmd+shift+equals", "command": "decrease_font_size" },
		{ "key": "cmd+shift+keypad_plus", "command": "decrease_font_size" },
		{ "key": "cmd+up", "command": "scroll_lines", "args": {"amount": -1.0 } },
		{ "key": "cmd+down", "command": "scroll_lines", "args": {"amount": 1.0 } },

		{ "key": "up", "command": "offset_line", "args": {"amount": -1.0 } },
		{ "key": "shift+up", "command": "offset_line", "args": {"amount": -1.0, "selection": true } },
		{ "key": "down", "command": "offset_line", "args": {"amount": 1.0 } },
		{ "key": "shift+down", "command": "offset_line", "args": {"amount": 1.0, "selection": true } },

		{ "key": "pageup", "command": "offset_line", "args": {"amount": -1.0, "unit": "page" } },
		{ "key": "shift+pageup", "command": "offset_line", "args": {"amount": -1.0, "unit": "page", "selection": true } },
		{ "key": "pagedown", "command": "offset_line", "args": {"amount": 1.0, "unit": "page" } },
		{ "key": "shift+pagedown", "command": "offset_line", "args": {"amount": 1.0, "unit": "page", "selection": true } },

		{ "key": "tab", "command": "increase_indent"},
		{ "key": "shift+tab", "command": "decrease_indent"},

		{ "mouse": "middle", "command": "drag_region" }

	],
	"tabcontrol": [
		{ "key": "cmd+o", "command": "open_file" },
		
		{ "key": "cmd+w", "command": "close_tab" },
		{ "key": "cmd+n", "command": "new_tab" },
		{ "key": "cmd+shift+t", "command": "reopen_last_file" },

		{ "key": "cmd+shift+]", "command": "next_tab" },
		{ "key": "cmd+shift+[", "command": "prev_tab" },
		{ "key": "ctrl+tab", "command": "next_tab" },
		{ "key": "ctrl+shift+tab", "command": "prev_tab" }
	],
	"findtext": [
		{"key": "enter", "command": "find_next"},
		{"key": "shift+enter", "command": "find_prev"},
		{"key": "tab", "command": "shift_focus_forward"},
		{"key": "shift+tab", "command": "shift_focus_backward"},
		{"key": "escape", "command": "close"}
	],
	"goto": [
		{ "key": "enter", "command": "confirm" },
		{ "key": "escape", "command": "cancel" }
	],
	"searchbox": [
		{ "key": "enter", "command": "confirm" },
		{ "key": "escape", "command": "cancel" }
	]
}
)DEFAULTSETTINGS";
