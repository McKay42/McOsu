#version 110

uniform float time;

varying vec2 tex_coord;
varying float vtx_alpha;

void main()
{	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.x, gl_Vertex.y, 0.0, 1.0);
	gl_FrontColor = gl_Color;
	
	tex_coord = gl_MultiTexCoord0.xy;
	vtx_alpha = gl_Vertex.z;
}