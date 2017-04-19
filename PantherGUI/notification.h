#pragma once

#include "button.h"
#include "container.h"

enum PGNotificationType {
	PGNotificationTypeNone,
	PGNotificationTypeError,
	PGNotificationTypeWarning
};

class Label;

class PGNotification : public PGContainer {
public:
	PGNotification(PGWindowHandle window, PGNotificationType type, PGScalar width, std::string text);
	~PGNotification();

	void Draw(PGRendererHandle renderer);

	void AddButton(PGControlDataCallback, Control* control, void* data, std::string button_text);
private:
	void RecomputeHeight();

	PGNotificationType type;

	Label* label;

	struct ButtonData {
		PGControlDataCallback callback;
		void* data;
		Button* button;
		Control* control;
	};
	std::vector<ButtonData*> buttons;
	PGFontHandle font;
};