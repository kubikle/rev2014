#include "globals.hlsl"
//]
//--------------------------------------------------------------------------------------
// Geometry shader
//--------------------------------------------------------------------------------------
//[

[maxvertexcount(4)]
void GS(point BufferStruct input[1], inout TriangleStream<BufferStruct> ss)
{
	BufferStruct o;

	float4 g_offsets[] = {
		float4( 1,  1, 0, 1),
		float4( 1, -1, 0, 1),
		float4(-1, 1, 0, 1),
		float4(-1, -1, 0, 1),
	};	

	for (int i = 0; i < 4; i++) {
		o.color = input[0].color;
		o.position = g_offsets[i];
		o.uv = (g_offsets[i].xy+1)/2;
		o.uv.y = 1-o.uv.y;
		ss.Append(o);
	}
	ss.RestartStrip();
}
//]