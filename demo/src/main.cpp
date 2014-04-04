//--------------------------------------------------------------------------------------
// File: demo.cpp
//
// This application demonstrates creating a Direct3D 11 device
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#include <windows.h>
#include <d3d11.h>
#include <d3dx11.h>

#include <dxgi.h>

#include "../resource.h"
#include "../include/options.h"

#pragma comment(lib, "dxgi.lib")



//--------------------------------------------------------------------------------------
// Global Variables
//--------------------------------------------------------------------------------------
HINSTANCE               g_hInst = NULL;
HWND                    g_hWnd = NULL;
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL;
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0;
ID3D11Device*           g_pd3dDevice = NULL;
ID3D11DeviceContext*    g_pImmediateContext = NULL;
IDXGISwapChain*         g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pBackBuffer = NULL;
ID3D11DepthStencilView* g_pDepthStencilView = NULL;
ID3D11DepthStencilView* g_particleDepthStencilView = NULL;

extern DemoOptions		g_options;

//--------------------------------------------------------------------------------------
// Forward declarations
//--------------------------------------------------------------------------------------
extern void Render(double row, double delta);
extern void InitAudio();
extern double GetAudioRow();
extern void InitOptions();
extern void LoadShaders();
extern void DemoInit();
extern void PlayMusic();


HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
HRESULT InitDevice();
void CleanupDevice();
LRESULT CALLBACK    WndProc( HWND, UINT, WPARAM, LPARAM );

void Quit(LPCTSTR lpText);

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

	g_options.bpp = 32;
	g_options.dWidth = 1280;
	g_options.dHeight = 1280/16*9;
	g_options.iWidth = 1280/4;
	g_options.iHeight = 720/4;
	g_options.offsetX = 0;
	g_options.offsetY = 0;
	g_options.sWidth = 0;
	g_options.sHeight = 0;
	g_options.aspectRatio = 4/3;
#ifdef SYNC_PLAYER
	g_options.fullscreen = true;
#else
	g_options.fullscreen = false;
#endif
	g_options.saveImages = false;
	g_options.saveFrameRate = 60.0;
		
    if( FAILED( InitWindow( hInstance, nCmdShow ) ) ) {
		Quit("Failed to initialize window");
	}

#ifdef SYNC_PLAYER
	InitOptions();
#endif

	CalculateScreenSize();

	SetWindowPos(g_hWnd, NULL, 0, 0, g_options.dWidth, g_options.dHeight, NULL);
	ShowWindow( g_hWnd, nCmdShow );

    if( FAILED( InitDevice() ) )
    {
		Quit("Failed to initialize device");
    }
	
	BOOL *fullscreen = new BOOL;

	g_pSwapChain->GetFullscreenState(fullscreen,NULL);
	while(*fullscreen != g_options.fullscreen) {
		g_pSwapChain->GetFullscreenState(fullscreen,NULL);
		Sleep(1000);
	}

#ifdef SYNC_PLAYER
	ShowCursor(FALSE);
#endif

	InitAudio();
	LoadShaders();
	DemoInit();

	PlayMusic();
    // Main message loop
    MSG msg = {0};
#ifndef SYNC_PLAYER
	double lastRow = -1;
#endif
    while( true)
    {
		if(WM_QUIT == msg.message) break;
#ifdef SYNC_PLAYER
		if(GetAsyncKeyState(VK_ESCAPE)) break;
#endif
		
        if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) )
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
        else
        {
			double row = GetAudioRow();
			double delta = row - lastRow;
			if(row > 4000) break;
#ifndef SYNC_PLAYER
			if(delta < 0) {
				delta = 0;
			}
			if(row == lastRow) {
				// Turha syödä kaikki resursseja, jos pause päällä.
				Sleep(100);
			}
			
#endif
			
			lastRow = row;
            Render(row, delta);
        }
    }

    CleanupDevice();

    return ( int )msg.wParam;
}


//--------------------------------------------------------------------------------------
// Register class and create window
//--------------------------------------------------------------------------------------
HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = NULL;
    wcex.hCursor = LoadCursor( NULL, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "cubicle";
    wcex.hIconSm = NULL;
	wcex.lpszMenuName = NULL;
    if( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    g_hInst = hInstance;
    RECT rc = { 0, 0, g_options.dWidth, g_options.dHeight };
    AdjustWindowRect( &rc, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU, FALSE );
    g_hWnd = CreateWindow( "cubicle", "Demo", WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
                           CW_USEDEFAULT, CW_USEDEFAULT, rc.right - rc.left, rc.bottom - rc.top, NULL, NULL, hInstance,
                           NULL );
    if( !g_hWnd )
        return E_FAIL;

    return S_OK;
}

//--------------------------------------------------------------------------------------
// Called every time the application receives a message
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    PAINTSTRUCT ps;
    HDC hdc;

    switch( message )
    {
		case WM_SYSCOMMAND: 
			if (wParam==SC_SCREENSAVE || wParam==SC_MONITORPOWER) {
				return 0;
			}
			break;
        case WM_PAINT:
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
            return 0;

        case WM_DESTROY:
            PostQuitMessage( 0 );
			CleanupDevice();
            return 0;
    }


	return DefWindowProc( hWnd, message, wParam, lParam );
}



HRESULT CreateDepthStencil(int width, int height, ID3D11DepthStencilView** depthStencilView) {
	HRESULT hr = S_OK;
// Create depth stencil texture
	ID3D11Texture2D* pDepthStencil = NULL;
	D3D11_TEXTURE2D_DESC descDepth;
	descDepth.Width = width;
	descDepth.Height =  height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_pd3dDevice->CreateTexture2D( &descDepth, NULL, &pDepthStencil );
	if( FAILED( hr ) )
		return hr;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = descDepth.Format;
	descDSV.Flags = 0;
	if( descDepth.SampleDesc.Count > 1 )
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
	else
		descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	return g_pd3dDevice->CreateDepthStencilView( pDepthStencil, &descDSV, depthStencilView);
}


//--------------------------------------------------------------------------------------
// Create Direct3D device and swap chain
//--------------------------------------------------------------------------------------
HRESULT InitDevice()
{
    HRESULT hr = S_OK;

    RECT rc;
    GetClientRect( g_hWnd, &rc );

    UINT createDeviceFlags = 0;
	// Needs windows update or something
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
		D3D_FEATURE_LEVEL_11_0
    };
	UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = g_options.dWidth;
    sd.BufferDesc.Height = g_options.dHeight;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = !g_options.fullscreen;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( NULL, g_driverType, NULL, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = NULL;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, NULL, &g_pBackBuffer );
    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pBackBuffer, NULL );

	if( FAILED( CreateDepthStencil(g_options.iWidth, g_options.iHeight, &g_pDepthStencilView) ) )
		return hr;
	if( FAILED( CreateDepthStencil(g_options.iWidth * 2, g_options.iHeight * 2, &g_particleDepthStencilView) ) )
		return hr;
    // Setup the viewport
    D3D11_VIEWPORT vp;
	vp.Width = g_options.dWidth;
	vp.Height = (double)g_options.dWidth*g_options.aspectRatio;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );

    return S_OK;
}



//--------------------------------------------------------------------------------------
// Clean up the objects we've created
//--------------------------------------------------------------------------------------
void ShutdownWindows()
{
	// Show the mouse cursor.
	ShowCursor(true);

	// Fix the display settings if leaving full screen mode.
	if(g_options.fullscreen)
	{
		ChangeDisplaySettings(NULL, 0);
	}

	// Remove the window.
	DestroyWindow(g_hWnd);
	g_hWnd = NULL;

	return;
}

void CleanupDevice()
{
	ShutdownWindows();
	if( g_pImmediateContext ) g_pImmediateContext->ClearState();

	if( g_pBackBuffer ) g_pBackBuffer->Release();
	if( g_pSwapChain ) g_pSwapChain->Release();
	if( g_pImmediateContext ) g_pImmediateContext->Release();
	if( g_pd3dDevice ) g_pd3dDevice->Release();

#ifdef _DEBUG 
	ID3D11Debug *debugDev;
	g_pd3dDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&debugDev));
	debugDev->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL);

#endif

}


void Quit(LPCTSTR lpText) 
{

	MessageBox(g_hWnd, lpText, NULL, MB_OK | MB_ICONERROR);
	CleanupDevice();

	exit(EXIT_FAILURE);
}