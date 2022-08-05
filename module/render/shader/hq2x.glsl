/*
* HQX Shader
*
* Copyright (C) 2003 Maxim Stepin ( maxst@hiend3d.com )
* Copyright (C) 2010 Cameron Zemek ( grom@zeminvaders.net )
* Copyright (C) 2014 Jules Blok ( jules@aerix.nl )
*
* License: GNU Lesser General Public License v2.1
*/

#define SCALE 2.0		// 2, 3, or 4

#if defined(VERTEX)

layout(location = 0) in vec3 vertexCoord;
layout(location = 1) in vec2 texCoord;
 
uniform mat4 MVPMatrix;
uniform vec2 TextureSize;

out vec4 vTexCoord[4];

void main()
{
	gl_Position = MVPMatrix * vec4(vertexCoord, 1.0);

	vec2 ps = 1.0/TextureSize;
	float dx = ps.x;
	float dy = ps.y;

	vTexCoord[0].zw = ps;
	vTexCoord[0].xy = texCoord.xy;
	vTexCoord[1] = texCoord.xxxy + vec4(-dx, 0.0, dx, -dy);	//  w1 | w2 | w3
	vTexCoord[2] = texCoord.xxxy + vec4(-dx, 0.0, dx, 0.0);	//  w4 | w5 | w6
	vTexCoord[3] = texCoord.xxxy + vec4(-dx, 0.0, dx,  dy);	//  w7 | w8 | w9
}

#elif defined(FRAGMENT)

uniform sampler2D Texture;
uniform sampler2D LUT;
uniform vec2 TextureSize;

in vec4 vTexCoord[4];
out vec4 fragColor;

const mat3 yuv_matrix = mat3(0.299, 0.587, 0.114, -0.169, -0.331, 0.5, 0.5, -0.419, -0.081);
const vec3 yuv_threshold = vec3(48.0/255.0, 7.0/255.0, 6.0/255.0);
const vec3 yuv_offset = vec3(0.0, 0.5, 0.5);

bool diff(vec3 yuv1, vec3 yuv2)
{
	bvec3 res = greaterThan(abs((yuv1 + yuv_offset) - (yuv2 + yuv_offset)), yuv_threshold);
	return res.x || res.y || res.z;
}

void main()
{
	vec2 fp = fract(vTexCoord[0].xy*TextureSize);
	vec2 quad = sign(-0.5 + fp);
	mat3 yuv = transpose(yuv_matrix);

	float dx = vTexCoord[0].z;
	float dy = vTexCoord[0].w;
	vec3 p1  = texture(Texture, vTexCoord[0].xy).rgb;
	vec3 p2  = texture(Texture, vTexCoord[0].xy + vec2(dx, dy) * quad).rgb;
	vec3 p3  = texture(Texture, vTexCoord[0].xy + vec2(dx, 0.0) * quad).rgb;
	vec3 p4  = texture(Texture, vTexCoord[0].xy + vec2(0.0, dy) * quad).rgb;
	mat4x3 pixels = mat4x3(p1, p2, p3, p4);

	vec3 w1  = yuv * texture(Texture, vTexCoord[1].xw).rgb;
	vec3 w2  = yuv * texture(Texture, vTexCoord[1].yw).rgb;
	vec3 w3  = yuv * texture(Texture, vTexCoord[1].zw).rgb;

	vec3 w4  = yuv * texture(Texture, vTexCoord[2].xw).rgb;
	vec3 w5  = yuv * p1;
	vec3 w6  = yuv * texture(Texture, vTexCoord[2].zw).rgb;

	vec3 w7  = yuv * texture(Texture, vTexCoord[3].xw).rgb;
	vec3 w8  = yuv * texture(Texture, vTexCoord[3].yw).rgb;
	vec3 w9  = yuv * texture(Texture, vTexCoord[3].zw).rgb;

	bvec3 pattern[3];
	pattern[0] =  bvec3(diff(w5, w1), diff(w5, w2), diff(w5, w3));
	pattern[1] =  bvec3(diff(w5, w4), false       , diff(w5, w6));
	pattern[2] =  bvec3(diff(w5, w7), diff(w5, w8), diff(w5, w9));
	bvec4 cross = bvec4(diff(w4, w2), diff(w2, w6), diff(w8, w4), diff(w6, w8));

	vec2 index;
	index.x = dot(vec3(pattern[0]), vec3(1.0, 2.0, 4.0)) +
			  dot(vec3(pattern[1]), vec3(8.0, 0.0, 16.0)) +
			  dot(vec3(pattern[2]), vec3(32.0, 64.0, 128.0));
	index.y = dot(vec4(cross), vec4(1.0, 2.0, 4.0, 8.0)) * (SCALE * SCALE) +
			  dot(floor(fp * SCALE), vec2(1.0, SCALE));

	vec2 step = 1.0 / vec2(256.0, 16.0 * (SCALE * SCALE));
	vec2 offset = step / 2.0;
	vec4 weights = texture(LUT, index * step + offset);
	float sum = dot(weights, vec4(1.0));
	vec3 res = pixels * (weights / sum);

	fragColor.rgb = res;
}

#endif
