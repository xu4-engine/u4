/*
  World Tilemap Shader
*/

#if defined(VERTEX)

uniform mat4 transform;
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 uv;
out vec4 texCoord;
out vec2 shadowCoord;

void main() {
	texCoord = uv;
	gl_Position = transform * vec4(position, 1.0);
	shadowCoord = (gl_Position.xy + 1.0) * 0.5;
};

#elif defined(FRAGMENT)

uniform sampler2D cmap;
uniform sampler2D mmap;
uniform sampler2D shadowMap;
uniform vec2 scroll;
in vec4 texCoord;
in vec2 shadowCoord;
out vec4 fragColor;

void main() {
	vec4 texel;
	vec4 material = texture(mmap, texCoord.st);
	if (material.b > 0.95) {
		vec2 tc = texCoord.sq;
		float nv = texCoord.p - scroll.t * 0.3;
		tc.t += (nv - floor(nv)) * scroll.s;
		texel = texture(cmap, tc);
		/*
		float nv = texCoord.p + scroll.t;
		texel = vec4(vec3(nv - floor(nv)), 1.0);
		*/
	} else {
		texel = texture(cmap, texCoord.st);
	}
	vec4 shade = texture(shadowMap, shadowCoord);
	fragColor = vec4(shade.aaa, 1.0) * texel;
}

#endif
