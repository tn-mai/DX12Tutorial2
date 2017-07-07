/**
* @file TerrainVS.hlsl
*/

struct TerrainData
{
  float scale;
  float width;
  float depth;
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

struct VS_OUTPUT
{
  float3 worldPosition : WORLDPOS;
};

struct VS_INPUT
{
  float3 position : POSITION;
};

VS_OUTPUT main(VS_INPUT input)
{
  VS_OUTPUT output;
  output.worldPosition = input.position;
  return output;
}