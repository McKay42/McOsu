#version 110

uniform float time;
uniform sampler2D tex;

varying vec2 tex_coord;
varying float vtx_alpha;

void main()
{
	vec4 color = texture2D(tex, tex_coord);
	color.a *= vtx_alpha;
	
	/*
	float speedMultiplier = 0.8;
	float spreadMultiplier = 2.0;
	color.r = sin(time*speedMultiplier + vtx_alpha*spreadMultiplier + 0.0)*0.5 + 0.5;
	color.g = sin(time*speedMultiplier + vtx_alpha*spreadMultiplier + 2.0)*0.5 + 0.5;
	color.b = sin(time*speedMultiplier + vtx_alpha*spreadMultiplier + 4.0)*0.5 + 0.5;
	*/
	
	gl_FragColor = color; 
}