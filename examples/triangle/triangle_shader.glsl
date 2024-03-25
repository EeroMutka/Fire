#ifdef GPU_STAGE_VERTEX
	layout (location = 0) in vec2 vs_position;
	layout (location = 1) in vec3 vs_color;

	layout(location = 0) out vec3 fs_color;
	
	void main() {
		fs_color = vs_color;
		gl_Position = vec4(vs_position, 0, 1);
	}
#else
	layout(location = 0) in vec3 fs_color;

	layout (location = 0) out vec4 out_color;
	
	void main() {
		out_color = vec4(fs_color, 1);
	}
#endif