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
	// ‚±‚±‚ÉiMouse‘Š“–‚Ì•Ï”‚ğ’Ç‰Á‚µ‚½‚¢ê‡‚Í’Ç‹L‰Â
	// float2 iMouse;
};

#define ITE_MAX 15

float2 rot(float2 p, float a)
{
	return float2(cos(a) * p.x - sin(a) * p.y, sin(a) * p.x + cos(a) * p.y);
}

float3 tex(float2 uv)
{
	float3 c = frac(float3(uv.x, uv.y, uv.y));
	if (fmod(uv.x * 2.0, 2.0) < 0.9) return float3(0, 0, 0);
	if (fmod(uv.y * 1.0, 1.0) < 0.9) return float3(0, 0, 0);
	return c;
}

float4 main(VS_OUT pin) : SV_TARGET
{
	float2 iResolution = float2(1920/2, 1080/2);
	float iTime = time;

	// iMouse‚ª–³‚¢‚½‚ß (0,0) ŒÅ’è‚É‚·‚é
	float2 iMouse = float2(0, 0);

	float M = iTime * 0.5;
	float fog = 1.0;
	float2 uv = 2.0 * (pin.position.xy / iResolution.xy) - 1.0;
	uv *= float2(iResolution.x / iResolution.y, 1.0);
	uv = rot(uv, -iMouse.y * 0.015);

	float3 c = float3(0, 0, 0);
	for (int i = 0; i < ITE_MAX; i++)
	{
		float divisor = abs(uv.y / (float(i) + 1.0));
		if (divisor == 0) divisor = 0.0001; // ƒ[ƒœZ–h~
		c = tex(float2(uv.x / divisor + M + iMouse.x * 0.015, abs(uv.y)));
		if (length(c) > 0.5) break;
		uv = uv.yx * 1.3;
		fog *= 0.9;
	}

	float4 fragColor = 1.0 - float4(c.x, c.y, c.y, c.y) * (fog * fog);
	fragColor.a = fog;
	return fragColor;
}