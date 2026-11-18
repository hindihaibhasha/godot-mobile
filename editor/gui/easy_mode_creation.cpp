#include "easy_mode_bar.h"
#include "easy_logic_editor.h"
#include "easy_map_canvas.h"
#include "editor/editor_node.h"
#include "editor/editor_data.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/polygon_2d.h"
#include "scene/2d/physics/static_body_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/label.h"
#include "scene/gui/option_button.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/material.h"

void EasyModeBar::_open_blocks() {
	Node *root = _get_root();
	if (!root) { _show_message(TTR("Create a scene first using New 2D or New 3D.")); return; }
	List<Node *> selected = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
	Node *node = selected.is_empty() ? root : selected.front()->get();
	if (node->has_meta("easy_logic_program")) { node = node->get_parent(); }
	logic_editor->open_for(node);
}

void EasyModeBar::_open_map() {
	if (!_get_root()) { _show_message(TTR("Tap New 2D or New 3D first, then Map.")); return; }
	map_root = _get_root()->get_instance_id();
	Dictionary cells;
	Node *map = nullptr;
	for (int i = 0; i < _get_root()->get_child_count(); i++) {
		if (_get_root()->get_child(i)->has_meta("easy_map_cells")) { map = _get_root()->get_child(i); break; }
	}
	map_cell_size->set_value(_is_3d_scene() ? 2 : 64);
	map_mode->select(_is_3d_scene() ? 1 : 0);
	if (map) {
		cells = map->get_meta("easy_map_cells");
		map_cell_size->set_value(map->get_meta("easy_map_size", _is_3d_scene() ? 2 : 64));
		map_mode->select(map->get_meta("easy_map_mode", 0));
	}
	map_canvas->set_cells(cells);
	map_dialog->set_title(_is_3d_scene() ? TTR("3D map - top view, X / Z") : TTR("2D map - paint your level"));
	map_dialog->popup_centered_clamped(Size2(820, 670) * EDSCALE, 0.96);
}

void EasyModeBar::_map_brush(int p_index) { map_canvas->set_brush(p_index); }

void EasyModeBar::_map_preset(int p_index) {
	Dictionary cells;
	if (p_index == 0) {
		for (int x = 0; x < 24; x++) { cells[Vector2i(x, 14)] = 1; }
		for (int x = 5; x < 10; x++) { cells[Vector2i(x, 10)] = 1; }
		cells[Vector2i(3, 11)] = 3;
		for (int x = 5; x < 10; x++) { cells[Vector2i(x, 8)] = 4; }
		map_mode->select(0);
	} else {
		for (int x = 1; x < 23; x++) {
			for (int y = 1; y < 15; y++) {
				if (_is_3d_scene()) { cells[Vector2i(x, y)] = 1; }
				if (x == 1 || x == 22 || y == 1 || y == 14) { cells[Vector2i(x, y)] = 2; }
			}
		}
		cells[Vector2i(4, 5)] = 3;
		cells[Vector2i(8, 5)] = 4;
		cells[Vector2i(10, 5)] = 5;
		map_mode->select(1);
	}
	map_canvas->set_cells(cells);
}

void EasyModeBar::_apply_map() {
	Node *root = _get_root();
	if (!root || root->get_instance_id() != map_root) { _show_message(TTR("Scene changed. Reopen Map.")); return; }
	Dictionary cells = map_canvas->get_cells();
	const bool spatial = _is_3d_scene();
	const real_t size = map_cell_size->get_value();
	Node *map = spatial ? static_cast<Node *>(memnew(Node3D)) : static_cast<Node *>(memnew(Node2D));
	map->set_name("PaintedMap");
	map->set_meta("easy_map_cells", cells);
	map->set_meta("easy_map_size", size);
	map->set_meta("easy_map_mode", map_mode->get_selected());
	Array keys = cells.keys();
	for (int i = 0; i < keys.size() * 2; i++) {
		Vector2i cell = keys[i / 2];
		int brush = cells[keys[i / 2]];
		// Markers also need a supporting floor in the 3D top-view map.
		if (i % 2 == 0) {
			if (!spatial || brush < 3) { continue; }
			brush = 1;
		}
		Vector2 xy = (Vector2(cell) + Vector2(0.5, 0.5)) * size;
		Node *object = nullptr;
		if (brush == 1 || brush == 2) {
			Color color = brush == 1 ? Color(0.25, 0.65, 0.37) : Color(0.4, 0.43, 0.5);
			if (spatial) {
				StaticBody3D *body = memnew(StaticBody3D);
				Vector3 dimensions(size, brush == 1 ? size * 0.2 : size * 2, size);
				body->set_position(Vector3(xy.x, brush == 1 ? -dimensions.y / 2 : dimensions.y / 2, xy.y));
				Ref<BoxMesh> mesh; mesh.instantiate(); mesh->set_size(dimensions);
				Ref<StandardMaterial3D> material; material.instantiate(); material->set_albedo(color); mesh->set_material(material);
				MeshInstance3D *look = memnew(MeshInstance3D); look->set_mesh(mesh); body->add_child(look);
				Ref<BoxShape3D> shape; shape.instantiate(); shape->set_size(dimensions);
				CollisionShape3D *collision = memnew(CollisionShape3D); collision->set_shape(shape); body->add_child(collision);
				object = body;
			} else {
				StaticBody2D *body = memnew(StaticBody2D); body->set_position(xy);
				Polygon2D *look = memnew(Polygon2D);
				Vector<Vector2> points; real_t h = size / 2;
				points.push_back(Vector2(-h, -h)); points.push_back(Vector2(h, -h)); points.push_back(Vector2(h, h)); points.push_back(Vector2(-h, h));
				look->set_polygon(points); look->set_color(color); body->add_child(look);
				Ref<RectangleShape2D> shape; shape.instantiate(); shape->set_size(Vector2(size, size));
				CollisionShape2D *collision = memnew(CollisionShape2D); collision->set_shape(shape); body->add_child(collision);
				object = body;
			}
			object->set_name(brush == 1 ? "Ground" : "Wall");
		} else if (brush >= 3 && brush <= 5) {
			int kind = spatial ? (brush == 3 ? OBJ_3D_PLAYER : brush == 4 ? OBJ_3D_COIN : OBJ_3D_CUBE) : (brush == 3 ? (map_mode->get_selected() == 0 ? OBJ_2D_PLAYER_PLATFORMER : OBJ_2D_PLAYER_TOPDOWN) : brush == 4 ? OBJ_2D_COIN : OBJ_2D_BOX);
			String action;
			object = _build_object(kind, root, action);
			if (spatial) { Object::cast_to<Node3D>(object)->set_position(Vector3(xy.x, size, xy.y)); }
			else { Object::cast_to<Node2D>(object)->set_position(xy); }
		}
		if (object) { map->add_child(object, true); }
	}
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(TTR("Paint Map"), UndoRedo::MERGE_DISABLE, root);
	for (int i = 0; i < root->get_child_count(); i++) {
		Node *old = root->get_child(i);
		if (old->has_meta("easy_map_cells")) {
			ur->add_do_method(root, "remove_child", old);
			ur->add_undo_method(root, "add_child", old, true);
			ur->add_undo_method(root, "move_child", old, i);
			List<Node *> pending; pending.push_back(old);
			while (!pending.is_empty()) {
				Node *n = pending.front()->get(); pending.pop_front();
				if (n->get_owner() == root) { ur->add_undo_method(n, "set_owner", root); }
				for (int j = 0; j < n->get_child_count(); j++) { pending.push_back(n->get_child(j)); }
			}
			ur->add_undo_reference(old);
		}
	}
	ur->add_do_method(root, "add_child", map, true);
	_set_owner_recursive(map, root, ur);
	ur->add_do_reference(map);
	ur->add_undo_method(root, "remove_child", map);
	ur->commit_action();
	EditorNode::get_singleton()->get_editor_selection()->clear();
	EditorNode::get_singleton()->get_editor_selection()->add_node(map);
	_back_to_scene();
	_update_state();
}

void EasyModeBar::_duplicate_selected() {
	Node *root = _get_root();
	if (!root) { return; }
	List<Node *> selected = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
	if (selected.is_empty()) { _show_message(TTR("Choose an object in Objects first.")); return; }
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(TTR("Duplicate Objects"), UndoRedo::MERGE_DISABLE, root);
	for (Node *original : selected) {
		if (original == root || !root->is_ancestor_of(original)) { continue; }
		Node *copy = original->duplicate();
		if (!copy) { continue; }
		if (Node2D *n = Object::cast_to<Node2D>(copy)) { n->set_position(n->get_position() + Vector2(64, 0)); }
		if (Node3D *n = Object::cast_to<Node3D>(copy)) { n->set_position(n->get_position() + Vector3(2, 0, 0)); }
		ur->add_do_method(original->get_parent(), "add_child", copy, true);
		_set_owner_recursive(copy, root, ur);
		ur->add_do_reference(copy);
		ur->add_undo_method(original->get_parent(), "remove_child", copy);
	}
	ur->commit_action();
}

void EasyModeBar::_init_creation_tools() {
	logic_editor = memnew(EasyLogicEditor);
	add_child(logic_editor);
	map_dialog = memnew(ConfirmationDialog);
	map_dialog->set_ok_button_text(TTR("Build map"));
	map_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EasyModeBar::_apply_map));
	add_child(map_dialog);
	VBoxContainer *vb = memnew(VBoxContainer);
	map_dialog->add_child(vb);
	Label *help = memnew(Label);
	help->set_text(TTR("Paint cells with your finger. Use scrollbars to pan. Green = ground, grey = wall, blue = player, yellow = coin. Build replaces only PaintedMap; Undo restores it. Rebuilding resets manual edits inside PaintedMap."));
	help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	vb->add_child(help);
	HBoxContainer *tools = memnew(HBoxContainer);
	vb->add_child(tools);
	OptionButton *brush = memnew(OptionButton);
	const char *brushes[] = { "Eraser", "Ground", "Wall", "Player", "Coin", "Box" };
	for (const char *name : brushes) { brush->add_item(TTR(name)); }
	brush->select(1);
	brush->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
	brush->connect(SceneStringName(item_selected), callable_mp(this, &EasyModeBar::_map_brush));
	tools->add_child(brush);
	map_mode = memnew(OptionButton);
	map_mode->add_item(TTR("Platformer")); map_mode->add_item(TTR("Top-down / 3D"));
	map_mode->set_fit_to_longest_item(false);
	map_mode->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	tools->add_child(map_mode);
	map_cell_size = memnew(SpinBox);
	map_cell_size->set_min(0.5); map_cell_size->set_max(256); map_cell_size->set_step(0.5);
	map_cell_size->set_tooltip_text(TTR("World units per cell: normally 64 pixels in 2D or 2 meters in 3D."));
	tools->add_child(map_cell_size);
	HBoxContainer *presets = memnew(HBoxContainer);
	vb->add_child(presets);
	for (int i = 0; i < 2; i++) {
		Button *b = memnew(Button);
		b->set_text(i == 0 ? TTR("Platform layout") : TTR("Room layout"));
		b->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
		b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		b->connect(SceneStringName(pressed), callable_mp(this, &EasyModeBar::_map_preset).bind(i));
		presets->add_child(b);
	}
	ScrollContainer *scroll = memnew(ScrollContainer);
	scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_ALWAYS);
	scroll->set_vertical_scroll_mode(ScrollContainer::SCROLL_MODE_SHOW_ALWAYS);
	vb->add_child(scroll);
	map_canvas = memnew(EasyMapCanvas);
	scroll->add_child(map_canvas);
}
