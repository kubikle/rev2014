#include "globals.hlsl"
float rm(inout float3 rayPosition, float3 rd, float k, float maxIterations, out int materialID, out float steps, out float volumeFog);
void Checkers(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex: SV_GroupIndex);

float intersect( float3 p, out float materialID )
{ 
	//p.zy = R(p.zy,p.x);
	
	float dist = 1000.0f;
	float tmpDist = 600.0f;
		
	materialID = 0; 	
	
	float3 op = p;
	p.y = fmod(abs(p.y),synk5*2)-synk5;	
	p.z = fmod(abs(p.z),synk5*2)-synk5;	
	float r = synk6/100;
	float s = synk6/100;
	tmpDist =				udRoundBox(p+float3(0,-2,-cam),float3(10000,s,s),r);
	tmpDist = min(tmpDist,	udRoundBox(p+float3(0, 1,-cam),float3(10000,s,s),r));

	p = op;

	p.y = fmod(abs(p.y),synk5*2)-synk5;	
	p.x = fmod(abs(p.x),synk5)-synk5/2;	
	
	tmpDist = min(tmpDist,	udRoundBox(p+float3(0,-2,-cam),float3(s,s,1000),r));
	tmpDist = min(tmpDist,	udRoundBox(p+float3(0, 1,-cam),float3(s,s,1000),r));

	p = op;
	if(tmpDist < dist) {
		materialID = 1;
		dist = tmpDist+perlin(p)*(synk7/10);
	}

		
	float size = synk4 / 100;
	float size2 = camTime / 100;
	p.yz = R(p.yz,camRot.x*2);
	p.xz = R(p.xz,camRot.y*2);
	p.xy = R(p.xy,camRot.z*2);
	tmpDist = sdTorus(p, float2(10*size,1*size2));
		
	if(tmpDist < dist) {
		materialID = 2;
		dist = tmpDist+perlin(p)*(cam);
	}

	return dist;
}



float4 main(float2 uv) : COLOR
{	
	float3 light = 0.57703;
	light = normalize(light);
	//sphere1.x = 0.5+cos(beat);
	//sphere1.z = 0.5+sin(beat);

	float2 originalUV = uv;
	float3 rd = {(-1+2*uv)*float2(2.25,1), camFov < .01 ? .01 : camFov};
	
	
	//float3 ro = {synk2,-1*(synk2/2),synk3};
	float3 ro = {0,5,-10-cam};
	rd.yz = R(rd.yz, .2);

	rd=normalize(rd);

	float3 rayPosition = ro + rd;

	float3 color = 1;
	float id= 0;
	float steps;
	float volumeFog;
	//ray march
	float dist = rm(rayPosition, rd,.25, 400, id, steps, volumeFog); 
	//if(id==4) color=.1;
	
	float tempId = 0;
	if(id > 1) {
		tempId = id;
		float3 N = normalize(grad(rayPosition));
		rd = reflect(rd, N);			
		float foo;
		dist = rm(rayPosition,rd,.25,60, id, steps, foo);
		//color = float3(1,0,0);
		//dist += pow(perlin(float3(rayPosition.x, rayPosition.y, 0)),2)*-20;
	} 
	
	if(id > 0)  {dist += pow(perlin(float3(rayPosition.x*20,rayPosition.x*20,rayPosition.x*20)),4)*20;
		color = float3(0,0,1)+(dist/200);
	}
	if(tempId > 0) color = float3(1,0,0) * (dist/200);
	
	
	color = saturate(color);
	//color += saturate((float3(.1,.7,1)*steps/100)/(dist/10));
	
	return float4(color,1); 
}


