
#include "statusnotification.h"
#include "style.h"

PGStatusNotification::PGStatusNotification(PGWindowHandle window, PGFontHandle font, PGStatusType type,
	std::string display_text, std::string tooltip, bool progress_bar) :
	Control(window), font(font), type(type), text(display_text), 
	completion_percentage(progress_bar ? 0 : -1),
	remaining_time(0) {
	if (tooltip.size() > 0) {
		SetTooltip(tooltip);
	}
}

void PGStatusNotification::SetText(std::string text) {
	this->text = text;
	this->Invalidate();
}

void PGStatusNotification::SetProgress(double progress) {
	this->completion_percentage = progress;
	this->Invalidate();
}

void PGStatusNotification::SetType(PGStatusType type) {
	this->type = type;
	this->Invalidate();
}

void PGStatusNotification::ResolveSize(PGSize new_size) {
	if (font) {
		this->fixed_width = MeasureTextWidth(font, text);
	}
	Control::ResolveSize(new_size);
}

void PGStatusNotification::Draw(PGRendererHandle renderer) {
	PGColor edge_color;
	PGColor fill_color;
	switch (type) {
		case PGStatusInProgress:
			fill_color = PGStyleManager::GetColor(PGColorNotificationInProgress);
			break;
		case PGStatusWarning:
			fill_color = PGStyleManager::GetColor(PGColorNotificationWarning);
			break;
		case PGStatusError:
			fill_color = PGStyleManager::GetColor(PGColorNotificationError);
			break;
	}
	edge_color = PGColor(fill_color.r * 0.8, fill_color.g * 0.8, fill_color.b * 0.8);
	PGRect rect = PGRect(X(), Y(), this->width, this->height);
	RenderRectangle(renderer, rect, fill_color, PGDrawStyleFill);
	if (completion_percentage > 0) {
		PGRect fill_rect = PGRect(rect.x, rect.y, rect.width * completion_percentage, rect.height);
		RenderRectangle(renderer, fill_rect, edge_color, PGDrawStyleFill);
	}
	RenderRectangle(renderer, rect, edge_color, PGDrawStyleStroke);

	SetTextColor(font, PGStyleManager::GetColor(PGColorTextFieldText));
	RenderText(renderer, font, text.c_str(), text.size(), 
		rect.x + rect.width / 2, rect.y + (rect.height - GetTextHeight(font)) / 2, PGTextAlignHorizontalCenter | PGTextAlignVerticalCenter);

	Control::Draw(renderer);
}
