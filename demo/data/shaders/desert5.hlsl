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
	
	float dist = 600.0f;
	float tmpDist = 600.0f;
		
	materialID = 3;
	dist = p.y+synk1;
	tmpDist = -p.y+synk2;
	if(tmpDist < dist) {
		materialID = 1;
		dist = tmpDist;
	}
	 	
	dist += perlin(p/5)*(synk7);

	float3 op = p;
	//p.xy = R(p.xy,p.z);
	
	p.z = fmod(abs(p.z),2)-1;	
	//p.x = fmod(abs(p.x),20)-10;
	p.yz = R(p.yz,PI/2);
	if(synk3==2)
		tmpDist = sdTorusN( p, float2(5+cos(op.z/5+beat/10)*2+sin(op.z/5+beat/10), .4),2);
	if(synk3==4)
		tmpDist = sdTorusN( p, float2(5+cos(op.z/5+beat/10)*2+sin(op.z/5+beat/10), .3),4);
	if(synk3==8)
		tmpDist = sdTorusN( p, float2(5+cos(op.z/5+beat/10)*2+sin(op.z/5+beat/10), .2),8);

	p=op;	
	
	if(tmpDist < dist) {
		materialID = 5;
		if(synk3==10) materialID = 5;
		dist = tmpDist;
	}

p=op;	
	
	if(synk3==10) {
		p.y = fmod(abs(p.y),16)-4;
		tmpDist = rotBox(p,8,1, beat/10);
		tmpDist = min(tmpDist, rotBox(p,16,2, beat/5));
		if(tmpDist < dist) {
			materialID = 4;
			dist = tmpDist;
		}	
	}

	return dist;
}



float4 main(float2 uv) : COLOR
{	
	if(noiseAmount > 10) return 0;	

	float2 originalUV = uv;
	float3 rd = {(-1+2*uv)*float2(2.25,1), camFov < .01 ? .01 : camFov};
	
	
	float3 ro = {synk4,synk5,beat/2};
	//float3 ro = {0,5,beat/2};
	
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
	float tempId = 0;
	if(id > 3) {
		tempId = id;
		float3 N = normalize(grad(rayPosition));
		rd = reflect(rd, N);			
		float foo;
		dist = rm(rayPosition,rd,.25,60, id, steps, foo);
		
	} 

	
	color = saturate(color);
	//color = saturate((float3(.1,.7,1)*steps/100)/(dist/2));
	dist += pow(perlin(float3(rayPosition.x*10, 0, 0)),2)*20;
	color = pow((dist/300)*float3(.7,.4,.2),2);
	color += saturate((float3(.1,.7,1)*steps/100)/(dist/10));

	if(tempId > 0) color = float3(1,0,0) * (dist/200);
	if(tempId > 4) color = float3(.8,.8,1) * (dist/200);

	return float4(color,1); 
}


