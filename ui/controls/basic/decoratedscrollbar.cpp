
#include "decoratedscrollbar.h"


DecoratedScrollbar::DecoratedScrollbar(Control* parent, PGWindowHandle window, bool horizontal, bool arrows) :
	Scrollbar(parent, window, horizontal, arrows) {

}

void DecoratedScrollbar::Draw(PGRendererHandle renderer) {
	if (!visible) return;
	PGScalar x = X();
	PGScalar y = Y();
	DrawBackground(renderer);
	// render decorations
	std::sort(center_decorations.begin(), center_decorations.end());
	for (auto it = center_decorations.begin(); it != center_decorations.end(); it++) {
		PGRect rect = PGRect(
			horizontal ? x + (this->width - scrollbar_area.width) * it->position : x,
			horizontal ? y : y + (this->height - scrollbar_area.height) * it->position, 4, 4);
		RenderRectangle(renderer, rect, it->color, PGDrawStyleFill);
	}
	DrawScrollbar(renderer);
	Control::Draw(renderer);
}