/**************************************************************************/
/*  easy_mode_bar.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "easy_mode_bar.h"

#include "core/config/engine.h"
#include "core/config/project_settings.h"
#include "core/io/dir_access.h"
#include "core/io/file_access.h"
#include "core/io/resource_loader.h"
#include "core/object/callable_mp.h"
#include "core/object/script_language.h"
#include "editor/docks/editor_dock.h"
#include "editor/docks/editor_dock_manager.h"
#include "editor/docks/scene_tree_dock.h"
#include "editor/editor_data.h"
#include "editor/editor_interface.h"
#include "editor/editor_main_screen.h"
#include "editor/editor_node.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/file_system/editor_file_system.h"
#include "editor/run/editor_run_bar.h"
#include "editor/scene/canvas_item_editor_plugin.h"
#include "editor/settings/editor_settings.h"
#include "editor/themes/editor_scale.h"
#include "scene/2d/camera_2d.h"
#include "scene/2d/physics/area_2d.h"
#include "scene/2d/physics/character_body_2d.h"
#include "scene/2d/physics/collision_shape_2d.h"
#include "scene/2d/physics/rigid_body_2d.h"
#include "scene/2d/physics/static_body_2d.h"
#include "scene/2d/polygon_2d.h"
#include "scene/2d/sprite_2d.h"
#include "scene/3d/camera_3d.h"
#include "scene/3d/label_3d.h"
#include "scene/3d/light_3d.h"
#include "scene/3d/mesh_instance_3d.h"
#include "scene/3d/physics/area_3d.h"
#include "scene/3d/physics/character_body_3d.h"
#include "scene/3d/physics/collision_shape_3d.h"
#include "scene/3d/physics/rigid_body_3d.h"
#include "scene/3d/physics/static_body_3d.h"
#include "scene/3d/world_environment.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/dialogs.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/label.h"
#include "scene/gui/popup.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/separator.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/2d/circle_shape_2d.h"
#include "scene/resources/2d/rectangle_shape_2d.h"
#include "scene/resources/3d/box_shape_3d.h"
#include "scene/resources/3d/capsule_shape_3d.h"
#include "scene/resources/3d/primitive_meshes.h"
#include "scene/resources/3d/sky_material.h"
#include "scene/resources/3d/sphere_shape_3d.h"
#include "scene/resources/environment.h"
#include "scene/resources/material.h"
#include "scene/resources/sky.h"
#include "scene/resources/style_box_flat.h"
#include "servers/display/display_server.h"

// ---------------------------------------------------------------------------
// Ready-made scripts. They are written to `res://scripts/` the first time they
// are needed, so beginners can open and change them like any other script.
// ---------------------------------------------------------------------------

static const char *SCRIPT_PLAYER_2D_PLATFORMER = R"GD(extends CharacterBody2D
# EASY MODE - Platformer player (side view, like Mario).
#
# PHONE:    touch the LEFT part of the screen  = walk left
#           touch the RIGHT part of the screen = walk right
#           touch the MIDDLE of the screen     = jump
# KEYBOARD: arrow keys to walk, Space or Up to jump.
#
# Try changing these numbers and press Play again!

@export var speed := 350.0
@export var jump_power := 650.0
@export var gravity := 1600.0

# All fingers that are touching the screen right now.
var touches := {}


func _input(event):
	if event is InputEventScreenTouch:
		if event.pressed:
			touches[event.index] = event.position
		else:
			touches.erase(event.index)
	elif event is InputEventScreenDrag:
		touches[event.index] = event.position


func _physics_process(delta):
	# 1) Fall down when we are in the air.
	if not is_on_floor():
		velocity.y += gravity * delta

	# 2) Read the keyboard.
	var direction := Input.get_axis("ui_left", "ui_right")
	var want_jump := Input.is_action_just_pressed("ui_accept") or Input.is_action_just_pressed("ui_up")

	# 3) Read the fingers.
	var screen_width := get_viewport().get_visible_rect().size.x
	for pos in touches.values():
		if pos.x < screen_width / 3.0:
			direction = -1.0
		elif pos.x > screen_width * 2.0 / 3.0:
			direction = 1.0
		else:
			want_jump = true

	# 4) Jump and walk.
	if want_jump and is_on_floor():
		velocity.y = -jump_power
	velocity.x = direction * speed

	move_and_slide()
)GD";

static const char *SCRIPT_PLAYER_2D_TOPDOWN = R"GD(extends CharacterBody2D
# EASY MODE - Top-down player (seen from above, like Pokemon).
#
# PHONE:    touch and hold anywhere = the player walks to your finger
# KEYBOARD: arrow keys.
#
# Try changing the speed and press Play again!

@export var speed := 300.0

var finger = null # Where the finger is (or null when nobody touches).


func _input(event):
	if event is InputEventScreenTouch:
		finger = event.position if event.pressed else null
	elif event is InputEventScreenDrag:
		finger = event.position


func _physics_process(_delta):
	var direction := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")

	if finger != null:
		# Convert the screen position into a position in the game world.
		var target: Vector2 = get_canvas_transform().affine_inverse() * finger
		var to_target := target - global_position
		if to_target.length() > 10.0:
			direction = to_target.normalized()

	velocity = direction * speed
	move_and_slide()
)GD";

static const char *SCRIPT_COIN_2D = R"GD(extends Area2D
# EASY MODE - Coin. It spins, and disappears when the player touches it.


func _ready():
	body_entered.connect(_on_body_entered)


func _process(delta):
	rotation += 3.0 * delta


func _on_body_entered(body):
	if body is CharacterBody2D:
		print("Coin collected!")
		queue_free()
)GD";

static const char *SCRIPT_PLAYER_3D = R"GD(extends CharacterBody3D
# EASY MODE - 3D player.
#
# PHONE:    put a finger on the LEFT half of the screen and slide it = walk
#           (it works like a joystick)
#           tap the RIGHT half of the screen = jump
# KEYBOARD: arrow keys to walk, Space to jump.
#
# Try changing these numbers and press Play again!

@export var speed := 6.0
@export var jump_power := 6.0
@export var gravity := 15.0

var stick_finger := -1 # Which finger is used as joystick (-1 = none).
var stick_start := Vector2.ZERO
var stick := Vector2.ZERO
var jump_request := false


func _input(event):
	var half := get_viewport().get_visible_rect().size.x / 2.0
	if event is InputEventScreenTouch:
		if event.pressed:
			if event.position.x < half and stick_finger == -1:
				stick_finger = event.index
				stick_start = event.position
				stick = Vector2.ZERO
			elif event.position.x >= half:
				jump_request = true
		elif event.index == stick_finger:
			stick_finger = -1
			stick = Vector2.ZERO
	elif event is InputEventScreenDrag and event.index == stick_finger:
		stick = (event.position - stick_start).limit_length(80.0) / 80.0


func _physics_process(delta):
	# 1) Fall down when we are in the air.
	if not is_on_floor():
		velocity.y -= gravity * delta

	# 2) Jump.
	if (Input.is_action_just_pressed("ui_accept") or jump_request) and is_on_floor():
		velocity.y = jump_power
	jump_request = false

	# 3) Walk (keyboard or finger joystick).
	var move := Input.get_vector("ui_left", "ui_right", "ui_up", "ui_down")
	if stick.length() > 0.15:
		move = stick
	velocity.x = move.x * speed
	velocity.z = move.y * speed

	move_and_slide()
)GD";

static const char *SCRIPT_COIN_3D = R"GD(extends Area3D
# EASY MODE - 3D coin. It spins, and disappears when the player touches it.


func _ready():
	body_entered.connect(_on_body_entered)


func _process(delta):
	rotate_y(3.0 * delta)


func _on_body_entered(body):
	if body is CharacterBody3D:
		print("Coin collected!")
		queue_free()
)GD";

// ---------------------------------------------------------------------------
// Small helpers to build nodes.
// ---------------------------------------------------------------------------

static Polygon2D *_make_rect_polygon(const Size2 &p_size, const Color &p_color) {
	Polygon2D *poly = memnew(Polygon2D);
	poly->set_name("Look");
	Vector<Vector2> points;
	const Vector2 h = p_size / 2;
	points.push_back(Vector2(-h.x, -h.y));
	points.push_back(Vector2(h.x, -h.y));
	points.push_back(Vector2(h.x, h.y));
	points.push_back(Vector2(-h.x, h.y));
	poly->set_polygon(points);
	poly->set_color(p_color);
	return poly;
}

static Polygon2D *_make_circle_polygon(float p_radius, const Color &p_color) {
	Polygon2D *poly = memnew(Polygon2D);
	poly->set_name("Look");
	Vector<Vector2> points;
	const int segments = 16;
	for (int i = 0; i < segments; i++) {
		const float a = Math::TAU * i / segments;
		points.push_back(Vector2(Math::cos(a), Math::sin(a)) * p_radius);
	}
	poly->set_polygon(points);
	poly->set_color(p_color);
	return poly;
}

static CollisionShape2D *_make_rect_collision_2d(const Size2 &p_size) {
	CollisionShape2D *cs = memnew(CollisionShape2D);
	cs->set_name("Collision");
	Ref<RectangleShape2D> shape;
	shape.instantiate();
	shape->set_size(p_size);
	cs->set_shape(shape);
	return cs;
}

static Ref<StandardMaterial3D> _make_material(const Color &p_color) {
	Ref<StandardMaterial3D> mat;
	mat.instantiate();
	mat->set_albedo(p_color);
	return mat;
}

static MeshInstance3D *_make_mesh(const Ref<PrimitiveMesh> &p_mesh, const Color &p_color) {
	p_mesh->set_material(_make_material(p_color));
	MeshInstance3D *mi = memnew(MeshInstance3D);
	mi->set_name("Look");
	mi->set_mesh(p_mesh);
	return mi;
}

static CollisionShape3D *_make_collision_3d(const Ref<Shape3D> &p_shape) {
	CollisionShape3D *cs = memnew(CollisionShape3D);
	cs->set_name("Collision");
	cs->set_shape(p_shape);
	return cs;
}

static String _to_file_name(const String &p_name) {
	String s = p_name.to_snake_case().validate_filename().replace(" ", "_");
	if (s.is_empty()) {
		s = "node";
	}
	return s;
}

static String _unique_path(const String &p_dir, const String &p_base, const String &p_ext) {
	String path = p_dir.path_join(p_base + "." + p_ext);
	int i = 2;
	while (FileAccess::exists(path)) {
		path = p_dir.path_join(p_base + "_" + itos(i) + "." + p_ext);
		i++;
	}
	return path;
}

// ---------------------------------------------------------------------------
// EasyModeBar
// ---------------------------------------------------------------------------

bool EasyModeBar::is_enabled_by_default() {
	return DisplayServer::get_singleton()->is_touchscreen_available();
}

Button *EasyModeBar::_make_button(const String &p_text, const StringName &p_icon, const Callable &p_callable) {
	Button *b = memnew(Button);
	b->set_text(p_text);
	b->set_meta("easy_icon", p_icon);
	b->set_focus_mode(FOCUS_NONE);
	b->set_icon_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	b->set_vertical_icon_alignment(VERTICAL_ALIGNMENT_TOP);
	b->set_expand_icon(false);
	b->connect(SceneStringName(pressed), p_callable);
	b->set_h_size_flags(SIZE_EXPAND_FILL);
	b->set_clip_text(true);
	buttons_box->add_child(b);
	action_buttons.push_back(b);
	return b;
}

void EasyModeBar::_apply_sizes() {
	// Five equal-width actions fit without sideways scrolling on phones.
	const Size2 button_size = Size2(48, 56) * EDSCALE;
	for (Button *b : action_buttons) {
		b->set_custom_minimum_size(button_size);
		for (const String &state : { String("normal"), String("hover"), String("pressed") }) {
			Ref<StyleBoxFlat> tile;
			tile.instantiate();
			Color color = get_theme_color(SNAME("dark_color_2"), SNAME("Editor"));
			if (b == play_button || state == "pressed") {
				color = get_theme_color(SNAME("accent_color"), SNAME("Editor")).darkened(0.55);
			} else if (state == "hover") {
				color = color.lightened(0.12);
			}
			tile->set_bg_color(color);
			tile->set_corner_radius_all(10 * EDSCALE);
			tile->set_content_margin_all(6 * EDSCALE);
			b->add_theme_style_override(state, tile);
		}
		b->add_theme_font_size_override(SceneStringName(font_size), 13 * EDSCALE);
		b->add_theme_constant_override("icon_max_width", 22 * EDSCALE);
		if (b->has_meta("easy_icon")) {
			b->set_button_icon(get_editor_theme_icon(b->get_meta("easy_icon")));
		}
	}
	show_button->set_custom_minimum_size(Size2(0, 40) * EDSCALE);
	show_button->set_button_icon(get_editor_theme_icon(SNAME("GuiVisibilityVisible")));
	hint_label->add_theme_font_size_override(SceneStringName(font_size), 14 * EDSCALE);
}

void EasyModeBar::_sync_layout() {
	const bool enabled = is_visible() && !collapsed;
	if (enabled != mobile_layout) {
		mobile_layout = enabled;
		EditorNode::get_singleton()->set_easy_workspace(enabled);
	}
}

void EasyModeBar::_open_more() {
	more_popup->popup_centered_clamped(Size2(360, 360) * EDSCALE, 0.92);
}

void EasyModeBar::_back_to_scene() {
	_focus_main_screen(_is_3d_scene() ? "3D" : "2D");
}

void EasyModeBar::_append_objects(Node *p_node, int p_depth) {
	Button *b = memnew(Button);
	b->set_text(String("  ").repeat(MIN(p_depth, 4)) + String(p_node->get_name()));
	b->set_clip_text(true);
	b->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	b->set_custom_minimum_size(Size2(0, 52) * EDSCALE);
	b->add_theme_font_size_override(SceneStringName(font_size), 16 * EDSCALE);
	b->connect(SceneStringName(pressed), callable_mp(this, &EasyModeBar::_select_object).bind(p_node->get_instance_id()));
	objects_list->add_child(b);
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		if (child->get_owner() == _get_root()) {
			_append_objects(child, p_depth + 1);
		}
	}
}

void EasyModeBar::_open_objects() {
	for (int i = objects_list->get_child_count() - 1; i >= 0; i--) {
		memdelete(objects_list->get_child(i));
	}
	if (_get_root()) {
		_append_objects(_get_root(), 0);
	} else {
		_show_message(TTR("Tap More, then New 2D or New 3D to create your first scene."));
		return;
	}
	objects_popup->popup_centered_clamped(Size2(360, 460) * EDSCALE, 0.92);
}

void EasyModeBar::_select_object(ObjectID p_id) {
	Node *node = Object::cast_to<Node>(ObjectDB::get_instance(p_id));
	if (!node || !_get_root() || (node != _get_root() && !_get_root()->is_ancestor_of(node))) {
		return;
	}
	EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
	selection->clear();
	selection->add_node(node);
	EditorNode::get_singleton()->edit_node(node);
	objects_popup->hide();
	_back_to_scene();
	_update_state();
}

void EasyModeBar::_open_properties() {
	List<Node *> selected = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
	if (selected.is_empty()) {
		_show_message(TTR("Choose an object from Objects first, then tap Edit."));
		return;
	}
	Node *node = selected.front()->get();
	Node2D *n2 = Object::cast_to<Node2D>(node);
	Node3D *n3 = Object::cast_to<Node3D>(node);
	Control *control = Object::cast_to<Control>(node);
	if (!n2 && !n3 && !control) {
		_show_message(TTR("This object has no position or size. Use More > Full editor for advanced settings."));
		return;
	}
	Vector3 pos;
	Vector3 scale;
	if (n3) {
		pos = n3->get_position();
		scale = n3->get_scale();
	} else {
		Vector2 p = n2 ? n2->get_position() : control->get_position();
		Vector2 s = n2 ? n2->get_scale() : control->get_scale();
		pos = Vector3(p.x, p.y, 0);
		scale = Vector3(s.x, s.y, 1);
	}
	property_target = node->get_instance_id();
	property_root = _get_root()->get_instance_id();
	properties_dialog->set_title(vformat(TTR("Edit %s"), node->get_name()));
	for (int i = 0; i < 3; i++) {
		position_fields[i]->set_value(pos[i]);
		scale_fields[i]->set_value(scale[i]);
		Object::cast_to<Control>(position_fields[i]->get_parent())->set_visible(i < 2 || n3);
	}
	properties_dialog->popup_centered_clamped(Size2(360, 320) * EDSCALE, 0.92);
}

void EasyModeBar::_apply_properties() {
	Node *node = Object::cast_to<Node>(ObjectDB::get_instance(property_target));
	Node *root = _get_root();
	if (!node || !root || root->get_instance_id() != property_root || (node != root && !root->is_ancestor_of(node))) {
		return;
	}
	Vector3 pos;
	Vector3 scale;
	for (int i = 0; i < 3; i++) {
		pos[i] = position_fields[i]->get_value();
		scale[i] = scale_fields[i]->get_value();
	}
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(TTR("Edit Object"), UndoRedo::MERGE_DISABLE, node);
	ur->add_undo_property(node, "position", node->get("position"));
	ur->add_undo_property(node, "scale", node->get("scale"));
	if (Object::cast_to<Node3D>(node)) {
		ur->add_do_property(node, "position", pos);
		ur->add_do_property(node, "scale", scale);
	} else {
		ur->add_do_property(node, "position", Vector2(pos.x, pos.y));
		ur->add_do_property(node, "scale", Vector2(scale.x, scale.y));
	}
	ur->commit_action();
}

void EasyModeBar::_request_delete() {
	delete_dialog->popup_centered_clamped(Size2(320, 160) * EDSCALE, 0.9);
}

bool EasyModeBar::_is_3d_scene() const {
	return Object::cast_to<Node3D>(_get_root()) != nullptr;
}

Node *EasyModeBar::_get_root() const {
	return EditorNode::get_singleton()->get_edited_scene();
}

Node *EasyModeBar::_get_parent_for_new_object() const {
	// Keep things simple for beginners: new objects always go into the main
	// (root) node of the scene, so the scene tree stays flat and easy to read.
	return _get_root();
}

void EasyModeBar::_show_message(const String &p_text) {
	help_dialog->hide();
	message_dialog->set_text(p_text);
	message_dialog->popup_centered_clamped(Size2(360, 0) * EDSCALE, 0.9);
}

void EasyModeBar::_focus_main_screen(const String &p_name) {
	EditorDock *dock = EditorNode::get_singleton()->get_editor_main_screen()->get_dock_by_name(p_name);
	if (dock) {
		EditorDockManager::get_singleton()->force_focus_dock(dock);
	}
}

void EasyModeBar::_update_state() {
	if (collapsed) {
		return;
	}
	Node *root = _get_root();
	const bool has_scene = root != nullptr;
	add_button->set_disabled(!has_scene);
	code_button->set_disabled(!has_scene);
	const bool has_selection = !EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list().is_empty();
	delete_button->set_disabled(!has_selection);
	properties_button->set_disabled(!has_selection);
	objects_button->set_disabled(!has_scene);
	save_button->set_disabled(!has_scene);
	const bool playing = EditorRunBar::get_singleton()->is_playing();
	play_button->set_disabled(!has_scene && !playing);
	play_button->set_text(playing ? TTR("Stop") : TTR("Play"));
	play_button->set_button_icon(get_editor_theme_icon(playing ? SNAME("Stop") : SNAME("Play")));

	bool has_player = false;
	if (has_scene) {
		for (int i = 0; i < root->get_child_count(); i++) {
			Node *c = root->get_child(i);
			if (Object::cast_to<CharacterBody2D>(c) || Object::cast_to<CharacterBody3D>(c)) {
				has_player = true;
				break;
			}
		}
	}

	String hint;
	if (!has_scene) {
		hint = TTR("START HERE: More > New 2D or New 3D");
	} else if (!has_player) {
		hint = TTR("Step 2: Tap \"Add\" and pick a Player. Then add Ground, Boxes and Coins.");
	} else if (root->get_scene_file_path().is_empty()) {
		hint = TTR("Step 3: Tap \"Play\" to try your game! (It is saved automatically.)");
	} else {
		List<Node *> selection = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
		hint = selection.is_empty() ? TTR("Objects: select  |  More: edit, code, save") : vformat(TTR("%s selected  |  More > Edit or Code"), selection.front()->get()->get_name());
	}
	if (hint_label->get_text() != hint) {
		hint_label->set_text(hint);
	}
}

void EasyModeBar::_set_collapsed(bool p_collapsed) {
	collapsed = p_collapsed;
	main_vb->set_visible(!collapsed);
	show_button->set_visible(collapsed);
	_sync_layout();
	if (!collapsed) {
		_update_state();
	}
}

// ----- Actions -------------------------------------------------------------

void EasyModeBar::_new_game(bool p_3d) {
	EditorNode *en = EditorNode::get_singleton();
	if (_get_root()) {
		// Open a fresh, empty tab so we never overwrite the current scene.
		en->new_scene();
	}

	Node *root = nullptr;
	if (p_3d) {
		Node3D *world = memnew(Node3D);
		world->set_name("World");
		root = world;

		// Sky + sun so it looks nice right away.
		WorldEnvironment *we = memnew(WorldEnvironment);
		we->set_name("Sky");
		Ref<Environment> env;
		env.instantiate();
		env->set_background(Environment::BG_SKY);
		Ref<Sky> sky;
		sky.instantiate();
		Ref<ProceduralSkyMaterial> sky_mat;
		sky_mat.instantiate();
		sky->set_material(sky_mat);
		env->set_sky(sky);
		env->set_tonemapper(Environment::TONE_MAPPER_FILMIC);
		we->set_environment(env);
		world->add_child(we);

		DirectionalLight3D *sun = memnew(DirectionalLight3D);
		sun->set_name("Sun");
		sun->set_rotation_degrees(Vector3(-50, -30, 0));
		sun->set_shadow(true);
		world->add_child(sun);

		Camera3D *cam = memnew(Camera3D);
		cam->set_name("Camera");
		cam->set_position(Vector3(0, 6, 10));
		cam->set_rotation_degrees(Vector3(-30, 0, 0));
		world->add_child(cam);
	} else {
		Node2D *world = memnew(Node2D);
		world->set_name("World");
		root = world;
	}

	// Children must be owned by the root so they are saved with the scene.
	for (int i = 0; i < root->get_child_count(); i++) {
		root->get_child(i)->set_owner(root);
	}

	SceneTreeDock::get_singleton()->add_root_node(root);
	en->edit_node(root);
	_focus_main_screen(p_3d ? "3D" : "2D");

	// A game without ground is not much fun: add one right away.
	_add_object(p_3d ? OBJ_3D_FLOOR : OBJ_2D_GROUND);
	if (!p_3d && CanvasItemEditor::get_singleton()) {
		// Zoom so the whole game screen (and the ground) is visible.
		callable_mp(CanvasItemEditor::get_singleton(), &CanvasItemEditor::frame_selection).call_deferred();
	}
	_update_state();
}

void EasyModeBar::_open_add_popup() {
	Node *root = _get_root();
	if (!root) {
		_show_message(TTR("First tap \"New 2D\" or \"New 3D\" to start a game."));
		return;
	}

	for (int i = add_list->get_child_count() - 1; i >= 0; i--) {
		Node *c = add_list->get_child(i);
		add_list->remove_child(c);
		memdelete(c);
	}

	struct Item {
		int kind;
		const char *text;
		const char *desc;
		const char *icon;
	};

	static const Item items_2d[] = {
		{ OBJ_2D_PLAYER_PLATFORMER, TTRC("Player (jumping game)"), TTRC("Walks and jumps. Ready to play!"), "CharacterBody2D" },
		{ OBJ_2D_PLAYER_TOPDOWN, TTRC("Player (top-down game)"), TTRC("Walks in all directions, seen from above."), "CharacterBody2D" },
		{ OBJ_2D_GROUND, TTRC("Ground / Wall"), TTRC("A solid block the player can stand on."), "StaticBody2D" },
		{ OBJ_2D_BOX, TTRC("Box (falls down)"), TTRC("A box with physics. It falls and can be pushed."), "RigidBody2D" },
		{ OBJ_2D_COIN, TTRC("Coin"), TTRC("Disappears when the player touches it."), "Area2D" },
		{ OBJ_2D_TEXT, TTRC("Text"), TTRC("Shows a message on the screen."), "Label" },
		{ OBJ_2D_PICTURE, TTRC("Picture"), TTRC("Shows an image (the Godot icon by default)."), "Sprite2D" },
		{ OBJ_2D_CAMERA, TTRC("Camera"), TTRC("Decides what part of the world is shown."), "Camera2D" },
	};
	static const Item items_3d[] = {
		{ OBJ_3D_PLAYER, TTRC("Player"), TTRC("Walks and jumps, with a camera. Ready to play!"), "CharacterBody3D" },
		{ OBJ_3D_FLOOR, TTRC("Floor"), TTRC("A big flat ground to walk on."), "StaticBody3D" },
		{ OBJ_3D_CUBE, TTRC("Cube (falls down)"), TTRC("A box with physics."), "BoxMesh" },
		{ OBJ_3D_BALL, TTRC("Ball (falls down)"), TTRC("A ball with physics. It rolls!"), "SphereMesh" },
		{ OBJ_3D_COIN, TTRC("Coin"), TTRC("Disappears when the player touches it."), "Area3D" },
		{ OBJ_3D_LIGHT, TTRC("Lamp"), TTRC("A light bulb that lights up things around it."), "OmniLight3D" },
		{ OBJ_3D_TEXT, TTRC("3D Text"), TTRC("Floating text in the world."), "Label3D" },
		{ OBJ_3D_CAMERA, TTRC("Camera"), TTRC("Decides what part of the world is shown."), "Camera3D" },
	};

	const bool is_3d = _is_3d_scene();
	const Item *items = is_3d ? items_3d : items_2d;
	const int count = is_3d ? (int)std_size(items_3d) : (int)std_size(items_2d);

	for (int i = 0; i < count; i++) {
		Button *b = memnew(Button);
		b->set_text(TTR(String(items[i].text)) + "\n" + TTR(String(items[i].desc)));
		b->set_button_icon(get_editor_theme_icon(StringName(items[i].icon)));
		b->set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
		b->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
		b->set_custom_minimum_size(Size2(0, 60) * EDSCALE);
		b->add_theme_constant_override("icon_max_width", 28 * EDSCALE);
		b->set_focus_mode(FOCUS_NONE);
		b->connect(SceneStringName(pressed), callable_mp(this, &EasyModeBar::_add_object).bind(items[i].kind));
		b->connect(SceneStringName(pressed), callable_mp((Window *)add_popup, &Window::hide));
		add_list->add_child(b);
	}

	add_popup->popup_centered_clamped(Size2(380, 560) * EDSCALE, 0.9);
}

Ref<Script> EasyModeBar::_get_or_create_script(const String &p_file_name, const String &p_source) {
	const String dir = "res://scripts";
	const String path = dir.path_join(p_file_name);

	if (!FileAccess::exists(path)) {
		DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(dir));
		Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(f.is_null(), Ref<Script>(), "Easy Mode: cannot write " + path);
		f->store_string(p_source);
		f.unref();
		if (EditorFileSystem::get_singleton()) {
			EditorFileSystem::get_singleton()->update_file(path);
		}
	}
	return ResourceLoader::load(path, "Script");
}

void EasyModeBar::_set_owner_recursive(Node *p_node, Node *p_owner, EditorUndoRedoManager *p_undo_redo) {
	if (p_node != p_owner) {
		p_undo_redo->add_do_method(p_node, "set_owner", p_owner);
	}
	for (int i = 0; i < p_node->get_child_count(); i++) {
		_set_owner_recursive(p_node->get_child(i), p_owner, p_undo_redo);
	}
}

Node *EasyModeBar::_build_object(int p_kind, Node *p_scene_root, String &r_action_name) {
	const int n = p_scene_root->get_child_count();
	// Spread new objects a bit so they don't all land in the same spot.
	const Vector2 pos_2d = Vector2(576 + (n % 6) * 100 - 250, 300 - ((n / 6) % 3) * 90); // Never exactly on the player spot (576, 300).
	const Vector3 pos_3d = Vector3(((n % 5) - 2) * 2.0, 3.0, -((n / 5) % 4) * 2.0);

	switch (p_kind) {
		case OBJ_2D_PLAYER_PLATFORMER:
		case OBJ_2D_PLAYER_TOPDOWN: {
			const bool platformer = p_kind == OBJ_2D_PLAYER_PLATFORMER;
			r_action_name = TTR("Add Player");
			CharacterBody2D *body = memnew(CharacterBody2D);
			body->set_name("Player");
			body->set_position(Vector2(576, 300));
			if (!platformer) {
				body->set_motion_mode(CharacterBody2D::MOTION_MODE_FLOATING);
			}
			body->add_child(_make_rect_polygon(Size2(48, 64), Color(0.25, 0.55, 1.0)));
			Polygon2D *eye = _make_rect_polygon(Size2(10, 10), Color(1, 1, 1));
			eye->set_name("Eye");
			eye->set_position(Vector2(10, -14));
			body->add_child(eye);
			body->add_child(_make_rect_collision_2d(Size2(48, 64)));
			Camera2D *cam = memnew(Camera2D);
			cam->set_name("Camera");
			cam->set_position_smoothing_enabled(true);
			body->add_child(cam);
			body->set_script(platformer ? _get_or_create_script("player_2d_platformer.gd", SCRIPT_PLAYER_2D_PLATFORMER) : _get_or_create_script("player_2d_topdown.gd", SCRIPT_PLAYER_2D_TOPDOWN));
			return body;
		}
		case OBJ_2D_GROUND: {
			r_action_name = TTR("Add Ground");
			StaticBody2D *body = memnew(StaticBody2D);
			body->set_name("Ground");
			body->set_position(n <= 1 ? Vector2(576, 620) : pos_2d + Vector2(0, 120));
			const Size2 size = n <= 1 ? Size2(1600, 60) : Size2(300, 40);
			body->add_child(_make_rect_polygon(size, Color(0.3, 0.7, 0.3)));
			body->add_child(_make_rect_collision_2d(size));
			return body;
		}
		case OBJ_2D_BOX: {
			r_action_name = TTR("Add Box");
			RigidBody2D *body = memnew(RigidBody2D);
			body->set_name("Box");
			body->set_position(pos_2d);
			body->add_child(_make_rect_polygon(Size2(56, 56), Color(0.7, 0.45, 0.2)));
			body->add_child(_make_rect_collision_2d(Size2(56, 56)));
			return body;
		}
		case OBJ_2D_COIN: {
			r_action_name = TTR("Add Coin");
			Area2D *area = memnew(Area2D);
			area->set_name("Coin");
			area->set_position(pos_2d);
			area->add_child(_make_circle_polygon(18, Color(1.0, 0.85, 0.1)));
			CollisionShape2D *cs = memnew(CollisionShape2D);
			cs->set_name("Collision");
			Ref<CircleShape2D> shape;
			shape.instantiate();
			shape->set_radius(18);
			cs->set_shape(shape);
			area->add_child(cs);
			area->set_script(_get_or_create_script("coin_2d.gd", SCRIPT_COIN_2D));
			return area;
		}
		case OBJ_2D_TEXT: {
			r_action_name = TTR("Add Text");
			Label *label = memnew(Label);
			label->set_name("Text");
			label->set_text(TTR("Hello! Tap me in the editor to change this text."));
			label->add_theme_font_size_override(SceneStringName(font_size), 32);
			label->set_position(Vector2(40, 40));
			return label;
		}
		case OBJ_2D_PICTURE: {
			r_action_name = TTR("Add Picture");
			Sprite2D *sprite = memnew(Sprite2D);
			sprite->set_name("Picture");
			sprite->set_position(pos_2d);
			if (ResourceLoader::exists("res://icon.svg")) {
				sprite->set_texture(ResourceLoader::load("res://icon.svg"));
			}
			return sprite;
		}
		case OBJ_2D_CAMERA: {
			r_action_name = TTR("Add Camera");
			Camera2D *cam = memnew(Camera2D);
			cam->set_name("Camera");
			cam->set_position(Vector2(576, 324));
			return cam;
		}
		case OBJ_3D_PLAYER: {
			r_action_name = TTR("Add Player");
			CharacterBody3D *body = memnew(CharacterBody3D);
			body->set_name("Player");
			body->set_position(Vector3(0, 2, 0));
			Ref<CapsuleMesh> mesh;
			mesh.instantiate();
			mesh->set_radius(0.4);
			mesh->set_height(1.8);
			body->add_child(_make_mesh(mesh, Color(0.25, 0.55, 1.0)));
			Ref<CapsuleShape3D> shape;
			shape.instantiate();
			shape->set_radius(0.4);
			shape->set_height(1.8);
			body->add_child(_make_collision_3d(shape));
			Camera3D *cam = memnew(Camera3D);
			cam->set_name("Camera");
			cam->set_position(Vector3(0, 3, 6));
			cam->set_rotation_degrees(Vector3(-20, 0, 0));
			cam->set_current(true);
			body->add_child(cam);
			body->set_script(_get_or_create_script("player_3d.gd", SCRIPT_PLAYER_3D));
			return body;
		}
		case OBJ_3D_FLOOR: {
			r_action_name = TTR("Add Floor");
			StaticBody3D *body = memnew(StaticBody3D);
			body->set_name("Floor");
			const float size = n <= 3 ? 30.0 : 6.0;
			if (n > 3) {
				body->set_position(Vector3(pos_3d.x, 0, pos_3d.z));
			}
			Ref<BoxMesh> mesh;
			mesh.instantiate();
			mesh->set_size(Vector3(size, 0.4, size));
			MeshInstance3D *mi = _make_mesh(mesh, Color(0.35, 0.7, 0.35));
			mi->set_position(Vector3(0, -0.2, 0));
			body->add_child(mi);
			Ref<BoxShape3D> shape;
			shape.instantiate();
			shape->set_size(Vector3(size, 0.4, size));
			CollisionShape3D *cs = _make_collision_3d(shape);
			cs->set_position(Vector3(0, -0.2, 0));
			body->add_child(cs);
			return body;
		}
		case OBJ_3D_CUBE:
		case OBJ_3D_BALL: {
			const bool cube = p_kind == OBJ_3D_CUBE;
			r_action_name = cube ? TTR("Add Cube") : TTR("Add Ball");
			RigidBody3D *body = memnew(RigidBody3D);
			body->set_name(cube ? "Cube" : "Ball");
			body->set_position(pos_3d);
			if (cube) {
				Ref<BoxMesh> mesh;
				mesh.instantiate();
				body->add_child(_make_mesh(mesh, Color(0.8, 0.5, 0.25)));
				Ref<BoxShape3D> shape;
				shape.instantiate();
				body->add_child(_make_collision_3d(shape));
			} else {
				Ref<SphereMesh> mesh;
				mesh.instantiate();
				mesh->set_radius(0.5);
				mesh->set_height(1.0);
				body->add_child(_make_mesh(mesh, Color(0.9, 0.3, 0.3)));
				Ref<SphereShape3D> shape;
				shape.instantiate();
				shape->set_radius(0.5);
				body->add_child(_make_collision_3d(shape));
			}
			return body;
		}
		case OBJ_3D_COIN: {
			r_action_name = TTR("Add Coin");
			Area3D *area = memnew(Area3D);
			area->set_name("Coin");
			area->set_position(Vector3(pos_3d.x, 1.0, pos_3d.z));
			Ref<CylinderMesh> mesh;
			mesh.instantiate();
			mesh->set_top_radius(0.4);
			mesh->set_bottom_radius(0.4);
			mesh->set_height(0.1);
			MeshInstance3D *mi = _make_mesh(mesh, Color(1.0, 0.85, 0.1));
			mi->set_rotation_degrees(Vector3(90, 0, 0));
			area->add_child(mi);
			Ref<SphereShape3D> shape;
			shape.instantiate();
			shape->set_radius(0.5);
			area->add_child(_make_collision_3d(shape));
			area->set_script(_get_or_create_script("coin_3d.gd", SCRIPT_COIN_3D));
			return area;
		}
		case OBJ_3D_LIGHT: {
			r_action_name = TTR("Add Lamp");
			OmniLight3D *light = memnew(OmniLight3D);
			light->set_name("Lamp");
			light->set_position(Vector3(pos_3d.x, 3.0, pos_3d.z));
			light->set_param(Light3D::PARAM_RANGE, 10.0);
			light->set_param(Light3D::PARAM_ENERGY, 2.0);
			light->set_color(Color(1.0, 0.9, 0.7));
			return light;
		}
		case OBJ_3D_TEXT: {
			r_action_name = TTR("Add 3D Text");
			Label3D *label = memnew(Label3D);
			label->set_name("Text");
			label->set_text(TTR("Hello!"));
			label->set_font_size(96);
			label->set_billboard_mode(StandardMaterial3D::BILLBOARD_ENABLED);
			label->set_position(Vector3(pos_3d.x, 2.5, pos_3d.z));
			return label;
		}
		case OBJ_3D_CAMERA: {
			r_action_name = TTR("Add Camera");
			Camera3D *cam = memnew(Camera3D);
			cam->set_name("Camera");
			cam->set_position(Vector3(0, 6, 10));
			cam->set_rotation_degrees(Vector3(-30, 0, 0));
			return cam;
		}
	}
	return nullptr;
}

void EasyModeBar::_add_object(int p_kind) {
	Node *root = _get_root();
	if (!root) {
		_show_message(TTR("First tap \"New 2D\" or \"New 3D\" to start a game."));
		return;
	}
	Node *parent = _get_parent_for_new_object();

	String action_name;
	Node *node = _build_object(p_kind, root, action_name);
	ERR_FAIL_NULL(node);

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(action_name);
	ur->add_do_method(parent, "add_child", node, true);
	_set_owner_recursive(node, root, ur);
	ur->add_do_reference(node);
	ur->add_undo_method(parent, "remove_child", node);
	ur->commit_action();

	EditorSelection *selection = EditorNode::get_singleton()->get_editor_selection();
	selection->clear();
	selection->add_node(node);
	EditorNode::get_singleton()->edit_node(node);
	if (Object::cast_to<CanvasItem>(node) && CanvasItemEditor::get_singleton()) {
		// Show the new object in the middle of the 2D view.
		callable_mp(CanvasItemEditor::get_singleton(), &CanvasItemEditor::focus_selection).call_deferred();
	}
	_update_state();
}

void EasyModeBar::_open_code() {
	Node *root = _get_root();
	if (!root) {
		return;
	}
	List<Node *> selected = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
	Node *node = selected.is_empty() ? root : selected.front()->get();

	Ref<Script> existing = node->get_script();
	if (existing.is_valid()) {
		EditorInterface::get_singleton()->edit_script(existing);
		_focus_main_screen("Script");
		return;
	}

	// No script yet: create a friendly starter script and attach it, no dialogs.
	ScriptLanguage *lang = ScriptServer::get_language_for_extension("gd");
	if (!lang) {
		SceneTreeDock::get_singleton()->attach_script_to_selected(false);
		return;
	}

	String source = vformat(R"GD(extends %s
# Code for "%s".
# Tip: lines starting with # are notes for humans. The game ignores them.


# This runs ONE time, when the game starts.
func _ready():
	print("Hello from ", name)


# This runs MANY times per second (every frame).
# delta = how many seconds passed since the last frame.
func _process(delta):
	pass
)GD",
			node->get_class(), node->get_name());

	const String dir = "res://scripts";
	DirAccess::make_dir_recursive_absolute(ProjectSettings::get_singleton()->globalize_path(dir));
	const String path = _unique_path(dir, _to_file_name(node->get_name()), "gd");
	{
		Ref<FileAccess> f = FileAccess::open(path, FileAccess::WRITE);
		if (f.is_null()) {
			_show_message(TTR("Could not create the script file."));
			return;
		}
		f->store_string(source);
	}
	EditorFileSystem::get_singleton()->update_file(path);
	Ref<Script> scr = ResourceLoader::load(path, "Script");
	if (scr.is_null()) {
		return;
	}

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(TTR("Add Code"));
	ur->add_do_method(node, "set_script", scr);
	ur->add_undo_method(node, "set_script", Variant());
	ur->commit_action();

	EditorInterface::get_singleton()->edit_script(scr);
	_focus_main_screen("Script");
}

void EasyModeBar::_delete_selected() {
	Node *root = _get_root();
	if (!root) {
		return;
	}
	List<Node *> selected = EditorNode::get_singleton()->get_editor_selection()->get_top_selected_node_list();
	if (selected.is_empty()) {
		_show_message(TTR("First tap an object to select it, then tap Delete."));
		return;
	}

	List<Node *> to_delete;
	for (Node *n : selected) {
		if (n != root && n->get_parent()) {
			to_delete.push_back(n);
		}
	}
	if (to_delete.is_empty()) {
		_show_message(TTR("The main node of the scene can't be deleted.\nTo start over, tap \"New 2D\" or \"New 3D\"."));
		return;
	}

	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(TTR("Delete"));
	for (Node *n : to_delete) {
		Node *parent = n->get_parent();
		ur->add_do_method(parent, "remove_child", n);
		ur->add_undo_method(parent, "add_child", n, true);
		ur->add_undo_method(parent, "move_child", n, n->get_index(false));
		ur->add_undo_method(n, "set_owner", root);
		List<Node *> stack;
		for (int i = 0; i < n->get_child_count(); i++) {
			stack.push_back(n->get_child(i));
		}
		while (!stack.is_empty()) {
			Node *c = stack.front()->get();
			stack.pop_front();
			if (c->get_owner() == root) {
				ur->add_undo_method(c, "set_owner", root);
			}
			for (int i = 0; i < c->get_child_count(); i++) {
				stack.push_back(c->get_child(i));
			}
		}
		ur->add_undo_reference(n);
	}
	ur->commit_action();
	EditorNode::get_singleton()->get_editor_selection()->clear();
	_update_state();
}

void EasyModeBar::_undo() {
	EditorUndoRedoManager::get_singleton()->undo();
	_update_state();
}

void EasyModeBar::_redo() {
	EditorUndoRedoManager::get_singleton()->redo();
	_update_state();
}

void EasyModeBar::_save() {
	Node *root = _get_root();
	if (!root) {
		return;
	}
	String path = root->get_scene_file_path();
	if (path.is_empty()) {
		// Never show a file dialog to a beginner: pick a sensible name.
		path = _unique_path("res://", _to_file_name(root->get_name()), "tscn");
		EditorNode::get_singleton()->save_scene_to_path(path, true);
	} else {
		EditorInterface::get_singleton()->save_scene();
	}

	// The first saved scene becomes the "main" scene that starts the game.
	const String main_scene = GLOBAL_GET("application/run/main_scene");
	if (main_scene.is_empty() || !ResourceLoader::exists(main_scene)) {
		ProjectSettings::get_singleton()->set_setting("application/run/main_scene", path);
		ProjectSettings::get_singleton()->save();
	}
	_update_state();
}

void EasyModeBar::_play() {
	if (EditorRunBar::get_singleton()->is_playing()) {
		EditorRunBar::get_singleton()->stop_playing();
		return;
	}
	if (!_get_root()) {
		return;
	}
	_save();
	EditorRunBar::get_singleton()->play_current_scene();
}

void EasyModeBar::_show_help() {
	message_dialog->hide();
	help_dialog->popup_centered_clamped(Size2(420, 560) * EDSCALE, 0.9);
}

void EasyModeBar::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_THEME_CHANGED: {
			_apply_sizes();
		} break;
		case NOTIFICATION_READY: {
			set_process(true);
			callable_mp(this, &EasyModeBar::_sync_layout).call_deferred();
			_update_state();
		} break;
		case NOTIFICATION_PROCESS: {
			// Cheap: only updates a label and a few disabled flags.
			if (Engine::get_singleton()->get_process_frames() % 20 == 0) {
				_sync_layout();
				_update_state();
			}
		} break;
		case EditorSettings::NOTIFICATION_EDITOR_SETTINGS_CHANGED: {
			if (EditorSettings::get_singleton()->check_changed_settings_in_group("interface/easy_mode")) {
				set_visible(EDITOR_GET("interface/easy_mode/enabled"));
			}
		} break;
	}
}

EasyModeBar::EasyModeBar() {
	set_name("EasyModeBar");
	set_theme_type_variation("BottomPanel");
	set_h_size_flags(SIZE_EXPAND_FILL);

	EDITOR_DEF_BASIC("interface/easy_mode/enabled", is_enabled_by_default());
	set_visible(EDITOR_GET("interface/easy_mode/enabled"));

	VBoxContainer *vb = memnew(VBoxContainer);
	main_vb = vb;
	add_child(vb);

	hint_label = memnew(Label);
	hint_label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	hint_label->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	hint_label->set_h_size_flags(SIZE_EXPAND_FILL);
	hint_label->set_focus_mode(FOCUS_NONE);
	vb->add_child(hint_label);

	HBoxContainer *nav = memnew(HBoxContainer);
	nav->set_h_size_flags(SIZE_EXPAND_FILL);
	vb->add_child(nav);
	buttons_box = nav;
	_make_button(TTR("Scene"), SNAME("Node2D"), callable_mp(this, &EasyModeBar::_back_to_scene));
	objects_button = _make_button(TTR("Objects"), SNAME("Node"), callable_mp(this, &EasyModeBar::_open_objects));
	add_button = _make_button(TTR("Add"), SNAME("Add"), callable_mp(this, &EasyModeBar::_open_add_popup));
	play_button = _make_button(TTR("Play"), SNAME("Play"), callable_mp(this, &EasyModeBar::_play));
	_make_button(TTR("More"), SNAME("GuiTabMenuHl"), callable_mp(this, &EasyModeBar::_open_more));

	more_popup = memnew(PopupPanel);
	add_child(more_popup);
	VBoxContainer *more_vb = memnew(VBoxContainer);
	more_popup->add_child(more_vb);
	Label *more_title = memnew(Label);
	more_title->set_text(TTR("MAKE IT YOURS"));
	more_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	more_vb->add_child(more_title);
	ScrollContainer *more_scroll = memnew(ScrollContainer);
	more_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	more_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	more_vb->add_child(more_scroll);
	GridContainer *more_grid = memnew(GridContainer);
	more_grid->set_columns(3);
	more_grid->set_h_size_flags(SIZE_EXPAND_FILL);
	more_scroll->add_child(more_grid);
	buttons_box = more_grid;
	new_2d_button = _make_button(TTR("New 2D"), SNAME("Node2D"), callable_mp(this, &EasyModeBar::_new_game).bind(false));
	new_3d_button = _make_button(TTR("New 3D"), SNAME("Node3D"), callable_mp(this, &EasyModeBar::_new_game).bind(true));
	properties_button = _make_button(TTR("Edit"), SNAME("Edit"), callable_mp(this, &EasyModeBar::_open_properties));
	code_button = _make_button(TTR("Code"), SNAME("Script"), callable_mp(this, &EasyModeBar::_open_code));
	delete_button = _make_button(TTR("Delete"), SNAME("Remove"), callable_mp(this, &EasyModeBar::_request_delete));
	undo_button = _make_button(TTR("Undo"), SNAME("UndoRedo"), callable_mp(this, &EasyModeBar::_undo));
	redo_button = _make_button(TTR("Redo"), SNAME("Redo"), callable_mp(this, &EasyModeBar::_redo));
	save_button = _make_button(TTR("Save"), SNAME("Save"), callable_mp(this, &EasyModeBar::_save));
	help_button = _make_button(TTR("Help"), SNAME("Help"), callable_mp(this, &EasyModeBar::_show_help));
	hide_button = _make_button(TTR("Full editor"), SNAME("GuiVisibilityHidden"), callable_mp(this, &EasyModeBar::_set_collapsed).bind(true));
	for (int i = 0; i < more_grid->get_child_count(); i++) {
		Button *b = Object::cast_to<Button>(more_grid->get_child(i));
		b->connect(SceneStringName(pressed), callable_mp((Window *)more_popup, &Window::hide));
	}

	add_button->set_tooltip_text(TTR("Add an object (player, ground, box, coin...) to your game."));
	code_button->set_tooltip_text(TTR("Write code for the selected object."));
	play_button->set_tooltip_text(TTR("Save and play your game."));

	show_button = memnew(Button);
	show_button->set_text(TTR("Back to mobile workspace"));
	show_button->set_flat(false);
	show_button->set_focus_mode(FOCUS_NONE);
	show_button->set_visible(false);
	show_button->connect(SceneStringName(pressed), callable_mp(this, &EasyModeBar::_set_collapsed).bind(false));
	add_child(show_button);

	// "Add object" popup.
	add_popup = memnew(PopupPanel);
	add_popup->set_title(TTR("Add to your game"));
	add_child(add_popup);
	VBoxContainer *popup_vb = memnew(VBoxContainer);
	add_popup->add_child(popup_vb);
	Label *popup_title = memnew(Label);
	popup_title->set_text(TTR("What do you want to add?"));
	popup_title->set_theme_type_variation("HeaderMedium");
	popup_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	popup_vb->add_child(popup_title);
	ScrollContainer *popup_scroll = memnew(ScrollContainer);
	popup_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	popup_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	popup_vb->add_child(popup_scroll);
	add_list = memnew(VBoxContainer);
	add_list->set_h_size_flags(SIZE_EXPAND_FILL);
	popup_scroll->add_child(add_list);

	Button *close_add = memnew(Button);
	close_add->set_text(TTR("Back to scene"));
	close_add->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
	close_add->connect(SceneStringName(pressed), callable_mp((Window *)add_popup, &Window::hide));
	popup_vb->add_child(close_add);

	objects_popup = memnew(PopupPanel);
	add_child(objects_popup);
	VBoxContainer *objects_vb = memnew(VBoxContainer);
	objects_popup->add_child(objects_vb);
	Label *objects_title = memnew(Label);
	objects_title->set_text(TTR("YOUR OBJECTS - tap to select"));
	objects_title->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
	objects_vb->add_child(objects_title);
	ScrollContainer *objects_scroll = memnew(ScrollContainer);
	objects_scroll->set_v_size_flags(SIZE_EXPAND_FILL);
	objects_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	objects_vb->add_child(objects_scroll);
	objects_list = memnew(VBoxContainer);
	objects_list->set_h_size_flags(SIZE_EXPAND_FILL);
	objects_scroll->add_child(objects_list);
	Button *close_objects = memnew(Button);
	close_objects->set_text(TTR("Back to scene"));
	close_objects->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
	close_objects->connect(SceneStringName(pressed), callable_mp((Window *)objects_popup, &Window::hide));
	objects_vb->add_child(close_objects);

	properties_dialog = memnew(ConfirmationDialog);
	properties_dialog->set_ok_button_text(TTR("Apply changes"));
	properties_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EasyModeBar::_apply_properties));
	add_child(properties_dialog);
	VBoxContainer *fields = memnew(VBoxContainer);
	properties_dialog->add_child(fields);
	Label *fields_hint = memnew(Label);
	fields_hint->set_text(TTR("Position moves your object. Size 1 = original."));
	fields_hint->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	fields->add_child(fields_hint);
	HBoxContainer *field_headers = memnew(HBoxContainer);
	fields->add_child(field_headers);
	for (const String &text : { TTR("Position"), TTR("Size multiplier") }) {
		Label *label = memnew(Label);
		label->set_text(text);
		label->set_h_size_flags(SIZE_EXPAND_FILL);
		label->set_horizontal_alignment(HORIZONTAL_ALIGNMENT_CENTER);
		field_headers->add_child(label);
	}
	for (int i = 0; i < 3; i++) {
		HBoxContainer *row = memnew(HBoxContainer);
		fields->add_child(row);
		Label *axis = memnew(Label);
		axis->set_text(i == 0 ? "X" : i == 1 ? "Y" : "Z");
		row->add_child(axis);
		for (int j = 0; j < 2; j++) {
			SpinBox *field = memnew(SpinBox);
			field->set_min(j == 0 ? -100000 : 0.01);
			field->set_max(j == 0 ? 100000 : 1000);
			field->set_step(0.01);
			field->set_h_size_flags(SIZE_EXPAND_FILL);
			field->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
			field->get_line_edit()->add_theme_font_size_override(SceneStringName(font_size), 16 * EDSCALE);
			field->set_tooltip_text(j == 0 ? TTR("Position") : TTR("Size multiplier"));
			row->add_child(field);
			if (j == 0) {
				position_fields[i] = field;
			} else {
				scale_fields[i] = field;
			}
		}
	}
	delete_dialog = memnew(ConfirmationDialog);
	delete_dialog->set_title(TTR("Delete selected objects?"));
	delete_dialog->set_text(TTR("Remove the selected objects from this scene? You can Undo this."));
	delete_dialog->set_ok_button_text(TTR("Delete"));
	delete_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EasyModeBar::_delete_selected));
	add_child(delete_dialog);

	// Help.
	help_dialog = memnew(AcceptDialog);
	help_dialog->set_title(TTR("How to make a game (Easy Mode)"));
	help_dialog->set_autowrap(true);
	help_dialog->set_text(TTR(
			"1. Tap More > New 2D (flat game) or New 3D (3D world).\n"
			"2. Tap \"Add\" and choose \"Player\". Add Ground, Boxes and Coins too.\n"
			"3. Tap \"Play\" to try your game. It is saved automatically.\n"
			"4. Tap an object to select it. Drag it with your finger to move it.\n"
			"   Or tap Objects to choose it from a large, scrollable list.\n"
			"5. More > Edit changes position and size. More > Code opens its script.\n"
			"6. More > Undo fixes mistakes. Scene returns to your canvas.\n"
			"7. More > Full editor opens advanced tools and project settings.\n\n"
			"Controls in your game on the phone:\n"
			"- 2D jumping game: touch left / right side to walk, middle to jump.\n"
			"- 2D top-down game: hold your finger where the player should walk.\n"
			"- 3D game: slide a finger on the left half to walk, tap the right half to jump.\n\n"
			"You can turn Easy Mode off in Editor Settings > Interface > Easy Mode."));
	add_child(help_dialog);

	message_dialog = memnew(AcceptDialog);
	message_dialog->set_title(TTR("Easy Mode"));
	message_dialog->set_autowrap(true);
	add_child(message_dialog);
}
