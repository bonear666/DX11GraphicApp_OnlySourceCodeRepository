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

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ

HINSTANCE               g_hInst = NULL; //указатель на struct, дескриптор(handle) данного приложения.
HWND                    g_hWnd = NULL; //указатель на struct, дескриптор(handle) окна данного приложения.
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL; //целочисленная константа. Переменная, обозначающая Тип драйвера, определяет, где производить вычисления.
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_9_1; //используемый feature level, точнее, это переменная, которая будет хранить самый высокий feature level, доступный на заданном адаптере.
ID3D11Device* g_pd3dDevice = NULL; //указатель на struct(Объект Интерфейса ID3D11Device). ID3D11Device это COM-интерфейс, который создает ресурсы(текстуры, трехмерные объекты и т.д.) для вывода на дисплей.
ID3D11DeviceContext* g_pImmediateContext = NULL; //указатель на struct(Объект Интерфейса ID3D11DeviceContext). ID3D11DeviceContext это COM-интерфейс, который занимается отрисовкой графической информации на Дисплей.
IDXGISwapChain* g_pSwapChain = NULL; //указатель на struct(Объект Интерфейса IDXGISwapChain). IDXGISwapChain это COM-интерфейс, который хранит в нескольких буферах несколько отрисованых Поверхностей перед их выводом на Дисплей.
ID3D11RenderTargetView* g_pRenderTargetView = NULL; //указатель на struct(Объект Интерфейса ID3D11RenderTargetView). ID3D11RenderTargetView это COM-интерфейс, который хранит ресурсы back buffer-а. 
ID3D11InputLayout* g_pInputLayoutObject = NULL; // указатель на input layout object
ID3D11VertexShader* g_pVertexshader = NULL; // указатель на интерфейс vertex shader
ID3D11PixelShader* g_pPixelShader = NULL; // указатель на интерфейс pixel shader
ID3DBlob* VS_Buffer = NULL; // указатель на интерфейс буфера с скомпилированным вершинным шейдером 
ID3DBlob* PS_Buffer = NULL; // указатель на интерфейс буфера с скомпилированным пиксельным шейдером 
ID3D11Buffer* pVertexBuffer = NULL; // указатель на буфер вершин

//ПРЕДВАРИТЕЛЬНЫЕ ОБЪЯВЛЕНИЯ ФУНКЦИЙ

// Функция окна
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); 
// Создание DirectX компонентов
HRESULT createDirect3DComponents(UINT widthParam, UINT heightParam);
// Создание окна
HRESULT createWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam);
//
HRESULT initGeometry();
// Обновление сцены
void updateScene();
// Рендеринг сцены
void drawScene();
// Освобождение COM-интерфейсов
void releaseObjects();
// компиляция шейдера
HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob* buffer);

// Главная функция, точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// Переменная для хранения кода, возвращаемого вызванным методом интерфейса
	HRESULT hr;

	// СОЗДАНИЕ ОКНА
	hr = createWindow(L"DX11GraphicApp", L"GraphicApp", 800, 600, hInstance, nShowCmd);
	if (FAILED(hr)) {
		return hr;
	}

	//ИНИЦИАЛИЗАЦИЯ DirectX КОМПОНЕНТОВ	
	hr = createDirect3DComponents(800, 600);
	if (FAILED(hr)) {
		return hr;
	}

	// Создание vertex buffer
	// Описание вершин
	struct Vertex {
		D3DVECTOR position;
		D3DXCOLOR color;
	};

	// массив вершин
	Vertex* vertexArray = new Vertex[]{
		Vertex{D3DVECTOR{0.0f, 0.5f, 0.5f}, D3DXCOLOR{0.0f, 0.0f, 0.0f, 0.0f}},
		Vertex{D3DVECTOR{0.5f, -0.5f, 0.5f}, D3DXCOLOR{0.0f, 0.0f, 0.0f, 0.0f}},
		Vertex{D3DVECTOR{-0.5f, -0.5f, 0.5f}, D3DXCOLOR{0.0f, 0.0f, 0.0f, 0.0f}}
	};

	// описание vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	// размер буфера
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 3;
	// уровень доступа CPU и GPU к буферу 
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	// к какой стадии конвейера привязывать буфер
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	// уровень доступа CPU к буферу
	vertexBufferDesc.CPUAccessFlags = 0;
	// дополнительные параметры буфера
	vertexBufferDesc.MiscFlags = 0;
	// размер каждого элемента буфера, если данный буфер является structured buffer. (не использую)
	vertexBufferDesc.StructureByteStride = 0;

	// информация для инициализации подресурса
	D3D11_SUBRESOURCE_DATA vertexBufferSubresourceInitData;
	// указатель на инициализируемые данные
	vertexBufferSubresourceInitData.pSysMem = vertexArray;
	// смещение
	vertexBufferSubresourceInitData.SysMemPitch = 0;
	// смещение
	vertexBufferSubresourceInitData.SysMemSlicePitch = 0;

	// создание vertex buffer
	hr = g_pd3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubresourceInitData, &pVertexBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	// привязка vertex buffer к IA stage
	// колчиство байт конкретного вершинного буфера, которое будет использоваться дальше
	UINT stride[] = {sizeof(Vertex)};
	// смещение до первого элемента конкретного вершинного буфера, который будет использован
	UINT offset[] = {0};
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pVertexBuffer, stride, offset);
	
	// указание какие примитивы собирать из вершинного буфера
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// компиляция шейдеров
	// модель шейдеров
	LPCSTR shadersModel;
	// компиляция вершинного шейдера
	hr = CompileShader(L"TriangleVertexShader", "VS", "vs_5_0", VS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}
	// компиляция пиксельного шейдера
	hr = CompileShader(L"TrianglePixelShader", "PS", "ps_5_0", PS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}

	// Описание Input-Layout Object
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	// Количество элементов в Input-Layout Object
	UINT numInputLayoutObject = sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Создание Input-Layout Object
	g_pd3dDevice->CreateInputLayout(layout, numInputLayoutObject, ); 

	// освобождение памяти, занятой массивом вершин
	delete [] vertexArray;
	vertexArray = NULL;

	MSG msg;// структура, описывающая сообщение
	ZeroMemory(&msg, sizeof(MSG));

	while (msg.message != WM_QUIT) { // цикл, чтобы не выйти из приложения раньше необходимого времени
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		drawScene();
	}
	// окночание работы приложения
	releaseObjects();

	return 0;
};

// функция-обработчик сообщений, поступающих окну
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {

	// структура, содержащая необходимую информацию для рисования в клентской части окна
	PAINTSTRUCT ps;
	// указатель на дескриптор(или просто дескриптор) Device Context
	HDC hdc;

	switch (message) {

	case(WM_PAINT):
		// заполнение структуры ps, и очистка Update Region
		hdc = BeginPaint(hWnd, &ps);

		// очистка Update Region, и освобождение Device Context
		EndPaint(hWnd, &ps);
		break;

	case(WM_DESTROY):
		//отправка сообщения WM_QUIT
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
};

HRESULT createDirect3DComponents(UINT widthParam, UINT heightParam) {
	HRESULT hr;

	//ИНИЦИАЛИЗАЦИЯ DirectX КОМПОНЕНТОВ	

	// Струкутра, описывающая back buffer SwapChain-a
	DXGI_MODE_DESC backBuffer;
	ZeroMemory(&backBuffer, sizeof(DXGI_MODE_DESC));
	// Ширина
	backBuffer.Width = widthParam;
	// Высота
	backBuffer.Height = heightParam;
	// Частота обновления монитора
	backBuffer.RefreshRate.Numerator = 60;
	backBuffer.RefreshRate.Denominator = 1;
	// Формат пикселей
	backBuffer.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	// Порядок создания изображения(Surface)
	backBuffer.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	// Флаг, обозначающий то, как будет растягиваться изображение для соответствия резрешению заданного монитора
	backBuffer.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Структура, описывающая цепь обмена (Swap Chain)
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(DXGI_SWAP_CHAIN_DESC));
	// Описание backBuffer-a, который входит в состав Swap Chain-a
	sd.BufferDesc = backBuffer;
	// Описание мультисемплинга(не использую)
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	// Цель использования буферов
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Количество буферов в Swap Chain-e (так как приложение работает в оконном режиме, то для двойной буферизации достаточно ввести кол-во буферов без учета дополнительного Front Buffer-a
	//(Front Buffer-ом является рабочий стол)), но для full-screen mode приложения необходимо ввести 2 буфера для двойной буферизации
	sd.BufferCount = 2;
	// Дескриптор окна приложения
	sd.OutputWindow = g_hWnd;
	// Вывод происходит в windowed mode или full-screen mode
	sd.Windowed = TRUE;
	// Что должен делать драйвер после того, как front buffer стал back buffer-ом
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// Дополнительные параметры Swap Chain-а
	sd.Flags = NULL;

	// Этап Создания Device, Device Contex, Swap Chain, Render Target View, View Port

	// Определим feature level, который поддерживается видеокартой, и определим используемый тип драйвера
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0 };

	// Количество элементов в массиве featureLevels
	UINT numFeatureLevels = sizeof(featureLevels) / sizeof(D3D_FEATURE_LEVEL);

	// Хотя я буду использовать только hardware type
	D3D_DRIVER_TYPE driverTypes[] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
		D3D_DRIVER_TYPE_SOFTWARE
	};

	// Количество элементов в массиве dreiverTypes
	UINT numDriverTypes = sizeof(driverTypes) / sizeof(D3D_DRIVER_TYPE);

	// Собственно создание Direct3D Device, Device Context, Swap Chain, View Port

	// Результат вызова CreateDeviceAndSwapChain
	HRESULT createDeviceDeviceContextSwapChainResult(S_OK);

	for (UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; ++driverTypeIndex) {
		createDeviceDeviceContextSwapChainResult = D3D11CreateDeviceAndSwapChain(NULL, driverTypes[driverTypeIndex], NULL, NULL, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
		if (SUCCEEDED(createDeviceDeviceContextSwapChainResult)) {
			goto createDeviceDeviceContextSwapChainLoopExit;
		}
	}
	// Неуспешный выход из цикла
	return createDeviceDeviceContextSwapChainResult;
	// Успешный выход из цикла
createDeviceDeviceContextSwapChainLoopExit:

	// Получение доступа к back buffer
	// Указатель на back buffer
	ID3D11Texture2D* pBackBuffer(NULL); 
	hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	// Создание Render Target View
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);

	// Освобождение интерфейса back buffer, back buffer больше не нужен 
	pBackBuffer->Release();
	pBackBuffer = NULL; // хотя занулять не обязательно, в данном случае

	// Создание View Port, области поверхности RTV, которая и будет отображаться на дисплей. Также view port переводит RTV из пиксельных координат к числовым координатам 
	D3D11_VIEWPORT viewPort;
	ZeroMemory(&viewPort, sizeof(D3D11_VIEWPORT));

	viewPort.TopLeftX = 0;
	viewPort.TopLeftY = 0;
	viewPort.Width = widthParam;
	viewPort.Height = heightParam;
	// Уровень удаленности объектов, которые будут отображаться в view port
	viewPort.MinDepth = 0;
	viewPort.MaxDepth = 1;

	// Связывание view port с графическим конвейером
	g_pImmediateContext->RSSetViewports(1, &viewPort);

	// Привязка RTV к Output-Merger Stage
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

	return S_OK;
};

HRESULT createWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam) {
	// Структура, описывающая класс Окна
	WNDCLASSEX wndClass;
	ZeroMemory(&wndClass, sizeof(WNDCLASSEX));
	// Размер структуры wndClass
	wndClass.cbSize = sizeof(WNDCLASSEX);
	// Стиль класса окна
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	// Указатель на функцию, обрабатывающую сообщения, поступающие окну
	wndClass.lpfnWndProc = (WNDPROC)WndProc;
	// Количество дополнительных байтов для структуры wndClass
	wndClass.cbClsExtra = NULL;
	// Количество дополнительных байтов для экземпляра окна
	wndClass.cbWndExtra = NULL;
	// Дескриптор приложения
	wndClass.hInstance = hInstanceParam;
	// Дексриптор иконки окна
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	// Дескриптор курсора окна
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	// Дескриптор кисти
	wndClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	// Указатель на строку-имя меню класса 
	wndClass.lpszMenuName = NULL;
	// Имя класса окна
	wndClass.lpszClassName = wndClassNameParam;
	// Дескриптор иконки окна, которая отображается на панели задач
	wndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// Регистрация класса окна
	if (RegisterClassEx(&wndClass) == 0) {
		return E_FAIL;
	}
	// Создание окна
	g_hWnd = CreateWindowEx(NULL, wndClassNameParam, wndNameParam, WS_OVERLAPPEDWINDOW, 0, 0, widthParam, heightParam, NULL, NULL, hInstanceParam, NULL);
	if (g_hWnd == 0) {
		return E_FAIL;
	}
	// Вывод окна на дисплей 
	ShowWindow(g_hWnd, nShowCmdParam);

	return S_OK;
};

void updateScene() {

};

void drawScene() {
	// Цвет пикселя
	const FLOAT backgroundColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

	// Устанавливает цвет всеч пикселей поверхности RTV к единому значению
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, backgroundColor);

	// Вывод на дисплей поверхности Back Buffer
	g_pSwapChain->Present(0, 0);
};

HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob* buffer) {
	HRESULT hr;
	ID3DBlob* errorsBuffer = NULL;

	hr = D3DCompileFromFile(srcName, NULL, NULL, entryPoint, target, NULL, NULL, &buffer, &errorsBuffer);
	// вывод ошибок компиляции, если они есть
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








