#include "constantbuffer.hlsl"

cbuffer cbSettings
{
	static const float gWeights[15] = 
	{
    	0.010925785,
0.022496235,
0.041448805,
0.068337522,
0.100821120,
0.133103310,
0.157243000,
0.132980750,
0.157243000,
0.133103310,
0.100821120,
0.068337522,
0.041448805,
0.022496235,
0.010925785,
	};
};

static const int gBlurRadius = 7*4;

Texture2D gInput : register (t0);
RWTexture2D<float4> gOutput : register (u0);

#define NUMBER_OF_THREADS 256
#define CACHE_SIZE (NUMBER_OF_THREADS + 2 * gBlurRadius)
groupshared float4 gCache[CACHE_SIZE];

[numthreads(NUMBER_OF_THREADS, 1, 1)]
void main(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
	//
	// Fill local thread storage to reduce bandwidth. To blur 
	// NUMBER_OF_THREADS pixels, we will need to 
    // load NUMBER_OF_THREADS + 2 * BlurRadius pixels due to the blur radius.
	//
	
	// This thread group runs NUMBER_OF_THREADS threads. To get the extra 2 * BlurRadius pixels, 
	// have 2 * BlurRadius threads sample an extra pixel.
	if(groupThreadID.x < gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = max(dispatchThreadID.x - gBlurRadius, 0);
		gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
	}

	if(groupThreadID.x >= NUMBER_OF_THREADS - gBlurRadius)
	{
		// Clamp out of bound samples that occur at image borders.
		int x = min(dispatchThreadID.x + gBlurRadius, gInput.Length.x - 1);
		gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
	}

	// Clamp out of bound samples that occur at image borders.
	gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, gInput.Length.xy - 1)];

	// Wait for all threads to finish.
	GroupMemoryBarrierWithGroupSync();
	
	//
	// Now blur each pixel.
	//

	float4 blurColor = float4(0, 0, 0, 0);
	
	[unroll]
	for(int i = -gBlurRadius; i <= gBlurRadius; ++i)
	{
		int k = groupThreadID.x + gBlurRadius + i;
		
		blurColor += gWeights[(i + gBlurRadius)/4]/4/1.2 * gCache[k];
	}
	
	gOutput[dispatchThreadID.xy] = blurColor;

    //gOutput[dispatchThreadID.xy] = gInput[dispatchThreadID.xy];
}