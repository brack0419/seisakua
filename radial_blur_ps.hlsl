
#include "fullscreen_quad.hlsli"

cbuffer radial_blur_constants : register(b2)
{
	float2 blur_center; // center point where the blur is applied
	float blur_strength; // blurring strength
	float blur_radius; // blurred radiu
	float blur_decay; // percentage of the maximum radius at which the intensity of the blur begins to decay
};

#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
#define LINEAR_BORDER_BLACK 3
#define LINEAR_BORDER_WHITE 4
#define LINEAR_CLAMP 5
SamplerState sampler_states[6] : register(s0);
Texture2D texture_maps[4] : register(t0);

float4 main(VS_OUT pin) : SV_TARGET
{
#if 0
	uint mip_level = 0, width, height, number_of_levels;
	texture_maps[0].GetDimensions(mip_level, width, height, number_of_levels);
#endif
	
	const int samples = 16;

	float2 center_to_pixel = pin.texcoord - blur_center;
	float distance = length(center_to_pixel);
	
	float factor = blur_strength / float(samples) * distance;
#if 1
	factor *= smoothstep(blur_radius, blur_radius * blur_decay, distance);
#endif
	
	float3 color = 0.0;
	for (int i = 0; i < samples; i++)
	{
		float sample_offset = 1.0 - factor * i;
		color += texture_maps[0].Sample(sampler_states[LINEAR_CLAMP], blur_center + (center_to_pixel * sample_offset)).rgb;
	}
	color /= float(samples);
	
	
	
	
	
	
	
#if 0
	// Tone mapping : HDR -> SDR
	const float exposure = 1.2;
	color = 1 - exp(-color * exposure);

	// Gamma process
	const float gamma = 2.2;
	color = pow(color, 1.0 / gamma);
#endif

	return float4(color, 1);
}
