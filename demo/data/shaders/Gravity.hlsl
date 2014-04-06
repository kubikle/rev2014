//--------------------------------------------------------------------------------------
// File: NBodyGravityCS11.hlsl
//
// Demonstrates how to use Compute Shader to do n-body gravity computation
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------


SamplerState samLinear : register( s1 );

static float softeningSquared = 0.0012500000*0.0012500000;
static float g_fG = 6.67300e-11f * 10000.0f;
static float g_fParticleMass = g_fG*10000.0f * 10000.0f;

#define xRes 32
#define yRes 18
//groupshared float4 sharedPos[blocksize];

cbuffer cbCS : register( b0 )
{
    uint4   g_param;    // pcbCS->param[0] = MAX_PARTICLES;
                        // pcbCS->param[1] = dimx;              
    float4  g_time;   // pcbCS->paramf[0] = 0.1f;
                        // pcbCS->paramf[1] = 1; 
	float	part1;
	float	part2;

	float	part3;
	float	part4;

};

struct PosVelo
{
    float4 pos;
    float4 velo;
	float4 color;
	float  size;
	float  timeToLive;
	float  mass;
	float temp;
};

StructuredBuffer<PosVelo> oldPosVelo : register (t0);
Texture2D gInput : register (t1);
RWStructuredBuffer<PosVelo> newPosVelo;

static float PI = acos(-1);

static float3x3 m = { 0.00,  0.80,  0.60,
			  -0.80,  0.36, -0.48,
			  -0.60, -0.48,  0.64 };
float hash( float n )
{
	return frac(sin(n)*43758.5453);
}


float noise(float3 x )
{
	float3 p = floor(x);
	float3 f = frac(x);

	f = f*f*(3.0-2.0*f);

	float n = p.x + p.y*57.0 + 113.0*p.z;

	float res = lerp(lerp(lerp( hash(n+  0.0), hash(n+  1.0),f.x),
						lerp( hash(n+ 57.0), hash(n+ 58.0),f.x),f.y),
					lerp(lerp( hash(n+113.0), hash(n+114.0),f.x),
						lerp( hash(n+170.0), hash(n+171.0),f.x),f.y),f.z);
	return res;
}

float fbm( float3 p )
{
	float f = 0.0;

	f += 0.5000*noise( p ); p = mul(m,p*2.02);
	f += 0.2500*noise( p ); p = mul(m,p*2.03);
	f += 0.1250*noise( p ); p = mul(m,p*2.01);
	f += 0.0625*noise( p );

	return f/0.9375;
}

float perlin(float3 p) {
	float3 i = floor(p);
	float4 a = dot(i, float3(1, 57, 21)) + float4(0, 57, 21, 78);
	float3 f = cos((p-i)*PI)*(-.5)+.5;
	a = sin(cos(a)*a)*(1-f.x) + sin(cos(1+a)*(1+a))* f.x;
	a.xy = a.xz * (1-f.y) + a.yw*f.y;
	return a.x*(1-f.z) + a.y*f.z;
}


[numthreads(xRes, yRes, 1)]
void CSMain( uint3 Gid : SV_GroupID, uint3 DTid : SV_DispatchThreadID, uint3 GTid : SV_GroupThreadID, uint GI : SV_GroupIndex  )
{
    // Each thread of the CS updates one of the particles
	//if ( DTid.x+DTid.x*xRes < g_param.z )
    //{
		int index = DTid.x+(DTid.y*xRes*40);
		
		//if(index > g_param.z) return;
		
		float4 pos = oldPosVelo[index].pos;
		float4 vel = oldPosVelo[index].velo;
		float4 color = oldPosVelo[index].color;
		float timeToLive = oldPosVelo[index].timeToLive;
		
		float mass = oldPosVelo[index].mass;

		float time = g_time.x/8;
		float delta = g_time.y;
	
		//timeToLive = 0;
		timeToLive -= delta;
		

		float x = (float)DTid.x/((float)xRes*40.0);
		float y = (float)DTid.y/((float)yRes*40.0);
		

		if(timeToLive < 0 || delta < 0) 
		{
			color = gInput[int2(x*g_param.x, g_param.y-y*g_param.y )];
			mass = length(color.xyz);

				//color = saturate(color);
				//color = 0;
				pos.xyz = float3((x-.5)*32,(y-.5)*16,0)*12.0;
				pos.z += (sin(x*PI))*16 + (sin(y*PI)*9);
				//timeToLive = abs(fbm((float3(0,0,y*10+time))))+1;
				timeToLive = abs(perlin(pos.xyz)*part1);
				newPosVelo[index].velo.x = timeToLive;
				//pos.w = 10000.0 * 10000.0;
		}

		
	
		float3 accel = 0;
	
		//pos.xyz += (perlin(pos.xyz))*.1;

		//pos.x+=.1;
		//pos.z += ((sin(x*5+time)*10+cos(y*10+time*2)*20)/1)*delta;
		//pos.z = saturate(pos.z)*10;
		//pos.z = (sin(x*PI))*16 + (sin(y*PI)*9)*(1+fmod(time,1));
		
		

		//pos.z /=2;
		
		//float t = (150+fmod(time,4));
		//pos.z = length(pos.xy) > t ? (fbm(float3(pos.xyz/10)+time))*-(abs(pos.x)-(t+10)) : pos.z;

		//pos.z -= pow(saturate(length(color.xyz)),2)*10;
		
		//reunat
		//pos.z += pow(length(pos.xy)/(4*12*3),10)*(fbm(float3(pos.xyz/20)+time/16));
		
		pos.xy += (fbm(float3(pos.xyz/10)+time/10)-.5)*delta*mass*4;
		
		pos.z += (fbm(float3(pos.xyz/10)+time/10))*pow(mass,2)*-5*delta;
		//pos.z +=delta*10;
		//if(abs(pos.x)>.5) pos.x = sign(pos.x)*.5;
		
        newPosVelo[index].pos = pos;
        //newPosVelo[index].velo = float4(vel.xyz, length(accel));
		newPosVelo[index].timeToLive = timeToLive;
		//newPosVelo[index].color = float4(color.xyz, .05);//float4(color.xyz,saturate((timeToLive/newPosVelo[index].velo.x)*10));

		//newPosVelo[index].color = float4(color.xyz,pow(saturate((timeToLive/newPosVelo[index].velo.x)),2)/20);
		newPosVelo[index].color = float4(color.xyz,.1);

		//newPosVelo[index].size = saturate((color.x+color.y+color.z)*2);
		newPosVelo[index].size = 1;
		newPosVelo[index].mass = mass;
    //}
}
