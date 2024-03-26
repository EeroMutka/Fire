// This file is part of the Fire UI library, see "fire_ui.h"

layout(push_constant) uniform Constants {
	vec2 pixel_size;
} constants;

#ifdef GPU_STAGE_VERTEX
	layout (location = 0) in vec2 vs_position;
	layout (location = 1) in vec2 vs_uv;
	layout (location = 2) in vec4 vs_color;

	layout(location = 0) out vec2 fs_uv;
	layout(location = 1) out vec4 fs_color;

	void main() {
		// Transform vertices from screen space to clip space
		gl_Position = vec4(2.*vs_position*constants.pixel_size - 1., 0., 1);
		fs_uv = vs_uv;
		fs_color = vs_color;
	}

#else
	GPU_BINDING(TEXTURE) texture2D TEXTURE;
	GPU_BINDING(SAMPLER_LINEAR_REPEAT) sampler SAMPLER_LINEAR_REPEAT;

	layout(location = 0) in vec2 fs_uv;
	layout(location = 1) in vec4 fs_color;

	layout (location = 0) out vec4 out_color;

	void main() {
		out_color = fs_color * texture(sampler2D(TEXTURE, SAMPLER_LINEAR_REPEAT), fs_uv);
	}

#endif