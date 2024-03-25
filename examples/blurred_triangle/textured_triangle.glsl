#ifdef GPU_STAGE_VERTEX
	layout(location = 0) in vec2 vs_position;
	
	layout(location = 0) out vec2 fs_position;
	
	void main() {
		fs_position = vs_position;
		gl_Position = vec4(vs_position, 0., 1);
	}
#else
	GPU_BINDING(MY_TEXTURE) texture2D my_texture;
	GPU_BINDING(MY_SAMPLER) sampler my_sampler;
	
	layout(location = 0) in vec2 fs_position;

	layout(location = 0) out vec4 out_color;
	
	void main() {
		out_color = texture(sampler2D(my_texture, my_sampler), fs_position*4.);
	}
#endif