#include "easy_block_button.h"
#include "editor/themes/editor_scale.h"
#include "scene/resources/style_box_flat.h"

void EasyBlockButton::setup(const Color &p_color, bool p_event) {
	block_color = p_color;
	event = p_event;
	set_custom_minimum_size(Size2(0, 64) * EDSCALE);
	set_h_size_flags(SIZE_EXPAND_FILL);
	set_text_alignment(HORIZONTAL_ALIGNMENT_LEFT);
	set_autowrap_mode(TextServer::AUTOWRAP_WORD_SMART);
	add_theme_color_override("font_color", Color(1, 1, 1));
	for (const String &state : { String("normal"), String("hover"), String("pressed") }) {
		Ref<StyleBoxFlat> box;
		box.instantiate();
		box->set_bg_color(state == "normal" ? p_color : p_color.lightened(0.15));
		box->set_corner_radius_all(6 * EDSCALE);
		box->set_content_margin_all(14 * EDSCALE);
		add_theme_style_override(state, box);
	}
}

void EasyBlockButton::_notification(int p_what) {
	if (p_what != NOTIFICATION_DRAW) { return; }
	float s = EDSCALE;
	if (!event) {
		draw_rect(Rect2(22 * s, 0, 24 * s, 6 * s), Color(0.075, 0.09, 0.13));
	}
	draw_rect(Rect2(22 * s, get_size().y - 2 * s, 24 * s, 8 * s), block_color);
}
