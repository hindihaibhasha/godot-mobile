/**************************************************************************/
/*  easy_mode_bar.h                                                       */
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

#pragma once

#include "scene/gui/panel_container.h"

class AcceptDialog;
class Button;
class HBoxContainer;
class GridContainer;
class ConfirmationDialog;
class SpinBox;
class Label;
class Node;
class PopupPanel;
class Script;
class VBoxContainer;
class EasyLogicEditor;
class EasyMapCanvas;
class OptionButton;

// "Easy Mode": a big-button, beginner friendly toolbar shown at the bottom of
// the editor. It is designed for small phone screens: every common action
// (new 2D/3D game, add an object, write code, play, save, undo) is one tap away.
class EasyModeBar : public PanelContainer {
	GDCLASS(EasyModeBar, PanelContainer);

public:
	enum ObjectKind {
		// 2D.
		OBJ_2D_PLAYER_PLATFORMER,
		OBJ_2D_PLAYER_TOPDOWN,
		OBJ_2D_GROUND,
		OBJ_2D_BOX,
		OBJ_2D_COIN,
		OBJ_2D_TEXT,
		OBJ_2D_PICTURE,
		OBJ_2D_CAMERA,
		// 3D.
		OBJ_3D_PLAYER,
		OBJ_3D_FLOOR,
		OBJ_3D_CUBE,
		OBJ_3D_BALL,
		OBJ_3D_COIN,
		OBJ_3D_LIGHT,
		OBJ_3D_CAMERA,
		OBJ_3D_TEXT,
		OBJ_MAX,
	};

private:
	VBoxContainer *main_vb = nullptr;
	Container *buttons_box = nullptr;
	Vector<Button *> action_buttons;
	Label *hint_label = nullptr;
	Button *objects_button = nullptr;
	Button *properties_button = nullptr;
	PopupPanel *more_popup = nullptr;
	PopupPanel *objects_popup = nullptr;
	VBoxContainer *objects_list = nullptr;
	ConfirmationDialog *properties_dialog = nullptr;
	ConfirmationDialog *delete_dialog = nullptr;
	SpinBox *position_fields[3] = {};
	SpinBox *scale_fields[3] = {};
	ObjectID property_target;
	ObjectID property_root;
	bool mobile_layout = false;
	EasyLogicEditor *logic_editor = nullptr;
	ConfirmationDialog *map_dialog = nullptr;
	EasyMapCanvas *map_canvas = nullptr;
	OptionButton *map_mode = nullptr;
	SpinBox *map_cell_size = nullptr;
	ObjectID map_root;
	void _open_blocks();
	void _open_map();
	void _map_brush(int p_index);
	void _map_preset(int p_index);
	void _apply_map();
	void _duplicate_selected();
	void _init_creation_tools();

	void _sync_layout();
	void _open_more();
	void _open_objects();
	void _append_objects(Node *p_node, int p_depth);
	void _select_object(ObjectID p_id);
	void _open_properties();
	void _apply_properties();
	void _request_delete();
	void _back_to_scene();

	Button *new_2d_button = nullptr;
	Button *new_3d_button = nullptr;
	Button *add_button = nullptr;
	Button *code_button = nullptr;
	Button *delete_button = nullptr;
	Button *undo_button = nullptr;
	Button *redo_button = nullptr;
	Button *save_button = nullptr;
	Button *play_button = nullptr;
	Button *help_button = nullptr;
	Button *hide_button = nullptr;
	Button *show_button = nullptr;

	PopupPanel *add_popup = nullptr;
	VBoxContainer *add_list = nullptr;
	AcceptDialog *help_dialog = nullptr;
	AcceptDialog *message_dialog = nullptr;

	bool collapsed = false;

	Button *_make_button(const String &p_text, const StringName &p_icon, const Callable &p_callable);
	void _apply_sizes();
	void _update_state();
	void _set_collapsed(bool p_collapsed);

	bool _is_3d_scene() const;
	Node *_get_root() const;
	Node *_get_parent_for_new_object() const;
	void _show_message(const String &p_text);
	void _focus_main_screen(const String &p_name);

	void _new_game(bool p_3d);
	void _open_add_popup();
	void _add_object(int p_kind);
	void _open_code();
	void _delete_selected();
	void _undo();
	void _redo();
	void _save();
	void _play();
	void _show_help();

	Node *_build_object(int p_kind, Node *p_scene_root, String &r_action_name);
	Ref<Script> _get_or_create_script(const String &p_file_name, const String &p_source);
	void _set_owner_recursive(Node *p_node, Node *p_owner, class EditorUndoRedoManager *p_undo_redo);

protected:
	void _notification(int p_what);
	static void _bind_methods() {}

public:
	static bool is_enabled_by_default();

	EasyModeBar();
};
