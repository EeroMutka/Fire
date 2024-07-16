
UI_API UI_Box* UI_ColorPicker(UI_Key key, float* hue, float* saturation, float* value, float* alpha);

UI_API UI_Box* UI_HueSaturationCircle(UI_Key key, float diameter, float* hue, float* saturation);

#ifdef /********************/ UI_IMPLEMENTATION /********************/

// r,g,b, should be in 0-1 range, returned h, s, v will be in 0-1 range
static void UI_RGBToHSV(float r, float g, float b, float* h, float* s, float* v) {
	float max = UI_Max(UI_Max(r, g), b);
	float min = UI_Min(UI_Min(r, g), b);
	float d = max - min;
	*v = max;
	*s = max == 0.f ? 0.f : d / max;
	if (max == min){
		*h = 0.f;
	}
	else {
		if (max == r) { *h = (g - b) / d + (g < b ? 6.f : 0.f); }
		else if (max == g) { *h = (b - r) / d + 2.f; }
		else { *h = (r - g) / d + 4.f; }
		*h /= 6.f;
	}
}

// h, s, v should be in 0-1 range, returned r, g, b will be in 0-1 range
static void UI_HSVToRGB(float h, float s, float v, float* r, float* g, float* b) {
	int i = (int)(h * 5.99999f);
	UI_CHECK(i >= 0 && i < 6);

	float f = h * 6.f - (float)i;
	float p = v * (1.f - s);
	float q = v * (1.f - f * s);
	float t = v * (1.f - (1.f - f) * s);

	switch (i) {
	case 0: { *r = v; *g = t; *b = p; } break;
	case 1: { *r = q; *g = v; *b = p; } break;
	case 2: { *r = p; *g = v; *b = t; } break;
	case 3: { *r = p; *g = q; *b = v; } break;
	case 4: { *r = t; *g = p; *b = v; } break;
	case 5: { *r = v; *g = p; *b = q; } break;
	}
}

static UI_Color UI_HSVToColor(float h, float s, float v, float alpha) {
	float r, g, b;
	UI_HSVToRGB(h, s, v, &r, &g, &b);
	UI_Color color = {(uint8_t)(r * 255.f), (uint8_t)(g * 255.f), (uint8_t)(b * 255.f), (uint8_t)(alpha * 255.f)};
	return color;
}

#define UI_COLOR_WHEEL_NUM_ROWS 2
#define UI_COLOR_WHEEL_NUM_COLUMNS 64

typedef struct UI_HueSaturationCircleData {
	float hue, saturation;
} UI_HueSaturationCircleData;

static UI_Key UI_GetHueSaturationCircleDataKey() { return UI_KEY(); }

static void UI_DrawHueSaturationCircle(UI_Box* box) {
	UI_HueSaturationCircleData* data = UI_BoxGetCustomVal(UI_HueSaturationCircleData, box, UI_GetHueSaturationCircleDataKey());

	float radius = 0.5f*box->computed_size.x;
	UI_Vec2 middle = {box->computed_position.x + radius, box->computed_position.y + radius};

	float circle_theta = 2.f * 3.1415926f * data->hue;
	UI_Vec2 circle_p_relative = {radius * data->saturation * cosf(circle_theta), radius * data->saturation * sinf(circle_theta)};
	UI_Vec2 circle_p = UI_AddV2(circle_p_relative, middle);

	uint32_t prev_wheel_first_vertex;
	for (int i = 0; i < UI_COLOR_WHEEL_NUM_ROWS; i++) {
		uint32_t wheel_first_vertex;
		UI_DrawVertex* wheel_vertices = UI_AddVertices(UI_COLOR_WHEEL_NUM_COLUMNS, &wheel_first_vertex);

		for (int j = 0; j < UI_COLOR_WHEEL_NUM_COLUMNS; j++) {
			float saturation = (float)i / (UI_COLOR_WHEEL_NUM_ROWS - 1.f);
			float hue = (float)j / UI_COLOR_WHEEL_NUM_COLUMNS;

			float theta = 2.f * 3.1415926f * hue;
			UI_Vec2 p_relative = {radius * saturation * cosf(theta), radius * saturation * sinf(theta)};
			UI_Vec2 p = UI_AddV2(p_relative, middle);
			UI_Color color = UI_HSVToColor(hue, saturation, 1.f, 1.f);
			wheel_vertices[j] = UI_DRAW_VERTEX{p, {0, 0}, color};

			if (i > 0) {
				int j_next = (j + 1) % UI_COLOR_WHEEL_NUM_COLUMNS;
				UI_AddQuadIndices(prev_wheel_first_vertex + j, prev_wheel_first_vertex + j_next, wheel_first_vertex + j_next, wheel_first_vertex + j, UI_TEXTURE_ID_NIL);
			}
		}
		prev_wheel_first_vertex = wheel_first_vertex;
	}

	UI_Color current_rgba = UI_HSVToColor(data->hue, data->saturation, 1.f, 1.f);
	UI_DrawCircle(circle_p, 6.f, 8, UI_COLOR{0, 0, 0, 188}, NULL);
	UI_DrawCircle(circle_p, 5.f, 8, current_rgba, NULL);
}

UI_API UI_Box* UI_HueSaturationCircle(UI_Key key, float diameter, float* hue, float* saturation) {
	UI_CHECK(*hue >= 0.f && *hue <= 1.f);
	UI_CHECK(*saturation >= 0.f && *saturation <= 1.f);

	UI_Box* box = UI_AddBox(key, diameter, diameter, UI_BoxFlag_DrawBorder);
	box->draw_override = UI_DrawHueSaturationCircle;

	if (box->prev_frame) {
		float radius = 0.5f*box->prev_frame->computed_size.x;
		UI_Vec2 middle = {box->prev_frame->computed_position.x + radius, box->prev_frame->computed_position.y + radius};

		if (UI_InputWasPressed(UI_Input_MouseLeft)) {
			UI_Vec2 mouse_pos_relative = UI_SubV2(UI_STATE.mouse_pos, middle);
			float saturation = sqrtf(mouse_pos_relative.x * mouse_pos_relative.x + mouse_pos_relative.y * mouse_pos_relative.y) / radius;
			if (saturation < 1.f) {
				UI_STATE.mouse_clicking_down_box = key;
			}
		}

		if (UI_STATE.mouse_clicking_down_box == key && UI_InputIsDown(UI_Input_MouseLeft)) {
			UI_STATE.mouse_clicking_down_box_new = key;

			UI_Vec2 mouse_pos_relative = UI_SubV2(UI_STATE.mouse_pos, middle);
			*hue = (atan2f(mouse_pos_relative.y, mouse_pos_relative.x) + 3.1415926f) / (3.1415926f * 2.f);
			*hue = fmodf(*hue + 0.5f, 1.f);

			*saturation = sqrtf(mouse_pos_relative.x * mouse_pos_relative.x + mouse_pos_relative.y * mouse_pos_relative.y) / radius;
			if (*saturation > 1.f) *saturation = 1.f;
		}
	}

	UI_HueSaturationCircleData data = {*hue, *saturation};
	UI_BoxAddCustomVal(box, UI_GetHueSaturationCircleDataKey(), data);
	return box;
}

typedef struct { float h, s, v; } UI_HueSaturationValueEditData;
static UI_Key UI_GetHueSaturationValueEditDataKey() { return UI_KEY(); }

static void UI_ColorPickerSaturationSliderDraw(UI_Box* box) {
	UI_HueSaturationValueEditData* data = UI_BoxGetCustomVal(UI_HueSaturationValueEditData, box, UI_GetHueSaturationValueEditDataKey());
	UI_Rect rect = box->computed_rect_clipped;

	uint32_t first_vertex;
	UI_DrawVertex* vertices = UI_AddVertices(4, &first_vertex);
	for (int i = 0; i < 2; i++) {
		float y_t = (float)i / (2 - 1.f);
		float y = UI_Lerp(rect.min.y, rect.max.y, y_t);
		UI_Color color = UI_HSVToColor(data->h, 1.f - y_t, 1.f, 1.f);
		vertices[i*2+0] = UI_DRAW_VERTEX{{rect.min.x, y}, {0, 0}, color};
		vertices[i*2+1] = UI_DRAW_VERTEX{{rect.max.x, y}, {0, 0}, color};
		if (i > 0) {
			UI_AddQuadIndices(first_vertex + i*2, first_vertex + i*2+1, first_vertex + i*2-1, first_vertex + i*2-2, UI_TEXTURE_ID_NIL);
		}
	}

	float tri_y = UI_Lerp(rect.max.y, rect.min.y, data->s);
	UI_DrawTriangle(
		UI_VEC2{rect.min.x - 5.f, tri_y + 5.f},
		UI_VEC2{rect.min.x - 5.f, tri_y - 5.f},
		UI_VEC2{rect.min.x + 5.f, tri_y},
		UI_WHITE, NULL);
}

static void UI_ColorPickerValueSliderDraw(UI_Box* box) {
	UI_HueSaturationValueEditData* data = UI_BoxGetCustomVal(UI_HueSaturationValueEditData, box, UI_GetHueSaturationValueEditDataKey());
	UI_Rect rect = box->computed_rect_clipped;

	uint32_t first_vertex;
	UI_DrawVertex* vertices = UI_AddVertices(4, &first_vertex);
	for (int i = 0; i < 2; i++) {
		float y_t = (float)i / (2 - 1.f);
		float y = UI_Lerp(rect.min.y, rect.max.y, y_t);
		UI_Color color = UI_HSVToColor(data->h, data->s, 1.f - y_t, 1.f);
		vertices[i*2+0] = UI_DRAW_VERTEX{{rect.min.x, y}, {0, 0}, color};
		vertices[i*2+1] = UI_DRAW_VERTEX{{rect.max.x, y}, {0, 0}, color};
		if (i > 0) {
			UI_AddQuadIndices(first_vertex + i*2, first_vertex + i*2+1, first_vertex + i*2-1, first_vertex + i*2-2, UI_TEXTURE_ID_NIL);
		}
	}

	float tri_y = UI_Lerp(rect.max.y, rect.min.y, data->v);
	UI_DrawTriangle(
		UI_VEC2{rect.min.x - 5.f, tri_y + 5.f},
		UI_VEC2{rect.min.x - 5.f, tri_y - 5.f},
		UI_VEC2{rect.min.x + 5.f, tri_y},
		UI_WHITE, NULL);
}

static void UI_DrawColorPickerBox(UI_Box* box) {
	UI_DrawRect(box->computed_rect_clipped, box->style->opaque_bg_color, NULL);
}

static void UI_DrawColorPickerBoxTransparent(UI_Box* box) {
	UI_DrawRect(box->computed_rect_clipped, UI_WHITE, NULL);

	float cell_size = box->computed_size.x / 4.f;
	for (int y = 0; y < 4; y++) {
		for (int x = (y & 1); x < 4; x += 2) {
			UI_Vec2 min = {box->computed_position.x + (float)x * cell_size, box->computed_position.y + (float)y * cell_size};
			UI_Vec2 max = {min.x + cell_size, min.y + cell_size};
			UI_DrawRect(UI_RECT{min, max}, UI_LIGHTGRAY, NULL);
		}
	}

	UI_DrawRect(box->computed_rect_clipped, box->style->opaque_bg_color, NULL);
}

UI_API UI_Box* UI_ColorPicker(UI_Key key, float* hue, float* saturation, float* value, float* alpha) {
	UI_Box* main_box = UI_AddBox(key, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_LayoutInX);
	UI_PushBox(main_box);
	UI_HueSaturationCircle(UI_KEY1(key), 200.f, hue, saturation);

	UI_AddBox(UI_KEY1(key), 5.f, UI_SizeFit(), 0); // pad

	UI_Box* sat_slider_box = UI_AddBox(UI_KEY1(key), 20.f, UI_SizeFlex(1.f), UI_BoxFlag_DrawBorder);
	sat_slider_box->draw_override = UI_ColorPickerSaturationSliderDraw;

	UI_AddBox(UI_KEY1(key), 5.f, UI_SizeFit(), 0); // pad

	UI_Box* val_slider_box = UI_AddBox(UI_KEY1(key), 20.f, UI_SizeFlex(1.f), UI_BoxFlag_DrawBorder);
	val_slider_box->draw_override = UI_ColorPickerValueSliderDraw;

	UI_AddBox(UI_KEY1(key), 5.f, UI_SizeFit(), 0); // pad

	UI_Box* right_box = UI_AddBox(UI_KEY1(key), 160.f, UI_SizeFlex(1.f), 0);
	UI_PushBox(right_box);

	UI_Box* color_box_1;
	UI_Box* color_box_2;
	{
		UI_Box* row = UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
		UI_PushBox(row);

		UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), 0); // pad

		color_box_1 = UI_AddBox(UI_KEY1(key), 80.f, 80.f, UI_BoxFlag_DrawOpaqueBackground);
		color_box_1->draw_override = UI_DrawColorPickerBox;

		color_box_2 = UI_AddBox(UI_KEY1(key), 80.f, 80.f, UI_BoxFlag_DrawOpaqueBackground);
		color_box_2->draw_override = UI_DrawColorPickerBoxTransparent;

		UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), 0); // pad

		UI_PopBox(row);
	}

	{
		UI_Box* right_area_row = UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
		UI_PushBox(right_area_row);

		{
			UI_Box* left_column = UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), 0);
			UI_PushBox(left_column);

			static const STR strings[] = {STR__("R"), STR__("G"), STR__("B"), STR__("A")};
			float rgba[4] = {0, 0, 0, *alpha};
			UI_HSVToRGB(*hue, *saturation, *value, &rgba[0], &rgba[1], &rgba[2]);

			bool edited_rgba = false;
			for (int i = 0; i < 4; i++) {
				UI_Key row_key = UI_HashInt(key, i);
				UI_Box* row = UI_AddBox(UI_KEY1(row_key), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
				UI_PushBox(row);
				UI_AddBoxWithText(UI_KEY1(row_key), UI_SizeFit(), UI_SizeFit(), 0, strings[i]);

				float value_before = rgba[i];
				UI_AddValFloat(UI_KEY1(row_key), UI_SizeFlex(1.f), UI_SizeFit(), &rgba[i]);
				if (rgba[i] != value_before) edited_rgba = true;

				rgba[i] = UI_Max(UI_Min(rgba[i], 1.f), 0.f);
				UI_PopBox(row);
			}

			if (edited_rgba) {
				UI_RGBToHSV(rgba[0], rgba[1], rgba[2], hue, saturation, value);
				*alpha = rgba[3];
			}

			UI_PopBox(left_column);
		}

		{
			UI_Box* right_column = UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), 0);
			UI_PushBox(right_column);

			static const STR strings[] = {STR__("H"), STR__("S"), STR__("V")};
			float* hsv[] = {hue, saturation, value};
			for (int i = 0; i < 3; i++) {
				UI_Key row_key = UI_HashInt(key, i);
				UI_Box* row = UI_AddBox(UI_KEY1(row_key), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
				UI_PushBox(row);
				UI_AddBoxWithText(UI_KEY1(row_key), UI_SizeFit(), UI_SizeFit(), 0, strings[i]);

				UI_AddValFloat(UI_KEY1(row_key), UI_SizeFlex(1.f), UI_SizeFit(), hsv[i]);

				*hsv[i] = UI_Max(UI_Min(*hsv[i], 1.f), 0.f);
				UI_PopBox(row);
			}

			UI_PopBox(right_column);
		}

		UI_PopBox(right_area_row);
	}

	UI_PopBox(right_box);

	UI_PopBox(main_box);

	// Handle input for saturation & value sliders
	if (sat_slider_box->prev_frame && val_slider_box->prev_frame) {
		UI_Rect sat_rect = sat_slider_box->prev_frame->computed_rect_clipped;
		UI_Rect val_rect = val_slider_box->prev_frame->computed_rect_clipped;

		if (UI_InputWasPressed(UI_Input_MouseLeft) && UI_PointIsInRect(sat_rect, UI_STATE.mouse_pos)) {
			UI_STATE.mouse_clicking_down_box = sat_slider_box->key;
		}

		if (UI_STATE.mouse_clicking_down_box == sat_slider_box->key && UI_InputIsDown(UI_Input_MouseLeft)) {
			UI_STATE.mouse_clicking_down_box_new = sat_slider_box->key;
			*saturation = (sat_rect.max.y - UI_STATE.mouse_pos.y) / (sat_rect.max.y - sat_rect.min.y);
			*saturation = UI_Max(UI_Min(*saturation, 1.f), 0.f);
		}

		if (UI_InputWasPressed(UI_Input_MouseLeft) && UI_PointIsInRect(val_rect, UI_STATE.mouse_pos)) {
			UI_STATE.mouse_clicking_down_box = val_slider_box->key;
		}

		if (UI_STATE.mouse_clicking_down_box == val_slider_box->key && UI_InputIsDown(UI_Input_MouseLeft)) {
			UI_STATE.mouse_clicking_down_box_new = val_slider_box->key;
			*value = (val_rect.max.y - UI_STATE.mouse_pos.y) / (val_rect.max.y - val_rect.min.y);
			*value = UI_Max(UI_Min(*value, 1.f), 0.f);
		}
	}

	UI_Color color = UI_HSVToColor(*hue, *saturation, *value, *alpha);
	color_box_1->style = UI_MakeStyle();
	color_box_1->style->opaque_bg_color = UI_COLOR{color.r, color.g, color.b, 255};
	color_box_2->style = UI_MakeStyle();
	color_box_2->style->opaque_bg_color = color;

	UI_HueSaturationValueEditData data = {*hue, *saturation, *value};
	UI_BoxAddCustomVal(sat_slider_box, UI_GetHueSaturationValueEditDataKey(), data);
	UI_BoxAddCustomVal(val_slider_box, UI_GetHueSaturationValueEditDataKey(), data);

	return main_box;
}

#endif // UI_IMPLEMENTATION