#pragma once
#ifndef GLOBALS_H
#define GLOBALS_H

#pragma warning( disable : 4996)
#pragma warning( disable : 4995)
#pragma warning( disable : 4244)

#include <d3d11.h>
#include <d3dx10math.h>
#include <d3dx11tex.h>
#include <D3DCompiler.h>
#include <d3dx11.h>
#include <dxgi.h>
#include <xnamath.h>

#include <vector>
#include <map>
#include <string>
#include <bass.h>
#include <math.h>
#include <Strsafe.h>

#include "audio.h"
#include "options.h"
#include "../../rocket-code/sync/sync.h"



using namespace std;

extern HWND				g_hWnd;
extern DemoOptions		g_options;

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


extern ID3D11DepthStencilView* g_particleDepthStencilView;

extern DemoOptions		g_options;

extern void Render(double row, double delta);
extern void InitAudio();
extern double GetAudioRow();
extern void InitOptions();
extern void LoadShaders();
extern void DemoInit();
extern void PlayMusic();

extern HRESULT CreateParticleResources();
extern void LoadShaders();
extern void Quit(LPCTSTR lpText);
extern void FrameRenderParticles(double row, double delta, ID3D11ShaderResourceView* inputSRV);
extern HRESULT CreateParticleResources();
extern ID3D11InputLayout* g_pParticleVertexLayout;
extern bool IsFileModified(string shader);

#endif