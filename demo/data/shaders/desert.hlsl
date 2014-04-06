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
	
	float dist = 1000.0f;
	float tmpDist = 600.0f;
	materialID = 1; 	
	float3 op = p;

	dist = p.y+synk5;
	tmpDist = -p.y+synk5;
	if(tmpDist < dist) {
		dist = tmpDist;
	}
	
	
	//p.xy = R(p.xy,p.z);
	
	p.z = fmod(abs(p.z),2)-1;	
	//p.x = fmod(abs(p.x),20)-10;
	p.yz = R(p.yz,PI/2);
	if(synk1==2)
		tmpDist = sdTorusN( p, float2(5+cos(op.z/5+beat/10)*2+sin(op.z/5+beat/10), .4),2);
	if(synk1==4)
		tmpDist = sdTorusN( p, float2(5+cos(op.z/5+beat/10)*2+sin(op.z/5+beat/10), .3),4);
	if(synk1==8)
		tmpDist = sdTorusN( p, float2(5+cos(op.z/5+beat/10)*2+sin(op.z/5+beat/10), .2),8);

	p=op;	
	
	if(synk1==10) {
	
		p.xy -=.5;
		p.xy = abs(p.xy);
		p.x = sqrt(pow(p.x,2)+pow(p.y,2));
		p.y = atan(p.y/p.x);

		p.z = fmod(abs(p.z),16)-4;
		tmpDist = rotBox(p,8,1, beat/10);
		tmpDist = min(tmpDist, rotBox(p,16,2, beat/5));
	}


	p=op;	
	
	if(tmpDist < dist) {
		materialID = 2;
		dist = tmpDist;
	}

	if(castShadow) {
		//float tmpDist2 = udRoundBox(p, float3(20+(sin(p.z/2-beat))+(cos(p.z*4-beat))/10,15+(sin(p.z/2-beat))+(cos(p.z*4-beat))/10,50000),4);
		p.yz = R(p.yz,PI/2);
		float tmpDist2 = sdCylinder(p, float3(0,0,synk6));
		tmpDist2+=abs(fbm2(p/28,beat/32)*32);//+sin(p.x/5+beat/2)+cos(p.z/8+beat/3)+cos(p.y/2+beat/4);

		if(-tmpDist2<dist) {
			materialID = 3; 			
			dist = -tmpDist2;		
	
		}
	}
	return dist;
}



float4 main(float2 uv) : COLOR
{	
	return 0;
	float3 light = 0.57703;
	light = normalize(light);
	//sphere1.x = 0.5+cos(beat);
	//sphere1.z = 0.5+sin(beat);

	float2 originalUV = uv;
	float3 rd = {(-1+2*uv)*float2(2.25,1), camFov < .01 ? .01 : camFov};
	
	
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
	float dist = rm(rayPosition, rd,.25, 400, id, steps, volumeFog); 
	//if(id==4) color=.1;
	//dist += pow(perlin(float3(rayPosition.xyz/10)),2)*200;
	color = pow((dist/600)*COL6/4,2);
	//color = pow((dist/600)*float3(85/255.,137/255.,125/255.),2);
	color += saturate((COL3*steps/100)/(dist/10));
	return float4(saturate(color),1); 
}


