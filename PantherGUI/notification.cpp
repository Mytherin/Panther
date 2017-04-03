
#include "notification.h"
#include "style.h"


PGNotification::PGNotification(PGWindowHandle window, PGNotificationType type, std::string text) :
	PGContainer(window), type(type), text(text) {
	font = PGCreateFont("myriad", false, true);
	SetTextFontSize(font, 13);
}

PGNotification::~PGNotification() {
	for (auto it = buttons.begin(); it != buttons.end(); it++) {
		delete *it;
	}
	buttons.clear();
}

void PGNotification::Draw(PGRendererHandle renderer) {
	PGScalar x = X();
	PGScalar y = Y();

	// render the background of the notification
	RenderRectangle(renderer, PGRect(x, y, this->width, this->height),
		PGStyleManager::GetColor(PGColorNotificationBackground), PGDrawStyleFill);

	SetTextColor(font, PGStyleManager::GetColor(PGColorNotificationText));
	// render the type of notification
	std::string text = "";
	PGColor color;
	switch (type) {
	case PGNotificationTypeError:
		text = "Error";
		color = PGStyleManager::GetColor(PGColorNotificationError);
		break;
	case PGNotificationTypeWarning:
		text = "Warning";
		color = PGStyleManager::GetColor(PGColorNotificationWarning);
		break;
	}
	PGScalar current_x = x;
	if (text.size() > 0) {
		PGScalar size = MeasureTextWidth(font, text.c_str());
		RenderRectangle(renderer, PGRect(x + 5, y + 5, size + 10, GetTextHeight(font)),
			color, PGDrawStyleFill);
		RenderText(renderer, font, text.c_str(), text.size(), x + 10, y + 3, PGTextAlignVerticalCenter);
		current_x += size + 20;
	}

	// render the text of the notification
	SetRenderBounds(renderer, PGRect(x, y, this->width, this->height));
	RenderText(renderer, font, this->text.c_str(), this->text.size(), current_x, y + 3, PGTextAlignVerticalCenter);
	ClearRenderBounds(renderer);

	// render the buttons (controls)
	PGContainer::Draw(renderer);
}

void PGNotification::AddButton(PGControlDataCallback callback, Control* c, void* data, std::string button_text) {
	ButtonData* b = new ButtonData();
	b->callback = callback;
	b->data = data;
	b->control = c;
	Button* button = new Button(window, this);
	button->SetText(button_text, font);
	b->button = button;
	this->buttons.push_back(b);

	PGPoint point = PGPoint(this->width, 2);
	for (auto it = buttons.begin(); it != buttons.end(); it++) {
		Button* button = (*it)->button;
		std::string text = button->GetText();
		point.x -= MeasureTextWidth(font, text) + 10;
	}

	button->SetPosition(point);
	button->SetSize(PGSize(MeasureTextWidth(font, button_text) + 10, this->height - 4));
	button->background_color = PGStyleManager::GetColor(PGColorNotificationButton);
	button->background_stroke_color = PGColor(0, 0, 0);
	button->OnPressed([](Button* b, void* data) {
		ButtonData* bd = ((ButtonData*)data);
		bd->callback(bd->control, bd->data);
	}, b);
	this->AddControl(button);
}