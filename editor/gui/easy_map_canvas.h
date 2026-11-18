#pragma once
#include "scene/gui/control.h"

class EasyMapCanvas : public Control {
	GDCLASS(EasyMapCanvas, Control);
	Dictionary cells;
	int brush = 1;
	int columns = 24;
	int rows = 16;
	Vector2i last_cell = Vector2i(-1, -1);
	bool drawing = false;
	void _paint(Vector2 p_position);
protected:
	static void _bind_methods() {}
	void _notification(int p_what);
	void gui_input(const Ref<InputEvent> &p_event) override;
public:
	void set_brush(int p_brush) { brush = p_brush; }
	void set_cells(const Dictionary &p_cells) { cells = p_cells.duplicate(true); queue_redraw(); }
	Dictionary get_cells() const { return cells.duplicate(true); }
	void set_dimensions(int p_columns, int p_rows);
	EasyMapCanvas();
};
