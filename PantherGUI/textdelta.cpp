
#include "textdelta.h"
#include "textfile.h"


void MultipleDelta::FinalizeDeltas(TextFile* file) {
	for (auto it = deltas.begin(); it != deltas.end(); it++) {
		ssize_t shift_lines = 0;
		ssize_t shift_lines_line = (*it)->linenr;
		ssize_t shift_lines_character = (*it)->characternr;
		ssize_t shift_characters = 0;
		ssize_t shift_characters_line = (*it)->linenr;
		ssize_t shift_characters_character = (*it)->characternr;
		switch ((*it)->TextDeltaType()) {
		case PGDeltaRemoveLine:
			shift_lines = -1;
			break;
		case PGDeltaRemoveManyLines:
			shift_lines = -((ssize_t)((RemoveLines*)(*it))->lines.size());
			break;
		case PGDeltaAddLine:
			shift_lines = 1;
			shift_characters = -((AddLine*)(*it))->characternr;
			break;
		case PGDeltaAddManyLines:
			shift_lines = ((AddLines*)(*it))->lines.size();
			shift_characters = -((AddLines*)(*it))->characternr;
			break;
		case PGDeltaAddText:
			shift_characters = (ssize_t)((AddText*)(*it))->text.size();
			break;
		case PGDeltaRemoveText:
			shift_characters = -((ssize_t)((RemoveText*)(*it))->charactercount);
			break;
		}

		for (auto it2 = it + 1; it2 != deltas.end(); it2++) {
			TextDelta* delta = *it2;
			if (delta->linenr == shift_characters_line &&
				delta->characternr >= shift_characters_character) {
				delta->characternr += shift_characters;
			}
			if (shift_lines != 0) {
				if (delta->linenr == shift_lines_line && shift_lines < 0) {
					delta->characternr = file->GetLine(delta->linenr - 1)->GetLength();
				}
				if (delta->linenr >= shift_lines_line) {
					delta->linenr += shift_lines;
				}
			}
		}
	}
}
