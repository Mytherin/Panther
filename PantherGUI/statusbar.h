#pragma once

#include "button.h"
#include "control.h"
#include "container.h"
#include "textfield.h"

#include "statusnotification.h"

#define STATUSBAR_HEIGHT 20

class StatusBar : public PGContainer {
public:
	StatusBar(PGWindowHandle window);
	~StatusBar();

	void SetText(std::string text);

	void SelectionChanged();

	void Draw(PGRendererHandle);

	void Invalidate() {
		PGIRect rect = PGIRect((int)X(), (int)Y(), (int)this->width, (int)this->height);
		RefreshWindow(this->window, rect);
	}

	PGStatusNotification* AddNotification(PGStatusType type, std::string text, std::string tooltip, bool progress_bar);
	void RemoveNotification(PGStatusNotification* notification);

	TextField* GetActiveTextField();

	virtual PGControlType GetControlType() { return PGControlTypeStatusBar; }
private:
	std::string status;

	Button* unicode_button;
	Button* language_button;
	Button* lineending_button;
	Button* tabwidth_button;

	PGStatusNotification* notifications = nullptr;

	PGFontHandle font;
};