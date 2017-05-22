#pragma once

const char* PANTHER_DEFAULT_STYLES = R"DEFAULTSTYLES(
[
	{
		"name": "Black Panther",
		"colors": {
			"toggle_button_toggled": "#333333",
			"notification_background": "#333333",
			"notification_text": "#EEEEEE",
			"notification_error": "#BE1100",
			"notification_in_progress": "#4169E1",
			"notification_warning": "#FF8C00",
			"notification_button": "#0E639C",
			"explorer_text": "#DEDEDE",
			"textfield_background": "#2D2D2D",
			"textfield_text": "#D3D0C8",
			"textfield_selection": "#3F3F3F",
			"textfield_caret": "#D4D0C8",
			"textfield_linenumber": "#807F7B",
			"textfield_error": "#FC4036",
			"textfield_findmatch": "#804020",
			"scrollbar_background": "#3E3E42",
			"scrollbar_foreground": "#686868",
			"scrollbar_hover": "#9E9E9E",
			"scrollbar_drag": "#EFEBEF",
			"minimap_hover": "#FFFFFF60",
			"minimap_drag": "#FFFFFF80",
			"mainmenu_background": "#2D2D2D",
			"menu_background": "#1B1B1C",
			"menu_text": "#FFFFFF",
			"menu_disabled": "#656565",
			"menu_hover": "#333334",
			"tab_text": "#E0E0E0",
			"tab_border": "#686868",
			"tab_background": "#1E1E1E",
			"tab_unsaved_text": "#DE4D4D",
			"tab_hover": "#1C97EA",
			"tab_selected": "#007ACC",
			"tab_temporary": "#68217A",
			"statusbar_background": "#575757",
			"statusbar_text": "#FFFFFF",
			"syntax_string": "#99CC99",
			"syntax_constant": "#F99157",
			"syntax_comment": "#747369",
			"syntax_keyword": "#F08080",
			"syntax_operator": ">syntax_string",
			"syntax_class1": "#F08080",
			"syntax_class2": "#F7F692",
			"syntax_class3": "#CC99CC",
			"syntax_class4": ">syntax_class1",
			"syntax_class5": ">syntax_class1",
			"syntax_class6": ">syntax_class1"
		}
	},
	{
		"name": "Black Panther (Green Comments)",
		"base": "Black Panther",
		"colors": {
			"syntax_comment": "#84C684"
		}
	}
]
)DEFAULTSTYLES";