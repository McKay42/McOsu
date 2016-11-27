#version 110

varying vec2 tex_coord;

void main()
{	
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	
	gl_FrontColor = gl_Color;
	tex_coord = gl_MultiTexCoord0.xy;
}