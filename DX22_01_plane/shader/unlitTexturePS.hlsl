#include "common.hlsl"

Texture2D g_Texture : register(t0);
SamplerState g_Sampler : register(s0);

// 
cbuffer MaterialBuffer : register(b0)
{
    float4 Diffuse;
};

cbuffer LightBuffer : register(b1)
{
    float4 LightDir;
    float4 LightColor;
};

float4 main(PS_IN input) : SV_Target
{
    // ?完全不使用 normal / light
    return g_Texture.Sample(g_Sampler, input.tex);
}
