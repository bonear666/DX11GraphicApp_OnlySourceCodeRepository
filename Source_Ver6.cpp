#include <windows.h>
#include <d3d11.h>
#include <iostream>

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ

HINSTANCE               g_hInst = NULL; //указатель на struct, дескриптор(handle) данного приложения.
HWND                    g_hWnd = NULL; //указатель на struct, дескриптор(handle) окна данного приложения.
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL; //целочисленная константа. Переменная, обозначающая Тип драйвера, определяет, где производить вычисления.
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_11_0; //целочисленная константа. Переменная, обозначающая какую версию DirectX использовать, соответственно, переменная определяет какие функции DirectX можно использовать.
ID3D11Device* g_pd3dDevice = NULL; //указатель на struct(Объект Интерфейса ID3D11Device). ID3D11Device это COM-интерфейс, который создает ресурсы(текстуры, трехмерные объекты и т.д.) для вывода на дисплей.
ID3D11DeviceContext* g_pImmediateContext = NULL; //указатель на struct(Объект Интерфейса ID3D11DeviceContext). ID3D11DeviceContext это COM-интерфейс, который занимается отрисовкой графической информации на Дисплей.
IDXGISwapChain* g_pSwapChain = NULL; //указатель на struct(Объект Интерфейса IDXGISwapChain). IDXGISwapChain это COM-интерфейс, который хранит в нескольких буферах несколько отрисованых Поверхностей перед их выводом на Дисплей.
ID3D11RenderTargetView* g_pRenderTargetView = NULL; //указатель на struct(Объект Интерфейса ID3D11RenderTargetView). ID3D11RenderTargetView это COM-интерфейс, который хранит ресурсы back buffer-а. 

//ПРЕДВАРИТЕЛЬНЫЕ ОБЪЯВЛЕНИЯ ФУНКЦИЙ

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // Функция окна.

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// СОЗДАНИЕ ОКНА

	// Имя класса окна
    CONST WCHAR* wndClassName(L"DX11GraphicApp");
	// Имя окна
	CONST WCHAR* wndName(L"GraphicApp");

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
	wndClass.hInstance = hInstance;
	// Дексриптор иконки окна
	wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	// Дескриптор курсора окна
	wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	// Дескриптор кисти
	wndClass.hbrBackground = CreateSolidBrush(RGB(0, 0, 0));
	// Указатель на строку-имя меню класса 
	wndClass.lpszMenuName = NULL;
	// Имя класса окна
	wndClass.lpszClassName = wndClassName;
	// Дескриптор иконки окна, которая отображается на панели задач
	wndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	// Регистрация класса окна
	if (RegisterClassEx(&wndClass) == 0) {
		return E_FAIL;
	}
	// Создание окна
	g_hWnd = CreateWindowEx(NULL, wndClassName, wndName, WS_OVERLAPPEDWINDOW, 0, 0 ,800, 600, NULL, NULL, hInstance, NULL);
	if (g_hWnd == 0) {
		return E_FAIL;
	}
	// Вывод окна на дисплей 
	ShowWindow(g_hWnd, nShowCmd);

	//ИНИЦИАЛИЗАЦИЯ DirectX КОМПОНЕНТОВ	

	// Струкутра, описывающая back buffer SwapChain-a
	DXGI_MODE_DESC backBuffer;
	ZeroMemory(&backBuffer, sizeof(DXGI_MODE_DESC));
	// Ширина
	backBuffer.Width = 800;
	// Высота
	backBuffer.Height = 600;
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
	sd.SampleDesc.Count = 0;
	// Цель использования буферов
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	// Количество буферов в Swap Chain-e (так как приложение работает в оконном режиме, то для двойной буферизации необходимо ввести кол-во буферов без учета дополнительного Front Buffer-a
	//(Front Buffer-ом является рабочий стол))
	sd.BufferCount = 1;
	// Дескриптор окна приложения
	sd.OutputWindow = g_hWnd;
	// Вывод происходит в windowed mode или full-screen mode
	sd.Windowed = TRUE;
	// Что должен делать драйвер после того, как front buffer стал back buffer-ом
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	// Дополнительные параметры Swap Chain-а
	sd.Flags = NULL;

	// Создание Device, Device Contex, Swap Chain, Render Target View
	// Определим feature level, который поддерживается видеокартой, и определим используемый тип драйвера
	D3D_FEATURE_LEVEL featureLevels[4] = { 
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0 };

	// Хотя я буду использовать только hardware type
	D3D_DRIVER_TYPE driverTypes[3] = {
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_REFERENCE,
		D3D_DRIVER_TYPE_SOFTWARE
	};

	// Create Factory Object

	// Указатель на дефолтный адаптер
	IDXGIAdapter* pDefaultAdapter;



	MSG msg;// структура, описывающая сообщение
	msg.message = 0; // чтобы мусор, находящйся в поле message, случайно не оказался равен WM_QUIT

	while (msg.message != WM_QUIT) { // цикл, чтобы не выйти из приложения раньше необходимого времени
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		//Render();
	}

	// окночание работы приложения
	//CleanupDevice(); 
	return 0;
}

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
}






