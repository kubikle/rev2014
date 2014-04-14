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
	float3 op = p;
	materialID = 1; 	
	//p.zy = R(p.zy,p.x);
	
	float dist = 1000.0f;
	float tmpDist = 600.0f;
	dist = p.y+synk5;
	tmpDist = -p.y+synk5;
	if(tmpDist < dist) {
		dist = tmpDist;
	}



	if(beat > 192) {
		p.xz = R(p.xz,sin(beat)*0.5);
		p.xy = R(p.xy,-synk8);
		p.x = abs(p.x);

	

		//p *= 5;
		tmpDist = udRoundBox(  p+float3(0,.2,0), float3(.3,.3,.1),.0);
	
		tmpDist = sminp(tmpDist,udRoundBox(  p-float3(0,.5,0), float3(.87,.3,.1),.0), synk9);	

		tmpDist = sminp(tmpDist,udRoundBox(  p-float3((1.2),0,0), float3(.1,1.2,.1),.0), synk9);

		// yla ala
		tmpDist = sminp(tmpDist,udRoundBox(  p-float3(0,1.2,0), float3(1.3,.1,.1),.0), synk9);
		tmpDist = sminp(tmpDist,udRoundBox(  p-float3(1,-1.2,0), float3(0.3,.1,.1),.0), synk9);



		//tmpDist = min(tmpDist,udRoundBox(  p-float3(4+1.6+5.4*2+1.6+1.4,0,0), float3(1.4,16,1),.2));
		//tmpDist = min(tmpDist,udRoundBox(  p+float3(4+1.6+5.4*2+1.6+1.4,0,0), float3(1.4*4/3,16,1),.2));
	
		//tmpDist = min(tmpDist,udRoundBox(  p+float3(16.25,-2.3+4+1.6+5.4*2+1.6+1.4,0), float3(4,1.4,1),.2));
		//tmpDist = min(tmpDist,udRoundBox(  p+float3(-16.25,-2.3+4+1.6+5.4*2+1.6+1.4,0), float3(4*4/3,1.4,1),.2));

		if(tmpDist < dist) {
			materialID = 2;
			dist = sminp(tmpDist, dist, synk9);

		
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
	float3 rd = {(-1+2*uv)*float2(ASPECT,1), camFov < .01 ? .01 : camFov};
	
	
	float3 ro = {synk2,synk3,synk4};
	
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
	
	color = pow((dist/300)*COL6,2);
	
	dist += pow(perlin(float3(rayPosition.x*10, 0, 0)),2)*20;
	color += saturate((COL3*steps/100)/(dist/10));

	if(id == 2) {
		//color = 0;
	}

	return saturate(float4(color,1))*fade; 
}


