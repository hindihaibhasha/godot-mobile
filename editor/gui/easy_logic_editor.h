// Touch-first structured block programming for Easy Mode.
#pragma once

#include "scene/gui/dialogs.h"

class VBoxContainer;
class OptionButton;
class SpinBox;
class LineEdit;
class Label;
class Node;

class EasyLogicEditor : public ConfirmationDialog {
	GDCLASS(EasyLogicEditor, ConfirmationDialog);

	ObjectID target_id;
	ObjectID root_id;
	Array program;
	VBoxContainer *stack = nullptr;
	Label *status = nullptr;
	ConfirmationDialog *block_dialog = nullptr;
	OptionButton *operation = nullptr;
	OptionButton *target = nullptr;
	LineEdit *text = nullptr;
	SpinBox *numbers[3] = {};
	Label *field_help = nullptr;
	int editing = -1;
	VBoxContainer *palette = nullptr;
	Control *palette_panel = nullptr;
	Vector<Array> undo_stack;
	Vector<Array> redo_stack;
	int insert_at = -1;
	void _category(int p_category);
	void _toggle_palette();
	void _insert(int p_op, int p_at);
	void _slot_pressed(int p_at);
	void _history(bool p_redo);
	void _remember();
	int _span_end(int p_index) const;
	Variant _drag(const Vector2 &p_position, Control *p_source);
	bool _can_drop(const Vector2 &p_position, const Variant &p_data, Control *p_slot) const;
	void _drop(const Vector2 &p_position, const Variant &p_data, Control *p_slot);
	void _add_slot(int p_at, int p_depth);
	void _delete_editing();

	void _refresh();
	void _edit_block(int p_index);
	void _operation_changed(int p_index);
	void _store_block();
	void _move_block(int p_index, int p_direction);
	void _remove_block(int p_index);
	void _apply();
	void _preset(int p_index);
	void _add_targets(Node *p_node, Node *p_owner);
	String _compile(String &r_error) const;

protected:
	static void _bind_methods() {}

public:
	void open_for(Node *p_node);
	EasyLogicEditor();
};
