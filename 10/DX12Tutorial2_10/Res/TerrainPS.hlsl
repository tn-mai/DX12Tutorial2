/**
* @file TerrainPS.hlsl
*/

Texture2D texTerrain : register(t0);
SamplerState sampler0 : register(s0);

struct TerrainData
{
  float2 reciprocalSize;
  float scale;
  float base;
};

struct PerFrameData
{
  float4x4 matViewProjection;
  float3 eye;
  float3 lightDir;
  float3 lightDiffuse;
  float3 lightSpecular;
  float3 lightAmbient;
};

cbuffer Constant : register(b0)
{
  TerrainData cbTerrain;
  PerFrameData cbFrame;
}

struct DS_OUTPUT
{
  float4 vPosition  : SV_POSITION;
  float3 worldPosition : WORLDPOS;
  float4 height4 : HEIGHT;
  float2 texcoord : TEXCOORD;
};

float4 Random4(float2 st)
{
  const float3 scale = { 12.9898f, 78.233f, 43758.5453123f };
  const float4 tmp = (st.xyxy + float4(0, 0, 1, 1)) * scale.xyxy;
  return frac(sin(tmp.xzxz + tmp.yyww) * scale.z);
}

float Noise(float2 st)
{
  const float2 i = floor(st);
  const float2 f = frac(st);
  const float2 u = f * f * (3.0f - 2.0f * f);
  const float2 ratio = { 1.0f - u.x, u.x };
  float4 abcd = Random4(i);
  abcd.zw -= abcd.xy;
  abcd.zw *= u.y;
  return dot(abcd.xyzw, ratio.xyxy);
}

float HeightMap(float2 texcoord)
{
  float freq = 6.0;
  float value = 0;
  //value += Noise(texcoord * freq) * 0.5;
  freq *= 2.01;
  //value += Noise(texcoord * freq) * 0.25;
  freq *= 2.02;
  value += Noise(texcoord * freq) * 0.125; freq *= 2.03;
  value += Noise(texcoord * freq) * 0.0625;
  return value;
}

float3 EstimateNormal(float2 texcoord)
{
#if 0
  texcoord *= 6.0 * 2.01 * 2.02 * 2.03 * 2.01;
  const float2 i = floor(texcoord);
  const float2 f = frac(texcoord);
  const float4 ff = saturate(f.xyxy + float2(-0.05, 0.05).xxyy);
  const float4 ratio = ff * ff * (3.0 - 2.0 * ff);
  const float4 oneMinusRatio = 1.0 - ratio;
  const float4 abcd = max(0, 1 - Random4(i) * 0.03125 * 2);

  float4 tmp = lerp(abcd.xxzz, abcd.yyww, ratio.xzxz);
  tmp = lerp(tmp.xyxy, tmp.zwzw, ratio.yyww);
  tmp = tmp.yxxz + tmp.wzyz;
  float2 slope = tmp.xz - tmp.yw;
  return float3(slope.x, 0, slope.y) * cbTerrain.scale;
#else
  const float2 invWH = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
//  float2 b = texcoord + float2(    0.0f, -invWH.y);
  float2 c = texcoord + float2( invWH.x, -invWH.y);
//  float2 d = texcoord + float2( invWH.x,     0.0f);
  float2 e = texcoord + float2( invWH.x,  invWH.y);
//  float2 f = texcoord + float2(    0.0f,  invWH.y);
  float2 g = texcoord + float2(-invWH.x,  invWH.y);
//  float2 h = texcoord + float2(-invWH.x,     0.0f);
  float2 i = texcoord + float2(-invWH.x, -invWH.y);

//  float zb = HeightMap(b) * cbTerrain.scale;
  float zc = HeightMap(c) * cbTerrain.scale;
//  float zd = HeightMap(d) * cbTerrain.scale;
  float ze = HeightMap(e) * cbTerrain.scale;
//  float zf = HeightMap(f) * cbTerrain.scale;
  float zg = HeightMap(g) * cbTerrain.scale;
//  float zh = HeightMap(h) * cbTerrain.scale;
  float zi = HeightMap(i) * cbTerrain.scale;

  const float x = ((zi + zg) - (zc + ze));
  const float z = ((zc + zi) - (ze + zg));
  //float x = zg + 2 * zh + zi - zc - 2 * zd - ze;
  //float z = 2 * zb + zc + zi - ze - 2 * zf - zg;
  float y = 4.0f;

  return normalize(float3(x, y, z));
#endif
}

float3 EstimateNormalXYZ(float2 texcoord, float4 height4)
{
  const float2 offset = float2(0.3f, 0.3f) * cbTerrain.reciprocalSize;
  float4 h4;
  h4.x = HeightMap(texcoord + float2(0, offset.y));
  h4.y = HeightMap(texcoord + float2(0, -offset.y));
  h4.z = HeightMap(texcoord + float2(offset.x, 0));
  h4.w = HeightMap(texcoord + float2(-offset.x, 0));
  h4 += height4;
  h4 = max(0, 1 - h4 * 2) * cbTerrain.scale;
  h4.xz -= h4.yw;
  return normalize(float3(h4.x, 4, h4.z));
}

float4 main(DS_OUTPUT input) : SV_TARGET
{
  float3 norm = EstimateNormalXYZ(input.texcoord, input.height4);
  float3 viewvector = cbFrame.eye - input.worldPosition;
  //float3 color = float1(HeightMap(input.worldPosition.xz * (1.0 / 100.0))).xxx;
  //float3 color = float3(0.8, 0.8, 0.8);
  float3 color = texTerrain.Sample(sampler0, float2(input.worldPosition.y / cbTerrain.scale, acos(norm.y))).xyz;
  float3 diffuse = cbFrame.lightDiffuse * dot(-cbFrame.lightDir, norm);
  float3 V = reflect(cbFrame.lightDir, norm);
  float3 toEye = normalize(cbFrame.eye - input.worldPosition);
  float3 specular = cbFrame.lightSpecular * pow(max(dot(V, toEye), 0.0f), 2.0f);

  return float4(max(diffuse + specular, cbFrame.lightAmbient) * color, 1);
}