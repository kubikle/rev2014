#include <D3D11.h>
#include <D3DCompiler.h>
#include <d3dx11.h>
#include <vector>
#include <map>
#include <string>

using namespace std;

extern bool IsFileModified(string shader);

#pragma warning( disable : 4996)

extern void Quit(LPCTSTR lpText);

HRESULT				    g_hResult;
extern ID3D11Device*    g_pd3dDevice;

typedef vector <void*> vector_pointer;

vector<ID3D11ComputeShader*>	g_pComputeShaders;
vector<ID3D11VertexShader*>		g_pVertexShader;
vector<ID3D11GeometryShader*>   g_pGeometryShader;
vector<ID3D11PixelShader*>      g_pPixelShaders;

map<string, int>				g_positionMap;


extern ID3D11InputLayout*                  g_pParticleVertexLayout;

void LoadShader(string fileName, string entryPoint, string shaderModel) 
{
	ID3DBlob* pBlob = NULL;
#ifdef _DEBUG
	DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	DWORD shaderFlags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL1;
#endif
	ID3DBlob* pErrorBlob;
	
	OutputDebugString(("\nCompiling: " + fileName + "\n").c_str());

	g_hResult = D3DX11CompileFromFile(("data/shaders/"+fileName).c_str(), NULL, NULL, entryPoint.c_str(), shaderModel.c_str(), shaderFlags, 0, NULL, &pBlob, &pErrorBlob, NULL);
	OutputDebugString(("Compiled: " + fileName + "\n").c_str());

	if (FAILED(g_hResult)) {
#ifndef _DEBUG
		char s[500];
		if (pErrorBlob != NULL) {
			Quit((char*) pErrorBlob->GetBufferPointer());	
		}
		_snprintf_s(s,500,"Failed to compile shader: %s", fileName);
		Quit(s);
#endif
		OutputDebugString("!!!!!!!! Failed to compile shader\n");
		OutputDebugString((fileName + ":" + entryPoint + ":" + shaderModel + "\n").c_str());

		if (pErrorBlob != NULL) {
			OutputDebugString((char*) pErrorBlob->GetBufferPointer());
			//pErrorBlob->Release();			
		}	
	}
	if (pErrorBlob) {
		pErrorBlob->Release();
	}		

	string key = (fileName + ":" + entryPoint + ":" + shaderModel);
	if(shaderModel.compare("cs_5_0") == 0) {
		if(pBlob != NULL) {		
			ID3D11ComputeShader* tempShader;
			int i = g_positionMap[key];
			if(i) {				
				g_pComputeShaders[i-1]->Release();
			}

			g_pd3dDevice->CreateComputeShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &tempShader);

			if(!i) {
				g_positionMap[key] = g_pComputeShaders.size()+1;
				g_pComputeShaders.push_back(tempShader);
			} else {
				g_pComputeShaders[i-1] = tempShader;
			}

			pBlob->Release();
		}
	}
	if(shaderModel.compare("ps_5_0") == 0) {
		if(pBlob != NULL) {
			ID3D11PixelShader* tempShader = (ID3D11PixelShader*)g_positionMap[key];

			int i = g_positionMap[key];
			if(i) {				
				g_pPixelShaders[i-1]->Release();
			}

			g_pd3dDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &tempShader);

			if(!i) {
				g_positionMap[key] = g_pPixelShaders.size()+1;
				g_pPixelShaders.push_back(tempShader);
			} else {
				g_pPixelShaders[i-1] = tempShader;
			}
			
			pBlob->Release();
		}
	}
	if(shaderModel.compare("vs_5_0") == 0) {
		if(pBlob != NULL) {
			ID3D11VertexShader* tempShader;
			
			int i = g_positionMap[key];
			if(i) {				
				g_pVertexShader[i-1]->Release();
			}

			g_pd3dDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &tempShader);

			if(!i) {
				g_positionMap[key] = g_pVertexShader.size()+1;
				g_pVertexShader.push_back(tempShader);

				if(fileName.compare("ParticleDraw.hlsl") == 0) {
					const D3D11_INPUT_ELEMENT_DESC layout[] =
					{
						{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
					};
					g_pd3dDevice->CreateInputLayout( layout, sizeof( layout ) / sizeof( layout[0] ),
						pBlob->GetBufferPointer(), pBlob->GetBufferSize(), &g_pParticleVertexLayout );
				}

			} else {
				g_pVertexShader[i-1] = tempShader;
			}

			pBlob->Release();
		}
	}
	if(shaderModel.compare("gs_5_0") == 0) {
		if(pBlob != NULL) {	
			ID3D11GeometryShader* tempShader;

			int i = g_positionMap[key];
			if(i) {				
				g_pGeometryShader[i-1]->Release();
			}

			g_pd3dDevice->CreateGeometryShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), NULL, &tempShader);

			if(!i) {
				g_positionMap[key] = g_pGeometryShader.size()+1;
				g_pGeometryShader.push_back(tempShader);
			} else {
				g_pGeometryShader[i-1] = tempShader;
			}
			pBlob->Release();
		}
	}
}

void LoadShaders() 
{
	const string shaderPath = "data/shaders/";
	if(IsFileModified(shaderPath + "desert.hlsl")) {
		LoadShader("desert.hlsl", "Checkers", "cs_5_0");
	}
	//if(IsFileModified(shaderPath + "desert2.hlsl")) {
	//	LoadShader("desert2.hlsl", "Checkers", "cs_5_0");
	//}
	//if(IsFileModified(shaderPath + "desert3.hlsl")) {
	//	LoadShader("desert3.hlsl", "Checkers", "cs_5_0");
	//}
	//if(IsFileModified(shaderPath + "desert4.hlsl")) {
	//	LoadShader("desert4.hlsl", "Checkers", "cs_5_0");
	//}
	//if(IsFileModified(shaderPath + "desert5.hlsl")) {
	//	LoadShader("desert5.hlsl", "Checkers", "cs_5_0");
	//}
	if(IsFileModified(shaderPath + "vs.hlsl")) {
		LoadShader("vs.hlsl", "VS","vs_5_0");
	}
	if(IsFileModified(shaderPath + "gs.hlsl")) {
		LoadShader("gs.hlsl", "GS", "gs_5_0");
	}
	if(IsFileModified(shaderPath + "fxaa.hlsl")) {
		LoadShader("fxaa.hlsl", "FXAA_alpha", "ps_5_0");
		LoadShader("fxaa.hlsl", "FXAA", "ps_5_0");
	}
	if(IsFileModified(shaderPath + "post.hlsl")) {
		LoadShader("post.hlsl", "Post", "ps_5_0");
	}
	if(IsFileModified(shaderPath + "scale.hlsl")) {
		LoadShader("scale.hlsl", "Scale", "ps_5_0");
	}

	if(IsFileModified(shaderPath + "HorizontalBlurCS.hlsl")) {
		LoadShader("HorizontalBlurCS.hlsl", "main", "cs_5_0");
	}
	if(IsFileModified(shaderPath + "VerticalBlurCS.hlsl")) {
		LoadShader("VerticalBlurCS.hlsl", "main", "cs_5_0");
	}

	if(IsFileModified(shaderPath + "ParticleDraw.hlsl")) {
		LoadShader("ParticleDraw.hlsl", "VSParticleDraw", "vs_5_0");
		LoadShader("ParticleDraw.hlsl", "GSParticleDraw", "gs_5_0");
		LoadShader("ParticleDraw.hlsl", "PSParticleDraw", "ps_5_0");
	}
	if(IsFileModified(shaderPath + "Gravity.hlsl")) {
		LoadShader("Gravity.hlsl", "CSMain", "cs_5_0");
	}
}