#include <windows.h>
#include <d3d11.h>
//#include <cstring>
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

HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow); //инициализация окна.
HRESULT InitDevice(); // Инициализация устройств DirectX.
void CleanupDevice(); // Удаление созданнных устройств DirectX.
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // Функция окна.
void Render(); // Функция рисования.


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow) {

	UNREFERENCED_PARAMETER(hPrevInstance); //hPrevInstance не используется.
	UNREFERENCED_PARAMETER(lpCmdLine); //lpCmdLine не используется.

	if (FAILED(InitWindow(hInstance, nCmdShow))) {
		return 0;
	}
	if (FAILED(InitDevice())) {
		CleanupDevice();
		return 0;		
	}

	// СОЗДАНИЕ ОКНА

	// Имя класса окна
    CONST WCHAR* wndClassName(L"DX11GraphicApp");
    
	// Структура, описывающая класс Окна
	WNDCLASSEX wndClass;

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
	g_hWnd = CreateWindowEx()



	//инициализация DirectX компонентов
	
	// Структура, описывающая цепь связи (Swap Chain)
	DXGI_SWAP_CHAIN_DESC sd;





	MSG msg;// структура, описывающая сообщение
	msg.message = 0; // чтобы мусор, находящйся в поле message, случайно не оказался равен WM_QUIT

	while (msg.message != WM_QUIT) { // цикл, чтобы не выйти из приложения раньше необходимого времени
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		Render();
	}

	// окночание работы приложения
	CleanupDevice(); 
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






