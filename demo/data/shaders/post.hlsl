#include "globals.hlsl"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 Post(BufferStruct i) : SV_Target
{

	
	i.color = txDiffuse.Sample( samLinear, i.uv);

	i.color +=pow(txBloom.Sample( samLinear, i.uv ),bloomPower);
	
	float4 rad = 0;
	for(float j = 1; j < 2+sin(beat)/2; j+=.01) {
		// One does not simply j*j*j*j*j*j*j*j*10...
		rad += saturate(txDiffuse.Sample( samLinear, (i.uv/j-(.5/j-.5))))/(j*j*j*j*j*j*j*j*10);
	}
	i.color+=rad*(radial/10);
	
	
	i.color.r += saturate(random(i.uv+beat/10))*(noiseAmount/100);
	i.color.g += saturate(random(i.uv+beat/10+3))*(noiseAmount/100);
	i.color.b += saturate(random(i.uv+beat/10+5))*(noiseAmount/100);

	return i.color;
}
