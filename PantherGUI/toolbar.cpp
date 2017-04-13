
#include "toolbar.h"


PGToolbar::PGToolbar(PGWindowHandle window) : 
	PGContainer(window) {

}

PGToolbar::~PGToolbar() {

}

void PGToolbar::Draw(PGRendererHandle renderer) {
	RenderRectangle(renderer, this->GetRectangle(), PGColor(30, 30, 30), PGDrawStyleFill, 1.0f);
	PGContainer::Draw(renderer);
}
