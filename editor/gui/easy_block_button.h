#pragma once
#include "scene/gui/button.h"

// Puzzle-shaped command block, with a top socket and bottom connector.
class EasyBlockButton : public Button {
	GDCLASS(EasyBlockButton, Button);
	Color block_color = Color(0.15, 0.4, 0.65);
	bool event = false;
protected:
	static void _bind_methods() {}
	void _notification(int p_what);
public:
	void setup(const Color &p_color, bool p_event);
};
