#include "constantbuffer.hlsl"
#define R(p,a) p=cos(a)*p+sin(a)*float2(-p.y,p.x);

Texture2D txDiffuse : register( t0 );
Texture2D txBloom : register( t1 );
SamplerState samLinear : register( s0 );
SamplerState samPoint : register( s1 );

struct BufferStruct {
	float4 position : SV_POSITION;
	float4 color : COLOR;
	float2 uv : TEXCOORD;
};


float4 main(float2 uv) : COLOR;
float intersect( float3 p, out float materialID );
struct Data {
	float4 color;
};

//[

//]
static float PI = acos(-1);
static int castShadow = 1;
static float ASPECT = 1.7777;

static float3 COL1 = float3(129,143,108)/255;
static float3 COL2 = float3(140,199,193)/255;
static float3 COL3 = float3(147,222,218)/255;
static float3 COL4 = float3(184,229,235)/255;
static float3 COL5 = float3(239,197,172)/255;
static float3 COL6 = float3(225,177,137)/255;

// For Gradient we need g_data as output
//[
RWTexture2D<float4> g_out : register(u1);
//]

float sminp(float a, float b, float k) {
    //float k=abs(sin(iGlobalTime))*2.+0.5;
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return lerp( b, a, h ) - k*h*(1.0-h);
}

static float3x3 m = { 0.00,  0.80,  0.60,
			  -0.80,  0.36, -0.48,
			  -0.60, -0.48,  0.64 };

float3 drawSpline(float3 spline[8], float time, int i) {
	return 0.5 *( (2 * spline[i+1]) +
		(spline[i+2] - spline[i]) * time +
		(2 * spline[i] - 5 * spline[i+1] + 4 * spline[i+2] - spline[i+3]) * time * time +
		(3 * spline[i+1] - 3 * spline[i+2] + spline[i+3] - spline[i]) * time * time * time );
}

float random(float2 co) {
	return frac(sin(dot(co.xy ,float2(12.9898,78.233))) * 43758.5453);
}

float3 Desaturate(float3 color, float Desaturation)
{
	float3 grayXfer = float3(0.3, 0.59, 0.11); 
	float3 gray = dot(grayXfer, color); 
	return float3(lerp(color, gray, Desaturation));
}

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

float fbm2( float3 p, float beat )
{
	float f = 0.0;

	f += 0.5000*noise( p+beat*2 ); p = mul(m,p*2.02);
	f += 0.2500*noise( p+beat ); p = mul(m,p*2.03);
	f += 0.1250*noise( p+beat/2 ); p = mul(m,p*2.01);
	f += 0.0625*noise( p+beat/4 );

	return f/0.9375;
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

float sdSphere( float3 center, float radius, float3 p )
{
	return length ( center - p ) -radius;
}

float3 opRep( float3 p, float3 c )
{
   return fmod(p,c)-0.5*c;
}

float udBox( float3 p, float3 b )
{
  return length(max(abs(p)-b,0));
}


float maxcomp(float3 mc) {
	return max(max(mc.x,mc.y),mc.z);
}

float sdBox( float3 p, float3 b )
{
  float3  di = abs(p) - b;
  float mc = maxcomp(di);
  return min(mc,length(max(di,0.0)));
}
float sdTorus( float3 p, float2 t )
{
	float2 q = float2(length(p.xz)-t.x,p.y);
	return length(q)-t.y;
}

float udRoundBox( float3 p, float3 b, float r )
{
  return length(max(abs(p)-b,0.0))-r;
}

float sdCylinder( float3 p, float3 c )
{
  return length(p.xz-c.xy)-c.z;
}


float3 grad(float3 pos){
   const float2 offset={0.001,-.001};

   //tetrahedron way 4x
   int materialID;
   float v1=intersect(pos+offset.xyy ,materialID); 
   float v2=intersect(pos+offset.yyx, materialID);
   float v3=intersect(pos+offset.yxy, materialID);
   float v4=intersect(pos+offset.x, materialID);
   
   float3 n=float3(
	  ((v4+v1)/2)-((v3+v2)/2),
	  ((v3+v4)/2)-((v1+v2)/2),
	  ((v2+v4)/2)-((v3+v1)/2));
   return n;
}

float ao(float3 p, float3 n, float d, float i) {
	float o,s=sign(d);
	int materialID;
	for (o=s*.5+.5;i>0.;i--) {
		o-=(i*d-intersect(p+n*i*d*s,materialID))/exp2(i);
	}
	return o;
}

float shadow( float3 ro, float3 rd, float mint, float maxt, float k )
{
	int materialID;
	float res = 1.0;
	//rd = normalize(rd);
	for( float t=mint; t < maxt; )
	{
		float h = intersect(ro + rd*t, materialID);
		if( h<0.001 )
			return 0.0;

		res = min( res, k*h/t );
		t += 0.1*h;
	}
	return res;
}



float rm(inout float3 rayPosition, float3 rd, float k, float maxIterations, out int materialID, out float steps, out float volumeFog){
	steps = 0;
	float totalDistance = 1;
	float t = 0;
	materialID = 0;
	volumeFog = 0;
	float res = 1;
	while ( ++steps < maxIterations )
	{
		//float t = min(rayPosition.y+2,intersect(rayPosition, id));
		float t = intersect(rayPosition, materialID);
		//volumeFog +=fbm(rayPosition/4+beat);
		if ( t < .001*totalDistance )
		{
			break;
		}

		// march on
		rayPosition += k*rd*t;		
		t=abs(t);
		totalDistance += t;
		res = min( res, 24*t/totalDistance );
	}
	rayPosition +=res;
	return totalDistance;
}	



//[	
float4 main(float2 uv) : COLOR;
[numthreads(32, 30, 1)]
void Checkers(uint3 groupId : SV_GroupID, uint3 groupThreadId : SV_GroupThreadID, uint3 dispatchThreadId : SV_DispatchThreadID, uint groupIndex: SV_GroupIndex) {	
	//uint i = dispatchThreadId.x + dispatchThreadId.y * xRes;
	float x = float(dispatchThreadId.x) / float(xRm);
	float y = float(dispatchThreadId.y) / float(yRm);

	//float3 color = saturate((perlin(float3(x,y,beat))+1)/2);
	//color = saturate((perlin(float3(x*10,y*10,beat/2))+1)/2);
	//g_out[dispatchThreadId.xy] = float4(color,1);
	g_out[dispatchThreadId.xy] = main(float2(x,1-y));
}
//]