#pragma once

#include "scrollbar.h"

struct PGScrollbarDecoration {
	double position;
	PGColor color;

	PGScrollbarDecoration(double position, PGColor color) : position(position), color(color) { }

	friend bool operator<(const PGScrollbarDecoration& l, const PGScrollbarDecoration& r) {
		return l.position < r.position;
	}
	friend bool operator> (const PGScrollbarDecoration& lhs, const PGScrollbarDecoration& rhs) { return rhs < lhs; }
	friend bool operator<=(const PGScrollbarDecoration& lhs, const PGScrollbarDecoration& rhs) { return !(lhs > rhs); }
	friend bool operator>=(const PGScrollbarDecoration& lhs, const PGScrollbarDecoration& rhs) { return !(lhs < rhs); }
	friend bool operator==(const PGScrollbarDecoration& lhs, const PGScrollbarDecoration& rhs) { return lhs.position == rhs.position; }
	friend bool operator!=(const PGScrollbarDecoration& lhs, const PGScrollbarDecoration& rhs) { return !(lhs == rhs); }
};

class DecoratedScrollbar : public Scrollbar {
public:
	DecoratedScrollbar(std::shared_ptr<Control> parent, PGWindowHandle window, bool horizontal, bool arrows);
	DecoratedScrollbar(Control* parent, PGWindowHandle window, bool horizontal, bool arrows) :
		DecoratedScrollbar(parent->shared_from_this(), window, horizontal, arrows) { }

	virtual void Draw(PGRendererHandle renderer);

	std::vector<PGScrollbarDecoration> center_decorations;
};