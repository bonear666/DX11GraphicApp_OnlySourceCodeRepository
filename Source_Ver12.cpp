#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3d10_1")
#pragma comment(lib, "d3d10")
#pragma comment(lib,"d3dcompiler")

#include <windows.h>
#include <D3D11.h>
#include <D3D10.h>
#include <DXGI.h>
#include <D3DX10math.h>
#include <d3dcompiler.h>
#include <iostream>
#include <new>

// ���������� ����������

HINSTANCE               g_hInst = NULL; //��������� �� struct, ����������(handle) ������� ����������.
HWND                    g_hWnd = NULL; //��������� �� struct, ����������(handle) ���� ������� ����������.
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL; //������������� ���������. ����������, ������������ ��� ��������, ����������, ��� ����������� ����������.
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_9_1; //������������ feature level, ������, ��� ����������, ������� ����� ������� ����� ������� feature level, ��������� �� �������� ��������.
ID3D11Device* g_pd3dDevice = NULL; //��������� �� struct(������ ���������� ID3D11Device). ID3D11Device ��� COM-���������, ������� ������� �������(��������, ���������� ������� � �.�.) ��� ������ �� �������.
ID3D11DeviceContext* g_pImmediateContext = NULL; //��������� �� struct(������ ���������� ID3D11DeviceContext). ID3D11DeviceContext ��� COM-���������, ������� ���������� ���������� ����������� ���������� �� �������.
IDXGISwapChain* g_pSwapChain = NULL; //��������� �� struct(������ ���������� IDXGISwapChain). IDXGISwapChain ��� COM-���������, ������� ������ � ���������� ������� ��������� ����������� ������������ ����� �� ������� �� �������.
ID3D11RenderTargetView* g_pRenderTargetView = NULL; //��������� �� struct(������ ���������� ID3D11RenderTargetView). ID3D11RenderTargetView ��� COM-���������, ������� ������ ������� back buffer-�. 
ID3D11InputLayout* g_pInputLayoutObject = NULL; // ��������� �� input layout object
ID3D11VertexShader* g_pVertexshader = NULL; // ��������� �� ��������� vertex shader
ID3D11PixelShader* g_pPixelShader = NULL; // ��������� �� ��������� pixel shader
ID3DBlob* VS_Buffer = NULL; // ��������� �� ��������� ������ � ���������������� ��������� �������� 
ID3DBlob* PS_Buffer = NULL; // ��������� �� ��������� ������ � ���������������� ���������� �������� 
ID3D11Buffer* pVertexBuffer = NULL; // ��������� �� ����� ������

//��������������� ���������� �������

// ������� ����
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); 
// �������� DirectX �����������
HRESULT createDirect3DComponents(UINT widthParam, UINT heightParam);
// �������� ����
HRESULT createWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam);
//
HRESULT initGeometry();
// ���������� �����
void updateScene();
// ��������� �����
void drawScene();
// ������������ COM-�����������
void releaseObjects();
// ���������� �������
HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob* buffer);

// ������� �������, ����� �����
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// ���������� ��� �������� ����, ������������� ��������� ������� ����������
	HRESULT hr;

	// �������� ����
	hr = createWindow(L"DX11GraphicApp", L"GraphicApp", 800, 600, hInstance, nShowCmd);
	if (FAILED(hr)) {
		return hr;
	}

	//������������� DirectX �����������	
	hr = createDirect3DComponents(800, 600);
	if (FAILED(hr)) {
		return hr;
	}

	// �������� vertex buffer
	// �������� ������
	struct Vertex {
		D3DVECTOR position;
		D3DXCOLOR color;
	};

	// ������ ������
	Vertex* vertexArray = new Vertex[]{
		Vertex{D3DVECTOR{0.0f, 0.5f, 0.5f}, D3DXCOLOR{0.0f, 0.0f, 0.0f, 0.0f}},
		Vertex{D3DVECTOR{0.5f, -0.5f, 0.5f}, D3DXCOLOR{0.0f, 0.0f, 0.0f, 0.0f}},
		Vertex{D3DVECTOR{-0.5f, -0.5f, 0.5f}, D3DXCOLOR{0.0f, 0.0f, 0.0f, 0.0f}}
	};

	// �������� vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	// ������ ������
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
	// ������� ������� CPU � GPU � ������ 
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	// � ����� ������ ��������� ����������� �����
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	// ������� ������� CPU � ������
	vertexBufferDesc.CPUAccessFlags = 0;
	// �������������� ��������� ������
	vertexBufferDesc.MiscFlags = 0;
	// ������ ������� �������� ������, ���� ������ ����� �������� structured buffer. (�� ���������)
	vertexBufferDesc.StructureByteStride = 0;

	// ���������� ��� ������������� ����������
	D3D11_SUBRESOURCE_DATA vertexBufferSubresourceInitData;
	// ��������� �� ���������������� ������
	vertexBufferSubresourceInitData.pSysMem = vertexArray;
	// ��������
	vertexBufferSubresourceInitData.SysMemPitch = 0;
	// ��������
	vertexBufferSubresourceInitData.SysMemSlicePitch = 0;

	// �������� vertex buffer
	hr = g_pd3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubresourceInitData, &pVertexBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	// �������� vertex buffer � IA stage
	// ��������� ���� ����������� ���������� ������, ������� ����� �������������� ������
	UINT stride[] = {sizeof(Vertex)};
	// �������� �� ������� �������� ����������� ���������� ������, ������� ����� �����������
	UINT offset[] = {0};
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pVertexBuffer, stride, offset);
	
	// �������� ����� ��������� �������� �� ���������� ������
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���������� ��������
	// ������ ��������
	LPCSTR shadersModel;
	// ���������� ���������� �������
	hr = CompileShader(L"TriangleVertexShader", "VS", "vs_5_0", VS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}
	// ���������� ����������� �������
	hr = CompileShader(L"TrianglePixelShader", "PS", "ps_5_0", PS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}

	// �������� Input-Layout Object
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// ���������� ��������� � Input-Layout Object
	UINT numInputLayoutObject = sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// �������� Input-Layout Object
	g_pd3dDevice->CreateInputLayout(layout, numInputLayoutObject, ); 

	// ������������ ������, ������� �������� ������
	delete [] vertexArray;
	vertexArray = NULL;

	MSG msg;// ���������, ����������� ���������
	ZeroMemory(&msg, sizeof(MSG));

	while (msg.message != WM_QUIT) { // ����, ����� �� ����� �� ���������� ������ ������������ �������
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		drawScene();
	}
	// ��������� ������ ����������
	releaseObjects();

	return 0;
};

// �������-���������� ���������, ����������� ����
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	// ���������, ���������� ����������� ���������� ��� ��������� � ��������� ����� ����
	PAINTSTRUCT ps;
	// ��������� �� ����������(��� ������ ����������) Device Context
	HDC hdc;

	switch (message) {

	case(WM_PAINT):
		// ���������� ��������� ps, � ������� Update Region
		hdc = BeginPaint(hWnd, &ps);

		// ������� Update Region, � ������������ Device Context
		EndPaint(hWnd, &ps);
		break;

	case(WM_DESTROY):
		//�������� ��������� WM_QUIT
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
};

HRESULT createDirect3DComponents(UINT widthParam, UINT heightParam) {
	HRESULT hr;

	//������������� DirectX �����������	

	// ���������, ����������� back buffer SwapChain-a
	DXGI_MODE_DESC backBuffer;
	ZeroMemory(&backBuffer, sizeof(DXGI_MODE_DESC));
	// ������
	backBuffer.Width = widthParam;
	// ������
	backBuffer.Height = heightParam;
	// ������� ���������� ��������
	backBuffer.RefreshRate.Numerator = 60;
	backBuffer.RefreshRate.Denominator = 1;
	// ������ ��������
	backBuffer.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// ������� �������� �����������(Surface)
	backBuffer.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	// ����, ������������ ��, ��� ����� ������������� ����������� ��� ������������ ���������� ��������� ��������
	backBuffer.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// ���������, ����������� ���� ������ (Swap Chain)
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(DXGI_SWAP_CHAIN_DESC));
	// �������� backBuffer-a, ������� ������ � ������ Swap Chain-a
	sd.BufferDesc = backBuffer;
	// �������� ���������������(�� ���������)
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	// ���� ������������� �������
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// ���������� ������� � Swap Chain-e (��� ��� ���������� �������� � ������� ������, �� ��� ������� ����������� ���������� ������ ���-�� ������� ��� ����� ��������������� Front Buffer-a
	//(Front Buffer-�� �������� ������� ����)), �� ��� full-screen mode ���������� ���������� ������ 2 ������ ��� ������� �����������
	sd.BufferCount = 2;
	// ���������� ���� ����������
	sd.OutputWindow = g_hWnd;
	// ����� ���������� � windowed mode ��� full-screen mode
	sd.Windowed = TRUE;
	// ��� ������ ������ ������� ����� ����, ��� front buffer ���� back buffer-��
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// �������������� ��������� Swap Chain-�
	sd.Flags = NULL;

	// ���� �������� Device, Device Contex, Swap Chain, Render Target View, View Port

	// ��������� feature level, ������� �������������� �����������, � ��������� ������������ ��� ��������
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0 };

	// ���������� ��������� � ������� featureLevels
	UINT numFeatureLevels = sizeof(featureLevels) / sizeof(D3D_FEATURE_LEVEL);

	// ���� � ���� ������������ ������ hardware type
	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
		D3D_DRIVER_TYPE_SOFTWARE
	};

	// ���������� ��������� � ������� dreiverTypes
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(D3D_DRIVER_TYPE);

	// ���������� �������� Direct3D Device, Device Context, Swap Chain, View Port

	// ��������� ������ CreateDeviceAndSwapChain
	HRESULT createDeviceDeviceContextSwapChainResult(S_OK);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex) {
		createDeviceDeviceContextSwapChainResult = D3D11CreateDeviceAndSwapChain(NULL, driverTypes[driverTypeIndex], NULL, NULL, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(createDeviceDeviceContextSwapChainResult)) {
			goto createDeviceDeviceContextSwapChainLoopExit;
		}
	}
	// ���������� ����� �� �����
	return createDeviceDeviceContextSwapChainResult;
	// �������� ����� �� �����
createDeviceDeviceContextSwapChainLoopExit:

	// ��������� ������� � back buffer
	// ��������� �� back buffer
	ID3D11Texture2D* pBackBuffer(NULL); 
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	// �������� Render Target View
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);

	// ������������ ���������� back buffer, back buffer ������ �� ����� 
	pBackBuffer->Release();
	pBackBuffer = NULL; // ���� �������� �� �����������, � ������ ������

	// �������� View Port, ������� ����������� RTV, ������� � ����� ������������ �� �������. ����� view port ��������� RTV �� ���������� ��������� � �������� ����������� 
	D3D11_VIEWPORT viewPort;
	ZeroMemory(&viewPort, sizeof(D3D11_VIEWPORT));

	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = widthParam;
	viewPort.Height = heightParam;
	// ������� ����������� ��������, ������� ����� ������������ � view port
	viewPort.MinDepth = 0;
	viewPort.MaxDepth = 1;

	// ���������� view port � ����������� ����������
	g_pImmediateContext->RSSetViewports(1, &viewPort);

	// �������� RTV � Output-Merger Stage
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

	return S_OK;
};

HRESULT createWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam) {
	// ���������, ����������� ����� ����
	WNDCLASSEX wndClass;
	ZeroMemory(&wndClass, sizeof(WNDCLASSEX));
	// ������ ��������� wndClass
	wndClass.cbSize = sizeof(WNDCLASSEX);
	// ����� ������ ����
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	// ��������� �� �������, �������������� ���������, ����������� ����
	wndClass.lpfnWndProc = (WNDPROC)WndProc;
	// ���������� �������������� ������ ��� ��������� wndClass
	wndClass.cbClsExtra = NULL;
	// ���������� �������������� ������ ��� ���������� ����
	wndClass.cbWndExtra = NULL;
	// ���������� ����������
	wndClass.hInstance = hInstanceParam;
	// ���������� ������ ����
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	// ���������� ������� ����
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	// ���������� �����
	wndClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	// ��������� �� ������-��� ���� ������ 
	wndClass.lpszMenuName = NULL;
	// ��� ������ ����
	wndClass.lpszClassName = wndClassNameParam;
	// ���������� ������ ����, ������� ������������ �� ������ �����
	wndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// ����������� ������ ����
	if (RegisterClassEx(&wndClass) == 0) {
		return E_FAIL;
	}
	// �������� ����
	g_hWnd = CreateWindowEx(NULL, wndClassNameParam, wndNameParam, WS_OVERLAPPEDWINDOW, 0, 0, widthParam, heightParam, NULL, NULL, hInstanceParam, NULL);
	if (g_hWnd == 0) {
		return E_FAIL;
	}
	// ����� ���� �� ������� 
	ShowWindow(g_hWnd, nShowCmdParam);

	return S_OK;
};

void updateScene() {

};

void drawScene() {
	// ���� �������
	const FLOAT backgroundColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

	// ������������� ���� ���� �������� ����������� RTV � ������� ��������
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, backgroundColor);

	// ����� �� ������� ����������� Back Buffer
	g_pSwapChain->Present(0, 0);
};

HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob* buffer) {
	HRESULT hr;
	ID3DBlob* errorsBuffer = NULL;

	hr = D3DCompileFromFile(srcName, NULL, NULL, entryPoint, target, NULL, NULL, &buffer, &errorsBuffer);
	// ����� ������ ����������, ���� ��� ����
	if (FAILED(hr)) {
		if (errorsBuffer != NULL) {
			OutputDebugStringA((char*)errorsBuffer->GetBufferPointer());
			errorsBuffer->Release();
		}
		if (buffer != NULL) {
			buffer->Release();
		}
	}
		return hr;
};

void releaseObjects() {
	if (g_pRenderTargetView != NULL) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = NULL;
	};
	if (g_pSwapChain != NULL) {
		g_pSwapChain->Release();
		g_pSwapChain = NULL;
	};
	if (g_pImmediateContext != NULL) {
		g_pImmediateContext->Release();
		g_pImmediateContext = NULL;
	};
	if (g_pd3dDevice != NULL) {
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
};








