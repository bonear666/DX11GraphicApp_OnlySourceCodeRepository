#include <windows.h>
#include <d3d11.h>
#include <iostream>

// ���������� ����������

HINSTANCE               g_hInst = NULL; //��������� �� struct, ����������(handle) ������� ����������.
HWND                    g_hWnd = NULL; //��������� �� struct, ����������(handle) ���� ������� ����������.
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL; //������������� ���������. ����������, ������������ ��� ��������, ����������, ��� ����������� ����������.
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0; //������������� ���������. ����������, ������������ ����� ������ DirectX ������������, ��������������, ���������� ���������� ����� ������� DirectX ����� ������������.
ID3D11Device* g_pd3dDevice = NULL; //��������� �� struct(������ ���������� ID3D11Device). ID3D11Device ��� COM-���������, ������� ������� �������(��������, ���������� ������� � �.�.) ��� ������ �� �������.
ID3D11DeviceContext* g_pImmediateContext = NULL; //��������� �� struct(������ ���������� ID3D11DeviceContext). ID3D11DeviceContext ��� COM-���������, ������� ���������� ���������� ����������� ���������� �� �������.
IDXGISwapChain* g_pSwapChain = NULL; //��������� �� struct(������ ���������� IDXGISwapChain). IDXGISwapChain ��� COM-���������, ������� ������ � ���������� ������� ��������� ����������� ������������ ����� �� ������� �� �������.
ID3D11RenderTargetView* g_pRenderTargetView = NULL; //��������� �� struct(������ ���������� ID3D11RenderTargetView). ID3D11RenderTargetView ��� COM-���������, ������� ������ ������� back buffer-�. 

//��������������� ���������� �������

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // ������� ����.

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// �������� ����

	// ��� ������ ����
    CONST WCHAR* wndClassName(L"DX11GraphicApp");
	// ��� ����
	CONST WCHAR* wndName(L"GraphicApp");

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
	wndClass.hInstance = hInstance;
	// ���������� ������ ����
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	// ���������� ������� ����
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	// ���������� �����
	wndClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	// ��������� �� ������-��� ���� ������ 
	wndClass.lpszMenuName = NULL;
	// ��� ������ ����
	wndClass.lpszClassName = wndClassName;
	// ���������� ������ ����, ������� ������������ �� ������ �����
	wndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// ����������� ������ ����
	if (RegisterClassEx(&wndClass) == 0) {
		return E_FAIL;
	}
	// �������� ����
	g_hWnd = CreateWindowEx(NULL, wndClassName, wndName, WS_OVERLAPPEDWINDOW, 0, 0 ,800, 600, NULL, NULL, hInstance, NULL);
	if (g_hWnd == 0) {
		return E_FAIL;
	}
	// ����� ���� �� ������� 
	ShowWindow(g_hWnd, nShowCmd);

	//������������� DirectX �����������	

	// ���������, ����������� back buffer SwapChain-a
	DXGI_MODE_DESC backBuffer;
	ZeroMemory(&backBuffer, sizeof(DXGI_MODE_DESC));
	// ������
	backBuffer.Width = 800;
	// ������
	backBuffer.Height = 600;
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
	sd.SampleDesc.Count = 0;
	// ���� ������������� �������
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// ���������� ������� � Swap Chain-e (��� ��� ���������� �������� � ������� ������, �� ��� ������� ����������� ���������� ������ ���-�� ������� ��� ����� ��������������� Front Buffer-a
	//(Front Buffer-�� �������� ������� ����))
	sd.BufferCount = 1;
	// ���������� ���� ����������
	sd.OutputWindow = g_hWnd;
	// ����� ���������� � windowed mode ��� full-screen mode
	sd.Windowed = TRUE;
	// ��� ������ ������ ������� ����� ����, ��� front buffer ���� back buffer-��
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// �������������� ��������� Swap Chain-�
	sd.Flags = NULL;

	// �������� Device, Device Contex, Swap Chain, Render Target View
	// ��������� feature level, ������� �������������� �����������, � ��������� ������������ ��� ��������
	D3D_FEATURE_LEVEL featureLevels[4] = { 
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0 };

	// ���� � ���� ������������ ������ hardware type
	D3D_DRIVER_TYPE driverTypes[3] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
		D3D_DRIVER_TYPE_SOFTWARE
	};

	// Create Factory Object

	// ��������� �� ��������� �������
	IDXGIAdapter* pDefaultAdapter;



	MSG msg;// ���������, ����������� ���������
	msg.message = 0; // ����� �����, ���������� � ���� message, �������� �� �������� ����� WM_QUIT

	while (msg.message != WM_QUIT) { // ����, ����� �� ����� �� ���������� ������ ������������ �������
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//Render();
	}

	// ��������� ������ ����������
	//CleanupDevice(); 
	return 0;
}

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
}






