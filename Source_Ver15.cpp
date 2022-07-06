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
#include <xnamath.h>
#include <d3dcompiler.h>
#include <iostream>
#include <cmath>

// �������� ��������

// �������� ������
struct Vertex {
	XMFLOAT4 position;
	XMFLOAT4 color;
};

// ������ ��������
struct ShaderModelDesc {
	LPCSTR vertexShaderModel;
	LPCSTR pixelShaderModel;
};

// �������
struct MatricesBuffer {
	XMMATRIX mWorld;              // ������� ����
	XMMATRIX mView;        // ������� ����
	XMMATRIX mProjection;  // ������� ��������
};

// ��������� ����
struct AngleConstantBuffer {
	float angle0;
	float angle1;
	float angle2;
	float angle3;
};

// ��������� ������
struct Camera {
	XMVECTOR z;
	XMVECTOR y;
};

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
ID3D11VertexShader* g_pVertexShader = NULL; // ��������� �� ��������� vertex shader
ID3D11PixelShader* g_pPixelShader = NULL; // ��������� �� ��������� pixel shader
ID3DBlob* VS_Buffer = NULL; // ��������� �� ��������� ������ � ���������������� ��������� �������� 
ID3DBlob* PS_Buffer = NULL; // ��������� �� ��������� ������ � ���������������� ���������� �������� 
ID3D11Buffer* pVertexBuffer = NULL; // ��������� �� ����� ������
ID3D11Buffer* pConstantBuffer = NULL; // ������������ �����
ID3D11Buffer* pIndexBuffer = NULL; // ����� ��������
ID3D11Buffer* pAngleBuffer = NULL; // ����� ���� 
ID3D11Buffer* constantBufferArray[] = {NULL, NULL};
AngleConstantBuffer angleCBufferData = { 0.0f, 0.0f, 0.0f, 0.0f }; // ���� ��������
ID3D11ShaderResourceView* pAngleBufferVSResource = NULL; // ������ ���������� �������, � ����� ��������� ����
ID3D11Texture2D* depthStencilTexture = NULL; // �������� depth ������
ID3D11DepthStencilView* g_pDepthStencilView = NULL; // ������� depth ������
ID3D11DepthStencilState* pDSState = NULL; // ��������� depth-stencil �����
ID3D11RasterizerState* pRasterizerState = NULL; // ��������� ������������� 

//��������������� ���������� �������

// ������� ����
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); 
// �������� DirectX �����������
HRESULT CreateDirect3DComponents(UINT widthParam, UINT heightParam);
// �������� ����
HRESULT MyCreateWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam);
// �������� ������ ������, ���������� ��������, ���������� �������� � ����������
HRESULT InitGeometry(Vertex* vertexArray, LPCWSTR vertexShaderName, LPCWSTR pixelShaderName, LPCSTR vsShaderEntryPoint, LPCSTR psShaderEntryPoint);
// ���������� �����
void UpdateScene();
// ��������� �����
void DrawScene();
// ������������ COM-�����������
void ReleaseObjects();
// ���������� �������
HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob** buffer);
// �������� ������������ ������ ������, ������������ ������ ����, ������ ������
HRESULT InitMatrices(WORD* indices);   
// ���������� ������
void SetMatrices();
// ���������� ������������ ������� (0 < angle < PI/2). angle = fov/2
void SetProjectionMatrix(MatricesBuffer* pMatricesBuffer, FLOAT angleHoriz, FLOAT angleVert, BOOL saveProportionsFlag);
// ���������� ��������� ��������, ��� ������ � ����. ��������� ����������� � ������������ � ����, � ������� ������ ��������� �������. 
void SaveProportions(MatricesBuffer* pMatricesBuffer, HWND hWnd);
// ���������� �������
FLOAT MaxElement(FLOAT arg0, FLOAT arg1);
// �������� �������� �������
HRESULT InvertMatrix(XMVECTOR zAxis, XMVECTOR yAxis, XMMATRIX* invertibleMatrix);
// �������� ������� �������� � ������ ������� ���������
HRESULT NewCoordinateSystemMatrix(XMVECTOR point, XMVECTOR zAxis, XMVECTOR yAxis, XMMATRIX* invertibleMatrix);
// �������� ������� ����
void SetWorldMatrix(XMVECTOR point, XMVECTOR scale, XMMATRIX* worldMatrix);
// ��������� ������� ������ ������
void InvertIndices(WORD* indicesArray, int size);
// ������� ������
Camera CameraRotation();
// ����� �������������� ������� � ���������
XMVECTOR FindOrthogonalVector(XMVECTOR vector);

// ������� �������, ����� �����
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// ���������� ��� �������� ����, ������������� ��������� ������� ����������
	HRESULT hr;

	// �������� ����
	hr = MyCreateWindow(L"DX11GraphicApp", L"GraphicApp", 1920, 800, hInstance, nShowCmd);
	if (FAILED(hr)) {
		return hr;
	}

	//������������� DirectX �����������	
	hr = CreateDirect3DComponents(1920, 800);
	if (FAILED(hr)) {
		return hr;
	}

	// ������ ������ (��������)
	Vertex* vertexArray = new Vertex[]{
		Vertex{XMFLOAT4{0.0f, 5.0f, 2.5f, 10.0f}, XMFLOAT4{1.0f, 0.0f, 0.0f, 1.0f}}, // a 0
		Vertex{XMFLOAT4{2.5f, 0.0f, 2.5f, 10.0f}, XMFLOAT4{1.0f, 1.0f, 0.0f, 1.0f}}, //b 1
		Vertex{XMFLOAT4{0.0f, 0.0f, 7.5f, 10.0f}, XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f}}, //c 2
		Vertex{XMFLOAT4{-2.5f, 0.0f, 2.5f, 10.0f}, XMFLOAT4{1.0f, 0.0f, 1.0f, 1.0f}} //d 3
	};

	// �������� ������ ������, ���������� ��������, ���������� �������� � ������ ������ � ����������
	hr = InitGeometry(vertexArray, L"TriangleVertexShader.hlsl", L"TrianglePixelShader.hlsl", "main", "main");
	if (FAILED(hr)) {
		return hr;
	}

	// ������������ ������, ������� �������� ������
	delete[] vertexArray;
	vertexArray = NULL;

	// ������� ������
	// ����� �� ������� �������� ������������ ������� � �����������, ������� ����� ��������
	WORD indices[] = { 
		0, 1, 2, //ab� ������� ����� (����� �� ������� �������)
		0, 2, 3, //a�d ������� ����� (����� �� ������� �������)
		3, 2, 1, //dcb ��������� ����� (����� ������ ������� �������)
		0, 3, 1 //adb  ��������� ����� (����� ������ ������� �������)
	};

	InvertIndices(indices, 12);

	// �������� ������������ ������ ������, ������������ ������ ����, ������ ������
	hr = InitMatrices(indices);
	if (FAILED(hr)) {
		return hr;
	}

	// ������������� ������
	MatricesBuffer matricesWVP;
	ZeroMemory(&matricesWVP, sizeof(MatricesBuffer));

	float vecAngle = -XM_PIDIV4;
	XMVECTOR eye = XMVectorSet(1.0f, 1.5f, 0.0f, 1.0f); // ������ �������
	// ����� ������������ XMScalarSinExt, XMScalarCosExt ������ XMScalarSin, XMScalarCos, ����� �������� ������� ���������� �����
	//XMVECTOR zAxis = XMVectorSet(0.0f, XMScalarSinEst(vecAngle), XMScalarCosEst(vecAngle), 1.0f); // ���� �������. 
	//XMVECTOR yAxis = XMVectorSet(0.0f, XMScalarCosEst(XM_PIDIV4), XMScalarSinEst(XM_PIDIV4), 1.0f); // ������� � ����, ���� �������

	XMVECTOR zAxis = XMVectorSet(0.0f, 0.0f, -1.0f, 1.0f); // ���� �������. 
	XMVECTOR yAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f); // ������� � ����, ���� �������

	NewCoordinateSystemMatrix(eye, zAxis, yAxis, &matricesWVP.mView);
	//matricesWVP.mView = XMMatrixLookToLH(eye, zAxis, yAxis);
	//matricesWVP.mView.r[3] = _mm_mul_ps(eye, XMVectorSet(-1.0f, -1.0f, -1.0f, 1.0f)); // ���������� �������� ��������� ������ � ������� ����, ��� ��� XMMatrixLookToLH ���-�� ������� ������� �������� ��������� ������
	matricesWVP.mView = XMMatrixTranspose(matricesWVP.mView); 

	// ������������� ������� ��������
	SetProjectionMatrix(&matricesWVP, XM_PI / 5.0f, XM_PI / 25.0f, true);

	MSG msg;// ���������, ����������� ���������
	ZeroMemory(&msg, sizeof(MSG));

	while (msg.message != WM_QUIT) { // ����, ����� �� ����� �� ���������� ������ ������������ �������
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		UpdateScene();
		DrawScene();
	}
	// ��������� ������ ����������
	ReleaseObjects();
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

HRESULT CreateDirect3DComponents(UINT widthParam, UINT heightParam) {
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
		createDeviceDeviceContextSwapChainResult = D3D11CreateDeviceAndSwapChain(NULL, driverTypes[driverTypeIndex], NULL, D3D11_CREATE_DEVICE_DEBUG, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
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

	// �������� depth stencil ������
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = widthParam;
	depthStencilDesc.Height = heightParam;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	g_pd3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilTexture);

	// �������� ����, ��� ����� ���������� z-test � stencil-test
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = TRUE;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;
	dsDesc.FrontFace = { D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_COMPARISON_ALWAYS };
	dsDesc.BackFace = { D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_COMPARISON_NEVER };

	// �������� ��������� depth-stencil �����
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSState);

	// ���������� �������� depth-stencil ����� � OM stage
	g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);

	// �������� depth-stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Flags = NULL;
	descDSV.Texture2D.MipSlice = 0;

	// �������� depth-stencil view
	g_pd3dDevice->CreateDepthStencilView(depthStencilTexture, &descDSV, &g_pDepthStencilView);

	// �������� RTV � DSV � Output-Merger Stage
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// �������� ��������� �������������
	D3D11_RASTERIZER_DESC rasterizerState;
	ZeroMemory(&rasterizerState, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.CullMode = D3D11_CULL_BACK;

	// �������� ��������� �������������
	g_pd3dDevice->CreateRasterizerState(&rasterizerState, &pRasterizerState);

	// ���������� ��������� ������������� � ������� ������������
	g_pImmediateContext->RSSetState(pRasterizerState);

	return S_OK;
};

HRESULT MyCreateWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam) {
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

void UpdateScene() {
	if (angleCBufferData.angle0 >= XM_2PI) {
		angleCBufferData.angle0 = angleCBufferData.angle0 - XM_2PI;
	}
	angleCBufferData.angle0 += 0.00003f;

	/*
	D3D11_MAPPED_SUBRESOURCE angleCBufferUpdateData;
	g_pImmediateContext->Map(pAngleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &angleCBufferUpdateData);
	memcpy(angleCBufferUpdateData.pData, &angleCBufferData, sizeof(float));
	g_pImmediateContext->Unmap(pAngleBuffer, 0);
	*/

	// ���������� �������� ����
	g_pImmediateContext->UpdateSubresource(constantBufferArray[1], 0, NULL, &angleCBufferData, 0, 0); 
	
};

void DrawScene() {
	// ���� �������
	const FLOAT backgroundColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

	// ������������� ���� ���� �������� ����������� RTV � ������� ��������
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, backgroundColor);

	// �������� depth-stencil ������
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	// ��������� ���������� 
	g_pImmediateContext->DrawIndexed(12, 0, 0);

	// ����� �� ������� ����������� Back Buffer
	g_pSwapChain->Present(0, 0);
};

HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob** buffer) {
	HRESULT hr;
	ID3DBlob* errorsBuffer = NULL;
	HANDLE errorsFileHandle = NULL;
	DWORD bytesWritten = NULL;

	hr = D3DCompileFromFile(srcName, NULL, NULL, entryPoint, target, NULL, NULL, buffer, &errorsBuffer);
	// ����� ������ ����������, ���� ��� ����
	if (FAILED(hr)) {
		if (errorsBuffer != NULL) {
			// ������� ��� ��������� ����, ��� ����� ��������� ����� � �������� ���������� �������
			errorsFileHandle = CreateFile(L"shaderErrors.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			// ���������� ������ ���������� � ����
			WriteFile(errorsFileHandle, errorsBuffer->GetBufferPointer(), errorsBuffer->GetBufferSize(), &bytesWritten, NULL);
			CloseHandle(errorsFileHandle);
			errorsBuffer->Release();
		}
		if ((*buffer) != NULL) {
			(*buffer)->Release();
			*buffer = NULL;
		}
	}
	return hr;
};

HRESULT InitGeometry(Vertex* vertexArray, LPCWSTR vertexShaderName, LPCWSTR pixelShaderName, LPCSTR vsShaderEntryPoint, LPCSTR psShaderEntryPoint) {
	HRESULT hr;

	// �������� vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	// ������ ������
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 4;
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
	UINT stride[] = { sizeof(Vertex) };
	// �������� �� ������� �������� ����������� ���������� ������, ������� ����� �����������
	UINT offset[] = { 0 };
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pVertexBuffer, stride, offset);

	// �������� ����� ��������� �������� �� ���������� ������
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// ���������� ��������
	ShaderModelDesc shadersModel;
	if (g_featureLevel >= D3D_FEATURE_LEVEL_11_0) {
		shadersModel = { "vs_5_0", "ps_5_0" };
	}
	else
		shadersModel = { "vs_4_0", "ps_4_0" };

	// ���������� ���������� �������
	hr = CompileShader(vertexShaderName, vsShaderEntryPoint, shadersModel.vertexShaderModel, &VS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}
	// ���������� ����������� �������
	hr = CompileShader(pixelShaderName, psShaderEntryPoint, shadersModel.pixelShaderModel, &PS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}

	// �������� ������� ���������� �������
	hr = g_pd3dDevice->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &g_pVertexShader);
	// �������� ������� ����������� �������
	hr = g_pd3dDevice->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &g_pPixelShader);
	
	// �������� ���������� ������� � ���������
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, NULL);
	// �������� ����������� ������� � ���������
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, NULL);
	
	// �������� Input-Layout Object
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	
	// ���������� ��������� � Input-Layout Object
	UINT numInputLayoutObject = sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// �������� Input-Layout Object
	hr = g_pd3dDevice->CreateInputLayout(layout, numInputLayoutObject, VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), &g_pInputLayoutObject);

	// ���������� Input-layout object � ����������
	g_pImmediateContext->IASetInputLayout(g_pInputLayoutObject);

	return S_OK;
}

HRESULT InitMatrices(WORD* indices) {
	HRESULT hr;

	// �������� ������������ ������ ������
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.ByteWidth = sizeof(MatricesBuffer);
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	// �������� ������������ ������ ������
	hr = g_pd3dDevice->CreateBuffer(&constantBufferDesc, NULL, &constantBufferArray[0]);
	if (FAILED(hr)) {
		return hr;
	}

	// �������� ������ ����
	D3D11_BUFFER_DESC angleBufferDesc;
	angleBufferDesc.ByteWidth = sizeof(AngleConstantBuffer);
	angleBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	angleBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	angleBufferDesc.CPUAccessFlags = 0;
	angleBufferDesc.MiscFlags = 0;
	angleBufferDesc.StructureByteStride = 0;

	// �������� ���������������� ���������� ������ ����
	D3D11_SUBRESOURCE_DATA angleBufferInitData;
	angleBufferInitData.pSysMem = &angleCBufferData;
	angleBufferInitData.SysMemPitch = 0;
	angleBufferInitData.SysMemSlicePitch = 0;

	// �������� ������ ����
	hr = g_pd3dDevice->CreateBuffer(&angleBufferDesc, &angleBufferInitData, &constantBufferArray[1]);
	if (FAILED(hr)) {
		return hr;
	}

	// ���������� ������������ ������ � �������� ������
	g_pImmediateContext->VSSetConstantBuffers(0, 2, constantBufferArray);
	/*
	//�������� ������� ���������� �������
	D3D11_SHADER_RESOURCE_VIEW_DESC angleBufferVSResorceDesc;
	angleBufferVSResorceDesc.Format = DXGI_FORMAT_R32_FLOAT;
	angleBufferVSResorceDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	angleBufferVSResorceDesc.Buffer.FirstElement = 0;
	angleBufferVSResorceDesc.Buffer.NumElements = 1;

	//�������� ������� ���������� �������
	hr = g_pd3dDevice->CreateShaderResourceView(pAngleBuffer, &angleBufferVSResorceDesc, &pAngleBufferVSResource);
	if (FAILED(hr)) {
		return hr;
	}

	// �������� ������� � ����� � ������� ������
	g_pImmediateContext->VSSetShaderResources(0, 1, &pAngleBufferVSResource);
	*/
	// �������� ������ ������
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.ByteWidth = sizeof(WORD) * 4 * 3;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// �������� ������ ������
	D3D11_SUBRESOURCE_DATA indexBufferInitData;
	indexBufferInitData.pSysMem = indices;
	indexBufferInitData.SysMemPitch = 0;
	indexBufferInitData.SysMemSlicePitch = 0;

	//�������� ������ ������
	hr = g_pd3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferInitData, &pIndexBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	//�������� ������ ������ � ���������
	g_pImmediateContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	return S_OK;
};

void SetProjectionMatrix(MatricesBuffer* pMatricesBuffer, FLOAT angleHoriz, FLOAT angleVert, BOOL saveProportionsFlag) {
	// (0 < angle < PI/2) => (0 < tg(angle) < +inf) => (0 < newCoeff < 1)

	FLOAT sinAngle;
	FLOAT cosAngle;
	FLOAT tangentAngle;
	FLOAT newCoeff;

	if (saveProportionsFlag == false) {
		XMScalarSinCos(&sinAngle, &cosAngle, angleHoriz);
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection._11 = newCoeff;

		XMScalarSinCos(&sinAngle, &cosAngle, angleVert);
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection._22 = newCoeff;

		pMatricesBuffer->mProjection._33 = 1.0f;
		pMatricesBuffer->mProjection._44 = 1.0f;

		g_pImmediateContext->UpdateSubresource(constantBufferArray[0], 0, 0, pMatricesBuffer, 0, 0);
	}
	else {
		XMScalarSinCos(&sinAngle, &cosAngle, MaxElement(angleHoriz, angleVert));
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection._11 = newCoeff;
		pMatricesBuffer->mProjection._22 = newCoeff;
		pMatricesBuffer->mProjection._33 = 0.25f * newCoeff;
		pMatricesBuffer->mProjection._44 = 1.0f;

		SaveProportions(pMatricesBuffer, g_hWnd);
	}
};

void SaveProportions(MatricesBuffer* pMatricesBuffer, HWND hWnd) {
	RECT rectangle;
	FLOAT windowWidth;
	FLOAT windowHeight;
	FLOAT windowCoeff;

	GetClientRect(hWnd, &rectangle);
	windowWidth = rectangle.right - rectangle.left;
	windowHeight = rectangle.bottom - rectangle.top;

	windowCoeff = windowHeight / windowWidth;

	if (windowCoeff >= 1.0f) {
		pMatricesBuffer->mProjection._22 *=  1 / windowCoeff;
	}
	else
	{
		pMatricesBuffer->mProjection._11 *= windowCoeff;
	}

	g_pImmediateContext->UpdateSubresource(constantBufferArray[0], 0, 0, pMatricesBuffer, 0, 0);
};

FLOAT MaxElement(FLOAT arg0, FLOAT arg1) {
	if (arg0 >= arg1) {
		return arg0;
	}
	return arg1;
};

HRESULT InvertMatrix(XMVECTOR zAxis, XMVECTOR yAxis, XMMATRIX* invertibleMatrix) {
	// �������� �������� �� ��������������� 
	XMFLOAT4 dotProduct;
	XMStoreFloat4(&dotProduct, XMVector3Dot(zAxis, yAxis));
	if (dotProduct.x != 0.0f) {
		return E_FAIL;
	}
	
	// ���������� ����� ��� X
	XMVECTOR xAxis = XMVector3Cross(yAxis, zAxis); // ������ ������ ��������
	// ������� ������� ����������, ������ ����� ������ �������� �������

	// ������� �������� �������� �������, ������� �������
	XMVECTOR axisArray[] = { xAxis, yAxis, zAxis };

	// ������� ����� � �������� �������, ������� ��������� � ���������� ������ �������� �������
	int invertibleMatrixRowsOrder[3];

	// ��������� �������
	XMMATRIX invertibleMatrixOutput = {
	 XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
	 XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
	 XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
	 XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
	};

	//����� �������� ������� ������� ������
	for (size_t axisIndex = 0; axisIndex <= 1; ++axisIndex) { // ����� ���� �� ������� ������� �������
		for (size_t i = 0; i <= 2; ++i) { // ����� �� ����������� ������ ������� ������
			float axisElement = XMVectorGetByIndex(axisArray[axisIndex], i);
			if (axisElement != 0) {
				axisArray[axisIndex] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
				invertibleMatrixOutput.r[axisIndex] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
				invertibleMatrixRowsOrder[axisIndex] = i;

				for (size_t nextAxisIndex = axisIndex + 1; nextAxisIndex <= 2; ++nextAxisIndex) { // ����������� ������ � ������ �������
					float nextAxisElement = XMVectorGetByIndex(axisArray[nextAxisIndex], i);
					XMVECTOR mulVector = axisArray[axisIndex] * XMVectorSet(-nextAxisElement, -nextAxisElement, -nextAxisElement, -nextAxisElement); // ������ �������� ��������� axisArray[axisIndex] �� -nextAxisElement ���������� �������� �� ������, ��� ��� � ���� ������ ��������� ������� �����������
					XMVECTOR invertMatrixMulVector = invertibleMatrixOutput.r[axisIndex] * XMVectorSet(-nextAxisElement, -nextAxisElement, -nextAxisElement, -nextAxisElement);

					axisArray[nextAxisIndex] += mulVector;
					invertibleMatrixOutput.r[nextAxisIndex] += invertMatrixMulVector;
				}
				break;
			}
		}
	};
	for (size_t i = 0; i <= 2; ++i) { // ����� ��������� ������ ������� �������
		float axisElement = XMVectorGetByIndex(axisArray[2], i);
		if (axisElement != 0) {
			axisArray[2] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
			invertibleMatrixOutput.r[2] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
			invertibleMatrixRowsOrder[2] = i;

			break;
		}
	};
	for (size_t axisIndex = 2; axisIndex >= 1; --axisIndex) { // ����� ����� �� ������� ������� �������
		for (size_t i = 0; i <= 2; ++i) { // ����� �� ����������� ������ ������� ������
			float axisElement = XMVectorGetByIndex(axisArray[axisIndex], i);
			if (axisElement != 0) {
				for (size_t nextAxisIndex = axisIndex - 1; nextAxisIndex < UINT_MAX; --nextAxisIndex) { // ����������� ������ � ������ �������
					float nextAxisElement = XMVectorGetByIndex(axisArray[nextAxisIndex], i);
					XMVECTOR mulVector = axisArray[axisIndex] * XMVectorSet(-nextAxisElement, -nextAxisElement, -nextAxisElement, -nextAxisElement);
					XMVECTOR invertMatrixMulVector = invertibleMatrixOutput.r[axisIndex] * XMVectorSet(-nextAxisElement, -nextAxisElement, -nextAxisElement, -nextAxisElement);

					axisArray[nextAxisIndex] += mulVector;
					invertibleMatrixOutput.r[nextAxisIndex] += invertMatrixMulVector;
				}
				break;
			}
		}
	};

	//�������������� �������� ������� � ����������� ������� �����
	for (int i = 0; i <= 2; ++i) {
		invertibleMatrix->r[invertibleMatrixRowsOrder[i]] = invertibleMatrixOutput.r[i];
	};
	invertibleMatrix->r[3] = invertibleMatrixOutput.r[3];

	return S_OK;
};

HRESULT NewCoordinateSystemMatrix(XMVECTOR point, XMVECTOR zAxis, XMVECTOR yAxis, XMMATRIX* invertibleMatrix) {
	if (FAILED(InvertMatrix(zAxis, yAxis, invertibleMatrix))) {
		return E_FAIL;
	}
	// ��� ��������� ������� �� ������� �������� � ����� ������� ��������� �������� ������ ��������� ����� ������� ���������
	point *= -1.0f;
	invertibleMatrix->r[3] = XMVectorSetByIndex(point, 1.0f, 3);

	return S_OK;
};
void SetWorldMatrix(XMVECTOR point, XMVECTOR scale, XMMATRIX* worldMatrix) {
	*worldMatrix = XMMatrixTranslationFromVector(point) * XMMatrixScalingFromVector(scale);
};

void InvertIndices(WORD* indicesArray, int size) {
	WORD triangleEnd;

	for (int i = 2; i < size; i = i + 3) {
		triangleEnd = indicesArray[i];

		indicesArray[i] = indicesArray[i-2];
		indicesArray[i - 2] = triangleEnd;
	}
};

XMVECTOR FindOrthogonalVector(XMVECTOR vector) { // ��������� ������������ ������������� �������� ����� ����, �� ������������� ����� ����� � �������� ����� �������������� �������
	XMFLOAT4 orthogonalVector;
	int zeroElementOrNot[3];

	for (int i = 0; i < 3; ++i) {
		if (XMVectorGetByIndex(vector, i) != 0.0f) {
			zeroElementOrNot[i] = 1;
		}
		zeroElementOrNot[i] = 0;
	}

	if ((zeroElementOrNot[1] != 0) || (zeroElementOrNot[2] != 0)) {
		orthogonalVector.x = 1.0f;

		if ((zeroElementOrNot[1] != 0) && (zeroElementOrNot[2] == 0)) {
			orthogonalVector.y = -XMVectorGetByIndex(vector, 0) / XMVectorGetByIndex(vector, 1);

			return XMVectorSet(orthogonalVector.x, orthogonalVector.y, 0.0f, 0.0f);
		}
		else
		{
			orthogonalVector.z = -XMVectorGetByIndex(vector, 0) / XMVectorGetByIndex(vector, 2);

			return XMVectorSet(orthogonalVector.x, 0.0f, orthogonalVector.z, 0.0f);
		}
	}
	else
	{
		return XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	}
};

XMVECTOR FindOrthogonalNormalizedVector(XMVECTOR vector) {
	XMFLOAT4 orthogonalVector;

	for (int i = 0; i < 3; ++i) {
		float coordinate = XMVectorGetByIndex(vector, i);
		if ((coordinate != 0.0f)) {
			// ���� ���������� x �� ����
			if (i == 0) {
				float zOfFirstVector = XMVectorGetByIndex(vector, 2);
				orthogonalVector.z = coordinate / (sqrt(coordinate * coordinate + zOfFirstVector * zOfFirstVector));
				orthogonalVector.x = (-orthogonalVector.z * zOfFirstVector) / coordinate;

				return XMVectorSet(orthogonalVector.x, 0.0f, orthogonalVector.z, 0.0f);
			}

			// ���� ���������� y �� ����
			if (i == 1) {
				float zOfFirstVector = XMVectorGetByIndex(vector, 2);
				orthogonalVector.z = coordinate / (sqrt(coordinate * coordinate + zOfFirstVector * zOfFirstVector));
				orthogonalVector.y = (-orthogonalVector.z * zOfFirstVector) / coordinate;

				return XMVectorSet(0.0f, orthogonalVector.y, orthogonalVector.z, 0.0f);
			}

			// ���� ���������� z �� ����
			if (i == 2) {
				float yOfFirstVector = XMVectorGetByIndex(vector, 1);
				orthogonalVector.y = coordinate / (sqrt(coordinate * coordinate + yOfFirstVector * yOfFirstVector));
				orthogonalVector.z = (-orthogonalVector.y * yOfFirstVector) / coordinate;

				return XMVectorSet(0.0f, orthogonalVector.y, orthogonalVector.z, 0.0f);
			}
		}
	}
	// ���� ��� ������� ������
	return XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
};

void RotationAroundAxis(XMVECTOR yAxis, XMVECTOR point, FLOAT angle, XMMATRIX* outputMatrix) {
	XMVECTOR zAxis = FindOrthogonalNormalizedVector(yAxis);
	XMMATRIX offsetMatrix = XMMatrixTranslationFromVector(_mm_mul_ps(point, XMVectorSet(-1.0f, -1.0f, -1.0f, 0.0f))); // ������� �������� �������, ������� ����� �������������� ������ ���, �� -point
	XMMATRIX backOffsetMatrix = XMMatrixTranslationFromVector(point); // ������� �������� ������������ �������, ������� ��� ��������� ������ ���, �� +point

	// ������� ��������� ������� �� ��������� � ������ ������ � ����������� � ����� ������, ��� �������� ��� �������� ���� Y
	XMMATRIX newCoordinates;
	NewCoordinateSystemMatrix(g_XMZero, zAxis, yAxis, &newCoordinates); // ������ xAxis, ������� ��������� � ���� ������� ����� ���������������, ��� ��� ������, ����������
																	 // � ���������� ���������� ������������ ��������������� � ������������� �������� ���� ����� ���������������  

	// ��� ��� �������� ��� �������� ���� Y, �� � �������� ����� ����������� ������ ��� Y
	XMMATRIX yAxisRotationMatrix = XMMatrixRotationY(angle);

	// ������� �������� � ������ �����������
	XMMATRIX transformationMatrix = {
		XMVector3Cross(yAxis, zAxis),
		yAxis,
		zAxis,
		g_XMIdentityR3 };

	// ����� ����������� ��� �������, ��� ��� ������������ ������ ������������, � �������� ���� �������. �� �������� � ������� �� ������ ������� ������ ��������� ������������ ������. 
	*outputMatrix = offsetMatrix * newCoordinates * yAxisRotationMatrix * transformationMatrix * backOffsetMatrix; 
}

void ReleaseObjects() {
	if (pIndexBuffer != NULL) {
		pIndexBuffer->Release();
		pIndexBuffer = NULL;
	}
	if (pAngleBufferVSResource != NULL) {
		pAngleBufferVSResource->Release();
		pAngleBufferVSResource = NULL;
	}
	if(pAngleBuffer != NULL) {
		pAngleBuffer->Release();
		pAngleBuffer = NULL;
	}
	for (int i = 0; i < 2; ++i) {
		if (constantBufferArray[i] != NULL) {
			constantBufferArray[i]->Release();
			constantBufferArray[i] = NULL;
		}
	}
	if (g_pInputLayoutObject != NULL) {
		g_pInputLayoutObject->Release();
		g_pInputLayoutObject = NULL;
	}
	if (g_pPixelShader != NULL) {
		g_pPixelShader->Release();
		g_pPixelShader = NULL;
	}
	if (g_pVertexShader != NULL) {
		g_pVertexShader->Release();
		g_pVertexShader = NULL;
	}
	if (PS_Buffer != NULL) {
		PS_Buffer->Release();
		PS_Buffer = NULL;
	}
	if (VS_Buffer != NULL) {
		VS_Buffer->Release();
		VS_Buffer = NULL;
	}
	if (pVertexBuffer != NULL) {
		pVertexBuffer->Release();
		pVertexBuffer = NULL;
	}
	if(pRasterizerState != NULL) {
		pRasterizerState->Release();
		pRasterizerState = NULL;
	}
	if (g_pDepthStencilView != NULL) {
		g_pDepthStencilView->Release();
		g_pDepthStencilView = NULL;
	}
	if (pDSState != NULL) {
		pDSState->Release();
		pDSState = NULL;
	}
	if (depthStencilTexture != NULL) {
		depthStencilTexture->Release();
		depthStencilTexture = NULL;
	}
	if (g_pRenderTargetView != NULL) {
		g_pRenderTargetView->Release();
		g_pRenderTargetView = NULL;
	}
	if (g_pSwapChain != NULL) {
		g_pSwapChain->Release();
		g_pSwapChain = NULL;
	}
	if (g_pImmediateContext != NULL) {
		g_pImmediateContext->Release();
		g_pImmediateContext = NULL;
	}
	if (g_pd3dDevice != NULL) {
		g_pd3dDevice->Release();
		g_pd3dDevice = NULL;
	}
};








