#include "skinned_mesh.hlsli"

#define POINT 0
#define LINEAR 1
#define ANISOTROPIC 2
SamplerState sampler_states[3] : register(s0);
Texture2D texture_maps[4] : register(t0);

float4 main(GS_OUT pin) : SV_TARGET
{
	float4 color = texture_maps[0].Sample(sampler_states[ANISOTROPIC], pin.texcoord);
	float alpha = color.a;

#if 1
	
	// Inverse gamma process
	const float GAMMA = 2.2;
	color.rgb = pow(color.rgb, GAMMA);
#endif

	float3 N = normalize(pin.face_normal.xyz);
	float3 L = normalize(-light_direction.xyz);
	float3 diffuse = color.rgb * max(0, dot(N, L));
	float3 V = normalize(camera_position.xyz - pin.world_position.xyz);
	float3 specular = pow(max(0, dot(N, normalize(V + L))), 128);

	return float4(diffuse/* + specular*/, alpha) * pin.color;
}