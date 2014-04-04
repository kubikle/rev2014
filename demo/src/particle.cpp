#include <d3d11.h>	
#include <d3dx11.h>

#include <d3dx10math.h>
#include <xnamath.h>
#include <vector>
#include <map>
#include <string>


#include "../include/audio.h"
#include "../include/options.h"
#include "../../rocket-code/sync/sync.h"

#define PARTICLE_X 640
#define PARTICLE_Y 360
#define MAX_PARTICLES PARTICLE_X*PARTICLE_Y         // the number of particles in the n-body simulation

typedef struct{
	ID3D11Texture2D*		   texture;
	ID3D11RenderTargetView*    renderTargetView;
	ID3D11ShaderResourceView*  shaderResourceView;
	ID3D11UnorderedAccessView* unorderedAccessView;
} View;

extern View particle;

using namespace std;

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if (p) { delete[] (p);   (p)=NULL; } }
#endif

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

extern ID3D11Device*					g_pd3dDevice;
extern ID3D11DeviceContext*				g_pImmediateContext;
extern vector<ID3D11ComputeShader*>		g_pComputeShaders;
extern vector<ID3D11VertexShader*>		g_pVertexShader;
extern vector<ID3D11GeometryShader*>    g_pGeometryShader;
extern vector<ID3D11PixelShader*>       g_pPixelShaders;
extern map<string, int>					g_positionMap;
extern ID3D11DepthStencilView*			g_pDepthStencilView;


extern SyncTracks						g_syncTracks;
extern DemoOptions						g_options;

//ID3D11RenderTargetView* pRTV;
//ID3D11DepthStencilView* pDSV;

ID3D11SamplerState*                 g_pSampleStateLinear = NULL;
ID3D11BlendState*                   g_pBlendingStateParticle = NULL;
ID3D11DepthStencilState*            g_pDepthStencilState = NULL;

ID3D11Buffer*                       g_pcbCS = NULL;

ID3D11Buffer*                       g_pParticlePosVelo0 = NULL;
ID3D11Buffer*                       g_pParticlePosVelo1 = NULL;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV0 = NULL;
ID3D11ShaderResourceView*           g_pParticlePosVeloRV1 = NULL;
ID3D11UnorderedAccessView*          g_pParticlePosVeloUAV0 = NULL;
ID3D11UnorderedAccessView*          g_pParticlePosVeloUAV1 = NULL;
ID3D11Buffer*                       g_pParticleBuffer = NULL;
ID3D11InputLayout*                  g_pParticleVertexLayout = NULL;

ID3D11Buffer*                       g_pcbGS = NULL;

ID3D11ShaderResourceView*           g_pParticleTexRV = NULL;

XMMATRIX                            g_World;
XMMATRIX                            g_View;
XMMATRIX                            g_Projection;

struct PARTICLE_VERTEX
{
    D3DXVECTOR4 Color;
};

struct CB_GS
{
    XMMATRIX m_WorldViewProj;
    XMMATRIX m_InvView;
};

struct CB_CS
{
    UINT param[4];
    float time[4];
};

#pragma pack(4)
struct PARTICLE
{
    D3DXVECTOR4 pos;
    D3DXVECTOR4 velo;
	D3DXVECTOR4 color;
	float		mass;
	float		timeToLive;
};

template <class T>
void SWAP( T* &x, T* &y )
{
    T* temp = x;
    x = y;
    y = temp;
}

extern void Quit(LPCTSTR lpText);

//--------------------------------------------------------------------------------------
float RPercent()
{
    float ret = ( float )( ( rand() % 10000 ) - 5000 );
    return ret / 5000.0f;
}

float
normalize(D3DXVECTOR3& vector)
{
    float dist = sqrtf(vector.x*vector.x + vector.y*vector.y + vector.z*vector.z);
    if (dist > 1e-6)
    {
        vector.x /= dist;
        vector.y /= dist;
        vector.z /= dist;
    }
    return dist;
}

void LoadParticles( PARTICLE* pParticles,
                   D3DXVECTOR3 Center, D3DXVECTOR4 Velocity, float Spread, UINT NumParticles )
{
    for( UINT i = 0; i < NumParticles; i++ )
    {
        D3DXVECTOR3 Delta( Spread, Spread, Spread );

        while( D3DXVec3LengthSq( &Delta ) > Spread * Spread )
        {
            Delta.x = RPercent() * Spread;
            Delta.y = RPercent() * Spread;
            Delta.z = RPercent() * Spread;
        }

        pParticles[i].pos.x = Center.x + Delta.x;
        pParticles[i].pos.y = Center.y + Delta.y;
        pParticles[i].pos.z = Center.z + Delta.z;
        pParticles[i].pos.w = 10000.0f * 10000.0f;

        pParticles[i].velo = Velocity;

		pParticles[i].timeToLive = 1;
    }    
}

HRESULT CreateParticleBuffer()
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC vbdesc =
    {
        MAX_PARTICLES * sizeof( PARTICLE_VERTEX ),
        D3D11_USAGE_DEFAULT,
        D3D11_BIND_VERTEX_BUFFER,
        0,
        0
    };
    D3D11_SUBRESOURCE_DATA vbInitData;
    ZeroMemory( &vbInitData, sizeof( D3D11_SUBRESOURCE_DATA ) );

    PARTICLE_VERTEX* pVertices = new PARTICLE_VERTEX[ MAX_PARTICLES ];
    if( !pVertices ){
		Quit("Failed to allocate: pVertices");
	}
    for( UINT i = 0; i < MAX_PARTICLES; i++ )
    {
        pVertices[i].Color = D3DXVECTOR4( 1, 1, 0.2f, 1 );
    }

    vbInitData.pSysMem = pVertices;
    hr = g_pd3dDevice->CreateBuffer( &vbdesc, &vbInitData, &g_pParticleBuffer );
	if(FAILED(hr)) {
		Quit("Failed to create: g_pParticleBuffer");
	}
    SAFE_DELETE_ARRAY( pVertices );

    return hr;
}


HRESULT CreateParticlePosVeloBuffers()
{
    HRESULT hr = S_OK;

    D3D11_BUFFER_DESC desc;
    ZeroMemory( &desc, sizeof(desc) );
    desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    desc.ByteWidth = MAX_PARTICLES * sizeof(PARTICLE);
    desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    desc.StructureByteStride = sizeof(PARTICLE);
    desc.Usage = D3D11_USAGE_DEFAULT;

    // Initialize the data in the buffers
    PARTICLE* pData1 = new PARTICLE[ MAX_PARTICLES ];
    if( !pData1 )
        return E_OUTOFMEMORY;    

    srand( GetTickCount() );   

    // Disk Galaxy Formation
    LoadParticles( pData1,
        D3DXVECTOR3( 0, 0, 0 ), D3DXVECTOR4( 0, 0, 0, 0 ),
        10, MAX_PARTICLES);


    D3D11_SUBRESOURCE_DATA InitData;
    InitData.pSysMem = pData1;
    hr = g_pd3dDevice->CreateBuffer( &desc, &InitData, &g_pParticlePosVelo0 );
	if(FAILED(hr)) {			
		Quit("Failed to create g_pParticlePosVelo0 buffer");
	}
    hr = g_pd3dDevice->CreateBuffer( &desc, &InitData, &g_pParticlePosVelo1 );
	if(FAILED(hr)) {			
		Quit("Failed to create g_pParticlePosVelo1 buffer");
	}

    SAFE_DELETE_ARRAY( pData1 );

    D3D11_SHADER_RESOURCE_VIEW_DESC DescRV;
    ZeroMemory( &DescRV, sizeof( DescRV ) );
    DescRV.Format = DXGI_FORMAT_UNKNOWN;
    DescRV.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    DescRV.Buffer.FirstElement = 0;
    DescRV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
    hr = g_pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo0, &DescRV, &g_pParticlePosVeloRV0 );
	if(FAILED(hr)) {			
		Quit("Failed to create g_pParticlePosVeloRV0 shaderresourceview");
	}

    hr = g_pd3dDevice->CreateShaderResourceView( g_pParticlePosVelo1, &DescRV, &g_pParticlePosVeloRV1 );
	if(FAILED(hr)) {			
		Quit("Failed to create g_pParticlePosVeloRV1 shaderresourceview");
	}

    D3D11_UNORDERED_ACCESS_VIEW_DESC DescUAV;
    ZeroMemory( &DescUAV, sizeof(D3D11_UNORDERED_ACCESS_VIEW_DESC) );
    DescUAV.Format = DXGI_FORMAT_UNKNOWN;
    DescUAV.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    DescUAV.Buffer.FirstElement = 0;
    DescUAV.Buffer.NumElements = desc.ByteWidth / desc.StructureByteStride;
    hr = g_pd3dDevice->CreateUnorderedAccessView( g_pParticlePosVelo0, &DescUAV, &g_pParticlePosVeloUAV0 );
    if(FAILED(hr)) {			
		Quit("Failed to create g_pParticlePosVeloUAV0");
	}

	hr = g_pd3dDevice->CreateUnorderedAccessView( g_pParticlePosVelo1, &DescUAV, &g_pParticlePosVeloUAV1 );
	if(FAILED(hr)) {			
		Quit("Failed to create g_pParticlePosVeloUAV1");
	}

    return hr;
}


void MoveParticles(double row, double delta, ID3D11ShaderResourceView* inputSRV)
{
    HRESULT hr;
    
        g_pImmediateContext->CSSetShader( g_pComputeShaders[g_positionMap["Gravity.hlsl:CSMain:cs_5_0"]-1], NULL, 0 );

        // For CS input            
        ID3D11ShaderResourceView* aRViews[ 2 ] = { g_pParticlePosVeloRV0, inputSRV };
        g_pImmediateContext->CSSetShaderResources( 0, 2, aRViews );

        // For CS output
        ID3D11UnorderedAccessView* aUAViews[ 1 ] = { g_pParticlePosVeloUAV1 };
        g_pImmediateContext->CSSetUnorderedAccessViews( 0, 1, aUAViews, (UINT*)(&aUAViews) );

        // For CS constant buffer
        D3D11_MAPPED_SUBRESOURCE MappedResource;
        hr = g_pImmediateContext->Map( g_pcbCS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
		if(FAILED(hr)) {			
			Quit("Failed to map particle pcbCS");
		}
        CB_CS* pcbCS = ( CB_CS* )MappedResource.pData;
        
        pcbCS->param[0] = PARTICLE_X;    
		pcbCS->param[1] = PARTICLE_Y;      
		pcbCS->param[2] = MAX_PARTICLES;
        pcbCS->time[0] = row;
        pcbCS->time[1] = delta;
        g_pImmediateContext->Unmap( g_pcbCS, 0 );
        ID3D11Buffer* ppCB[1] = { g_pcbCS };
        g_pImmediateContext->CSSetConstantBuffers( 0, 1, ppCB );

        // Run the CS
        g_pImmediateContext->Dispatch( (UINT)(g_options.iWidth/32), (UINT)(g_options.iHeight/18), 1 );

        // Unbind resources for CS
        ID3D11UnorderedAccessView* ppUAViewNULL[1] = { NULL };
        g_pImmediateContext->CSSetUnorderedAccessViews( 0, 1, ppUAViewNULL, (UINT*)(&aUAViews) );
        ID3D11ShaderResourceView* ppSRVNULL[2] = { NULL, NULL };
        g_pImmediateContext->CSSetShaderResources( 0, 2, ppSRVNULL );

        //g_pImmediateContext->CSSetShader( NULL, NULL, 0 );

        SWAP( g_pParticlePosVelo0, g_pParticlePosVelo1 );
        SWAP( g_pParticlePosVeloRV0, g_pParticlePosVeloRV1 );
        SWAP( g_pParticlePosVeloUAV0, g_pParticlePosVeloUAV1 );

}

//--------------------------------------------------------------------------------------
bool RenderParticles()
{
    ID3D11BlendState *pBlendState0 = NULL;
    ID3D11DepthStencilState *pDepthStencilState0 = NULL;
    UINT SampleMask0, StencilRef0;
    D3DXVECTOR4 BlendFactor0;
    g_pImmediateContext->OMGetBlendState( &pBlendState0, &BlendFactor0.x, &SampleMask0 );
    g_pImmediateContext->OMGetDepthStencilState( &pDepthStencilState0, &StencilRef0 );

	int key = g_positionMap["ParticleDraw.hlsl:VSParticleDraw:vs_5_0"];
    g_pImmediateContext->VSSetShader( g_pVertexShader[key-1], NULL, 0 );
	g_pImmediateContext->GSSetShader( g_pGeometryShader[g_positionMap["ParticleDraw.hlsl:GSParticleDraw:gs_5_0"]-1], NULL, 0 );
	g_pImmediateContext->PSSetShader( g_pPixelShaders[g_positionMap["ParticleDraw.hlsl:PSParticleDraw:ps_5_0"]-1], NULL, 0 );
    
    g_pImmediateContext->IASetInputLayout( g_pParticleVertexLayout );

    // Set IA parameters
    ID3D11Buffer* pBuffers[1] = { g_pParticleBuffer };
    UINT stride[1] = { sizeof( PARTICLE_VERTEX ) };
    UINT offset[1] = { 0 };
    g_pImmediateContext->IASetVertexBuffers( 0, 1, pBuffers, stride, offset );
    g_pImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );

    ID3D11ShaderResourceView* aRViews[ 1 ] = { g_pParticlePosVeloRV0 };
    g_pImmediateContext->VSSetShaderResources( 0, 1, aRViews );

    D3D11_MAPPED_SUBRESOURCE MappedResource;
    g_pImmediateContext->Map( g_pcbGS, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource);
    CB_GS* pCBGS = ( CB_GS* )MappedResource.pData; 
	pCBGS->m_WorldViewProj = XMMatrixMultiply(g_View, g_Projection);
	XMVECTOR identity = XMVECTOR();
	pCBGS->m_InvView = XMMatrixInverse(&identity, g_View);

    g_pImmediateContext->Unmap( g_pcbGS, 0 );
    g_pImmediateContext->GSSetConstantBuffers( 0, 1, &g_pcbGS );

    g_pImmediateContext->PSSetShaderResources( 0, 1, &g_pParticleTexRV );
    g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSampleStateLinear );

    g_pImmediateContext->OMSetBlendState( g_pBlendingStateParticle, D3DXVECTOR4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF  );
    g_pImmediateContext->OMSetDepthStencilState( g_pDepthStencilState, 0 );

    g_pImmediateContext->Draw( MAX_PARTICLES, 0 );

	ID3D11ShaderResourceView* ppSRVNULL[1] = { NULL };
	g_pImmediateContext->VSSetShaderResources( 0, 1, ppSRVNULL );
	g_pImmediateContext->PSSetShaderResources( 0, 1, ppSRVNULL );

	g_pImmediateContext->GSSetShader( NULL, NULL, 0 );
	g_pImmediateContext->OMSetBlendState( pBlendState0, &BlendFactor0.x, SampleMask0 ); SAFE_RELEASE(pBlendState0);
	g_pImmediateContext->OMSetDepthStencilState( pDepthStencilState0, StencilRef0 ); SAFE_RELEASE(pDepthStencilState0);

    return true;
}

void FrameRenderParticles(double row, double delta, ID3D11ShaderResourceView* inputSRV)
{
	//ID3D11ShaderResourceView* shaderResourceViews[] = { inputSRV };
	//g_pImmediateContext->CSSetShaderResources(0, 1, shaderResourceViews);
	MoveParticles(row, delta, inputSRV);

	
    float ClearColor[4] = { 0, 0, 0, 0.0 };
    
	g_pImmediateContext->ClearDepthStencilView( g_pDepthStencilView, D3D11_CLEAR_DEPTH, 1.0, 0 );
    
	g_pImmediateContext->ClearRenderTargetView( particle.renderTargetView, ClearColor );
	
	g_pImmediateContext->OMSetRenderTargets(1, &particle.renderTargetView, g_pDepthStencilView);
	
    // Get the projection & view matrix from the camera class
	
	g_World = XMMatrixIdentity();

//	float camFov = (float)sync_get_val(g_syncTracks.camFov, row);
	float camFov = 1.0f;
	if(camFov < 0.1) {
		camFov = 0.1;
	}

	g_Projection = XMMatrixPerspectiveFovLH(camFov , g_options.iWidth / g_options.iHeight, 0.1f, 1000.0f);

	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -500.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_View = XMMatrixLookAtLH( Eye, At, Up );
	
     // Render the particles
    RenderParticles();
}

//--------------------------------------------------------------------------------------
// Create any D3D11 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CreateParticleResources()
{
    HRESULT hr;

    // Create our vertex input layout
    //const D3D11_INPUT_ELEMENT_DESC layout[] =
    //{
    //    { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    //};
    //V_RETURN( g_pd3dDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
    //    pBlobRenderParticlesVS->GetBufferPointer(), pBlobRenderParticlesVS->GetBufferSize(), &g_pParticleVertexLayout ) );

	CreateParticleBuffer();
    CreateParticlePosVeloBuffers();

    // Setup constant buffer
    D3D11_BUFFER_DESC Desc;
    Desc.Usage = D3D11_USAGE_DYNAMIC;
    Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    Desc.MiscFlags = 0;
    Desc.ByteWidth = sizeof( CB_GS );
    hr = g_pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbGS );
	if(FAILED(hr)) {
		Quit("Failed to create: g_pcbGS");
	}

    Desc.ByteWidth = sizeof( CB_CS );
    hr = g_pd3dDevice->CreateBuffer( &Desc, NULL, &g_pcbCS );
	if(FAILED(hr)) {
		Quit("Failed to create: g_pcbCS");
	}

    // Load the Particle Texture
    hr = D3DX11CreateShaderResourceViewFromFile( g_pd3dDevice, "data/images/particle.png", NULL, NULL, &g_pParticleTexRV, NULL );
	if(FAILED(hr)) {
		Quit("Failed to load: data/images/particle.png");
	}
    
    D3D11_SAMPLER_DESC SamplerDesc;
    ZeroMemory( &SamplerDesc, sizeof(SamplerDesc) );
    SamplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    SamplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    hr = g_pd3dDevice->CreateSamplerState( &SamplerDesc, &g_pSampleStateLinear );
	if(FAILED(hr)) {
		Quit("Failed to create: g_pSampleStateLinear");
	}

    D3D11_BLEND_DESC BlendStateDesc;
    ZeroMemory( &BlendStateDesc, sizeof(BlendStateDesc) );
    BlendStateDesc.RenderTarget[0].BlendEnable = TRUE;
    BlendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
    BlendStateDesc.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
    BlendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    BlendStateDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
    BlendStateDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;
	hr = g_pd3dDevice->CreateBlendState( &BlendStateDesc, &g_pBlendingStateParticle );
		if(FAILED(hr)) {
		Quit("Failed to create: g_pBlendingStateParticle");
	}

    D3D11_DEPTH_STENCIL_DESC DepthStencilDesc;
    ZeroMemory( &DepthStencilDesc, sizeof(DepthStencilDesc) );
    DepthStencilDesc.DepthEnable = FALSE;
    DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
    g_pd3dDevice->CreateDepthStencilState( &DepthStencilDesc, &g_pDepthStencilState );


    return S_OK;
}