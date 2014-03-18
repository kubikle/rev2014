#include <d3d11.h>
#include <d3dx10math.h>
#include <xnamath.h>
#include <vector>
#include <map>
#include <string>
#include "../include/options.h"

extern ID3D11Device* g_pd3dDevice;
extern ID3D11DeviceContext*				g_pImmediateContext;
extern vector<ID3D11ComputeShader*>		g_pComputeShaders;
extern vector<ID3D11VertexShader*>		g_pVertexShader;
extern vector<ID3D11GeometryShader*>    g_pGeometryShader;
extern vector<ID3D11PixelShader*>       g_pPixelShaders;
extern map<string, int>					g_positionMap;
extern DemoOptions						g_options;

struct PARTICLE_VERTEX
{
	D3DXVECTOR4 Color;
};
#define MAX_PARTICLES 2000000         // the number of particles in the n-body simulation

struct CB_GS
{
	D3DXMATRIX m_WorldViewProj;
	D3DXMATRIX m_InvView;
};

struct CB_CS
{
	UINT param[4];
	float paramf[4];
};

struct PARTICLE
{
	D3DXVECTOR4 pos;
	D3DXVECTOR4 velo;
	D3DXVECTOR4 color;
	float		mass;
};

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

bool Particles()
{
    ID3D11BlendState *pBlendState0 = NULL;
    ID3D11DepthStencilState *pDepthStencilState0 = NULL;
    UINT SampleMask0, StencilRef0;
    D3DXVECTOR4 BlendFactor0;
    g_pImmediateContext->OMGetBlendState( &pBlendState0, &BlendFactor0.x, &SampleMask0 );
    g_pImmediateContext->OMGetDepthStencilState( &pDepthStencilState0, &StencilRef0 );

    g_pImmediateContext->VSSetShader( g_pRenderParticlesVS, NULL, 0 );
    g_pImmediateContext->GSSetShader( g_pRenderParticlesGS, NULL, 0 );
    g_pImmediateContext->PSSetShader( g_pRenderParticlesPS, NULL, 0 );
    
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

void RenderParticles() {
	float ClearColor[4] = { 0.0, 0.0, 0.0, 0.0 };
	ID3D11RenderTargetView* pRTV = DXUTGetD3D11RenderTargetView();
	g_pImmediateContext->ClearRenderTargetView( pRTV, ClearColor );
	ID3D11DepthStencilView* pDSV = DXUTGetD3D11DepthStencilView();
	g_pImmediateContext->ClearDepthStencilView( pDSV, D3D11_CLEAR_DEPTH, 1.0, 0 );

	// Get the projection & view matrix from the camera class

	XMVECTOR Eye = XMVectorSet( 0.0f, 0.0f, -40.0f, 0.0f );
	XMVECTOR At = XMVectorSet( 0.0f, 0.0f, 0.0f, 0.0f );
	XMVECTOR Up = XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f );
	g_View = XMMatrixLookAtLH( Eye, At, Up );

	// Render the particles
	RenderParticles();
}