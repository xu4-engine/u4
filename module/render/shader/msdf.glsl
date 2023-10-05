/*
  Multi-channel Signed Distance Field
  https://github.com/Chlumsky/msdfgen
*/

#if defined(VERTEX)

uniform mat4 transform;
uniform vec3 origin;
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 uv;
out vec4 texCoord;

void main() {
	texCoord = uv;
	gl_Position = transform * vec4(position + origin, 1.0);
}

#elif defined(FRAGMENT)

uniform sampler2D msdf;
uniform sampler2D cmap;
//uniform vec4 bgColor;
uniform vec4 fgColor;
uniform vec3 widgetFx;	// x: widget-id, y: mode (0=normal, 1=pressed)
//uniform float screenPxRange;
in vec4 texCoord;
out vec4 fragColor;

#define screenPxRange   texCoord.p
#define widgetId        texCoord.p
#define clutIndex       texCoord.q

float median(float r, float g, float b) {
	return max(min(r, g), min(max(r, g), b));
}

void main() {
	if (screenPxRange < 0.001f) {
		// Solid color
		fragColor = texture(cmap, texCoord.st);
		if (widgetId == widgetFx.x) {
			if (widgetFx.y > 0.9)
				fragColor.rgb = mix(fragColor.rgb, vec3(0.0), 0.2);
		}
	} else {
		vec4 msd = texture(msdf, texCoord.st);
		float sd = median(msd.r, msd.g, msd.b);
		float screenPxDistance = screenPxRange * (sd - 0.5);
		float opacity = clamp(screenPxDistance + 0.5, 0.0, 1.0);
		vec4 color;
		if (clutIndex > 0.0)
			color = texelFetch(cmap, ivec2(clutIndex, 0), 0);
		else
			color = fgColor;
		fragColor = vec4(color.rgb, opacity);
	}
}

#endif
