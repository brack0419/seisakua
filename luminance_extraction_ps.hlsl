
#include "fullscreen_quad.hlsli"

cbuffer PARAMETRIC_CONSTANT_BUFFER : register(b2)
{
	float extraction_threshold;
	float gaussian_sigma;
	float bloom_intensity;
	float exposure;
}

#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
#define LINEAR_BORDER_BLACK 4
#define LINEAR_BORDER_WHITE 5
SamplerState sampler_states[5] : register(s0);
Texture2D texture_maps[4] : register(t0);
float4 main(VS_OUT pin) : SV_TARGET
{
	float4 color = texture_maps[0].Sample(sampler_states[LINEAR_BORDER_BLACK], pin.texcoord);
	float alpha = color.a;

	color.rgb = step(extraction_threshold, dot(color.rgb, float3(0.299, 0.587, 0.114))) * color.rgb;

	return float4(color.rgb, alpha);
}