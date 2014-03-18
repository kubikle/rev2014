#include "globals.hlsl"
//--------------------------------------------------------------------------------------
// Vertex Shader
//--------------------------------------------------------------------------------------
struct VSParticleIn
{
    float4  color   : COLOR;
    uint    id      : SV_VERTEXID;
};

BufferStruct VS(VSParticleIn input)
{
	BufferStruct o;
	o.position = float4(0, 0, 0, 1);
	o.color = float4(1, 1, 1, 1);
	return o;
}