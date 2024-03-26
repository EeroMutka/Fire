struct vertexdata {
	float3 position : POS;
	float3 color    : COL;
};

struct pixeldata {
	float4 position : SV_POSITION;
};

pixeldata vertex_shader(vertexdata vertex) {
	pixeldata output;
	output.position = float4(vertex.position, 1);
	return output;
}

float4 pixel_shader(pixeldata input) : SV_TARGET { 
	return float4(1, 0, 0, 1);
}