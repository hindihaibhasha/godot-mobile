#include "easy_logic_editor.h"
#include "easy_block_button.h"
#include "scene/gui/grid_container.h"
#include "scene/gui/margin_container.h"

#include "core/object/callable_mp.h"
#include "core/object/script_language.h"
#include "editor/editor_node.h"
#include "editor/editor_data.h"
#include "editor/editor_undo_redo_manager.h"
#include "editor/themes/editor_scale.h"
#include "scene/gui/box_container.h"
#include "scene/gui/button.h"
#include "scene/gui/label.h"
#include "scene/gui/line_edit.h"
#include "scene/gui/option_button.h"
#include "scene/gui/scroll_container.h"
#include "scene/gui/spin_box.h"
#include "scene/resources/style_box_flat.h"

namespace {
const char *OPS[] = {
	"WHEN game starts", "WHEN every frame", "WHEN screen tapped", "WHEN body enters", "WHEN timer repeats",
	"IF variable >= number", "IF variable < number", "ELSE", "REPEAT count", "END section",
	"Move by X / Y / Z", "Move per second X / Y / Z", "Rotate degrees X / Y / Z", "Set position X / Y / Z",
	"Set variable", "Add to variable", "Show object", "Hide object", "Delete object", "Set label text",
	"Play sound / animation", "Reload level", "Go to scene file", "Spawn scene file", "Set velocity X / Y / Z",
	"IF input held", "IF on floor", "Jump strength", "Apply gravity", "Move and slide", "Set numeric property"
};
bool is_section(int p_op) { return p_op == 5 || p_op == 6 || p_op == 8 || p_op == 25 || p_op == 26; }
int category_for(int p_op) {
	if (p_op < 5) { return 0; }
	if (p_op <= 9 || p_op == 25 || p_op == 26) { return 1; }
	if (p_op <= 13 || (p_op >= 24 && p_op <= 29)) { return 2; }
	if (p_op == 14 || p_op == 15 || p_op == 30) { return 3; }
	if (p_op <= 20) { return 4; }
	return 5;
}
Color category_color(int p_category) {
	const Color colors[] = { Color(0.48, 0.29, 0.06), Color(0.48, 0.23, 0.13), Color(0.12, 0.32, 0.60), Color(0.43, 0.19, 0.53), Color(0.12, 0.40, 0.30), Color(0.37, 0.28, 0.47) };
	return colors[CLAMP(p_category, 0, 5)];
}
String quoted(const String &p_text) { return "\"" + p_text.json_escape() + "\""; }
Button *touch_button(Container *p_parent, const String &p_text, const Callable &p_action) {
	Button *b = memnew(Button);
	b->set_text(p_text);
	b->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
	b->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	b->connect(SceneStringName(pressed), p_action);
	p_parent->add_child(b);
	return b;
}
}

void EasyLogicEditor::_add_targets(Node *p_node, Node *p_owner) {
	int index = target->get_item_count();
	target->add_item(p_node == p_owner ? TTR("This object") : String(p_node->get_name()));
	target->set_item_metadata(index, String(p_owner->get_path_to(p_node)));
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *child = p_node->get_child(i);
		if (!child->has_meta("easy_logic_program")) {
			_add_targets(child, p_owner);
		}
	}
}

void EasyLogicEditor::open_for(Node *p_node) {
	target_id = p_node->get_instance_id();
	root_id = EditorNode::get_singleton()->get_edited_scene()->get_instance_id();
	program.clear();
	for (int i = 0; i < p_node->get_child_count(); i++) {
		Node *c = p_node->get_child(i);
		if (c->has_meta("easy_logic_program")) {
			Array saved = c->get_meta("easy_logic_program");
			program = saved.duplicate(true);
			break;
		}
	}
	target->clear();
	_add_targets(EditorNode::get_singleton()->get_edited_scene(), p_node);
	set_title(vformat(TTR("Logic blocks: %s"), p_node->get_name()));
	status->set_text(TTR("Choose a category. Drag a block to a + slot, or tap + then a block. Tap a placed block to change its values."));
	undo_stack.clear();
	redo_stack.clear();
	insert_at = -1;
	palette_panel->show();
	_category(0);
	_refresh();
	popup_centered_clamped(Size2(680, 720) * EDSCALE, 0.97);
}

void EasyLogicEditor::_refresh() {
	for (int i = stack->get_child_count() - 1; i >= 0; i--) {
		Node *c = stack->get_child(i);
		stack->remove_child(c);
		c->queue_free();
	}
	int depth = 0;
	for (int i = 0; i <= program.size(); i++) {
		_add_slot(i, depth);
		if (i == program.size()) { break; }
		Dictionary d = program[i];
		int op = d.get("op", 0);
		if (op < 0 || op >= (int)std_size(OPS)) { continue; }
		if (op < 5) { depth = 0; }
		if (op == 9 || op == 7) { depth = MAX(0, depth - 1); }
		MarginContainer *row = memnew(MarginContainer);
		row->add_theme_constant_override("margin_left", MIN(depth, 6) * 16 * EDSCALE);
		stack->add_child(row);
		if (op == 9) {
			Label *end = memnew(Label);
			end->set_text(TTR("└ end"));
			end->add_theme_color_override("font_color", category_color(1).lightened(0.5));
			row->add_child(end);
			continue;
		}
		String caption = TTR(OPS[op]);
		if (op >= 3 && op != 7) {
			String path = d.get("target", ".");
			caption += "\n[ " + (path == "." ? TTR("This object") : path.get_file()) + " ]";
			String words = d.get("text", "");
			if (!words.is_empty()) { caption += "  [ " + words + " ]"; }
			if (op != 3 && op != 16 && op != 17 && op != 18 && op != 19 && op != 20 && op != 21 && op != 22 && op != 25 && op != 26 && op != 29) {
				caption += "  [ " + String::num(double(d.get("x", 0))) + " ]";
			}
		}
		EasyBlockButton *card = memnew(EasyBlockButton);
		card->setup(category_color(category_for(op)), op < 5);
		card->set_text(caption);
		card->set_meta("block_index", i);
		card->connect(SceneStringName(pressed), callable_mp(this, &EasyLogicEditor::_edit_block).bind(i));
		card->set_drag_forwarding(callable_mp(this, &EasyLogicEditor::_drag).bind(card), Callable(), Callable());
		row->add_child(card);
		if (op < 5 || is_section(op) || op == 7) { depth++; }
	}
}

void EasyLogicEditor::_remember() {
	undo_stack.push_back(program.duplicate(true));
	if (undo_stack.size() > 60) { undo_stack.remove_at(0); }
	redo_stack.clear();
}

void EasyLogicEditor::_history(bool p_redo) {
	Vector<Array> &from = p_redo ? redo_stack : undo_stack;
	Vector<Array> &to = p_redo ? undo_stack : redo_stack;
	if (from.is_empty()) { return; }
	to.push_back(program.duplicate(true));
	program = from[from.size() - 1].duplicate(true);
	from.resize(from.size() - 1);
	insert_at = -1;
	_refresh();
}

int EasyLogicEditor::_span_end(int p_index) const {
	Dictionary d = program[p_index];
	int op = d.get("op", 0);
	if (op < 5) {
		int end = p_index + 1;
		while (end < program.size() && int(Dictionary(program[end]).get("op", 0)) >= 5) { end++; }
		return end;
	}
	if (!is_section(op)) { return p_index + 1; }
	int depth = 1;
	for (int i = p_index + 1; i < program.size(); i++) {
		int next = Dictionary(program[i]).get("op", 0);
		if (is_section(next)) { depth++; }
		if (next == 9 && --depth == 0) { return i + 1; }
		if (next < 5) { return i; }
	}
	return program.size();
}

void EasyLogicEditor::_category(int p_category) {
	for (int i = palette->get_child_count() - 1; i >= 0; i--) {
		Node *c = palette->get_child(i); palette->remove_child(c); c->queue_free();
	}
	for (int op = 0; op < (int)std_size(OPS); op++) {
		if (op == 9 || category_for(op) != p_category) { continue; }
		EasyBlockButton *b = memnew(EasyBlockButton);
		b->setup(category_color(p_category), op < 5);
		b->set_text(TTR(OPS[op]));
		b->set_meta("palette_op", op);
		b->connect(SceneStringName(pressed), callable_mp(this, &EasyLogicEditor::_insert).bind(op, -1));
		b->set_drag_forwarding(callable_mp(this, &EasyLogicEditor::_drag).bind(b), Callable(), Callable());
		palette->add_child(b);
	}
}

void EasyLogicEditor::_toggle_palette() { palette_panel->set_visible(!palette_panel->is_visible()); }
void EasyLogicEditor::_slot_pressed(int p_at) {
	insert_at = p_at;
	palette_panel->show();
	status->set_text(TTR("Now tap a block in the drawer to connect it here."));
}

void EasyLogicEditor::_add_slot(int p_at, int p_depth) {
	Button *slot = touch_button(stack, p_at == 0 ? TTR("+ Event / drop here") : TTR("+ Connect block here"), callable_mp(this, &EasyLogicEditor::_slot_pressed).bind(p_at));
	slot->set_custom_minimum_size(Size2(0, 36) * EDSCALE);
	slot->set_flat(true);
	slot->set_meta("slot", p_at);
	slot->set_tooltip_text(TTR("Drag a block here. IF and REPEAT move together with their contents."));
	slot->set_drag_forwarding(Callable(), callable_mp(this, &EasyLogicEditor::_can_drop).bind(slot), callable_mp(this, &EasyLogicEditor::_drop).bind(slot));
}

Variant EasyLogicEditor::_drag(const Vector2 &p_position, Control *p_source) {
	Dictionary payload;
	payload["editor"] = uint64_t(get_instance_id());
	payload["index"] = p_source->get_meta("block_index", -1);
	payload["op"] = p_source->get_meta("palette_op", -1);
	Label *preview = memnew(Label);
	preview->set_text(Object::cast_to<Button>(p_source)->get_text());
	preview->set_modulate(Color(1, 0.85, 0.4));
	p_source->set_drag_preview(preview);
	return payload;
}

bool EasyLogicEditor::_can_drop(const Vector2 &p_position, const Variant &p_data, Control *p_slot) const {
	if (p_data.get_type() != Variant::DICTIONARY) { return false; }
	Dictionary data = p_data;
	if (uint64_t(data.get("editor", 0)) != uint64_t(get_instance_id())) { return false; }
	int from = data.get("index", -1), op = data.get("op", -1);
	int at = p_slot->get_meta("slot", -1);
	if (at < 0 || at > program.size()) { return false; }
	if (from >= 0) {
		if (from >= program.size() || (at >= from && at <= _span_end(from))) { return false; }
		op = Dictionary(program[from]).get("op", -1);
	}
	return op >= 0 && op < (int)std_size(OPS) && op != 9;
}

void EasyLogicEditor::_drop(const Vector2 &p_position, const Variant &p_data, Control *p_slot) {
	if (!_can_drop(p_position, p_data, p_slot)) { return; }
	Dictionary data = p_data;
	int at = p_slot->get_meta("slot"), from = data.get("index", -1);
	if (from < 0) { _insert(data["op"], at); return; }
	Array before = program.duplicate(true);
	int end = _span_end(from);
	Array moving;
	for (int i = from; i < end; i++) { moving.push_back(program[i]); }
	for (int i = from; i < end; i++) { program.remove_at(from); }
	if (at > from) { at -= end - from; }
	for (int i = 0; i < moving.size(); i++) { program.insert(at + i, moving[i]); }
	String error; _compile(error);
	if (!error.is_empty()) { program = before; status->set_text(TTR("That connection does not fit. Put events outside IF / REPEAT and actions inside an event.")); return; }
	undo_stack.push_back(before); redo_stack.clear();
	insert_at = -1;
	callable_mp(this, &EasyLogicEditor::_refresh).call_deferred();
}

void EasyLogicEditor::_insert(int p_op, int p_at) {
	if (p_at < 0) { p_at = insert_at < 0 ? program.size() : insert_at; }
	p_at = CLAMP(p_at, 0, program.size());
	Array before = program.duplicate(true);
	Dictionary d;
	d["op"] = p_op; d["target"] = "."; d["text"] = (p_op == 5 || p_op == 6 || p_op == 14 || p_op == 15) ? "score" : p_op == 25 ? "ui_accept" : "";
	d["x"] = p_op == 8 ? 3 : p_op == 11 ? 100 : 1;
	if (program.is_empty() && p_op >= 5) { Dictionary event; event["op"] = 0; program.push_back(event); p_at = 1; }
	program.insert(p_at, d);
	if (is_section(p_op)) { Dictionary end; end["op"] = 9; program.insert(p_at + 1, end); }
	String error; _compile(error);
	if (!error.is_empty()) { program = before; status->set_text(TTR("This block does not fit here. Choose a + slot inside an event; ELSE belongs inside IF.")); return; }
	undo_stack.push_back(before); redo_stack.clear();
	insert_at = is_section(p_op) ? p_at + 1 : -1;
	status->set_text(is_section(p_op) ? TTR("Section connected. The next block goes inside it; the end is automatic.") : TTR("Connected! Tap the block to set its values. Save logic when ready."));
	callable_mp(this, &EasyLogicEditor::_refresh).call_deferred();
}

void EasyLogicEditor::_delete_editing() { _remove_block(editing); block_dialog->hide(); }

void EasyLogicEditor::_edit_block(int p_index) {
	editing = p_index;
	Dictionary d;
	if (p_index >= 0) { d = program[p_index]; }
	operation->select(int(d.get("op", 0)));
	text->set_text(d.get("text", "score"));
	const char *keys[] = { "x", "y", "z" };
	for (int i = 0; i < 3; i++) { numbers[i]->set_value(d.get(keys[i], i == 0 ? 1 : 0)); }
	for (int i = 0; i < target->get_item_count(); i++) {
		if (target->get_item_metadata(i) == d.get("target", ".")) { target->select(i); break; }
	}
	_operation_changed(operation->get_selected());
	block_dialog->set_title(TTR(OPS[operation->get_selected()]));
	block_dialog->popup_centered_clamped(Size2(420, 430) * EDSCALE, 0.93);
}

void EasyLogicEditor::_operation_changed(int p_index) {
	String help = TTR("Choose the affected object. X / Y / Z are numbers; in 2D use X / Y.");
	if (p_index == 3) { help = TTR("Body entered runs on an Area2D/3D or body with body_entered signal. Select that object in the target list."); }
	if (p_index == 4) { help = TTR("X = interval in seconds (minimum 0.05)."); }
	if (p_index == 5 || p_index == 6 || p_index == 14 || p_index == 15) { help = TTR("Text = shared variable name (e.g. score). X = number. Variables start at zero and are shared across this scene."); }
	if (p_index == 8) { help = TTR("X = repeat count, 1 to 100. Add END after the repeated actions."); }
	if (p_index == 19) { help = TTR("Text = message to display on the selected Label / Label3D."); }
	if (p_index == 20) { help = TTR("Select an audio player or AnimationPlayer. Text = animation name (leave empty for sound)."); }
	if (p_index == 22 || p_index == 23) { help = TTR("Text = res:// path to a saved .tscn scene. Spawn uses X / Y / Z as the position."); }
	if (p_index == 25) { help = TTR("Text = input action (ui_left, ui_right, ui_up, ui_down, ui_accept). Close this IF with END."); }
	if (p_index == 27 || p_index == 28) { help = TTR("Select a CharacterBody. X = positive jump strength or gravity. Use Move and slide afterwards. Do not combine with an existing movement script."); }
	if (p_index == 30) { help = TTR("Text = numeric property name (e.g. speed, energy). X = new value. The property must exist on the target."); }
	field_help->set_text(help);
	const bool vector = (p_index >= 10 && p_index <= 13) || p_index == 23 || p_index == 24;
	const bool scalar = vector || p_index == 4 || p_index == 5 || p_index == 6 || p_index == 8 || p_index == 14 || p_index == 15 || p_index == 27 || p_index == 28 || p_index == 30;
	const bool words = p_index == 5 || p_index == 6 || p_index == 14 || p_index == 15 || p_index == 19 || p_index == 20 || p_index == 22 || p_index == 23 || p_index == 25 || p_index == 30;
	text->set_visible(words);
	text->set_placeholder((p_index == 5 || p_index == 6 || p_index == 14 || p_index == 15) ? TTR("Variable name, e.g. score") : p_index == 19 ? TTR("Message to display") : TTR("Name or scene path"));
	target->set_visible(p_index == 3 || vector || (p_index >= 16 && p_index <= 20) || (p_index >= 26 && p_index <= 30));
	for (int i = 0; i < 3; i++) { Object::cast_to<Control>(numbers[i]->get_parent())->set_visible(i == 0 ? scalar : vector); }
}

void EasyLogicEditor::_store_block() {
	for (SpinBox *number : numbers) { number->apply(); }
	Dictionary d;
	d["op"] = operation->get_selected();
	d["text"] = text->get_text();
	d["target"] = target->get_selected_metadata();
	d["x"] = numbers[0]->get_value(); d["y"] = numbers[1]->get_value(); d["z"] = numbers[2]->get_value();
	_remember();
	if (editing < 0) { program.push_back(d); } else { program[editing] = d; }
	_refresh();
}

void EasyLogicEditor::_move_block(int p_index, int p_direction) {
	int to = p_index + p_direction;
	if (to < 0 || to >= program.size()) { return; }
	Variant item = program[p_index]; program[p_index] = program[to]; program[to] = item;
	_refresh();
}
void EasyLogicEditor::_remove_block(int p_index) {
	if (p_index < 0 || p_index >= program.size()) { return; }
	_remember();
	int end = _span_end(p_index);
	for (int i = p_index; i < end; i++) { program.remove_at(p_index); }
	insert_at = -1;
	_refresh();
}

String EasyLogicEditor::_compile(String &r_error) const {
	String functions, ready, physics, input;
	Vector<int> sections;
	Vector<bool> has_else;
	int event = -1;
	for (int i = 0; i < program.size(); i++) {
		Dictionary d = program[i];
		int op = d.get("op", -1);
		if (op < 0 || op >= (int)std_size(OPS)) { r_error = TTR("Unknown block."); return ""; }
		String path = quoted(d.get("target", "."));
		String value = quoted(d.get("text", ""));
		double nx = d.get("x", 0);
		String x = String::num(nx), y = String::num(double(d.get("y", 0))), z = String::num(double(d.get("z", 0)));
		if (op < 5) {
			if (!sections.is_empty()) { r_error = vformat(TTR("Block %d: add END before the next event."), i + 1); return ""; }
			event = i;
			String fn = "_event_" + itos(i);
			functions += "\nfunc " + fn + "(delta = 0.0):\n\tpass\n";
			if (op == 0) { ready += "\t" + fn + "()\n"; }
			if (op == 1) { physics += "\t" + fn + "(delta)\n"; }
			if (op == 2) { input += "\t\t" + fn + "()\n"; }
			if (op == 3) { ready += "\t_bind_body(" + path + ", " + fn + ")\n"; }
			if (op == 4) { ready += "\t_bind_timer(" + String::num(MAX(0.05, nx)) + ", " + fn + ")\n"; }
			continue;
		}
		if (event < 0) { r_error = TTR("The first block must be a WHEN event."); return ""; }
		if (op == 9) {
			if (sections.is_empty()) { r_error = vformat(TTR("Block %d: END has no matching IF or REPEAT."), i + 1); return ""; }
			sections.resize(sections.size() - 1); has_else.resize(has_else.size() - 1); continue;
		}
		if (op == 7) {
			if (sections.is_empty() || sections[sections.size() - 1] == 8 || has_else[has_else.size() - 1]) {
				r_error = TTR("ELSE needs an IF and can only appear once in that section."); return "";
			}
			has_else.write[has_else.size() - 1] = true;
			functions += String("\t").repeat(sections.size()) + "else:\n" + String("\t").repeat(sections.size() + 1) + "pass\n";
			continue;
		}
		String indent = String("\t").repeat(sections.size() + 1);
		String line;
		if (op == 5 || op == 6) { line = "if _var(" + value + ") " + (op == 5 ? ">= " : "< ") + x + ":"; }
		else if (op == 8) { line = "for _i" + itos(i) + " in range(" + itos(CLAMP(int(nx), 1, 100)) + "):"; }
		else if (op == 25) { line = "if Input.is_action_pressed(" + value + "):"; }
		else if (op == 26) { line = "if _on_floor(" + path + "):"; }
		else { line = "_act(" + itos(op) + ", " + path + ", " + value + ", Vector3(" + x + ", " + y + ", " + z + "), delta)"; }
		functions += indent + line + "\n";
		if (op == 5 || op == 6 || op == 8 || op == 25 || op == 26) {
			sections.push_back(op); has_else.push_back(false);
			functions += indent + "\tpass\n";
		}
	}
	if (!sections.is_empty()) { r_error = TTR("Add END to close each IF / REPEAT section."); return ""; }
	String source = "extends Node\n# Generated from Easy Mode blocks. Edit using the Blocks button.\n";
	source += "\nfunc _ready():\n\tpass\n" + ready;
	source += "\nfunc _physics_process(delta):\n\tpass\n" + physics;
	source += "\nfunc _unhandled_input(event):\n\tif (event is InputEventScreenTouch and event.pressed) or (event is InputEventMouseButton and event.device != -1 and event.pressed and event.button_index == MOUSE_BUTTON_LEFT):\n\t\tpass\n" + input;
	return source + functions + R"GD(
func _target(path):
	return get_parent().get_node_or_null(NodePath(path))

func _var(key):
	return float(get_tree().current_scene.get_meta("easy_var_" + key, 0.0))

func _on_floor(path):
	var n = _target(path)
	return is_instance_valid(n) and n.has_method("is_on_floor") and n.is_on_floor()

func _bind_body(path, action):
	var n = _target(path)
	if is_instance_valid(n) and n.has_signal("body_entered"):
		n.connect("body_entered", func(_body): action.call())

func _bind_timer(seconds, action):
	var timer = Timer.new()
	timer.wait_time = seconds
	timer.timeout.connect(action)
	add_child(timer)
	timer.start()

func _act(op, path, text, v, delta):
	var n = _target(path)
	var world = get_tree().current_scene
	if op == 14 or op == 15:
		world.set_meta("easy_var_" + text, v.x if op == 14 else _var(text) + v.x)
		return
	if op == 21:
		get_tree().call_deferred("reload_current_scene")
		return
	if op == 22:
		if ResourceLoader.exists(text): get_tree().call_deferred("change_scene_to_file", text)
		return
	if op == 23:
		if ResourceLoader.exists(text):
			var resource = load(text)
			if resource is PackedScene:
				var instance = resource.instantiate()
				world.add_child(instance)
				if instance is Node3D: instance.position = v
				elif instance is Node2D: instance.position = Vector2(v.x, v.y)
		return
	if not is_instance_valid(n) or n.is_queued_for_deletion(): return
	match op:
		10, 11, 13:
			var offset = v * delta if op == 11 else v
			if n is Node3D: n.position = offset if op == 13 else n.position + offset
			elif n is Node2D or n is Control:
				var p = Vector2(offset.x, offset.y)
				n.position = p if op == 13 else n.position + p
		12:
			if n is Node3D: n.rotation_degrees += v
			elif n is Node2D or n is Control: n.rotation += deg_to_rad(v.x)
		16, 17:
			if n is CanvasItem or n is Node3D: n.visible = op == 16
		18:
			if n != world: n.queue_free()
		19:
			if n is Label or n is Label3D: n.text = text
		20:
			if n is AnimationPlayer:
				if n.has_animation(text): n.play(text)
			elif n is AudioStreamPlayer or n is AudioStreamPlayer2D or n is AudioStreamPlayer3D: n.play()
		24:
			if n is CharacterBody3D: n.velocity = v
			elif n is CharacterBody2D: n.velocity = Vector2(v.x, v.y)
		27:
			if (n is CharacterBody2D or n is CharacterBody3D) and n.is_on_floor():
				n.velocity.y = -abs(v.x) if n is CharacterBody2D else abs(v.x)
		28:
			if n is CharacterBody2D: n.velocity.y += abs(v.x) * delta
			elif n is CharacterBody3D: n.velocity.y -= abs(v.x) * delta
		29:
			if n is CharacterBody2D or n is CharacterBody3D: n.move_and_slide()
		30:
			for property in n.get_property_list():
				if property.name == text and property.type in [TYPE_INT, TYPE_FLOAT]:
					n.set(text, v.x)
					break
)GD";
}

void EasyLogicEditor::_apply() {
	Node *node = Object::cast_to<Node>(ObjectDB::get_instance(target_id));
	Node *root = EditorNode::get_singleton()->get_edited_scene();
	if (!node || !root || root->get_instance_id() != root_id || (root != node && !root->is_ancestor_of(node))) {
		status->set_text(TTR("The selected scene/object changed. Close and reopen Blocks.")); return;
	}
	String error;
	String source = _compile(error);
	if (!error.is_empty()) { status->set_text(error); return; }
	ScriptLanguage *language = ScriptServer::get_language_for_extension("gd");
	if (!language) { status->set_text(TTR("GDScript is unavailable.")); return; }
	Ref<Script> script = language->make_template("", "", "Node");
	if (script.is_null()) { return; }
	script->set_source_code(source);
	if (script->reload() != OK) { status->set_text(TTR("Blocks could not compile. Please check the block sequence.")); return; }
	Node *behavior = nullptr;
	for (int i = 0; i < node->get_child_count(); i++) {
		if (node->get_child(i)->has_meta("easy_logic_program")) { behavior = node->get_child(i); break; }
	}
	EditorUndoRedoManager *ur = EditorUndoRedoManager::get_singleton();
	ur->create_action(TTR("Apply Logic Blocks"), UndoRedo::MERGE_DISABLE, node);
	if (!behavior) {
		behavior = memnew(Node);
		behavior->set_name("LogicBlocks");
		behavior->set_script(script);
		behavior->set_meta("easy_logic_program", program.duplicate(true));
		ur->add_do_method(node, "add_child", behavior, true);
		ur->add_do_method(behavior, "set_owner", root);
		ur->add_do_reference(behavior);
		ur->add_undo_method(node, "remove_child", behavior);
	} else {
		ur->add_do_method(behavior, "set_script", script);
		ur->add_do_method(behavior, "set_meta", "easy_logic_program", program.duplicate(true));
		ur->add_undo_method(behavior, "set_script", behavior->get_script());
		ur->add_undo_method(behavior, "set_meta", "easy_logic_program", behavior->get_meta("easy_logic_program"));
	}
	ur->commit_action();
	status->set_text(TTR("Blocks applied! Save the scene or press Play. Existing object scripts are preserved."));
}

void EasyLogicEditor::_preset(int p_index) {
	// Append a complete independent event, preserving existing user blocks.
	_remember();
	Dictionary event;
	event["op"] = p_index == 0 ? 1 : 2;
	program.push_back(event);
	Dictionary action;
	action["op"] = p_index == 0 ? 11 : 15;
	action["target"] = ".";
	action["text"] = "score";
	action["x"] = p_index == 0 ? 100 : 1;
	program.push_back(action);
	_refresh();
}

EasyLogicEditor::EasyLogicEditor() {
	set_hide_on_ok(false);
	set_ok_button_text(TTR("Apply blocks"));
	get_cancel_button()->set_text(TTR("Close"));
	connect(SceneStringName(confirmed), callable_mp(this, &EasyLogicEditor::_apply));
	VBoxContainer *vb = memnew(VBoxContainer);
	add_child(vb);
	status = memnew(Label);
	status->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	vb->add_child(status);
	HBoxContainer *tools = memnew(HBoxContainer);
	vb->add_child(tools);
	touch_button(tools, TTR("Blocks drawer"), callable_mp(this, &EasyLogicEditor::_toggle_palette));
	touch_button(tools, TTR("Undo"), callable_mp(this, &EasyLogicEditor::_history).bind(false));
	touch_button(tools, TTR("Redo"), callable_mp(this, &EasyLogicEditor::_history).bind(true));
	VBoxContainer *drawer = memnew(VBoxContainer);
	palette_panel = drawer;
	vb->add_child(drawer);
	GridContainer *categories = memnew(GridContainer);
	categories->set_columns(3);
	drawer->add_child(categories);
	const char *names[] = { "Events", "Control", "Movement", "Variables", "Looks / Sound", "Scenes" };
	for (int i = 0; i < 6; i++) {
		Button *b = touch_button(categories, TTR(names[i]), callable_mp(this, &EasyLogicEditor::_category).bind(i));
		Ref<StyleBoxFlat> style; style.instantiate(); style->set_bg_color(category_color(i)); style->set_content_margin_all(8 * EDSCALE);
		b->add_theme_style_override("normal", style);
	}
	ScrollContainer *drawer_scroll = memnew(ScrollContainer);
	drawer_scroll->set_custom_minimum_size(Size2(0, 140) * EDSCALE);
	drawer_scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	drawer->add_child(drawer_scroll);
	palette = memnew(VBoxContainer);
	palette->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	drawer_scroll->add_child(palette);
	HBoxContainer *examples = memnew(HBoxContainer);
	drawer->add_child(examples);
	touch_button(examples, TTR("Example: move"), callable_mp(this, &EasyLogicEditor::_preset).bind(0));
	touch_button(examples, TTR("Example: tap score"), callable_mp(this, &EasyLogicEditor::_preset).bind(1));
	ScrollContainer *scroll = memnew(ScrollContainer);
	scroll->set_horizontal_scroll_mode(ScrollContainer::SCROLL_MODE_DISABLED);
	scroll->set_v_size_flags(Control::SIZE_EXPAND_FILL);
	vb->add_child(scroll);
	stack = memnew(VBoxContainer);
	stack->set_h_size_flags(Control::SIZE_EXPAND_FILL);
	scroll->add_child(stack);
	block_dialog = memnew(ConfirmationDialog);
	block_dialog->set_title(TTR("Choose a logic block"));
	block_dialog->set_ok_button_text(TTR("Keep block"));
	block_dialog->connect(SceneStringName(confirmed), callable_mp(this, &EasyLogicEditor::_store_block));
	add_child(block_dialog);
	VBoxContainer *form = memnew(VBoxContainer);
	block_dialog->add_child(form);
	operation = memnew(OptionButton);
	operation->set_fit_to_longest_item(false);
	operation->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
	for (const char *op : OPS) { operation->add_item(TTR(op)); }
	operation->connect(SceneStringName(item_selected), callable_mp(this, &EasyLogicEditor::_operation_changed));
	form->add_child(operation);
	operation->hide();
	block_dialog->add_button(TTR("Delete block / section"))->connect(SceneStringName(pressed), callable_mp(this, &EasyLogicEditor::_delete_editing));
	target = memnew(OptionButton);
	target->set_fit_to_longest_item(false);
	target->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
	form->add_child(target);
	text = memnew(LineEdit);
	text->set_placeholder(TTR("Variable, message, action or scene path"));
	text->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
	form->add_child(text);
	HBoxContainer *values = memnew(HBoxContainer);
	form->add_child(values);
	for (int i = 0; i < 3; i++) {
		VBoxContainer *column = memnew(VBoxContainer);
		column->set_h_size_flags(Control::SIZE_EXPAND_FILL);
		values->add_child(column);
		Label *label = memnew(Label);
		label->set_text(i == 0 ? "X / value" : i == 1 ? "Y" : "Z");
		column->add_child(label);
		numbers[i] = memnew(SpinBox);
		numbers[i]->set_min(-1000000); numbers[i]->set_max(1000000); numbers[i]->set_step(0.01);
		numbers[i]->set_custom_minimum_size(Size2(0, 48) * EDSCALE);
		column->add_child(numbers[i]);
	}
	field_help = memnew(Label);
	field_help->set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	form->add_child(field_help);
}
