/*
  Multi-channel Signed Distance Field
  https://github.com/Chlumsky/msdfgen
*/

#if defined(VERTEX)

uniform mat4 transform;
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 uv;
out vec4 texCoord;

void main() {
	texCoord = uv;
	gl_Position = transform * vec4(position, 1.0);
}

#elif defined(FRAGMENT)

uniform sampler2D msdf;
uniform vec4 bgColor;
uniform vec4 fgColor;
//uniform float screenPxRange;
in vec4 texCoord;
out vec4 fragColor;

#define screenPxRange   texCoord.p

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

void main() {
	vec4 msd = texture(msdf, texCoord.st);
	if (screenPxRange < 0.001f) {
		// Solid color
		fragColor = msd;
	} else {
		float sd = median(msd.r, msd.g, msd.b);
		float screenPxDistance = screenPxRange * (sd - 0.5);
		float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
		fragColor = mix(bgColor, fgColor, opacity);
	}
}

#endif
