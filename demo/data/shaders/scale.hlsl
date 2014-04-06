#include "globals.hlsl"

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------
float4 Scale(BufferStruct i) : SV_Target
{
	float4 color;// = txBloom.Sample( samPoint, i.uv+frac(beat)/100 );

	float4 i_background = float4(.3,.6,1,1);
	/*
	float2 dir = 0.5 - i.uv;
	dir.y *=1/16*9;
	dir = sign(dir)*pow(dir*2,2);
	float dist = sqrt(dir.x*dir.x + dir.y * dir.y)/(50);
	*/
	//i.color.r = txDiffuse.Sample( samPoint, i.uv+dist ).r;
	//i.color.g = txDiffuse.Sample( samPoint, i.uv ).g;
	//i.color.b = txDiffuse.Sample( samPoint, i.uv-dist ).b;
	i.color = txDiffuse.Sample(samPoint, i.uv);
	//i.color+=color;
	
	
	//i.color +=i_background*max(0,1-sin(i.uv.y*PI)*sin(i.uv.x*PI))*sin(i.uv.y*240)*sin(i.uv.x*320);
	//i.color +=i_background*max(0,1-sin(i.uv.y*PI)*sin(i.uv.x*PI))*(sin(i.uv.y*500+random(beat)*10))*(perlin(float3(i.uv.xy,i.uv.x*i.uv.y+beat/8)*3));	
	i.color *=min(1,sin(i.uv.y*PI)*sin(i.uv.x*PI)*30);
	
	i.color.xyz = Desaturate(i.color.xyz,saturation);

	i.color = saturate(i.color);		

	
	color = txBloom.Sample( samPoint, i.uv);
	
	//return color;
	//if(imageFade > 1) {
		//float ifade = imageFade;
		//ifade = saturate(ifade - 2);
		//color.xyz *= color.a;
		//i.color = color*(ifade)+i.color*(1-color.a*ifade)+i.color*(1-ifade);
//
		//return i.color*fade;
	//} else {
		//i.color = abs((color*imageFade)-i.color);
	//}
	//return i.color;
	//float c0 = dot(i.color.xyz,float3(.33,.33,.33));
	//c0 -= synk8/10;
	//c0 *= synk9;
	//c0 = saturate(c0);
	return (saturate(i.color)/10+color)*fade;
	//return color;

	//return color*imageFade;
}