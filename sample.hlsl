// DYNAMIC_TEXTURE
#include "fullscreen_quad.hlsli"

cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
	row_major float4x4 view_projection;
	float4 light_direction;
	float4 camera_position;
	// DYNAMIC_TEXTURE
	float time;
	float elapsed_time;
};

static const float cloudscale = 1.1;
static const float speed = 0.03;
static const float clouddark = 0.5;
static const float cloudlight = 0.3;
static const float cloudcover = 0.2;
static const float cloudalpha = 8.0;
static const float skytint = 0.5;

static const float2x2 m = float2x2(1.6, 1.2, -1.2, 1.6);
static const float3 skycolour1 = float3(0.2, 0.4, 0.6);
static const float3 skycolour2 = float3(0.4, 0.7, 1.0);

// Hash and noise functions
float2 hash(float2 p) {
	p = float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)));
	return -1.0 + 2.0 * frac(sin(p) * 43758.5453123);
}

float noise(float2 p) {
	const float K1 = 0.366025404; // (sqrt(3)-1)/2;
	const float K2 = 0.211324865; // (3-sqrt(3))/6;
	float2 i = floor(p + (p.x + p.y) * K1);
	float2 a = p - i + (i.x + i.y) * K2;
	float2 o = (a.x > a.y) ? float2(1.0, 0.0) : float2(0.0, 1.0);
	float2 b = a - o + K2;
	float2 c = a - 1.0 + 2.0 * K2;
	float3 h = max(0.5 - float3(dot(a, a), dot(b, b), dot(c, c)), 0.0);
	float3 n = h * h * h * h * float3(dot(a, hash(i + 0.0)), dot(b, hash(i + o)), dot(c, hash(i + 1.0)));
	return dot(n, float3(70.0, 70.0, 70.0));
}

float fbm(float2 n) {
	float total = 0.0;
	float amplitude = 0.1;
	for (int i = 0; i < 7; i++) {
		total += noise(n) * amplitude;
		n = mul(n, m);
		amplitude *= 0.4;
	}
	return total;
}

float4 main(VS_OUT pin) : SV_TARGET
{
	float2 fragCoord = pin.position.xy;
	float2 iResolution = float2(512, 512);
	float iTime = time;

	// Normalized UVs
	float2 p = fragCoord / iResolution;
	float2 uv = p * float2(iResolution.x / iResolution.y, 1.0);
	float t = iTime * speed;
	float q = fbm(uv * cloudscale * 0.5);

	// Ridged noise
	float r = 0.0;
	uv *= cloudscale;
	uv -= q - t;
	float weight = 0.8;
	for (int i = 0; i < 8; i++) {
		r += abs(weight * noise(uv));
		uv = mul(uv, m) + t;
		weight *= 0.7;
	}

	// Main cloud shape
	float f = 0.0;
	uv = p * float2(iResolution.x / iResolution.y, 1.0);
	uv *= cloudscale;
	uv -= q - t;
	weight = 0.7;
	for (int i = 0; i < 8; i++) {
		f += weight * noise(uv);
		uv = mul(uv, m) + t;
		weight *= 0.6;
	}
	f *= r + f;

	// Cloud color texture
	float c = 0.0;
	t = iTime * speed * 2.0;
	uv = p * float2(iResolution.x / iResolution.y, 1.0);
	uv *= cloudscale * 2.0;
	uv -= q - t;
	weight = 0.4;
	for (int i = 0; i < 7; i++) {
		c += weight * noise(uv);
		uv = mul(uv, m) + t;
		weight *= 0.6;
	}

	// Additional ridge texture
	float c1 = 0.0;
	t = iTime * speed * 3.0;
	uv = p * float2(iResolution.x / iResolution.y, 1.0);
	uv *= cloudscale * 3.0;
	uv -= q - t;
	weight = 0.4;
	for (int i = 0; i < 7; i++) {
		c1 += abs(weight * noise(uv));
		uv = mul(uv, m) + t;
		weight *= 0.6;
	}

	c += c1;

	// Final color mix
	float3 skycolour = lerp(skycolour2, skycolour1, p.y);
	float3 cloudcolour = float3(1.1, 1.1, 0.9) * saturate(clouddark + cloudlight * c);
	f = cloudcover + cloudalpha * f * r;

	float3 result = lerp(skycolour, saturate(skytint * skycolour + cloudcolour), saturate(f + c));
	return float4(result, 1.0);
}
