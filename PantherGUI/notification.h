#pragma once

#include "button.h"
#include "container.h"

enum PGNotificationType {
	PGNotificationTypeNone,
	PGNotificationTypeError,
	PGNotificationTypeWarning
};

class PGNotification : public PGContainer {
public:
	PGNotification(PGWindowHandle window, PGNotificationType type, std::string text);
	~PGNotification();

	PGFontHandle GetFont() { return font; }

	void Draw(PGRendererHandle renderer, PGIRect* rect);

	void AddButton(PGControlDataCallback, Control* control, void* data, std::string button_text);
private:
	PGNotificationType type;
	std::string text;

	struct ButtonData {
		PGControlDataCallback callback;
		void* data;
		Button* button;
		Control* control;
	};
	std::vector<ButtonData*> buttons;
	PGFontHandle font;
};