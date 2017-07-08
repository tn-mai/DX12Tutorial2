/**
* @file TerrainPS.hlsl
*/

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

static const float3 colors[] = {
  { 0.35f, 0.5f, 0.18f },
  { 0.89f, 0.89f, 0.89f },
  { 0.31f, 0.25f, 0.2f },
  { 0.39f, 0.37f, 0.38f },
  { 0.1f, 0.2f, 0.89f },
  { 0.8f, 0.89f, 0.89f }
};

struct DS_OUTPUT
{
  float4 vPosition  : SV_POSITION;
  float3 worldPosition : WORLDPOS;
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
  float value = Noise(texcoord * freq) * 0.5; freq *= 2.01;
  value += Noise(texcoord * freq) * 0.25; freq *= 2.02;
  value += Noise(texcoord * freq) * 0.125; freq *= 2.03;
  //  value += Noise(texcoord * freq) * 0.0625;
  return max(0, 1 - value * 2);
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

float3 GetColorByHeight(float height, float low, float med, float high) {
  float bounds = cbTerrain.scale * 0.005f;
  float transition = cbTerrain.scale * 0.6f;
  float lowBlendStart = transition - 2 * bounds;
  float highBlendEnd = transition + 2 * bounds;
  float3 c;

  if (height < lowBlendStart) {
    c = colors[low];
  } else if (height < transition) {
    float3 c1 = colors[low];
    float3 c2 = colors[med];
    float blend = (height - lowBlendStart) * (1.0f / (transition - lowBlendStart));
    c = lerp(c1, c2, blend);
  } else if (height < highBlendEnd) {
    float3 c1 = colors[med];
    float3 c2 = colors[high];
    float blend = (height - transition) * (1.0f / (highBlendEnd - transition));
    c = lerp(c1, c2, blend);
  } else {
    c = colors[high];
  }
  return c;
}

float3 GetColorBySlope(float slope, float height) {
  float3 c;
  if (slope < 0.6f) {
    float blend = slope / 0.6f;
    float3 c1 = GetColorByHeight(height, 0, 3, 1);
    float3 c2 = GetColorByHeight(height, 2, 3, 3);
    c = lerp(c1, c2, blend);
  } else if (slope < 0.65f) {
    float blend = (slope - 0.6f) * (1.0f / (0.65f - 0.6f));
    float3 c1 = GetColorByHeight(height, 2, 3, 3);
    float3 c2 = colors[3];
    c = lerp(c1, c2, blend);
  } else {
    c = colors[3];
  }
#if 1
  const float threshould = 0.05 * cbTerrain.scale;
  if (height < threshould) {
    c = lerp(c, colors[4], (threshould - height) * 20 / cbTerrain.scale);
  }
#endif
  return c;
}

float4 main(DS_OUTPUT input) : SV_TARGET
{
  float3 norm = EstimateNormal((input.worldPosition.xz + float2(0, cbTerrain.base)) * cbTerrain.reciprocalSize);
  float3 viewvector = cbFrame.eye - input.worldPosition;
  //float3 color = float1(HeightMap(input.worldPosition.xz * (1.0 / 100.0))).xxx;
  //float3 color = float3(0.8, 0.8, 0.8);
  float3 color = GetColorBySlope(acos(norm.y), input.worldPosition.y);
  float3 diffuse = cbFrame.lightDiffuse * dot(-cbFrame.lightDir, norm);
  float3 V = reflect(cbFrame.lightDir, norm);
  float3 toEye = normalize(cbFrame.eye - input.worldPosition);
  float3 specular = cbFrame.lightSpecular * pow(max(dot(V, toEye), 0.0f), 2.0f);

  return float4(max(diffuse + specular, cbFrame.lightAmbient) * color, 1);
}