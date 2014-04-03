#include "globals.hlsl"
float rm(inout float3 rayPosition, float3 rd, float k, float maxIterations, out int materialID, out float steps, out float volumeFog);
void Checkers(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex: SV_GroupIndex);

float lengthN(float2 t, float n){
	return (pow((pow(t.x,n))+(pow(t.y,n)),(1./n)));
}


float sdTorusN( float3 p, float2 t , float n)
{
  float2 q = float2(lengthN(p.xz, n)-t.x,p.y);
  return lengthN(q, n)-t.y;
}

float rotBox(float3 p, float r, float s, float speed){
	float3 op = p;
	p.x = fmod(abs(p.x),r)-(r/2);	
	p.z = fmod(abs(p.z),r)-(r/2);	
	//p.y = fmod(abs(p.y),1)-.5;	
	
	p.yz = R(p.yz,floor(op.x/r)+floor(op.z/r)+speed*lerp(2,1,s/2));
	return udRoundBox(p,s-.1,s/20);
}

float intersect( float3 p, out float materialID )
{ 
	//p.zy = R(p.zy,p.x);
	
	float dist = 500.0f;
	float tmpDist = 500.0f;
	
	
	float3 op = p;
	p.y = fmod(abs(p.y),16)-8;
	if(p.z > 0) {
		//p.z = fmod(abs(p.z),16)-4;
	}
	//tmpDist = rotBox(p,8,1, beat/10);
	//tmpDist = min(tmpDist, rotBox(p,16,2, beat/5));

	float3 s = float3(2000,.1,.1);
	tmpDist = udRoundBox(p,s,s.z/10);
	


	p=op;	
	
	if(tmpDist < dist) {
		tmpDist +=fbm2(p,beat/20)/10;
		dist = tmpDist;
		materialID = 1; 	
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
	float3 rd = {(-1+2*uv)*float2(ASPECT,1), camFov < .1 ? .1 : camFov};
	
	
	float3 ro = {synk2,synk3,synk4};
	
	rd.yz = R(rd.yz,camRot.x);
	rd.xz = R(rd.xz,camRot.y);
	rd.xy = R(rd.xy,camRot.z);

	rd=normalize(rd);

	float3 rayPosition = ro + rd;

	float3 color = 0;
	float id= 0;
	float steps;
	float volumeFog;
	//ray march
	float dist = rm(rayPosition, rd,.25, 200, id, steps, volumeFog); 
	//if(id==4) color=.1;
	//dist += pow(perlin(float3(rayPosition.x*10, 0, 0)),2)*20;
	if(id==1) color = float3(244.,238.,180.)/255.;
	//color += steps/200;
	color -= 200/steps/20;
	//if(id==1) color = float3(66.,84.,84.)/255.;
	
	return float4(color,1); 
}


