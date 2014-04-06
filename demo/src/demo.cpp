#include <d3d11.h>
#include <d3dx10math.h>
#include <vector>
#include <map>
#include <string>
#include "../include/audio.h"
#include "../include/options.h"
#include "../../rocket-code/sync/sync.h"
#include <d3dx11tex.h>

using namespace std;

extern ID3D11Device*					g_pd3dDevice;
extern ID3D11DeviceContext*				g_pImmediateContext;
extern IDXGISwapChain*					g_pSwapChain;
extern ID3D11RenderTargetView*			g_pBackBuffer;
extern DemoOptions						g_options;
extern SyncTracks						g_syncTracks;
extern vector<ID3D11ComputeShader*>		g_pComputeShaders;
extern vector<ID3D11VertexShader*>		g_pVertexShader;
extern vector<ID3D11GeometryShader*>    g_pGeometryShader;
extern vector<ID3D11PixelShader*>       g_pPixelShaders;
extern map<string, int>					g_positionMap;

ID3D11ShaderResourceView*				g_pDataSRV;
ID3D11UnorderedAccessView*				g_pDataUAV;
ID3D11Buffer*							g_pData;
ID3D11Buffer*							g_pConstantBuffer;

ID3D11ShaderResourceView* mBlurredOutputTexSRV;
ID3D11UnorderedAccessView* mBlurredOutputTexUAV;

//vector <ID3D11Texture2D*>			g_renderTargetTextures;
//vector <ID3D11RenderTargetView*>	g_renderTargetViews;
//vector <ID3D11ShaderResourceView*>	g_shaderResourceViews;



typedef struct{
	ID3D11Texture2D*		   texture;
	ID3D11RenderTargetView*    renderTargetView;
	ID3D11ShaderResourceView*  shaderResourceView;
	ID3D11UnorderedAccessView* unorderedAccessView;
} View;

View rm;
View fxaaAlpha;
View fxaa;
View blurH;
View bloom;
View post;
View particle;

vector <ID3D11ShaderResourceView*>  g_pImages;

ID3D11SamplerState*         g_pSamplerLinear, *g_pSamplerPoint;


extern HRESULT CreateParticleResources();
extern void LoadShaders();
extern void Quit(LPCTSTR lpText);
extern void FrameRenderParticles(double row, double delta, ID3D11ShaderResourceView* inputSRV);
extern HRESULT CreateParticleResources();
extern ID3D11InputLayout* g_pParticleVertexLayout;
extern bool IsFileModified(string shader);
 
void Render(double row);

#pragma warning( disable : 4324 )
__declspec(align(128))
struct CB
{
	float		beat;
	float		cam;
	float		camTime;
	float		camFov;
	D3DXVECTOR3 camRot;
	float		saturation;
	float		fade;
	float		radial;
	float		synk1;
	float		synk2;
	float		synk3;
	float		synk4;
	float		synk5;
	float		synk6;
	float		synk7;
	float		synk8;
	float		synk9;
	float		xRes;
	float		yRes;
	float		xRm;
	float		yRm;
	float		noiseAmount;
	float		imageFade;
	float		blurPower;
	float		bloomPower;
};

struct Data {
	float r, g, b, a;
};

void InitializeRenderTexture(int textureWidth, 
							int textureHeight, 
							ID3D11RenderTargetView	  **renderTargetView,
							ID3D11Texture2D			  **renderTargetTexture,
							ID3D11ShaderResourceView  **shaderResourceView,
							ID3D11UnorderedAccessView **unorderedAccessView)
{
	HRESULT result;

	D3D11_TEXTURE2D_DESC textureDesc;
	D3D11_RENDER_TARGET_VIEW_DESC renderTargetViewDesc;
	D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;
	
	// Initialize the render target texture description.
	ZeroMemory(&textureDesc, sizeof(textureDesc));

	// Setup the render target texture description.
	textureDesc.Width = textureWidth;
	textureDesc.Height = textureHeight;
	textureDesc.MipLevels = 1;
	textureDesc.ArraySize = 1;
	textureDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.Usage = D3D11_USAGE_DEFAULT;
	textureDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	textureDesc.CPUAccessFlags = 0;
	textureDesc.MiscFlags = 0;

	// Create the render target texture.
	result = g_pd3dDevice->CreateTexture2D(&textureDesc, NULL, renderTargetTexture);
	if(FAILED(result))
	{
		Quit("Unable to create render target texture.");
	}

	// Initialize the render target view description.
	ZeroMemory(&renderTargetViewDesc, sizeof(renderTargetViewDesc));
	// Setup the description of the render target view.
	renderTargetViewDesc.Format = textureDesc.Format;
	renderTargetViewDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	renderTargetViewDesc.Texture2D.MipSlice = 0;

	// Create the render target view.
	result = g_pd3dDevice->CreateRenderTargetView(*renderTargetTexture, &renderTargetViewDesc, renderTargetView);
	if(FAILED(result))
	{
		Quit("Unable to create render target view.");
	}

	// Initialize the shader resource view description.
	ZeroMemory(&shaderResourceViewDesc, sizeof(shaderResourceViewDesc));
	// Setup the description of the shader resource view.
	shaderResourceViewDesc.Format = textureDesc.Format;
	shaderResourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
	shaderResourceViewDesc.Texture2D.MipLevels = 1;

	// Create the shader resource view.
	result = g_pd3dDevice->CreateShaderResourceView(*renderTargetTexture, &shaderResourceViewDesc, shaderResourceView);
	if(FAILED(result))
	{
		Quit("Unable to create shader resource view.");
	}

	if(unorderedAccessView) {
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		result = g_pd3dDevice->CreateUnorderedAccessView(*renderTargetTexture, &uavDesc, unorderedAccessView);
		if(FAILED(result)) {
			Quit("BlurFilterInit: failed to create unordered access view");
		}
	}

}

void BlurFilterInit()
{
	// Note, compressed formats cannot be used for UAV.  We get error like:
	// ERROR: ID3D11Device::CreateTexture2D: The format (0x4d, BC3_UNORM) 
	// cannot be bound as an UnorderedAccessView, or cast to a format that
	// could be bound as an UnorderedAccessView.  Therefore this format 
	// does not support D3D11_BIND_UNORDERED_ACCESS.
	D3D11_TEXTURE2D_DESC blurredTexDesc;
	blurredTexDesc.Width = (UINT)g_options.iWidth;
	blurredTexDesc.Height = (UINT)g_options.iHeight;
	blurredTexDesc.MipLevels = 1;
	blurredTexDesc.ArraySize = 1;
	blurredTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	blurredTexDesc.SampleDesc.Count   = 1;
	blurredTexDesc.SampleDesc.Quality = 0;
	blurredTexDesc.Usage = D3D11_USAGE_DEFAULT;
	blurredTexDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	blurredTexDesc.CPUAccessFlags = 0;
	blurredTexDesc.MiscFlags = 0;

	ID3D11Texture2D* blurredTex = NULL;
	HRESULT result = g_pd3dDevice->CreateTexture2D(&blurredTexDesc, 0, &blurredTex);
	if(FAILED(result)) {
		Quit("BlurFilterInit: failed to create texture");
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	result = g_pd3dDevice->CreateShaderResourceView(blurredTex, &srvDesc, &mBlurredOutputTexSRV);
	if(FAILED(result)) {
		Quit("BlurFilterInit: failed to create shader resource view");
	}

	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;
	result = g_pd3dDevice->CreateUnorderedAccessView(blurredTex, &uavDesc, &mBlurredOutputTexUAV);
	if(FAILED(result)) {
		Quit("BlurFilterInit: failed to create unordered access view");
	}

	// Views save a reference to the texture so we can release our reference.
	blurredTex->Release();
}

void LoadImage( string name ) 
{
	if(IsFileModified(name)) {
	
		ID3D11ShaderResourceView *view;
		int i = g_positionMap[name];
		if(i) {				
			g_pImages[i-1]->Release();
		} else {
			g_positionMap[name] = g_pImages.size()+1;				
			g_pImages.push_back(view);
		}
		HRESULT result = D3DX11CreateShaderResourceViewFromFile(g_pd3dDevice,
			name.c_str(),
			NULL, NULL, &g_pImages[g_positionMap[name]-1],NULL	);

		if( FAILED( result ) ) {
			Quit(("Couldn't load " + name).c_str());
		}
	}
}

void LoadImages() {
	LoadImage("data/images/c.png");	
	LoadImage("data/images/u.png");	
	LoadImage("data/images/b.png");	
	LoadImage("data/images/i.png");	
	LoadImage("data/images/c2.png");	
	LoadImage("data/images/l.png");	
	LoadImage("data/images/e.png");	
	LoadImage("data/images/eye.png");
	LoadImage("data/images/cubicle2d.png");
	LoadImage("data/images/htth.png");
	LoadImage("data/images/ronn.png");
	LoadImage("data/images/kolmas.png");
	LoadImage("data/images/aalto.png");
	LoadImage("data/images/id.png");
	LoadImage("data/images/korot.png");
	LoadImage("data/images/nousee.png");
	LoadImage("data/images/yyteet.png");
	LoadImage("data/images/tulee.png");

	LoadImage("data/images/kasetti1.png");
	LoadImage("data/images/kasetti2.png");
	LoadImage("data/images/kasetti3.png");

	LoadImage("data/images/isokeha.png");
	LoadImage("data/images/isokeha_tulee.png");

	LoadImage("data/images/skyfox.png");
	LoadImage("data/images/JuhoAP.png");
	LoadImage("data/images/tripper.png");

	LoadImage("data/images/julia.png");
}

void DemoInit()
{
	HRESULT result;
	D3D11_BUFFER_DESC sbDesc;
	ZeroMemory(&sbDesc, sizeof(sbDesc));
	sbDesc.StructureByteStride = sizeof(Data);
	sbDesc.ByteWidth = (UINT)(sizeof(Data) * g_options.iWidth * g_options.iHeight);
	sbDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	sbDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
	sbDesc.Usage = D3D11_USAGE_DEFAULT;
	result = g_pd3dDevice->CreateBuffer(&sbDesc, NULL, &g_pData);
	if(FAILED(result)) {
		Quit("Couldn't create compute shader buffer description");
	}
	
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	srvDesc.Buffer.NumElements =  (UINT)(g_options.iWidth * g_options.iHeight);
	result = g_pd3dDevice->CreateShaderResourceView(g_pData, &srvDesc, &g_pDataSRV);
	if(FAILED(result)) {
		Quit("Couldn't create compute shader resource view desc");
	}
	
	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	ZeroMemory( &uavDesc, sizeof(uavDesc));
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.NumElements = (UINT)(g_options.iWidth * g_options.iHeight);
	result = g_pd3dDevice->CreateUnorderedAccessView(g_pData, &uavDesc, &g_pDataUAV);
	if(FAILED(result)) {
		Quit("Couldn't create compute shader unordered access view desc");
	}


	// Create the Const Buffer
	D3D11_BUFFER_DESC constant_buffer_desc;
	ZeroMemory( &constant_buffer_desc, sizeof(constant_buffer_desc) );
	constant_buffer_desc.ByteWidth = sizeof(CB);
	constant_buffer_desc.Usage = D3D11_USAGE_DYNAMIC;
	constant_buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constant_buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	result = g_pd3dDevice->CreateBuffer( &constant_buffer_desc, NULL, &g_pConstantBuffer );
	if(FAILED(result)) {
		Quit("Couldn't create constant buffer.");
	}

	// Create the sample state
	D3D11_SAMPLER_DESC sampDesc;
	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerLinear );
	if(FAILED(result)) {
		Quit("Couldn't create linear sampler");
	}

	// Create the sample state
	sampDesc;
	ZeroMemory( &sampDesc, sizeof(sampDesc) );
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	result = g_pd3dDevice->CreateSamplerState( &sampDesc, &g_pSamplerPoint );
	if(FAILED(result)) {
		Quit("Couldn't create point sampler");
	}
	
		
	InitializeRenderTexture((int)g_options.iWidth, (int)g_options.iHeight, &rm.renderTargetView, &rm.texture, &rm.shaderResourceView, NULL);
	InitializeRenderTexture((int)g_options.iWidth, (int)g_options.iHeight, &fxaa.renderTargetView, &fxaa.texture, &fxaa.shaderResourceView, &fxaa.unorderedAccessView);
	InitializeRenderTexture((int)g_options.iWidth, (int)g_options.iHeight, &fxaaAlpha.renderTargetView, &fxaaAlpha.texture, &fxaaAlpha.shaderResourceView, NULL);	
	InitializeRenderTexture((int)g_options.iWidth, (int)g_options.iHeight, &post.renderTargetView, &post.texture, &post.shaderResourceView, &post.unorderedAccessView);	
	InitializeRenderTexture((int)g_options.iWidth, (int)g_options.iHeight, &bloom.renderTargetView, &bloom.texture, &bloom.shaderResourceView, &bloom.unorderedAccessView);	
	InitializeRenderTexture((int)g_options.iWidth*2, (int)g_options.iHeight*2, &particle.renderTargetView, &particle.texture, &particle.shaderResourceView, &particle.unorderedAccessView);	

	// Create UAV from render target
	result = g_pd3dDevice->CreateUnorderedAccessView(rm.texture, NULL, &rm.unorderedAccessView);
	if(FAILED(result)) {
		Quit("Couldn't create point UAV from render target");
	}
	LoadImages();

	
	BlurFilterInit();

	CreateParticleResources();

	//g_pImmediateContext->IASetVertexBuffers(0, 0, NULL, NULL, NULL);
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
	g_pImmediateContext->IASetInputLayout( g_pParticleVertexLayout );
}

void SetConstants( double row)
{	
	D3DXVECTOR3 camRot;

	camRot.x = (float)sync_get_val(g_syncTracks.camRotX, row);
	camRot.y = (float)sync_get_val(g_syncTracks.camRotY, row);
	camRot.z = (float)sync_get_val(g_syncTracks.camRotZ, row);

	CB cb = { 
		(float)row,
		(float)sync_get_val(g_syncTracks.cam, row),
		(float)sync_get_val(g_syncTracks.camTime, row),
		(float)sync_get_val(g_syncTracks.camFov, row),
		camRot,
		(float)sync_get_val(g_syncTracks.saturation, row),
		(float)sync_get_val(g_syncTracks.fade, row),
		(float)sync_get_val(g_syncTracks.radial, row),
		(float)sync_get_val(g_syncTracks.synk1, row),
		(float)sync_get_val(g_syncTracks.synk2, row),
		(float)sync_get_val(g_syncTracks.synk3, row),
		(float)sync_get_val(g_syncTracks.synk4, row),
		(float)sync_get_val(g_syncTracks.synk5, row),
		(float)sync_get_val(g_syncTracks.synk6, row),		
		(float)sync_get_val(g_syncTracks.synk7, row),
		(float)sync_get_val(g_syncTracks.synk8, row),
		(float)sync_get_val(g_syncTracks.synk9, row),
		(float)g_options.iWidth,
		(float)g_options.iHeight,
		(float)g_options.iWidth,
		(float)g_options.iHeight,
		(float)sync_get_val(g_syncTracks.noise, row),
		(float)sync_get_val(g_syncTracks.imageFade, row),
		(float)sync_get_val(g_syncTracks.blurPower, row),
		(float)sync_get_val(g_syncTracks.bloomPower, row)

	};

	D3D11_MAPPED_SUBRESOURCE MappedResource;
	g_pImmediateContext->Map( g_pConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource );
	memcpy( MappedResource.pData, &cb, sizeof(cb) );
	g_pImmediateContext->Unmap( g_pConstantBuffer, 0 );
	g_pImmediateContext->CSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
	g_pImmediateContext->PSSetConstantBuffers( 0, 1, &g_pConstantBuffer );
}

void RenderToScreen(double row, ID3D11ShaderResourceView *viewToRender, ID3D11ShaderResourceView *viewToRender2) 
{
	int imageId = (int)sync_get_val(g_syncTracks.image, row);
	g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBuffer, NULL);
	g_pImmediateContext->PSSetShaderResources( 0, 1, &viewToRender );
	//g_pImmediateContext->PSSetShaderResources( 1, 1, &viewToRender2);
	g_pImmediateContext->PSSetShaderResources( 1, 1, &viewToRender2 );
	g_pImmediateContext->PSSetShader(g_pPixelShaders[3], NULL, 0);
	g_pImmediateContext->Draw(1, 0);
}

void SetViewPortToXY(int x, int y) 
{
	D3D11_VIEWPORT vp;

	vp.Width = (float)x;
	vp.Height = (float)y;
	vp.MinDepth = 0.0;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;

	g_pImmediateContext->RSSetViewports(1, &vp);
}

void RenderFxaa(ID3D11ShaderResourceView *viewToRender) 
{
	// calculatealpha for fxaa
	g_pImmediateContext->OMSetRenderTargets(1, &fxaaAlpha.renderTargetView, NULL);
	g_pImmediateContext->PSSetShaderResources( 0, 1, &viewToRender );
	g_pImmediateContext->PSSetShader(g_pPixelShaders[0], NULL, 0);
	g_pImmediateContext->Draw(1, 0);

	//fxaa
	g_pImmediateContext->OMSetRenderTargets(1, &fxaa.renderTargetView, NULL);
	g_pImmediateContext->PSSetShaderResources( 0, 1, &fxaaAlpha.shaderResourceView );
	g_pImmediateContext->PSSetShader(g_pPixelShaders[1], NULL, 0);
	g_pImmediateContext->Draw(1, 0);
}

void RenderPost(ID3D11ShaderResourceView *view, double row) 
{
	//post
	int imageId = (int)sync_get_val(g_syncTracks.image, row);
	imageId %= g_pImages.size();
	g_pImmediateContext->OMSetRenderTargets(1, &post.renderTargetView, NULL);
	g_pImmediateContext->PSSetShaderResources( 0, 1, &view );
	g_pImmediateContext->PSSetShaderResources( 1, 1, &g_pImages[imageId] );
	g_pImmediateContext->PSSetShader(g_pPixelShaders[2], NULL, 0);
	g_pImmediateContext->Draw(1, 0);
}

void Raymarch( double row ) 
{
	UINT initCounts = 0;
	ID3D11UnorderedAccessView* uavNullView[2] = { NULL, NULL };
	//ID3D11RenderTargetView* rtvNullView[1] = { NULL };


	ID3D11UnorderedAccessView* uavViews[2] = { NULL, rm.unorderedAccessView };
	int sceneId = (int)sync_get_val(g_syncTracks.scene, row);

	g_pImmediateContext->CSSetShader(g_pComputeShaders[sceneId % g_pComputeShaders.size()], NULL, 0);
	g_pImmediateContext->CSSetShaderResources(0, 1, &g_pDataSRV);
	//g_pImmediateContext->CSSetShaderResources( 1, 1, &g_pTextureRV[imageId] );
	g_pImmediateContext->CSSetUnorderedAccessViews(0, 2, uavViews, &initCounts);
	g_pImmediateContext->Dispatch((UINT)(g_options.iWidth/32), (UINT)(g_options.iHeight/30), 1);	
	g_pImmediateContext->CSSetUnorderedAccessViews(0, 2, uavNullView, &initCounts);	
}

void SetSamplers() 
{
	g_pImmediateContext->CSSetSamplers( 0, 1, &g_pSamplerLinear );
	g_pImmediateContext->PSSetSamplers( 0, 1, &g_pSamplerLinear );
	g_pImmediateContext->CSSetSamplers( 1, 1, &g_pSamplerPoint );
	g_pImmediateContext->PSSetSamplers( 1, 1, &g_pSamplerPoint );
}

void Blur(ID3D11ShaderResourceView* inputSRV, ID3D11UnorderedAccessView* inputUAV, const UINT blurCount)
{
	//
	// Run the compute shader to blur the offscreen texture.
	// 
	if(blurCount < 1) {

		g_pImmediateContext->CSSetShader(NULL, NULL, 0);
		return;
	}

	for(size_t i = 0; i < blurCount; ++i)
	{
		//
		// Horizontal Blur Pass
		//

		// Set horizontal blur compute shader
		string key = "HorizontalBlurCS.hlsl:main:cs_5_0";
		int pos = g_positionMap[key];
		g_pImmediateContext->CSSetShader(g_pComputeShaders[pos-1], NULL, (UINT)0);

		// Set shaders resources
		ID3D11ShaderResourceView* shaderResourceViews[] = { inputSRV };
		g_pImmediateContext->CSSetShaderResources(0, 1, shaderResourceViews);

		ID3D11UnorderedAccessView* unorderedAccessViews[] = { mBlurredOutputTexUAV };
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, unorderedAccessViews, NULL);  

		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		const UINT numGroupsX = static_cast<UINT> (ceilf((float)g_options.iWidth / 256.0f));
		g_pImmediateContext->Dispatch(numGroupsX, g_options.iHeight, 1);

		// Unbind the input texture from the CS for good housekeeping.
		ID3D11ShaderResourceView* nullSRV[] = { NULL };
		g_pImmediateContext->CSSetShaderResources(0, 1, nullSRV);

		// Unbind output from compute shader (we are going to use this output as an input in the next pass, 
		// and a resource cannot be both an output and input at the same time.
		ID3D11UnorderedAccessView* nullUAV[] = { NULL };
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, nullUAV, NULL);

		//
		// Vertical Blur Pass
		//

		// Set vertical blur compute shader
		g_pImmediateContext->CSSetShader(g_pComputeShaders[g_positionMap["VerticalBlurCS.hlsl:main:cs_5_0"]-1], NULL, 0);

		// Set shaders resources
		shaderResourceViews[0] = mBlurredOutputTexSRV;
		g_pImmediateContext->CSSetShaderResources(0, 1, shaderResourceViews);

		unorderedAccessViews[0] = inputUAV;
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, unorderedAccessViews, NULL);  

		// How many groups do we need to dispatch to cover a column of pixels, where each
		// group covers 256 pixels  (the 256 is defined in the ComputeShader).
		const UINT numGroupsY = static_cast<UINT> (ceilf((float)g_options.iHeight / 256.0f));
		g_pImmediateContext->Dispatch(g_options.iWidth, numGroupsY, 1);

		g_pImmediateContext->CSSetShaderResources(0, 1, nullSRV);
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, nullUAV, NULL);
	}

	// Disable compute shader.
	g_pImmediateContext->CSSetShader(NULL, NULL, 0);
}

void Bloom(ID3D11ShaderResourceView* inputSRV, const UINT blurCount)
{
	//
	// Run the compute shader to blur the offscreen texture.
	// 
	if(blurCount < 1) {

		g_pImmediateContext->CSSetShader(NULL, NULL, 0);
		return;
	}
	for(size_t i = 0; i < blurCount; ++i)
	{
		//
		// Horizontal Blur Pass
		//

		// Set horizontal blur compute shader
		string key = "HorizontalBlurCS.hlsl:main:cs_5_0";
		int pos = g_positionMap[key];
		g_pImmediateContext->CSSetShader(g_pComputeShaders[pos-1], NULL, (UINT)0);
		ID3D11ShaderResourceView* shaderResourceViews[] = { inputSRV };
		if(i > 0) {
			shaderResourceViews[0] = bloom.shaderResourceView;			
		}
		g_pImmediateContext->CSSetShaderResources(0, 1, shaderResourceViews);
		ID3D11UnorderedAccessView* unorderedAccessViews[] = { mBlurredOutputTexUAV };
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, unorderedAccessViews, NULL);  

		// How many groups do we need to dispatch to cover a row of pixels, where each
		// group covers 256 pixels (the 256 is defined in the ComputeShader).
		const UINT numGroupsX = static_cast<UINT> (ceilf((float)g_options.iWidth / 256.0f));
		g_pImmediateContext->Dispatch(numGroupsX, g_options.iHeight, 1);

		// Unbind the input texture from the CS for good housekeeping.
		ID3D11ShaderResourceView* nullSRV[] = { NULL };
		g_pImmediateContext->CSSetShaderResources(0, 1, nullSRV);

		// Unbind output from compute shader (we are going to use this output as an input in the next pass, 
		// and a resource cannot be both an output and input at the same time.
		ID3D11UnorderedAccessView* nullUAV[] = { NULL };
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, nullUAV, NULL);
		//
		// Vertical Blur Pass
		//

		// Set vertical blur compute shader
		g_pImmediateContext->CSSetShader(g_pComputeShaders[g_positionMap["VerticalBlurCS.hlsl:main:cs_5_0"]-1], NULL, 0);

		// Set shaders resources
		shaderResourceViews[0] = mBlurredOutputTexSRV;
		g_pImmediateContext->CSSetShaderResources(0, 1, shaderResourceViews);

		unorderedAccessViews[0] = bloom.unorderedAccessView;
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, unorderedAccessViews, NULL);  

		// How many groups do we need to dispatch to cover a column of pixels, where each
		// group covers 256 pixels  (the 256 is defined in the ComputeShader).
		const UINT numGroupsY = static_cast<UINT> (ceilf((float)g_options.iHeight / 256.0f));
		g_pImmediateContext->Dispatch(g_options.iWidth, numGroupsY, 1);

		g_pImmediateContext->CSSetShaderResources(0, 1, nullSRV);
		g_pImmediateContext->CSSetUnorderedAccessViews(0, 1, nullUAV, NULL);
	}

	// Disable compute shader.
	g_pImmediateContext->CSSetShader(NULL, NULL, 0);
}

//--------------------------------------------------------------------------------------
// Render the frame
//--------------------------------------------------------------------------------------
int frame = 0;
char filename[256];
ID3D11Resource  *bbRes;

void Render(double row, double delta)
{
	
#ifndef SYNC_PLAYER
	LoadImages();
	LoadShaders();
#endif
	
	SetViewPortToXY(g_options.iWidth, g_options.iHeight);
	//SetViewPortToScreenSize();
	//g_pImmediateContext->OMSetRenderTargets(1, &g_pBackBuffer, NULL);
	g_pImmediateContext->IASetInputLayout( g_pParticleVertexLayout );
	
	ID3D11ShaderResourceView* srvNullView[1] = { NULL };
	g_pImmediateContext->VSSetShaderResources( 0, 1, srvNullView );
	g_pImmediateContext->PSSetShaderResources( 0, 1, srvNullView );
	g_pImmediateContext->GSSetShader( NULL, NULL, 0 );

	SetConstants(row);	
	SetSamplers();
	g_pImmediateContext->VSSetShader(g_pVertexShader[0], NULL, 0);
	Raymarch(row);

	//g_pImmediateContext->VSSetShader(g_pVertexShader[0], NULL, 0);
	g_pImmediateContext->GSSetShader(g_pGeometryShader[0], NULL, 0);

	//RenderFxaa(rm.shaderResourceView);

	//g_pImmediateContext->OMSetRenderTargets(1, &blurH.renderTargetView, NULL);
	//Bloom(rm.shaderResourceView, (UINT)sync_get_val(g_syncTracks.bloom, row));

	RenderPost(rm.shaderResourceView, row);

	g_pImmediateContext->OMSetRenderTargets(1, &blurH.renderTargetView, NULL);
	Blur(post.shaderResourceView, post.unorderedAccessView, (UINT)sync_get_val(g_syncTracks.blur, row));
	
	g_pImmediateContext->PSSetShaderResources( 0, 1, &srvNullView[0] );
	g_pImmediateContext->PSSetShaderResources( 1, 1, &srvNullView[0] );

	SetViewPortToXY(g_options.iWidth*2, g_options.iHeight*2);
	
	FrameRenderParticles(row, delta, post.shaderResourceView);

	SetConstants(row);	
	SetSamplers();

	g_pImmediateContext->PSSetShaderResources( 0, 1, &srvNullView[0] );
	g_pImmediateContext->PSSetShaderResources( 1, 1, &srvNullView[0] );


	g_pImmediateContext->VSSetShader(g_pVertexShader[0], NULL, 0);
	g_pImmediateContext->GSSetShader(g_pGeometryShader[0], NULL, 0);
	SetViewPortToXY(g_options.sWidth, g_options.sHeight);

	RenderToScreen( row, post.shaderResourceView, particle.shaderResourceView);

    g_pSwapChain->Present( 0, 0 );

	if (g_options.saveImages) {
		g_pBackBuffer->GetResource(&bbRes);
		sprintf_s(filename, sizeof(filename), "image_%06d.png", frame++);

		D3DX11SaveTextureToFile(g_pImmediateContext, bbRes,D3DX11_IFF_PNG,filename);
	}
}