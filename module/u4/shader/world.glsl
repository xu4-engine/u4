/*
  World Tilemap Shader
*/

#if defined(VERTEX)

uniform mat4 transform;
layout(location = 0) in vec3 position;
layout(location = 1) in vec4 uv;
out vec3 vertex;
out vec4 texCoord;
out vec2 shadowCoord;

void main() {
	vertex = position;
	texCoord = uv;
	gl_Position = transform * vec4(position, 1.0);
	shadowCoord = (gl_Position.xy + 1.0) * 0.5;
};

#elif defined(FRAGMENT)

uniform sampler2D cmap;
uniform sampler2D mmap;
uniform sampler2D shadowMap;
uniform sampler2D noise2D;
uniform vec2 scroll;
in vec3 vertex;
in vec4 texCoord;
in vec2 shadowCoord;
out vec4 fragColor;

#define TIME	scroll.t

float noise(vec2 p) {
	p *= 0.04;
	float f = texture(noise2D, p).r + texture(noise2D, p*2.0).g * 0.5;
	return clamp(f*f*f*0.7, 0.0, 1.0);
}

float fbm(vec2 uv) {
	float f;
	mat2 m = mat2(1.6, 1.2, -1.2, 1.6);
	f  = 0.5000 * noise(uv); uv = m*uv;
	f += 0.2500 * noise(uv); uv = m*uv;
	f += 0.1250 * noise(uv); uv = m*uv;
	f += 0.0625 * noise(uv); uv = m*uv;
	return 0.5 + 0.5*f;
}

vec4 flame(vec2 uv, float burnRate) {
	vec2 q = vec2(uv.s - 0.5, uv.t * 2.0 - 0.25);
	float n = fbm(burnRate * q - vec2(0.0, burnRate * TIME));
	float baseW = length(q*vec2(1.8+q.y*1.5, 1.0)) - n * max(0.0, q.y + 0.25);
	float c = 1.0 - 16.0 * pow(max(0.0, baseW), 1.2);
	float c1 = n * c * (1.2 - pow(2.5 * uv.y, 4.0));
	c1 = clamp(c1, 0.0, 1.0);
	float c3 = c1*c1*c1;
	vec3 col = vec3(1.5*c1, 1.5*c3, c3*c3);
	float a = c * (1.0 - pow(uv.y, 3.0));
	return vec4(mix(vec3(0.0), col, a), a);
}

float sdBox(vec2 p, vec2 b) {
    p = abs(p) - b;
    return length(max(p, 0.0)) + min(0.0, max(p.x, p.y));
}

#define fillDraw(dist, scol) color = mix(scol, color, step(0.0, dist))

vec4 flag(vec2 p) {
	const vec3 flagColor = vec3(1.0, 0.2, 0.2);
	const vec3 stripeCol = vec3(1.0, 1.0, 0.0);
	vec3 color = vec3(0.0);
	float flagW = 0.5;

	p = p * 2.0 - 1.0;		// Unit view -1.0 to 1.0

	flagW -= sin(3.0 * TIME) * 0.03;
	vec2 flagSize = vec2(flagW, 0.5);
	vec2 fp = p - vec2(flagW, 0.0);		// Flag position with pole at 0.0 X.

	// Shear with a waving motion.  Use vertex to randomize by position.
	float wave = sin(4.0 * (p.x - TIME) + vertex.x);
	float nwave = noise(vec2(p.x*0.5 - TIME * 0.5, p.y + vertex.y));
	fp.y += 0.1 * p.x * (wave + nwave);

	// Shadow the wave ripples.
	float shadow = 0.5 * (wave + nwave * 0.3);
	shadow *= shadow;

	// Flag background & stripes
	fillDraw(sdBox(fp, flagSize), flagColor - shadow);
	fillDraw(sdBox(fp + vec2(0.0,0.40), vec2(flagW,0.05)), stripeCol - shadow);
	fillDraw(sdBox(fp - vec2(0.0,0.40), vec2(flagW,0.05)), stripeCol - shadow);
	return vec4(color, step(0.01, color.r));
}

void main() {
	vec4 texel;
	vec4 material = texture(mmap, texCoord.st);

	// Check texCoord.p first or else the material.b path gets executed for
	// some pixels at the bottom of flag quads.

	if (texCoord.p == 2.0) {
		texel = flag(texCoord.st);
	} else if (material.b > 0.95) {
		vec2 tc = texCoord.sq;
		float nv = texCoord.p - scroll.t * 0.3;
		tc.t += (nv - floor(nv)) * scroll.s;
		texel = texture(cmap, tc);
		/*
		float nv = texCoord.p + scroll.t;
		texel = vec4(vec3(nv - floor(nv)), 1.0);
		*/
	} else if (material.r > 0.05) {
		texel = max(texture(cmap, texCoord.st),
					flame(vec2(texCoord.p, texCoord.q*0.6), 1.0));
	} else {
		texel = texture(cmap, texCoord.st);
	}
	vec4 shade = texture(shadowMap, shadowCoord);
	fragColor = vec4(shade.aaa, 1.0) * texel;
}

#endif
