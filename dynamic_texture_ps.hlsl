#include "fullscreen_quad.hlsli"

Texture2D iChannel0 : register(t0);
SamplerState sampler0 : register(s0);

cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 light_direction;
    float4 camera_position;
    float time;
    float elapsed_time;
    float2 iMouse;        // optional, if mouse input needed
};

float message(float2 uv) {
    uv -= float2(1.0, 10.0);
    if ((uv.x < 0.0) || (uv.x >= 32.0) || (uv.y < 0.0) || (uv.y >= 3.0)) return -1.0;

    int i = 1;
    int bit = int(pow(2.0, floor(32.0 - uv.x)));
    if (int(uv.y) == 2) i = 757737252 / bit;
    if (int(uv.y) == 1) i = 1869043565 / bit;
    if (int(uv.y) == 0) i = 623593060 / bit;

    return float(1 - i + 2 * (i / 2));
}

float iSampleRate = 44100.0; // 音声サンプルレートなど任意設定

float4 main(VS_OUT pin) : SV_TARGET
{
    float2 fragCoord = pin.position.xy;
    float2 iResolution = float2(512, 512);
    float4 fragColor = float4(0, 0, 0, 1);

    // message overlay
    if (iResolution.y < 200.0)
    {
        float c = message(fragCoord.xy / 8.0);
        if (c >= 0.0) { fragColor = float4(c, c, c, 1); return fragColor; }
    }

    float2 uv = fragCoord.xy / iResolution.xy;
    float fmax = iSampleRate / 4.0;
    float T = pow(10.0, iMouse.y / iResolution.y);

    // update only one column
    if (fmod(time / T - uv.x, 1.0) > 0.1) { clip(-1); }

    float zoom = 5.0;
    uv.y /= zoom;
    float f = uv.y * fmax;

    // bars
    if (fmod(fragCoord.x, 8.0) < 2.0)
    {
        if (abs(f - 440.0) < fmax / (zoom * iResolution.y))
        { fragColor = float4(1.0, 0.7, 0.0, 0.0); }
        else if (fmod(log(f / 440.0) / log(2.0), 1.0) < 0.25 / (iResolution.y * uv.y))
        { fragColor = float4(0.7, 0.0, 0.0, 0.0); }
        else if (fmod(f, 440.0) < fmax / (zoom * iResolution.y))
        { fragColor = float4(0.0, 0.7, 0.0, 0.0); }
    }

    // data from texture
    float c = iChannel0.Sample(sampler0, float2(uv.y, 0.25)).r;
    c = (c - 0.3) / 0.7;
    fragColor = float4(1.5 * c, c, 0.7 * c, 1.0);

    return fragColor;
}
