#version 110

uniform float time;
uniform mat4 modelMatrixInverseTransposed;

varying vec3 vertex;
varying vec3 vertexCam;
varying vec3 normal;
varying vec3 normalCam;

mat3 mat4_to_mat3(mat4 m4)
{
	return mat3(m4[0][0], m4[0][1], m4[0][2],
				m4[1][0], m4[1][1], m4[1][2],
				m4[2][0], m4[2][1], m4[2][2]);
}

void main()
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_FrontColor = gl_Color;
	
	mat3 normalMatrix = mat4_to_mat3(modelMatrixInverseTransposed);
	normal = normalize(normalMatrix * gl_Normal);
	normalCam = normalize(gl_NormalMatrix * gl_Normal);
	vertex = vec3(gl_Vertex);
	vertexCam = vec3(gl_ModelViewMatrix * gl_Vertex);
}