layout(push_constant) uniform Constants {
	vec2 blur_unit;
	float time;
} constants;

#ifdef GPU_STAGE_VERTEX
	layout(location = 0) out vec2 fs_uv;

	// Generate fullscreen triangle
	void main() {
		vec2 uv = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2) * 2. - vec2(1.);
		fs_uv = uv*0.5 + 0.5;
		gl_Position = vec4(uv.x, uv.y, 0, 1);
	}
#else
	GPU_BINDING(BLUR_INPUT) texture2D BLUR_INPUT;
	GPU_BINDING(BLUR_SAMPLER) sampler BLUR_SAMPLER;
	
	layout(location = 0) in vec2 fs_uv;
	layout(location = 0) out vec4 out_color;
	
	const int SAMPLE_COUNT = 11;

	const float OFFSETS[11] = float[11](
		-9.406430666971303,
		-7.425801606895373,
		-5.445401742210555,
		-3.465172537482815,
		-1.485055021558738,
		0.4950160492928826,
		2.4751038298192056,
		4.455269417428358,
		6.435576703455285,
		8.41608382089975,
		10
	);

	const float WEIGHTS[11] = float[11](
		0.0276904183309881,
		0.05417056378718292,
		0.09049273288108622,
		0.12908964856395883,
		0.15725301673321052,
		0.16358389071865348,
		0.14531705460040129,
		0.11023607138371759,
		0.0714102715628023,
		0.03950209624702099,
		0.011254235190977919
	);

	void main() {
		// https://lisyarus.github.io/blog/graphics/2023/02/24/blur-coefficients-generator.html
		
		if (fs_uv.x + 0.2*sin(constants.time) > 0.5) {
			out_color = vec4(0);
			for (int i = 0; i < SAMPLE_COUNT; i++) {
				vec2 offset = constants.blur_unit * OFFSETS[i];
				out_color += texture(sampler2D(BLUR_INPUT, BLUR_SAMPLER), fs_uv + offset) * WEIGHTS[i];
			}
		}
		else {
			out_color = texture(sampler2D(BLUR_INPUT, BLUR_SAMPLER), fs_uv);
		}
	}

#endif