#include "easy_map_canvas.h"
#include "core/input/input_event.h"
#include "editor/themes/editor_scale.h"

void EasyMapCanvas::set_dimensions(int p_columns, int p_rows) {
	columns = p_columns;
	rows = p_rows;
	set_custom_minimum_size(Vector2(columns, rows) * 32 * EDSCALE);
	queue_redraw();
}

void EasyMapCanvas::_paint(Vector2 p_position) {
	Vector2i cell = Vector2i((p_position / (32 * EDSCALE)).floor());
	if (cell.x < 0 || cell.y < 0 || cell.x >= columns || cell.y >= rows || cell == last_cell) { return; }
	last_cell = cell;
	if (brush == 0) { cells.erase(cell); }
	else {
		if (brush == 3) {
			Array keys = cells.keys();
			for (int i = 0; i < keys.size(); i++) { if (int(cells[keys[i]]) == 3) { cells.erase(keys[i]); } }
		}
		cells[cell] = brush;
	}
	queue_redraw();
}

void EasyMapCanvas::gui_input(const Ref<InputEvent> &p_event) {
	Ref<InputEventMouseButton> mouse = p_event;
	Ref<InputEventMouseMotion> motion = p_event;
	Ref<InputEventScreenTouch> touch = p_event;
	Ref<InputEventScreenDrag> drag = p_event;
	if (mouse.is_valid() && mouse->get_button_index() == MouseButton::LEFT) {
		drawing = mouse->is_pressed(); last_cell = Vector2i(-1, -1);
		if (drawing) { _paint(mouse->get_position()); }
		accept_event();
	} else if (motion.is_valid() && drawing) { _paint(motion->get_position()); accept_event(); }
	else if (touch.is_valid() && touch->get_index() == 0) {
		drawing = touch->is_pressed(); last_cell = Vector2i(-1, -1);
		if (drawing) { _paint(touch->get_position()); }
		accept_event();
	} else if (drag.is_valid() && drag->get_index() == 0 && drawing) { _paint(drag->get_position()); accept_event(); }
}

void EasyMapCanvas::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW) { return; }
	const float size = 32 * EDSCALE;
	draw_rect(Rect2(Vector2(), get_size()), Color(0.055, 0.07, 0.11));
	const Color colors[] = { Color(), Color(0.25, 0.65, 0.37), Color(0.4, 0.43, 0.5), Color(0.2, 0.6, 1), Color(1, 0.8, 0.15), Color(0.65, 0.36, 0.17) };
	Array keys = cells.keys();
	for (int i = 0; i < keys.size(); i++) {
		Vector2i cell = keys[i];
		int kind = cells[keys[i]];
		if (kind < 1 || kind > 5) { continue; }
		Vector2 pos = Vector2(cell) * size;
		if (kind == 4) { draw_circle(pos + Vector2(size, size) / 2, size * 0.28, colors[kind]); }
		else { draw_rect(Rect2(pos + Vector2(2, 2), Vector2(size - 4, size - 4)), colors[kind]); }
		if (kind == 3) { draw_circle(pos + Vector2(size * 0.65, size * 0.32), size * 0.1, Color(1, 1, 1)); }
	}
	for (int x = 0; x <= columns; x++) { draw_line(Vector2(x * size, 0), Vector2(x * size, rows * size), Color(0.2, 0.24, 0.3)); }
	for (int y = 0; y <= rows; y++) { draw_line(Vector2(0, y * size), Vector2(columns * size, y * size), Color(0.2, 0.24, 0.3)); }
}

EasyMapCanvas::EasyMapCanvas() {
	set_mouse_filter(MOUSE_FILTER_STOP);
	set_dimensions(24, 16);
}
