struct ViewUniform {
  viewportSize : vec2f,
  pointSize : f32,
  padding : f32,
};
@group(0) @binding(0) var<uniform> viewUniform : ViewUniform;

struct VertexOutput {
    @builtin(position) position : vec4f,
    @location(0) color : vec3f
};

@vertex
fn vs_main(@location(0) vertexPosition : vec3f, @location(1) vertexColor : vec3f) -> VertexOutput {
    var vertexOut : VertexOutput;
    vertexOut.position = vec4f(vertexPosition, 1.0);
    vertexOut.color = vertexColor;
	return vertexOut;
}

@fragment
fn fs_main(vertex : VertexOutput) -> @location(0) vec4f {
	return vec4f(vertex.color, 1.0);
}