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
};

struct PosVelo
{
    float4 pos;
    float4 velo;
	float4 color;
	float  mass;
	float  timeToLive;
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
		int index = DTid.x+(DTid.y*xRes*20);
		
		//if(index > g_param.z) return;
		
		float4 pos = oldPosVelo[index].pos;
		float4 vel = oldPosVelo[index].velo;
		float4 color = oldPosVelo[index].color;
		float timeToLive = oldPosVelo[index].timeToLive;
		
		float time = g_time.x/40;
		float delta = g_time.y;
	
		//timeToLive = 0;
		timeToLive -= delta;
		

		float x = (float)DTid.x/((float)xRes*20);
		float y = (float)DTid.y/((float)yRes*20);
		

		if(timeToLive < 0) 
		{
			color = gInput[int2(x*g_param.x, g_param.y-y*g_param.y )];

			if(color.x +color.y + color.z> 0) {

				//color = saturate(color);
				//color = 0;
				pos.xyz = float3((x-.5)*32,(y-.5)*16,0)*28.0;
				timeToLive = (perlin(float3(pos.xyz)+g_time.x))*2;
				//timeToLive = 10;
				newPosVelo[index].velo.x = timeToLive;
				//pos.w = 10000.0 * 10000.0;
			} else {
				color = 0;
			}
			//pos = 10000;
		}

		
	
		float3 accel = 0;
		float mass = g_fParticleMass;
	
		//pos.xyz += (perlin(pos.xyz))*.1;

		//pos.x+=.1;
		//pos.xyz += (perlin(float3(pos.xyz/100)+time))*delta*1;
		pos.z += ((sin(x*5+time)*10+cos(y*10+time*2)*20)/1)*delta*10;
		//pos.z = saturate(pos.z)*10;

		float t = (450+sin(time)*10);
		pos.xyz = length(pos.xyz) > t ? normalize(pos.xyz)*t : pos.xyz;
		//pos.z = (length(color.xyz))*10;

		//if(abs(pos.x)>.5) pos.x = sign(pos.x)*.5;
		
        newPosVelo[index].pos = pos;
        //newPosVelo[index].velo = float4(vel.xyz, length(accel));
		newPosVelo[index].timeToLive = timeToLive;
		newPosVelo[index].color = color; //float4(color.xyz,(timeToLive/newPosVelo[index].velo.x));
    //}
}
