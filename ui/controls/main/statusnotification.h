#pragma once

#include "utils.h"
#include "control.h"

enum PGStatusType {
	PGStatusInProgress,
	PGStatusWarning,
	PGStatusError,
};

class PGStatusNotification : public Control {
public:
	PGStatusNotification(PGWindowHandle window, PGFontHandle font, PGStatusType type, std::string display_text, std::string tooltip, bool progress_bar);

	void SetText(std::string text);
	void SetProgress(double progress);
	void SetType(PGStatusType type);

	void ResolveSize(PGSize new_size);

	PGControlType GetControlType() { return PGControlTypeStatusNotification; }

	void Draw(PGRendererHandle);
private:
	PGFontHandle font;
	std::string text;
	std::string tooltip;
	PGStatusType type;
	double completion_percentage;
};
