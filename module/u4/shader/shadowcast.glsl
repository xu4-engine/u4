/*
  2D Shadowcasting Shader
*/

#if defined(VERTEX)

uniform mat4 transform;
layout(location = 0) in vec3 position;

void main() {
	gl_Position = transform * vec4(position, 1.0);
}

#elif defined(FRAGMENT)

uniform vec4 vport;			// Viewport pixel (x, y, width, height)
uniform vec3 viewer;	    // World (x, y, scale)
uniform ivec3 shape_count;	// (left, center, right)
uniform vec3 shapes[128];	// (x, y, type) for each shape.
out vec4 fragColor;

vec3 shapeCube = vec3(0.5, 0.5, 0.5);
const float farClip = 20.0;

float sdBox(vec3 p, vec3 b) {
	vec3 q = abs(p) - b;
	return length(max(q,0.0)) + min(max(q.x,q.z),0.0);	// 2D test.
}

float sceneSDF(vec3 pnt, ivec4 group) {
	ivec2 it;
	float nd = farClip;

	if (pnt.x < 0.0)
		it = group.xy;
	else
		it = group.zw;

	for ( ; it.x < it.y; it.x++) {
		vec3 spos = shapes[it.x];
		float d = sdBox(pnt - vec3(spos.x, 0.0, spos.y), shapeCube);
		nd = min(nd, d);
	}
	return min(1.0, nd);    // Cap ray advance to handle group transition.
}

void main() {
	const float surfEpsilon = 0.01;
	vec3 pnt;
	float dist;

	vec2 uv = (gl_FragCoord.xy - vport.xy - vport.zw * 0.5) / vport.zw;
	vec2 vp = viewer.xy;

	uv *= viewer.z;

	vec3 rayStart = vec3(uv.s, 0.0,-uv.t);
	vec3 toViewer = vec3(vp.x, 0.0, vp.y) - rayStart;
	vec3 rayDir = normalize(toViewer);
	float rayLen = length(toViewer);
	float rpos = 0.0;
	float visible = 1.0;
	float inside = 0.0;
	ivec4 group;
	int i;

	// Setup the left and right shape group iterators.
	i = shape_count.x + shape_count.y;
	group = ivec4(0, i, shape_count.x, i + shape_count.z);

	dist = sceneSDF(rayStart, group);
	if (dist < 0.0) {
		rpos = 1.0;
		inside = 1.0;
	}

	for (i = 0; i < 32; i++) {
		if (rpos >= rayLen)
			break;                  // Reached viewer.
		pnt = rayStart + rayDir * rpos;
		dist = sceneSDF(pnt, group);
		if (dist < surfEpsilon) {
			visible = 0.0;          // Inside a surface.
			break;
		}
		rpos += dist;               // Advance along view ray.
	}

	// If the ray is "slowed" travelling parallel to a series of blocks
	// and reaches the loop limit just mark the fragment as shadowed.
	if (i == 32)
		visible = 0.0;

	fragColor = vec4(0.0, inside, 0.0, visible);
}

#endif
