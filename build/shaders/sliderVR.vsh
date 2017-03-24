#version 110

uniform mat4 matrix;
varying vec2 tex_coord;

void main()
{	
	tex_coord = gl_MultiTexCoord0.xy;
	vec4 vertex = gl_Vertex;
	gl_Position = matrix * gl_ModelViewMatrix * vertex;
	gl_FrontColor = gl_Color;
}

