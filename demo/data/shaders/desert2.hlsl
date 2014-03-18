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
	p.z = fmod(abs(p.z),synk5)-synk5/2;	
	float r = synk6/100;
	float s = synk6/100;
	tmpDist =				udRoundBox(p+float3(0,-2,0),float3(10000,s,s),r);
	tmpDist = min(tmpDist,	udRoundBox(p+float3(0, 1,0),float3(10000,s,s),r));

	p = op;

	p.x = fmod(abs(p.x),synk5)-synk5/2;	
	
	tmpDist = min(tmpDist,	udRoundBox(p+float3(0,-2,0),float3(s,s,1000),r));
	tmpDist = min(tmpDist,	udRoundBox(p+float3(0, 1,0),float3(s,s,1000),r));

	p = op;
	if(tmpDist < dist) {
		materialID = 1;
		dist = tmpDist+perlin(p)*(synk7/10);
	}

	if(synk1 >= 1) {
		if(synk1 >= 2) {
			p.x = fmod(abs(p.x),synk5)-synk5/2;	
		}
		if(synk1 >= 3) {
			p.y = fmod(abs(p.y),synk5)-synk5/2;	
		}
		if(synk1 >= 4) {
			p.z = fmod(abs(p.z),synk5*2)-synk5*2/2;	
		}
		float r = synk4/100;
		tmpDist =				udRoundBox(p,float3(.25,.025,.025),r);
		tmpDist = min(tmpDist,	udRoundBox(p,float3(.025,.25,.025),r));
		
		if(tmpDist < dist) {
			materialID = 2;
			dist = tmpDist;
		}
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
	
	
	float3 ro = {synk3*cos(synk2),0,synk3*sin(synk2)};
	
	rd.yz = R(rd.yz,camRot.x);
	rd.xz = R(rd.xz,camRot.y);
	rd.xy = R(rd.xy,camRot.z);

	rd=normalize(rd);

	float3 rayPosition = ro + rd;

	float3 color = 1;
	float id= 0;
	float steps;
	float volumeFog;
	//ray march
	float dist = rm(rayPosition, rd,.25, 400, id, steps, volumeFog); 
	//if(id==4) color=.1;
	//dist += pow(perlin(float3(rayPosition.x*10, 0, 0)),2)*20;
	
	
	if(id > 0) color = 0;
	if(id > 1) color = float3(1,0,0);
	color += (dist/100);
	color = saturate(color);
	//color += saturate((float3(.1,.7,1)*steps/100)/(dist/10));
	
	return float4(color,1); 
}


