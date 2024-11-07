#version 110

varying vec3 vertex;
varying vec3 vertexCam;
varying vec3 normal;
varying vec3 normalCam;

uniform float time;
uniform mat4 modelMatrix;
uniform vec3 eye;
uniform float brightness;
uniform float ambient;
uniform vec3 lightPosition;
uniform float diffuse;
uniform float phong;
uniform float phongExponent;
uniform float rim;

void main()
{
	// constants
	float brightnessHDR = brightness;
	vec3 ambient_color = vec3(1.0, 1.0, 1.0);
	vec3 light_position = lightPosition + vec3(0.0, 0.0, -modelMatrix[3].z);
	vec3 light_color = vec3(1.0, 1.0, 1.0);
	float light_diffuse = ((1.0 - ambient) / 3.0) * diffuse;
	float light_phong = ((1.0 - ambient) / 3.0) * phong;
	float light_rim = ((1.0 - ambient) / 3.0) * rim;
	
	vec4 out_color = vec4(0.0, 0.0, 0.0, 1.0);
	
	vec3 position = vec3(modelMatrix * vec4(vertex, 1.0));
	vec3 pointToLightVector = light_position - position;
	
	// ambient
	out_color.rgb += ambient_color * ambient;

	// lambert
	float distFromLightToPoint = length(pointToLightVector);
	float diffuseFactor = clamp(dot(normal, pointToLightVector) / distFromLightToPoint, 0.0, 1.0);
	float diffuseMultiplier = smoothstep(0.0, 1.0, diffuseFactor);
	out_color.rgb += light_color * diffuseMultiplier * light_diffuse;
	
	// phong
	vec3 eyeToPointVector = normalize(eye - position);
	vec3 reflectedPointToLightVector = normalize(-reflect(pointToLightVector, normal));
	float specularDot = max(dot(reflectedPointToLightVector, eyeToPointVector), 0.0);
	float specularMultiplier = clamp(pow(specularDot, phongExponent), 0.0, 1.0);
	out_color.rgb += light_color * specularMultiplier * light_phong;
	float specularHDR = clamp(pow(specularDot, phongExponent * 5.0), 0.0, 1.0);
	out_color.rgb += light_color * specularHDR * 0.25 * phong;
	
	// rim
	float rimDot = max(dot(normalCam, normalize(-vertexCam)), 0.0);
	float rimFactor = 1.0 - rimDot;
	float rimMultiplier = smoothstep(0.375, 1.0, rimFactor);
	out_color.rgb += light_color * rimMultiplier * light_rim;
	
	// brightness
	out_color.rgb *= 1.0 + (brightnessHDR - ambient - (diffuseMultiplier * light_diffuse) - (specularMultiplier * light_phong) - (rimMultiplier * light_rim));
	
	// (regular color mod)
	gl_FragColor = out_color * gl_Color;
}