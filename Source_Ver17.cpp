#pragma comment(lib, "dxgi")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3d10_1")
#pragma comment(lib, "d3d10")
#pragma comment(lib,"d3dcompiler")

#include <windows.h>
#include <D3D11.h>
#include <D3D10.h>
#include <DXGI.h>
#include <DirectXMath.h>
//#include <xnamath.h>
#include <d3dcompiler.h>
#include <iostream>
#include <cmath>
#include <hidusage.h>

using namespace DirectX;

// начальные количества структур 
#define STATIC_HIT_BOX_AMOUNT 20
/*
#define HIT_BOX_WIDTH_AND_HEIGHT_AMOUNT 20
#define HIT_BOX_CENTER_AMOUNT 20
#define HIT_BOX_ANGLE_AMOUNT 20
#define STATIC_HIT_BOX_DESC_AMOUNT 20
*/
#define STATIC_HIT_BOX_AREA_AMOUNT 6
#define STATIC_HIT_BOX_AREA_LEAF_AMOUNT 4
#define STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_0 2
#define STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_1 2
#define STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_2 2
#define STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_3 2
#define DYNAMIC_HIT_BOX_AMOUNT 3
#define DYNAMIC_HIT_BOX_MOVEVECTOR_LENGTH 0.001f
#define ROTATION_ANGLE 0.00007f
#define W_COORD 0.01f

#define SHADERS_MODEL(FEATURE_LEVEL) if (FEATURE_LEVEL >= D3D_FEATURE_LEVEL_11_0) {\
											shadersModel = { "vs_5_0", "ps_5_0" };\
										}\
										else\
											shadersModel = { "vs_4_0", "ps_4_0" };
// ОПИСАНИЕ СТРУКТУР

// Описание вершин
struct Vertex {
	XMFLOAT4 position;
	XMFLOAT4 color;
};

// модель шейдеров
struct ShaderModelDesc {
	LPCSTR vertexShaderModel;
	LPCSTR pixelShaderModel;
};

// матрицы
struct MatricesBuffer {
	XMMATRIX mWorld;              // Матрица мира
	XMMATRIX mView;        // Матрица вида
	XMMATRIX mProjection;  // Матрица проекции
	XMMATRIX mRotationAroundAxis; // Матрица поворота вокруг оси
};

// структура угла
struct AngleConstantBuffer {
	float angle0;
	float angle1;
	float angle2;
	float angle3;
};

// Ширина и высота хитбокса
typedef XMFLOAT2 HitBoxWidthAndHeight;
// угол хитбокса
typedef FLOAT HitBoxAngle;
// смещения в массиве
typedef unsigned __int8 ArrayOffset8Bit;
typedef unsigned __int16 ArrayOffset16Bit;

// структура центра хитбокса, и позиции камеры в глобальных координатах
typedef struct {
	float x;
	float z;
} HitBoxCenter, CameraPosition;

// структура хитбокса
struct HitBox {
	float centerX;
	float centerZ;
	XMFLOAT3 widthHeightLength;
	const XMMATRIX* angleMatrixRotation; // матрица смещения и поворота
	const XMMATRIX* invertAngleMatrixRotation; // обратная матрица смещения и поворота
};

// структура описания хитбокса
struct HitBoxDesc {
	ArrayOffset8Bit widthAndHeightArrayPos; // позиция в массиве структур, которые описывают ширину и высоту хитбокса
	ArrayOffset8Bit centerArrayPos; // позиция в массиве структур, которые описывают центр хитбокса
	ArrayOffset16Bit angleArrayPos; // позиция в массиве структур, которые описывают угол хитбокса
};// хотя такая структура скорее всего будет медленее работать, чем если бы вместо 1-байтных интов я использовал обычные 4-байтные инты
// по идее процессору предется каждый раз логически умножать 1-байтный инт на маску чтобы занулить ненужные биты и смещать его в начало регистра, я так предполагаю

// структура области неподвижных хитбоксок
struct StaticHitBoxArea {
	StaticHitBoxArea* leftNode; //левый узел в бинарном дереве
	// у каждого узла, который не является листком, всегда есть два узла-потомка
	// у листка нет потомков, значит можно ввести такое объединение:
	union {
		StaticHitBoxArea* rightNode; //правый узел в бинарном дереве
		HitBox** staticHitBoxesArray; //указатель на массив, в котором храянтся указатели хитбоксов, которые находятся в данной зоне
	};
	StaticHitBoxArea* parentNode; //родительский узел в бинарном дереве
	float centerX; //центр области
	float centerZ;
	float lowX; // отрезок, характеризующий ширину области(координаты отрезка представлены в глобальной системе координат)
	float highX;
	float lowZ; // отрезок, характеризующий длину области(координаты отрезка представлены в глобальной системе координат)
	float highZ;
	int staticHitBoxesAmount; // количество статических хитбоксов в массиве статических массивов(актуально только для листьев)
	bool nodeSideFlag; // 0 если узел является левым узлом parentNode, 1 если узел является правым узлом parentNode
};

// структура динамических хитбоксов
struct DynamicHitBox {
	float halfOfShellEdge;
	XMFLOAT3 position;
	XMFLOAT3 halvesOfWidthHeightLength;
	float angle;
	XMVECTOR* moveVectorPtr;
	float activeCyclesAmount;
	XMFLOAT3 startPosition; // куда возвращаться, когда хитбокс вышел за пределы карты
};

// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ

HINSTANCE               g_hInst = NULL; //указатель на struct, дескриптор(handle) данного приложения.
HWND                    g_hWnd = NULL; //указатель на struct, дескриптор(handle) окна данного приложения.
LONG StartUpWndProcPtr = NULL; // указатель на дефолтную оконную процедуру
D3D_DRIVER_TYPE         g_driverType = D3D_DRIVER_TYPE_NULL; //целочисленная константа. Переменная, обозначающая Тип драйвера, определяет, где производить вычисления.
D3D_FEATURE_LEVEL       g_featureLevel = D3D_FEATURE_LEVEL_9_1; //используемый feature level, точнее, это переменная, которая будет хранить самый высокий feature level, доступный на заданном адаптере.
ID3D11Device* g_pd3dDevice = NULL; //указатель на struct(Объект Интерфейса ID3D11Device). ID3D11Device это COM-интерфейс, который создает ресурсы(текстуры, трехмерные объекты и т.д.) для вывода на дисплей.
ID3D11DeviceContext* g_pImmediateContext = NULL; //указатель на struct(Объект Интерфейса ID3D11DeviceContext). ID3D11DeviceContext это COM-интерфейс, который занимается отрисовкой графической информации на Дисплей.
IDXGISwapChain* g_pSwapChain = NULL; //указатель на struct(Объект Интерфейса IDXGISwapChain). IDXGISwapChain это COM-интерфейс, который хранит в нескольких буферах несколько отрисованых Поверхностей перед их выводом на Дисплей.
ID3D11RenderTargetView* g_pRenderTargetView = NULL; //указатель на struct(Объект Интерфейса ID3D11RenderTargetView). ID3D11RenderTargetView это COM-интерфейс, который хранит ресурсы back buffer-а. 
ShaderModelDesc shadersModel;
ID3D11InputLayout* g_pInputLayoutObject = NULL; // указатель на input layout object
ID3D11VertexShader* g_pVertexShader = NULL; // указатель на интерфейс vertex shader
ID3D11PixelShader* g_pPixelShader = NULL; // указатель на интерфейс pixel shader
ID3DBlob* VS_Buffer = NULL; // указатель на интерфейс буфера с скомпилированным вершинным шейдером 
ID3DBlob* PS_Buffer = NULL; // указатель на интерфейс буфера с скомпилированным пиксельным шейдером 
ID3D11Buffer* pVertexBuffer = NULL; // указатель на буфер вершин пирамиды
ID3D11Buffer* pWallsVertexBuffer = NULL; // указатель на буфер вершин стен
ID3D11Buffer* pConstantBuffer = NULL; // констнантный буфер
ID3D11Buffer* pIndexBuffer = NULL; // буфер индексов
ID3D11Buffer* pAngleBuffer = NULL; // буфер угла 
ID3D11Buffer* constantBufferArray[] = {NULL, NULL, NULL}; // массив указателей на интерфейсы константных буферов
AngleConstantBuffer angleCBufferData = { 0.0f, 0.0f, 0.0f, 0.0f }; // угол поворота
ID3D11ShaderResourceView* pAngleBufferVSResource = NULL; // ресурс вершинного шейдера, к оторм находится угол
ID3D11Texture2D* depthStencilTexture = NULL; // текстура depth буфера
ID3D11DepthStencilView* g_pDepthStencilView = NULL; // ресурсы depth буфера
ID3D11DepthStencilState* pDSState = NULL; // состояние depth-stencil теста
ID3D11RasterizerState* pRasterizerState = NULL; // состояние растеризатора 
MatricesBuffer matricesWVP; // матрицы
XMVECTOR objectsPositions[] = { //массив точек, в которых располагаются объекты
	XMVECTORF32{-25.0f, 0.0f, 25.0f, 0.0f}, // {-25.0f, 0.0f, 25.0f, 0.0f}
	XMVECTORF32{25.f, 0.0f, 25.0f, 0.0f},
	XMVECTORF32{0.0f, 0.0f, -25.0f, 0.0f}
};
XMMATRIX moveAheadMatrix = XMMatrixTranspose(XMMatrixTranslation(0.0f, 0.0f, 0.4f)); // матрица движения вперед
XMVECTOR moveAheadVector = XMVectorSet(0.0f, 0.0f, -0.1f, 0.0f); // вектор движения в положительном направлении оси(в системе координат камеры) 
XMVECTOR moveAheadVectorInGlobalCoord = XMVECTORF32{ 0.0f, 0.0f, 0.1f, 0.0f }; // вектор движения в положительном направлении оси(в глобальной системе координат)
XMVECTOR moveBackVector = XMVectorSet(0.0f, 0.0f, 0.1f, 0.0f); // вектор движения в отрицательном направлении оси
XMVECTOR moveRightVector = XMVectorSet(0.0f, 0.0f, 0.0f, -0.1f); 
XMVECTOR moveRightVectorInGlobalCoord = XMVECTORF32{ 0.1f, 0.0f, 0.0f, 0.0f }; 
XMVECTOR moveLeftVector = XMVectorSet(0.0f, 0.0f, 0.0f, 0.1f); 
DWORD pageSize; // размер страницы виртуальной памяти
DWORD allocationGranularity; // выравнивание адресов в виртуальной памяти

// переменные, относящиеся к хитбоксам
XMVECTOR halvesOfWidthHeightLengthOfMap = XMVECTORF32{50.0f, 0.0f, 50.0f, 0.0f}; // половины значений глобальной карты
LPVOID dynamicMemory; // сырая память с учетом байтов для выравнивания
SIZE_T pageAmount; // количество страниц, необходимое для того чтобы вместить все хитбоксы
HitBox* StaticHitBoxArray; // указатель на массив статических хитбоксов, выровненный по границе 4 байта
StaticHitBoxArea* StaticHitBoxAreaArray; // указатель на массив StaticHitBoxArea структур
// указатели на массивы позиций в StaticHitBoxDescArray, которые определяют какие статические хитбоксы есть в листьях staticHitBoxArea
HitBox** Leaf0Array; // 0 лист
HitBox** Leaf1Array; // 1 лист
HitBox** Leaf2Array; // 2 лист
HitBox** Leaf3Array; // 3 лист
StaticHitBoxArea* currentHitBoxArea; // указатель на hitboxarea, в которой находится камера в данный момент
XMFLOAT3 currentCameraPos = {0.0f, 0.0f ,0.0f}; // текущая позиция камеры в координатах x,z
XMFLOAT3 previousCameraPos; // позиция камеры на предыдущем кадре
// массив матриц поворота статических хитбоксов
// чтобы выяснить попадает ли позиция камеры внутрь хитбокса, нам нужно сместить центр хитбокса так, чтобы ширина хитбокса совпадала с осью X,
// а длина хитбокса совпадала с осью Z
// первая матрица - смещаем центр координат в центр хибокса
// вторая матрица - поворачиваем точку и хитбокс так, чтобы ширина хитбокса была || оси X , а длинна хитбокса || оси Z
// третья матрица - смещаем центр координат в левый нижний угол хитбокса
const XMMATRIX staticHitBoxesRotationMatricesArray[4] = { // аргументы первой функции - отрицательное значение позиции хитбокса
	// аргументы второй функции - отрицательное значение угла поворота хитбокса, относительно системы координат, проходящей через центр хитбокса
	XMMatrixTranslation(0.0f, 0.0f, -55.0f) * XMMatrixRotationY(0.0f) * XMMatrixTranslation(50.0f, 0.0f, 2.5f), // матрица поворота для 0 хитбокса
	XMMatrixTranslation(-55.0f, 0.0f, 0.0f) * XMMatrixRotationY(0.0f) * XMMatrixTranslation(2.5f, 0.0f, 50.0f), // матрица поворота для 1 хитбокса
	XMMatrixTranslation(0.0f, 0.0f, 55.0f) * XMMatrixRotationY(0.0f) * XMMatrixTranslation(50.0f, 0.0f, 2.5f), // матрица поворота для 2 хитбокса
	XMMatrixTranslation(55.0f, 0.0f, 0.0f) * XMMatrixRotationY(0.0f) * XMMatrixTranslation(2.5f, 0.0f, 50.0f) // матрица поворота для 3 хитбокса
	// было бы правильнее сразу посчитать произведение этих матриц и записать его в таком виде:
	//XMMATRIX{XMVECTORF32{0.0f, 0.0f, 0.0f, 0.0f},
	//XMVECTORF32{0.0f, 0.0f, 0.0f, 0.0f},
	//XMVECTORF32{0.0f, 0.0f, 0.0f, 0.0f},
	//XMVECTORF32{0.0f, 0.0f, 0.0f, 0.0f}}
}; 
// массив обратных матриц к матрицам из staticHitBoxesRotationMatricesArray
const XMMATRIX invertStaticHitBoxesRotationMatricesArray[4] = {
	XMMatrixTranslation(-50.0f, 0.0f, - 2.5f) * XMMatrixRotationY(0.0f) * XMMatrixTranslation(0.0f, 0.0f, 55.0f), // матрица поворота для 0 хитбокса
	XMMatrixTranslation(-2.5f, 0.0f, -50.0f)* XMMatrixRotationY(0.0f)* XMMatrixTranslation(55.0f, 0.0f, 0.0f), // матрица поворота для 1 хитбокса
	XMMatrixTranslation(-50.0f, 0.0f, -2.5f)* XMMatrixRotationY(0.0f)* XMMatrixTranslation(0.0f, 0.0f, -55.0f), // матрица поворота для 2 хитбокса
	XMMatrixTranslation(-2.5f, 0.0f, -50.0f)* XMMatrixRotationY(0.0f)* XMMatrixTranslation(-55.0f, 0.0f, 0.0f), // матрица поворота для 3 хитбокса
};
// массив динамических хитбоксов
DynamicHitBox* DynamicHitBoxesArray;
// массив векторов движения динамических хитбоксов
XMVECTOR DynamicHitBoxesMoveVectorsArray[3];
// переменная, куда кладем произвольные переменные типа XMFLOAT4
XMVECTOR sseProxyRegister0;
XMVECTOR sseProxyRegister1;
XMVECTOR sseProxyRegister2;
XMVECTOR sseProxyRegister3;
XMVECTOR sseProxyRegister4;
XMMATRIX sseProxyRegister0_Matrix;
// переменная, куда кладем произвольные переменные типа XMVECTOR
XMFLOAT3 xmfloat4Storage0;


//ПРЕДВАРИТЕЛЬНЫЕ ОБЪЯВЛЕНИЯ ФУНКЦИЙ

// Функция окна
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); 
// Создание DirectX компонентов
HRESULT CreateDirect3DComponents(UINT widthParam, UINT heightParam);
// Создание окна
HRESULT MyCreateWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam);
// Создание буфера вершин, компиляция шейдеров, связывание шейдеров с конвейером
HRESULT InitGeometry(Vertex* vertexArray, LPCWSTR vertexShaderName, LPCWSTR pixelShaderName, LPCSTR vsShaderEntryPoint, LPCSTR psShaderEntryPoint);
// Обновление сцены
void UpdateScene();
// Рендеринг сцены
void DrawScene(XMVECTOR* objectsPositionsArray);
// Освобождение COM-интерфейсов
void ReleaseObjects();
// компиляция шейдера
HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob** buffer);
// Создание константного буфера матриц, константного буфера угла, буфера вершин
HRESULT InitMatrices(WORD* indices);   
// Обновление матриц
void SetMatrices();
// Обновление проекционной матрицы (0 < angle < PI/2). angle = fov/2
// nearZ, farZ координаты плоскости отсечений, деленные на W
// nearZ не должен быть равен нулю, так как исходя из формулы, Z координата вершины после умножения на матрицу проекции будет всегда равна 1
void SetProjectionMatrix(MatricesBuffer* pMatricesBuffer, FLOAT angleHoriz, FLOAT angleVert, FLOAT nearZ, FLOAT farZ, BOOL saveProportionsFlag);
// Сохранение пропорций объектов, при выводе в окно. Пропорции сохраняются в соответствии с осью, у которой меньше единичный отрезок. 
void SaveProportions(MatricesBuffer* pMatricesBuffer, HWND hWnd);
// Наибольший элемент
FLOAT MaxElement(FLOAT arg0, FLOAT arg1);
// Создание обратной матрицы
HRESULT InvertMatrix(XMVECTOR zAxis, XMVECTOR yAxis, XMMATRIX* invertibleMatrix);
// Создание матрицы перехода к другой системе координат
HRESULT NewCoordinateSystemMatrix(XMVECTOR point, XMVECTOR zAxis, XMVECTOR yAxis, XMMATRIX* invertibleMatrix);
// Создание матрицы мира
void SetWorldMatrix(XMVECTOR point, XMVECTOR scale, XMMATRIX* worldMatrix);
// изменение порядка обхода вершин
void InvertIndices(WORD* indicesArray, int size);
// поиск ортогонального вектора к заданному
XMVECTOR FindOrthogonalVector(XMVECTOR vector);
// поиск ортгонального нормализованного вектора к заданному
XMVECTOR FindOrthogonalNormalizedVector(XMVECTOR vector);
// матрица вращения вершины вокруг произвольного вектора
void RotationAroundAxis(XMVECTOR yAxis, XMVECTOR point, FLOAT angle, XMMATRIX* outputMatrix);
// Обновление проекционной матрицы, с учетом расстояния от камеры до ближней плоскости отсечения (0 < angle < PI/2). angle = fov/2
// camera Distance <= nearZ
// используя эту функцию можно визуально опознать ближнюю плоскость отсечения
// cameraDistance - значение расстояния от камеры до ближней плоскости отсечения, задается деленным на W координату
// nearZ, farZ координаты плоскости отсечений, деленные на W
// cameraDistance не должен быть равен нулю, так как исходя из формулы, Z координата вершины после умножения на матрицу проекции будет всегда равна 1
void SetProjectionMatrixWithCameraDistance(MatricesBuffer* pMatricesBuffer, FLOAT angleHoriz, FLOAT angleVert, FLOAT nearZ, FLOAT farZ, FLOAT cameraDistance, BOOL saveProportionsFlag);
// инициализация глобальных статических переменных с размером страницы виртуальной памяти, и переменной с гранулярностью памяти
void InitPageSizeAndAllocGranularityVariables(); 
// проверка на изменение области статических хитбоксов,т.е. изменилась ли позиция камеры относительно текущей области статических хитбоксов
bool ChangesOfStaticHtBoxesArea();
// определенние текущей области статических хитбоксов, в которой находится камера в данный момент
void DefineCurrentStaticHtBoxesArea();
// проверка на столкновение камеры со статическими хитбоксами в текущей области статических хитбоксов
void StaticHitBoxesCollisionDetection();
// инициализация хитбоксов
void InitHitBoxes();
// смещение и поворот динмаического хитбокса
inline void __vectorcall MoveAndRotationDynamicHitBox(FXMVECTOR moveVector, float rotationAngle, DynamicHitBox* dynamicHitBox);
// проверка на столкновение камеры с динмаическими хитбоксами
void DynamicHitBoxesCollisionDetection();
// через сколько циклов центр динамического хитбокса выйдет за пределы карты
float DynamicHitBoxCyclesAmount(DynamicHitBox* hitBox);
// создание новго вектора движения динмаического хитбокса
void CreateNewMoveVectorForDynamicHitBox(DynamicHitBox* hitBox);
// начальная инициализация векторов движения динамических хитбоксов
inline void InitMoveVectorsAndActiveCyclesAmountForDynamicHitBoxes();
// оконная процедура, которая обрабатывает все сообщения по дефолту
LRESULT CALLBACK StartUpWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
// 
HRESULT InitWallsVertices(Vertex* wallsVertexArray, LPCWSTR wallsVertexShaderName, LPCWSTR wallsPixelShaderName, LPCSTR vsShaderEntryPoint, LPCSTR psShaderEntryPoint);


// Главная функция, точка входа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	// Переменная для хранения кода, возвращаемого вызванным методом интерфейса
	HRESULT hr;

	// СОЗДАНИЕ ОКНА
	hr = MyCreateWindow(L"DX11GraphicApp", L"GraphicApp", 1280, 720, hInstance, nShowCmd);
	if (FAILED(hr)) {
		return hr;
	}

	//ИНИЦИАЛИЗАЦИЯ DirectX КОМПОНЕНТОВ	
	hr = CreateDirect3DComponents(1280, 720);
	if (FAILED(hr)) {
		return hr;
	}

	// прикрепляем к окну процедуру, которая будет обрабатывать нажатия клавиш и т.д.
	StartUpWndProcPtr = SetWindowLong(g_hWnd, GWLP_WNDPROC, (LONG)WndProc);

	// массив вершин (пирамида)
	Vertex* vertexArray = new Vertex[4]{
		Vertex{XMFLOAT4{-2.0f, -2.0f, 2.0f, 1.0f}, XMFLOAT4{1.0f, 0.0f, 0.0f, 1.0f}}, // a 0
		Vertex{XMFLOAT4{2.0f, -2.0f, 2.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 0.0f, 1.0f}}, //b 1
		Vertex{XMFLOAT4{0.0f, -2.0f, -2.0f, 1.0f}, XMFLOAT4{0.0f, 0.0f, 0.0f, 1.0f}}, //c 2
		Vertex{XMFLOAT4{0.0f, 4.0f, 0.0f, 1.0f}, XMFLOAT4{1.0f, 0.0f, 1.0f, 1.0f}} //d 3 вершина пирамиды
	};

	Vertex* wallsVertices = new Vertex[8]{
		Vertex{XMFLOAT4{-50.0f, -2.0f, 50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}}, 
		Vertex{XMFLOAT4{-50.0f, 6.0f, 50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}},
		Vertex{XMFLOAT4{50.0f, -2.0f, 50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}},
		Vertex{XMFLOAT4{50.0f, 6.0f, 50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}},
		Vertex{XMFLOAT4{-50.0f, -2.0f, -50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}},
		Vertex{XMFLOAT4{-50.0f, 6.0f, -50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}},
		Vertex{XMFLOAT4{50.0f, -2.0f, -50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}},
		Vertex{XMFLOAT4{50.0f, 6.0f, -50.0f, 1.0f}, XMFLOAT4{1.0f, 1.0f, 1.0f, 1.0f}}
	};

	// создание буфера вершин, компиляция шейдеров, связывание шейдеров и буфера вершин с конвейером
	hr = InitGeometry(vertexArray, L"TriangleVertexShader.hlsl", L"TrianglePixelShader.hlsl", "main", "main");
	if (FAILED(hr)) {
		return hr;
	}

	// освобождение памяти, занятой массивом вершин
	delete[] vertexArray;
	vertexArray = NULL;

	// индексы вершин
	// обход по часовой стрелеке относительно нормали к поверхности, которую нужно показать
	WORD indices[] = { 
		3, 0, 1, //abс видимая грань (обход по часовой стрелке)
		3, 1, 2, //aсd видимая грань (обход по часовой стрелке)
		3, 2, 0, //dcb невидимая грань (обход против часовой стрелки)
		2, 1, 0 //adb  невидимая грань (обход против часовой стрелки)
	};

	//InvertIndices(indices, 12);

	// Создание константного буфера матриц, константного буфера угла, буфера вершин
	hr = InitMatrices(indices);
	if (FAILED(hr)) {
		return hr;
	}

	// инициализация матриц
	float vecAngle = -XM_PIDIV4;
	XMVECTOR eye = XMVectorSet(0.0f, 0.0f, 0.0f, 100.0f); // откуда смотрим
	// Лучше использовать XMScalarSinExt, XMScalarCosExt вместо XMScalarSin, XMScalarCos, чтобы получать хорошое округление чисел
	//XMVECTOR zAxis = XMVectorSet(0.0f, XMScalarSinEst(vecAngle), XMScalarCosEst(vecAngle), 1.0f); // куда смотрим. 
	//XMVECTOR yAxis = XMVectorSet(0.0f, XMScalarCosEst(XM_PIDIV4), XMScalarSinEst(XM_PIDIV4), 1.0f); // нормаль к тому, куда смотрим

	XMVECTOR zAxis = XMVectorSet(0.0f, 0.0f, 1.0f, 100.0f); // куда смотрим. 
	XMVECTOR yAxis = XMVectorSet(0.0f, 1.0f, 0.0f, 100.0f); // нормаль к тому, куда смотрим

	//NewCoordinateSystemMatrix(eye, zAxis, yAxis, &matricesWVP.mView);
	matricesWVP.mView = XMMatrixLookToLH(eye, zAxis, yAxis);
	//matricesWVP.mView.r[3] = _mm_mul_ps(eye, XMVectorSet(-1.0f, -1.0f, -1.0f, 1.0f)); // приходится заменять последнюю строку в матрице вида, так как XMMatrixLookToLH как-то странно считает значение последней строки
	matricesWVP.mView = XMMatrixTranspose(matricesWVP.mView); 

	// инициализация матрицы проекции
	float fovAngle = XM_PI / 2.5f;
	float ratio = 1280.0F / 720.0F;
	matricesWVP.mProjection = XMMatrixPerspectiveFovLH(fovAngle, ratio, 0.1F, 200.0F);
	matricesWVP.mProjection.r[0] *= XMVECTORF32{W_COORD, 1.0f, 1.0f, 1.0f};
	matricesWVP.mProjection.r[1] *= XMVECTORF32{1.0f, W_COORD, 1.0f, 1.0f};
	matricesWVP.mProjection.r[2] *= XMVECTORF32{1.0f, 1.0f, W_COORD, W_COORD};
	matricesWVP.mProjection.r[3] *= XMVECTORF32{1.0f, 1.0f, W_COORD, 1.0f};
	matricesWVP.mProjection = XMMatrixTranspose(matricesWVP.mProjection);
	/*
	float sin1;
	float cos1;
	XMScalarSinCos(&sin1, &cos1, 0.5f * fovAngle);
	float myCoeff = 1.0f / (1.0f - (sin1 / cos1));
	
	matricesWVP.mProjection = XMMatrixPerspectiveFovLH(fovAngle, ratio, 0.1F, 200.0F);
	matricesWVP.mProjection.r[0] *= XMVECTORF32{0.0f, 1.0f, 1.0f, 1.0f};
	matricesWVP.mProjection.r[1] *= XMVECTORF32{1.0f, 0.0f, 1.0f, 1.0f};
	matricesWVP.mProjection.r[0] += XMVECTORF32{1.0f * myCoeff, 0.0f, 0.0f, 0.0f};
	matricesWVP.mProjection.r[1] += XMVECTORF32{0.0f, ratio * myCoeff, 0.0f, 0.0f};
	
	matricesWVP.mProjection.r[0] *= XMVECTORF32{W_COORD, 1.0f, 1.0f, 1.0f};
	matricesWVP.mProjection.r[1] *= XMVECTORF32{1.0f, W_COORD, 1.0f, 1.0f};
	matricesWVP.mProjection.r[2] *= XMVECTORF32{1.0f, 1.0f, W_COORD, W_COORD};
	matricesWVP.mProjection.r[3] *= XMVECTORF32{1.0f, 1.0f, W_COORD, 1.0f};
	matricesWVP.mProjection = XMMatrixTranspose(matricesWVP.mProjection); */
	//SetProjectionMatrix(&matricesWVP, 0, XM_PI / 100.0f, 0.001f, 7.35f, true);
	//SetProjectionMatrixWithCameraDistance(&matricesWVP, XM_PI / 5.0f, XM_PI / 25.0f, 0.5f, 2.2f, 0.0001f, true);

	// инициализация массива неподвижных хитбоксов
	InitPageSizeAndAllocGranularityVariables();
	
	// инициализация хитбоксов
	InitHitBoxes();
	
	MSG msg;// структура, описывающая сообщение
	ZeroMemory(&msg, sizeof(MSG));

	while (msg.message != WM_QUIT) { // цикл, чтобы не выйти из приложения раньше необходимого времени
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		UpdateScene();
		DrawScene(objectsPositions);

		// проверка на столкновение камеры с статическими и динамическими хитбоксами
		// если камера перешла в другую область статических хитбоксов
		if (ChangesOfStaticHtBoxesArea) {
			DefineCurrentStaticHtBoxesArea();
		}
		//StaticHitBoxesCollisionDetection();
		DynamicHitBoxesCollisionDetection();
	}
	// окночание работы приложения
	ReleaseObjects();
	return 0;
};

LRESULT CALLBACK StartUpWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	return DefWindowProc(hWnd, message, wParam, lParam);
};


// функция-обработчик сообщений, поступающих окну
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	// структура, содержащая необходимую информацию для рисования в клентской части окна
	PAINTSTRUCT ps;
	// указатель на дескриптор(или просто дескриптор) Device Context
	HDC hdc;

	switch (message) {
	// обработка сообщений от мыши или клавиатуры
	case(WM_INPUT): {
		UINT dwSize = sizeof(RAWINPUT);
		static BYTE lpRawInput[sizeof(RAWINPUT)];

		UINT resData = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpRawInput, &dwSize, sizeof(RAWINPUTHEADER));
		DWORD resData2 = ((RAWINPUT*)lpRawInput)->header.dwType;
		if (resData2 == RIM_TYPEKEYBOARD) { // если сообщение поступило от клавиатуры
			USHORT pressedKey = ((RAWINPUT*)lpRawInput)->data.keyboard.VKey;

			switch (pressedKey) {
			case(0x57): {// W
				// можно было бы просто прибавить moveAheadVector к последней строчке view matrix, но эта матрица уже транспонирована, поэтому так сделать не получится 
				matricesWVP.mView = XMMatrixTranspose(XMMatrixTranslationFromVector(moveAheadVector)) * matricesWVP.mView;
				//бляяя зачем я решил хранить позицию камеры в xmfloat3, нужно было в xmvector
				// меняем позицию камеры относительно глобальных координат
				sseProxyRegister0 = XMLoadFloat3(&currentCameraPos); 
				sseProxyRegister0 += moveAheadVectorInGlobalCoord;
				XMStoreFloat3(&currentCameraPos, sseProxyRegister0);
				break;
			}
			
			case(0x53): { // S
				matricesWVP.mView = XMMatrixTranspose(XMMatrixTranslationFromVector(moveBackVector)) * matricesWVP.mView;

				sseProxyRegister0 = XMLoadFloat3(&currentCameraPos);
				sseProxyRegister0 -= moveAheadVectorInGlobalCoord;
				XMStoreFloat3(&currentCameraPos, sseProxyRegister0);
				break;
			}

			case(0x44): { // D
				matricesWVP.mView.r[0] = matricesWVP.mView.r[0] + moveRightVector;

				sseProxyRegister0 = XMLoadFloat3(&currentCameraPos);
				sseProxyRegister0 += moveRightVectorInGlobalCoord;
				XMStoreFloat3(&currentCameraPos, sseProxyRegister0);
				break;
			}

			case(0x41): { // A
				matricesWVP.mView.r[0] = matricesWVP.mView.r[0] + moveLeftVector;

				sseProxyRegister0 = XMLoadFloat3(&currentCameraPos);
				sseProxyRegister0 -= moveRightVectorInGlobalCoord;
				XMStoreFloat3(&currentCameraPos, sseProxyRegister0);
				break;
			}

			default:
				break;
			}
		}
		else // если сообщение поступило от мыши
		{
			LONG mouseX = ((RAWINPUT*)lpRawInput)->data.mouse.lLastX;
			LONG mouseY = ((RAWINPUT*)lpRawInput)->data.mouse.lLastY;
			static XMVECTOR newYAxisRotation = g_XMIdentityR1;

			XMMATRIX matrixRotationNewX = XMMatrixRotationX(-XM_PI * 0.0005 * mouseY); //матрица поворота вокруг оси X
			XMMATRIX matrixRotationNewY = XMMatrixRotationNormal(newYAxisRotation, -XM_PI * 0.0005 * mouseX); //матрица поворота вокруг оси Y
			newYAxisRotation = XMVector3Transform(newYAxisRotation, matrixRotationNewX); //статичная ось Y, вокруг которой происходит поворот
			moveAheadVector = XMVector3Transform(moveAheadVector, matrixRotationNewX);
			moveBackVector = (-1.0f) * moveAheadVector;

			matricesWVP.mView = XMMatrixTranspose(matrixRotationNewX * matrixRotationNewY) * matricesWVP.mView;

			//то же самое, только реализовано через построение новой матрицы вида
			/*
			static XMVECTOR newXAxisRotation = g_XMIdentityR0;
			static XMVECTOR cameraZCoordinateInWorldSpace = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
			static XMVECTOR cameraYCoordinateInWorldSpace = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f);
			static XMVECTOR cameraPositionInWorldSpace = XMVectorSet(0.0f, 0.3f, -3.0f, 1.0f);

			XMMATRIX matrixRotationNewY = XMMatrixRotationY(XM_PI * 0.0005 * mouseX);
			XMMATRIX matrixRotationNewX = XMMatrixRotationNormal(newXAxisRotation, XM_PI * 0.0005 * mouseY);
			newXAxisRotation = XMVector3Transform(newXAxisRotation, matrixRotationNewY);

			XMMATRIX transformMatrix = matrixRotationNewX * matrixRotationNewY;
			cameraZCoordinateInWorldSpace = XMVector3Transform(cameraZCoordinateInWorldSpace, transformMatrix);
			cameraYCoordinateInWorldSpace = XMVector3Transform(cameraYCoordinateInWorldSpace, transformMatrix);

			matricesWVP.mView = XMMatrixLookToLH(cameraPositionInWorldSpace, cameraZCoordinateInWorldSpace, cameraYCoordinateInWorldSpace);
			matricesWVP.mView = XMMatrixTranspose(matricesWVP.mView);
			*/

			//изменение moveAheadVector в глобальной системе координат
			sseProxyRegister0_Matrix = XMMatrixRotationY(XM_PI * 0.0005 * mouseX);
			moveAheadVectorInGlobalCoord = XMVector3Transform(moveAheadVectorInGlobalCoord, sseProxyRegister0_Matrix);
			//изменение moveRightVector в глобальной системе координат
			moveRightVectorInGlobalCoord = XMVector3Transform(moveRightVectorInGlobalCoord, sseProxyRegister0_Matrix);
		}
		break; 
	}

	case(WM_PAINT): {
		// заполнение структуры ps, и очистка Update Region
		hdc = BeginPaint(hWnd, &ps);

		// очистка Update Region, и освобождение Device Context
		EndPaint(hWnd, &ps);
		break;
	} 
	case(WM_DESTROY): {
		//отправка сообщения WM_QUIT
		PostQuitMessage(0);
		break;
	}
	default:
		DefWindowProc(hWnd, message, wParam, lParam);
	}

	UpdateScene();
	DrawScene(objectsPositions);
	// проверка на столкновение камеры с статическими и динамическими хитбоксами
	// если камера перешла в другую область статических хитбоксов
	if (ChangesOfStaticHtBoxesArea) {
		DefineCurrentStaticHtBoxesArea();
	}
	//StaticHitBoxesCollisionDetection();
	DynamicHitBoxesCollisionDetection();

	return 0;
};

HRESULT CreateDirect3DComponents(UINT widthParam, UINT heightParam) {
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
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
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
		createDeviceDeviceContextSwapChainResult = D3D11CreateDeviceAndSwapChain(NULL, driverTypes[driverTypeIndex], NULL, D3D11_CREATE_DEVICE_DEBUG, featureLevels, numFeatureLevels, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext);
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
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;

	// Связывание view port с графическим конвейером
	g_pImmediateContext->RSSetViewports(1, &viewPort);

	// Описание depth stencil буфера
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = widthParam;
	depthStencilDesc.Height = heightParam;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; //DXGI_FORMAT_D32_FLOAT_S8X24_UINT DXGI_FORMAT_D24_UNORM_S8_UINT DXGI_FORMAT_D16_UNORM
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	g_pd3dDevice->CreateTexture2D(&depthStencilDesc, NULL, &depthStencilTexture);

	// описание того, как будет выполнятся z-test и stencil-test
	D3D11_DEPTH_STENCIL_DESC dsDesc;
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	dsDesc.StencilEnable = TRUE;
	dsDesc.StencilReadMask = 0xFF;
	dsDesc.StencilWriteMask = 0xFF;
	dsDesc.FrontFace = { D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_COMPARISON_ALWAYS };
	dsDesc.BackFace = { D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_STENCIL_OP_KEEP , D3D11_COMPARISON_NEVER };

	// создание состояния depth-stencil теста
	g_pd3dDevice->CreateDepthStencilState(&dsDesc, &pDSState);

	// связывание настроек depth-stencil теста с OM stage
	g_pImmediateContext->OMSetDepthStencilState(pDSState, 1);

	// описание depth-stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	descDSV.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Flags = NULL;
	descDSV.Texture2D.MipSlice = 0;

	// создание depth-stencil view
	g_pd3dDevice->CreateDepthStencilView(depthStencilTexture, &descDSV, &g_pDepthStencilView);

	// Привязка RTV и DSV к Output-Merger Stage
	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);

	// описание состояния растеризатора
	D3D11_RASTERIZER_DESC rasterizerState;
	ZeroMemory(&rasterizerState, sizeof(D3D11_RASTERIZER_DESC));
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.CullMode = D3D11_CULL_BACK;
	rasterizerState.DepthClipEnable = TRUE;

	// создание состояния растеризатора
	g_pd3dDevice->CreateRasterizerState(&rasterizerState, &pRasterizerState);

	// связывание состояния растеризатора с стадией растеризации
	g_pImmediateContext->RSSetState(pRasterizerState);

	return S_OK;
};

HRESULT MyCreateWindow(CONST WCHAR* wndClassNameParam, CONST WCHAR* wndNameParam, int widthParam, int heightParam, HINSTANCE hInstanceParam, int nShowCmdParam) {
	// Структура, описывающая класс Окна
	WNDCLASSEX wndClass;
	ZeroMemory(&wndClass, sizeof(WNDCLASSEX));
	// Размер структуры wndClass
	wndClass.cbSize = sizeof(WNDCLASSEX);
	// Стиль класса окна
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	// Указатель на функцию, обрабатывающую сообщения, поступающие окну
	wndClass.lpfnWndProc = StartUpWndProc;
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

	// регистрация устройств клавиатуры и мыши
	RAWINPUTDEVICE rid[2];
	// клавиатура
	rid[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[0].usUsage = HID_USAGE_GENERIC_KEYBOARD;
	rid[0].dwFlags = RIDEV_NOLEGACY;
	rid[0].hwndTarget = g_hWnd;
	// мышь
	rid[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
	rid[1].usUsage = HID_USAGE_GENERIC_MOUSE;
	rid[1].dwFlags = RIDEV_NOLEGACY;
	rid[1].hwndTarget = g_hWnd;
	RegisterRawInputDevices(rid, 2, sizeof(rid[0]));

	// Вывод окна на дисплей 
	ShowWindow(g_hWnd, nShowCmdParam);

	return S_OK;
};

void UpdateScene() {
	// обновление угла поворота пирамиды
	if (angleCBufferData.angle0 >= XM_2PI) {
		angleCBufferData.angle0 = angleCBufferData.angle0 - XM_2PI + ROTATION_ANGLE;
	}
	angleCBufferData.angle0 += ROTATION_ANGLE;

	/*
	D3D11_MAPPED_SUBRESOURCE angleCBufferUpdateData;
	g_pImmediateContext->Map(pAngleBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &angleCBufferUpdateData);
	memcpy(angleCBufferUpdateData.pData, &angleCBufferData, sizeof(float));
	g_pImmediateContext->Unmap(pAngleBuffer, 0);
	*/

	// обновление значений угла
	g_pImmediateContext->UpdateSubresource(constantBufferArray[1], 0, NULL, &angleCBufferData, 0, 0); 
	
	// обновление матрицы поворота вокруг оси
	// проверяю то, что сам написал
	//RotationAroundAxis(g_XMIdentityR1, g_XMZero, angleCBufferData.angle0, &matricesWVP.mRotationAroundAxis);
	//matricesWVP.mRotationAroundAxis = XMMatrixTranspose(matricesWVP.mRotationAroundAxis);

	// обновление матрицы поворота вокруг оси
	matricesWVP.mRotationAroundAxis = XMMatrixTranspose(XMMatrixRotationAxis(g_XMIdentityR1, angleCBufferData.angle0));
	/**/
	//изменение динамических хитбоксов
	for (int i = 0; i < DYNAMIC_HIT_BOX_AMOUNT; ++i) {
		// если хитбокс вышел за пределы карты
		if (DynamicHitBoxesArray[i].activeCyclesAmount == 0.0f) {
			//возвращаем хитбокс в точку его спавна
			DynamicHitBoxesArray[i].position = DynamicHitBoxesArray[i].startPosition;
			//новый вектор движения
			CreateNewMoveVectorForDynamicHitBox(&DynamicHitBoxesArray[i]);
			//через сколько циклов хитбокс снова покинет карту
			DynamicHitBoxesArray[i].activeCyclesAmount = DynamicHitBoxCyclesAmount(&DynamicHitBoxesArray[i]);
			// поворот хитбокса
			if (DynamicHitBoxesArray[i].angle >= XM_2PI) {
				DynamicHitBoxesArray[i].activeCyclesAmount += -XM_2PI + ROTATION_ANGLE;
			}
			DynamicHitBoxesArray[i].activeCyclesAmount += ROTATION_ANGLE;
			//измеяем позицию пирамиды
			objectsPositions[i] = XMLoadFloat3(&DynamicHitBoxesArray[i].startPosition);
		}
		else {
			MoveAndRotationDynamicHitBox(*DynamicHitBoxesArray[i].moveVectorPtr, ROTATION_ANGLE, &DynamicHitBoxesArray[i]);
			--DynamicHitBoxesArray[i].activeCyclesAmount;
			//измеяем позицию пирамиды
			objectsPositions[i] = XMLoadFloat3(&DynamicHitBoxesArray[i].position);
		}
	}
};

void DrawScene(XMVECTOR* objectsPositionsArray) {
	// Цвет пикселя
	const FLOAT backgroundColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };

	// Устанавливает цвет всеч пикселей поверхности RTV к единому значению
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, backgroundColor);

	// оччистка depth-stencil буфера
	g_pImmediateContext->ClearDepthStencilView(g_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	//отрисовка каждой пирамиды
	for (int i = 0; i < 3; ++i) {
		SetWorldMatrix(objectsPositionsArray[i], XMVECTORF32{ 1.0f, 1.0f, 1.0f, 0.0f }, & matricesWVP.mWorld);
		g_pImmediateContext->UpdateSubresource(constantBufferArray[0], 0, NULL, &matricesWVP, 0, 0);

		g_pImmediateContext->DrawIndexed(12, 0, 0);
	}
	// отрисовка примитивов 
	//g_pImmediateContext->DrawIndexed(12, 0, 0);

	// Вывод на дисплей поверхности Back Buffer
	g_pSwapChain->Present(0, 0);

	g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, g_pDepthStencilView);
};

HRESULT CompileShader(LPCWSTR srcName, LPCSTR entryPoint, LPCSTR target, ID3DBlob** buffer) {
	HRESULT hr;
	ID3DBlob* errorsBuffer = NULL;
	HANDLE errorsFileHandle = NULL;
	DWORD bytesWritten = NULL;

	hr = D3DCompileFromFile(srcName, NULL, NULL, entryPoint, target, D3DCOMPILE_DEBUG, NULL, buffer, &errorsBuffer);
	// вывод ошибок компиляции, если они есть
	if (FAILED(hr)) {
		if (errorsBuffer != NULL) {
			// создаем или открываем файл, где будет находится текст с ошибками компиляции шейдера
			errorsFileHandle = CreateFile(L"shaderErrors.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			// записываем ошибки компиляции в файл
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

	// описание vertex buffer
	D3D11_BUFFER_DESC vertexBufferDesc;
	// размер буфера
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 4;
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
	UINT stride[] = { sizeof(Vertex) };
	// смещение до первого элемента конкретного вершинного буфера, который будет использован
	UINT offset[] = { 0 };
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pVertexBuffer, stride, offset);

	// указание какие примитивы собирать из вершинного буфера
	g_pImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// компиляция шейдеров
	SHADERS_MODEL(g_featureLevel)

	// компиляция вершинного шейдера
	hr = CompileShader(vertexShaderName, vsShaderEntryPoint, shadersModel.vertexShaderModel, &VS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}
	// компиляция пиксельного шейдера
	hr = CompileShader(pixelShaderName, psShaderEntryPoint, shadersModel.pixelShaderModel, &PS_Buffer);
	if (FAILED(hr)) {
		return hr;
	}

	// создание объекта вершинного шейдера
	hr = g_pd3dDevice->CreateVertexShader(VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), NULL, &g_pVertexShader);
	// создание объекта пиксельного шейдера
	hr = g_pd3dDevice->CreatePixelShader(PS_Buffer->GetBufferPointer(), PS_Buffer->GetBufferSize(), NULL, &g_pPixelShader);
	
	// привязка вершинного шейдера к конвейеру
	g_pImmediateContext->VSSetShader(g_pVertexShader, NULL, NULL);
	// привязка пиксельного шейдера к конвейеру
	g_pImmediateContext->PSSetShader(g_pPixelShader, NULL, NULL);
	
	// Описание Input-Layout Object
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	
	// Количество элементов в Input-Layout Object
	UINT numInputLayoutObject = sizeof(layout) / sizeof(D3D11_INPUT_ELEMENT_DESC);

	// Создание Input-Layout Object
	hr = g_pd3dDevice->CreateInputLayout(layout, numInputLayoutObject, VS_Buffer->GetBufferPointer(), VS_Buffer->GetBufferSize(), &g_pInputLayoutObject);

	// Связывание Input-layout object с конвейером
	g_pImmediateContext->IASetInputLayout(g_pInputLayoutObject);

	return S_OK;
}

HRESULT InitMatrices(WORD* indices) {
	HRESULT hr;

	// описание константного буфера матриц
	D3D11_BUFFER_DESC constantBufferDesc;
	constantBufferDesc.ByteWidth = sizeof(MatricesBuffer);
	constantBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	constantBufferDesc.CPUAccessFlags = 0;
	constantBufferDesc.MiscFlags = 0;
	constantBufferDesc.StructureByteStride = 0;

	// создание константного буфера матриц
	hr = g_pd3dDevice->CreateBuffer(&constantBufferDesc, NULL, &constantBufferArray[0]);
	if (FAILED(hr)) {
		return hr;
	}

	// описание буфера угла
	D3D11_BUFFER_DESC angleBufferDesc;
	angleBufferDesc.ByteWidth = sizeof(AngleConstantBuffer);
	angleBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	angleBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	angleBufferDesc.CPUAccessFlags = 0;
	angleBufferDesc.MiscFlags = 0;
	angleBufferDesc.StructureByteStride = 0;

	// описание инициализирующей информации буфера угла
	D3D11_SUBRESOURCE_DATA angleBufferInitData;
	angleBufferInitData.pSysMem = &angleCBufferData;
	angleBufferInitData.SysMemPitch = 0;
	angleBufferInitData.SysMemSlicePitch = 0;

	// создание буфера угла
	hr = g_pd3dDevice->CreateBuffer(&angleBufferDesc, &angleBufferInitData, &constantBufferArray[1]);
	if (FAILED(hr)) {
		return hr;
	}

	// связывание констнатного буфера с шейдером вершин
	g_pImmediateContext->VSSetConstantBuffers(0, 2, constantBufferArray);
	/*
	//описание ресурса вершинного шейдера
	D3D11_SHADER_RESOURCE_VIEW_DESC angleBufferVSResorceDesc;
	angleBufferVSResorceDesc.Format = DXGI_FORMAT_R32_FLOAT;
	angleBufferVSResorceDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	angleBufferVSResorceDesc.Buffer.FirstElement = 0;
	angleBufferVSResorceDesc.Buffer.NumElements = 1;

	//создание ресурса вершинного шейдера
	hr = g_pd3dDevice->CreateShaderResourceView(pAngleBuffer, &angleBufferVSResorceDesc, &pAngleBufferVSResource);
	if (FAILED(hr)) {
		return hr;
	}

	// привязка ресурса с углом к шейдеру вершин
	g_pImmediateContext->VSSetShaderResources(0, 1, &pAngleBufferVSResource);
	*/
	// описание индекс буфера
	D3D11_BUFFER_DESC indexBufferDesc;
	indexBufferDesc.ByteWidth = sizeof(WORD) * 4 * 3;
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// значения индекс буфера
	D3D11_SUBRESOURCE_DATA indexBufferInitData;
	indexBufferInitData.pSysMem = indices;
	indexBufferInitData.SysMemPitch = 0;
	indexBufferInitData.SysMemSlicePitch = 0;

	//создание индекс буфера
	hr = g_pd3dDevice->CreateBuffer(&indexBufferDesc, &indexBufferInitData, &pIndexBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	//привязка индекс буфера к конвейеру
	g_pImmediateContext->IASetIndexBuffer(pIndexBuffer, DXGI_FORMAT_R16_UINT, 0);

	return S_OK;
};

void SetProjectionMatrix(MatricesBuffer* pMatricesBuffer, FLOAT angleHoriz, FLOAT angleVert, FLOAT nearZ, FLOAT farZ, BOOL saveProportionsFlag) {
	// (0 < angle < PI/2) => (0 < tg(angle) < +inf) => (0 < newCoeff < 1)

	FLOAT sinAngle;
	FLOAT cosAngle;
	FLOAT tangentAngle;
	FLOAT newCoeff;

	//FLOAT nearZ = 2.5f; // ближняя плоскоть отсечения
	//FLOAT farZ = 3.2f; // дальняя плоскость отсечения
	//FLOAT mulCoeff = (farZ - nearZ + 1.0f) / (farZ - nearZ);

	FLOAT zCoeff = farZ / (farZ - nearZ);
	//FLOAT wCoeff = farZ - farZ * zCoeff;
	FLOAT wCoeff = -nearZ * zCoeff;


	if (saveProportionsFlag == false) {
		XMScalarSinCos(&sinAngle, &cosAngle, angleHoriz);
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection.r[0] = XMVectorSet(newCoeff, 0.0f, 0.0f, 0.0f);

		XMScalarSinCos(&sinAngle, &cosAngle, angleVert);
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection.r[1] = XMVectorSet(0.0f, newCoeff, 0.0f, 0.0f);

		pMatricesBuffer->mProjection.r[2] = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		pMatricesBuffer->mProjection.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		pMatricesBuffer->mProjection = XMMatrixTranspose(pMatricesBuffer->mProjection);
		g_pImmediateContext->UpdateSubresource(constantBufferArray[0], 0, 0, pMatricesBuffer, 0, 0);
	}
	else {
		XMScalarSinCos(&sinAngle, &cosAngle, MaxElement(angleHoriz, angleVert));
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection.r[0] = XMVectorSet(newCoeff, 0.0f, 0.0f, 0.0f);
		pMatricesBuffer->mProjection.r[1] = XMVectorSet(0.0f, newCoeff, 0.0f, 0.0f);
		pMatricesBuffer->mProjection.r[2] = XMVectorSet(0.0f, 0.0f, zCoeff * 0.25f * newCoeff, 0.25f * newCoeff); // 0.25f * newCoeff
		pMatricesBuffer->mProjection.r[3] = XMVectorSet(0.0f, 0.0f, wCoeff, 0.0f);

		SaveProportions(pMatricesBuffer, g_hWnd);
	}
};

void SetProjectionMatrixWithCameraDistance(MatricesBuffer* pMatricesBuffer, FLOAT angleHoriz, FLOAT angleVert, FLOAT nearZ, FLOAT farZ, FLOAT cameraDistance, BOOL saveProportionsFlag) {
	// (0 < angle < PI/2) => (0 < tg(angle) < +inf) => (0 < newCoeff < 1)

	FLOAT sinAngle;
	FLOAT cosAngle;
	FLOAT tangentAngle;
	FLOAT newCoeff;

	//nearZ = 0.6f; // ближняя плоскоть отсечения, деленная на W координату
	//farZ = 4.2f; // дальняя плоскость отсечения, деленная на W координату

	FLOAT zCoeff = (farZ - nearZ + cameraDistance) / (farZ - nearZ);
	FLOAT wCoeff = farZ - nearZ + cameraDistance - farZ * zCoeff;

	if (saveProportionsFlag == false) {
		XMScalarSinCos(&sinAngle, &cosAngle, angleHoriz);
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection.r[0] = XMVectorSet(newCoeff, 0.0f, 0.0f, 0.0f);

		XMScalarSinCos(&sinAngle, &cosAngle, angleVert);
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection.r[1] = XMVectorSet(0.0f, newCoeff, 0.0f, 0.0f);

		pMatricesBuffer->mProjection.r[2] = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
		pMatricesBuffer->mProjection.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		pMatricesBuffer->mProjection = XMMatrixTranspose(pMatricesBuffer->mProjection);
		g_pImmediateContext->UpdateSubresource(constantBufferArray[0], 0, 0, pMatricesBuffer, 0, 0);
	}
	else {
		XMScalarSinCos(&sinAngle, &cosAngle, MaxElement(angleHoriz, angleVert));
		tangentAngle = sinAngle / cosAngle;
		newCoeff = 1.0f / (1.0f + tangentAngle);
		pMatricesBuffer->mProjection.r[0] = XMVectorSet(newCoeff, 0.0f, 0.0f, 0.0f);
		pMatricesBuffer->mProjection.r[1] = XMVectorSet(0.0f, newCoeff, 0.0f, 0.0f);
		pMatricesBuffer->mProjection.r[2] = XMVectorSet(0.0f, 0.0f, zCoeff * 0.25f * newCoeff, 0.25f * newCoeff); // 0.25f * newCoeff
		pMatricesBuffer->mProjection.r[3] = XMVectorSet(0.0f, 0.0f, wCoeff, cameraDistance - nearZ);

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
		//pMatricesBuffer->mProjection._22 *=  1 / windowCoeff;
		pMatricesBuffer->mProjection.r[1] = _mm_mul_ps(pMatricesBuffer->mProjection.r[1], XMVectorSet(1.0f, 1.0f / windowCoeff, 1.0f, 1.0f));
	}
	else
	{
		//pMatricesBuffer->mProjection._11 *= windowCoeff;
		pMatricesBuffer->mProjection.r[0] = _mm_mul_ps(pMatricesBuffer->mProjection.r[0], XMVectorSet(windowCoeff, 1.0f, 1.0f, 1.0f));
	}

	pMatricesBuffer->mProjection = XMMatrixTranspose(pMatricesBuffer->mProjection);
	g_pImmediateContext->UpdateSubresource(constantBufferArray[0], 0, 0, pMatricesBuffer, 0, 0);
};

FLOAT MaxElement(FLOAT arg0, FLOAT arg1) {
	if (arg0 >= arg1) {
		return arg0;
	}
	return arg1;
};

HRESULT InvertMatrix(XMVECTOR zAxis, XMVECTOR yAxis, XMMATRIX* invertibleMatrix) {
	// проверка векторов на ортогональность 
	XMFLOAT4 dotProduct;
	XMStoreFloat4(&dotProduct, XMVector3Dot(zAxis, yAxis));
	if (dotProduct.x != 0.0f) {
		return E_FAIL;
	}
	
	// нахождение новой оси X
	XMVECTOR xAxis = XMVector3Cross(yAxis, zAxis); // правая тройка векторов
	// векторы линейно независимы, значит можно искать обратную матрицу

	// матрица обратная обратной матрице, обычная матрица
	XMVECTOR axisArray[] = { xAxis, yAxis, zAxis };

	// порядок строк в обратной матрице, который получился в результате поиска обратной матрицы
	int invertibleMatrixRowsOrder[3];

	// единичная матрица
	XMMATRIX invertibleMatrixOutput = {
	 XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f),
	 XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f),
	 XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f),
	 XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
	};

	//поиск обратной матрицы методом гаусса
	for (size_t axisIndex = 0; axisIndex <= 1; ++axisIndex) { // обход вниз по строкам обычной матрицы
		for (size_t i = 0; i <= 2; ++i) { // обход по координатам строки обычной матицы
			float axisElement = XMVectorGetByIndex(axisArray[axisIndex], i);
			if (axisElement != 0) {
				axisArray[axisIndex] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
				invertibleMatrixOutput.r[axisIndex] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
				invertibleMatrixRowsOrder[axisIndex] = i;

				for (size_t nextAxisIndex = axisIndex + 1; nextAxisIndex <= 2; ++nextAxisIndex) { // прибавление строки к другим строкам
					float nextAxisElement = XMVectorGetByIndex(axisArray[nextAxisIndex], i);
					XMVECTOR mulVector = axisArray[axisIndex] * XMVectorSet(-nextAxisElement, -nextAxisElement, -nextAxisElement, -nextAxisElement); // вместо простого умножения axisArray[axisIndex] на -nextAxisElement приходится умножать на вектор, так как в ином случае возникает сильная погрешность
					XMVECTOR invertMatrixMulVector = invertibleMatrixOutput.r[axisIndex] * XMVectorSet(-nextAxisElement, -nextAxisElement, -nextAxisElement, -nextAxisElement);

					axisArray[nextAxisIndex] += mulVector;
					invertibleMatrixOutput.r[nextAxisIndex] += invertMatrixMulVector;
				}
				break;
			}
		}
	};
	for (size_t i = 0; i <= 2; ++i) { // обход полседней строки обычной матрицы
		float axisElement = XMVectorGetByIndex(axisArray[2], i);
		if (axisElement != 0) {
			axisArray[2] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
			invertibleMatrixOutput.r[2] /= XMVectorSet(axisElement, axisElement, axisElement, axisElement);
			invertibleMatrixRowsOrder[2] = i;

			break;
		}
	};
	for (size_t axisIndex = 2; axisIndex >= 1; --axisIndex) { // обход вверх по строкам обычной матрицы
		for (size_t i = 0; i <= 2; ++i) { // обход по координатам строки обычной матицы
			float axisElement = XMVectorGetByIndex(axisArray[axisIndex], i);
			if (axisElement != 0) {
				for (size_t nextAxisIndex = axisIndex - 1; nextAxisIndex < UINT_MAX; --nextAxisIndex) { // прибавление строки к другим строкам
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

	//преобразование обратной матрицы к правильному порядку строк
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
	// при умножении вершины на матрицу перехода к новой системе координат вычитаем начало координат новой системы координат
	point *= XMVectorSet(-1.0f, -1.0f, -1.0f, 1.0f);
	*invertibleMatrix = XMMatrixTranslationFromVector(point) * (*invertibleMatrix);

	return S_OK;
};

void SetWorldMatrix(XMVECTOR point, XMVECTOR scale, XMMATRIX* worldMatrix) {
	*worldMatrix = XMMatrixTranspose(XMMatrixScalingFromVector(scale) * XMMatrixTranslationFromVector(point));
};

void InvertIndices(WORD* indicesArray, int size) {
	WORD triangleEnd;

	for (int i = 2; i < size; i = i + 3) {
		triangleEnd = indicesArray[i];

		indicesArray[i] = indicesArray[i-2];
		indicesArray[i - 2] = triangleEnd;
	}
};

XMVECTOR FindOrthogonalVector(XMVECTOR vector) { // скалярное произведение ортогональных векторов равно нулю, на использовании этого факта и строится поиск ортогонального вектора
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
			// если координата x не ноль
			if (i == 0) {
				float zOfFirstVector = XMVectorGetByIndex(vector, 2);
				orthogonalVector.z = coordinate / (sqrt(coordinate * coordinate + zOfFirstVector * zOfFirstVector));
				orthogonalVector.x = (-orthogonalVector.z * zOfFirstVector) / coordinate;

				return XMVectorSet(orthogonalVector.x, 0.0f, orthogonalVector.z, 0.0f);
			}

			// если координата y не ноль
			if (i == 1) {
				float zOfFirstVector = XMVectorGetByIndex(vector, 2);
				orthogonalVector.z = coordinate / (sqrt(coordinate * coordinate + zOfFirstVector * zOfFirstVector));
				orthogonalVector.y = (-orthogonalVector.z * zOfFirstVector) / coordinate;

				return XMVectorSet(0.0f, orthogonalVector.y, orthogonalVector.z, 0.0f);
			}

			// если координата z не ноль
			if (i == 2) {
				float yOfFirstVector = XMVectorGetByIndex(vector, 1);
				orthogonalVector.y = coordinate / (sqrt(coordinate * coordinate + yOfFirstVector * yOfFirstVector));
				orthogonalVector.z = (-orthogonalVector.y * yOfFirstVector) / coordinate;

				return XMVectorSet(0.0f, orthogonalVector.y, orthogonalVector.z, 0.0f);
			}
		}
	}
	// если дан нулевой вектор
	return XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
};

void RotationAroundAxis(XMVECTOR yAxis, XMVECTOR point, FLOAT angle, XMMATRIX* outputMatrix) {
	yAxis = XMVector3Normalize(yAxis);
	XMVECTOR zAxis = FindOrthogonalNormalizedVector(yAxis);
	XMMATRIX offsetMatrix = XMMatrixTranslationFromVector(_mm_mul_ps(point, XMVectorSet(-1.0f, -1.0f, -1.0f, 0.0f))); // матрица переноса вершины, которая будет поворачиваться вокруг оси, на -point
	XMMATRIX backOffsetMatrix = XMMatrixTranslationFromVector(point); // матрица переноса перенесенной вершины, которую уже повернули вокруг оси, на +point

	// переход координат вершины от координат в старом базисе к координатам в новом базисе, где заданная ось является осью Y
	XMMATRIX newCoordinates;
	NewCoordinateSystemMatrix(g_XMZero, zAxis, yAxis, &newCoordinates); // вектор xAxis, который находится в этой функции будет нормализованным, так как вектор, полученный
																	 // в результате векторного произведения нормализованных и ортогональных векторов тоже будет нормализованным  

	// так как заданная ось является осью Y, то и вращение будет происходить вокруг оси Y
	XMMATRIX yAxisRotationMatrix = XMMatrixRotationY(angle);

	// матрица возврата к старым координатам
	XMMATRIX transformationMatrix = {
		XMVector3Cross(yAxis, zAxis),
		yAxis,
		zAxis,
		g_XMIdentityR3 };

	// можно перемножить все матрицы, так как перемножение матриц ассоциативно, и получить одну матрицу. Не придется в шейдере на каждую вершину делать несколько перемножений матриц. 
	*outputMatrix = offsetMatrix * newCoordinates * yAxisRotationMatrix * transformationMatrix * backOffsetMatrix; 
}

void InitPageSizeAndAllocGranularityVariables() {
	SYSTEM_INFO systemInfo;
	GetNativeSystemInfo(&systemInfo);
	pageSize = systemInfo.dwPageSize;
	allocationGranularity = systemInfo.dwAllocationGranularity;
}

bool ChangesOfStaticHtBoxesArea() {
	// если позиция камеры находится внутри текущей области статических хитбоксов
	if ((currentCameraPos.x >= currentHitBoxArea->lowX) and
		(currentCameraPos.x <= currentHitBoxArea->highX) and
		(currentCameraPos.z >= currentHitBoxArea->lowZ) and
		(currentCameraPos.z <= currentHitBoxArea->highZ))
	{
		return false;
	}
		return true;
};

void DefineCurrentStaticHtBoxesArea() {
	// так как нет корня в бинарном дереве, то приходится отдельно проверять в какой из двух первых(0 и 1) областей 
	// находится камера
	// если камера находится внутри нулевой области статических хитбоксов
	if ((currentCameraPos.x >= StaticHitBoxAreaArray[0].lowX) and
		(currentCameraPos.x <= StaticHitBoxAreaArray[0].highX) and
		(currentCameraPos.z >= StaticHitBoxAreaArray[0].lowZ) and
		(currentCameraPos.z <= StaticHitBoxAreaArray[0].highZ)) 
	{
		currentHitBoxArea = &StaticHitBoxAreaArray[0];
	}
	else {
		currentHitBoxArea = &StaticHitBoxAreaArray[1];
	}

	// пока не дошли до листа продолжаем определять в какой области находится камера
	while (currentHitBoxArea->leftNode != NULL) {
		// если камера находится внутри нулевой области статических хитбоксов
		if ((currentCameraPos.x >= currentHitBoxArea->leftNode->lowX) and
			(currentCameraPos.x <= currentHitBoxArea->leftNode->highX) and
			(currentCameraPos.z >= currentHitBoxArea->leftNode->lowZ) and
			(currentCameraPos.z <= currentHitBoxArea->leftNode->highZ)) 
		{
			currentHitBoxArea = currentHitBoxArea->leftNode;
		}
		else {
			currentHitBoxArea = currentHitBoxArea->rightNode;
		}
	}
};

void StaticHitBoxesCollisionDetection() {
	// кладем текущую позицую камеры
	sseProxyRegister0 = XMLoadFloat3(&currentCameraPos);

	// если позиция камеры оказалась внутри хитбокса(т.е. произошло столкновение), значит нужно камеру вытолкнуть за пределы хитбокса
	// обходим все хибоксы из текущей области хитбоксов
	for (int i = 0; i < currentHitBoxArea->staticHitBoxesAmount; ++i) { 
		// смещаем и поворачиваем текущую позиция камеры
		sseProxyRegister0 = XMVector3Transform(sseProxyRegister0, *(currentHitBoxArea->staticHitBoxesArray[i]->angleMatrixRotation));
		
		// кладем ширину высоту длину хитбокса
		sseProxyRegister1 = XMLoadFloat3(&currentHitBoxArea->staticHitBoxesArray[i]->widthHeightLength);
		// если 0 <= x <= width && 0 <= y <= height && 0 <= z <= length, где xyz принадлежат текущей позиции камеры, а width height length - хитбоксу
		// то есть если позиция камеры внутри хитбокса
		if (XMVector3LessOrEqual(sseProxyRegister0, sseProxyRegister1) and
			XMVector3GreaterOrEqual(sseProxyRegister0, g_XMIdentityR3))
		{
			// находим расстояние позиции камеры до правых верхних граней хитбокса
			sseProxyRegister1 = sseProxyRegister1 - sseProxyRegister0;

			// находим минимум между расстоянием позиции камеры до нижних левых граней хитбокса(это координаты позиции камеры) и 
			// расстоянием позици камеры до верхних правых граней хитбокса
			sseProxyRegister2 = XMVectorMin(sseProxyRegister0, sseProxyRegister1);

			float minX = XMVectorGetX(sseProxyRegister2);
			float minZ = XMVectorGetZ(sseProxyRegister2);

			// если минимальное расстояние до граней находится по оси X
			if (minX < minZ) {
				//если минимальное расстояние явлется минимальным расстоянием от точки до левой грани хитбокса
				if (minX == XMVectorGetX(sseProxyRegister0)) {
					sseProxyRegister0 = XMVectorSetX(sseProxyRegister0, -0.005f); // 0.0005 это расстояние на которое отодвигается камера от грани хитбокса
				} 
				else { //если минимальное расстояние явлется минимальным расстоянием от точки до правой грани хитбокса
					sseProxyRegister1 = XMVectorSetX(sseProxyRegister1, 0.005f);
					sseProxyRegister2 += sseProxyRegister1;
					sseProxyRegister0 += XMVectorAndInt(sseProxyRegister2, g_XMMaskX);
				}
			}
			else {// если минимальное расстояние до граней находится по оси Z
				//если минимальное расстояние явлется минимальным расстоянием от точки до нижней грани хитбокса
				if (minZ == XMVectorGetZ(sseProxyRegister0)) {
					sseProxyRegister0 = XMVectorSetZ(sseProxyRegister0, -0.005f);
				}
				else { //если минимальное расстояние явлется минимальным расстоянием от точки до верхней грани хитбокса
					sseProxyRegister1 = XMVectorSetZ(sseProxyRegister1, 0.005f);
					sseProxyRegister2 += sseProxyRegister1;
					sseProxyRegister0 += XMVectorAndInt(sseProxyRegister2, g_XMMaskZ);
				}
			}
			// умножить на обратную матрицу
			sseProxyRegister0 = XMVector3Transform(sseProxyRegister0, *(currentHitBoxArea->staticHitBoxesArray[i]->invertAngleMatrixRotation));
			// сохранить измененную позицию в currentCameraPosition
			XMStoreFloat3(&currentCameraPos, sseProxyRegister0);

			//меняем view matrix, так чтобы она описывала систему координат в новой, смещенной точке
			//находим координаты новой позиции в текущей системе координат камеры
			sseProxyRegister0 = XMVector3Transform(sseProxyRegister0, matricesWVP.mView);
			//переносим систему координат камеры в новую точку
			matricesWVP.mView.r[3] -= sseProxyRegister0;
		}
	}
};

void InitHitBoxes() {
	// предположим, что размер страницы всегда является степенью двойки
	// тогда кол-во страниц, необходимое для того чтобы вместить все хитбоксы, можно вычислить так:

	//определяем есть ли выравнивание по 4 байта у гранулярности выделения памяти
	// остаток от деления гранулярности на 4 байта
	DWORD allocationGranularityAlignModulo = allocationGranularity & (4UL - 1UL);
	// дополнительные байты чтобы адрес выделенной памяти был выровнен
	SIZE_T extraBytes = 0;

	if (allocationGranularityAlignModulo != 0UL) { // если адрес выделяемой памяти невыровнен 
		extraBytes += (4UL - allocationGranularityAlignModulo);
	}
	// p.s. хотя наверно в windows всегда гранулярность выделения памяти через virtualalloc будет кратна 4 байтам

	// размер памяти, занимаемы хитбоксами
	SIZE_T hitBoxAmount = sizeof(extraBytes +
		STATIC_HIT_BOX_AMOUNT * sizeof(HitBox) +
		STATIC_HIT_BOX_AREA_AMOUNT * sizeof(StaticHitBoxArea) +
		STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_0 * sizeof(HitBox*) +
		STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_1 * sizeof(HitBox*) +
		STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_2 * sizeof(HitBox*) +
		STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_3 * sizeof(HitBox*) +
		DYNAMIC_HIT_BOX_AMOUNT * sizeof(DynamicHitBox));

	// количество страниц, необходимое для того чтобы вместить все хитбоксы
	pageAmount = hitBoxAmount & (0xFFFFFFFF ^ (pageSize - 1)); // сейчас здесь хранится только целая часть от деления hitBoxAmount на pageSize
	//SIZE_T moduloPart = hitBoxAmount & (pageSize - 1);
	//SIZE_T integerPart = hitBoxAmount & (0xFFFFFFFF ^ (pageSize - 1));

	// если остаток от деления hitBoxAmount на pageSize не ноль
	if ((hitBoxAmount & (pageSize - 1UL)) != 0UL) {
		++pageAmount;
	}
	// p.s. конечно легче и понятнее было бы посчитать количество занимаемх страниц памяти с помощью div и mod

	// выделение памяти под хитбоксы
	// сырая память с учетом байтов для выравнивания
	dynamicMemory = VirtualAlloc(NULL, pageAmount * pageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	// указатель на массив статических хитбоксов, выровненный по границе 4 байта
	StaticHitBoxArray = (HitBox*)((BYTE*)dynamicMemory + extraBytes);
	// указатель на массив StaticHitBoxArea структур
	StaticHitBoxAreaArray = (StaticHitBoxArea*)(StaticHitBoxArray + STATIC_HIT_BOX_AMOUNT);
	// указатели на массивы позиций в StaticHitBoxDescArray, которые определяют какие статические хитбоксы есть в листьях staticHitBoxArea
	// 0 лист
	Leaf0Array = (HitBox**)(StaticHitBoxAreaArray + STATIC_HIT_BOX_AREA_AMOUNT);
	// 1 лист
	Leaf1Array = Leaf0Array + STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_0;
	// 2 лист
	Leaf2Array = Leaf1Array + STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_1;
	// 3 лист
	Leaf3Array = Leaf2Array + STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_2;

	// кладем данные в массивы статических хитбоксов
	// 0 хитбокс
	StaticHitBoxArray[0] = {
		0.0f,
		55.0f,
		{100.0f, 0.0f, 5.0f},
		&staticHitBoxesRotationMatricesArray[0],
		&invertStaticHitBoxesRotationMatricesArray[0]
	};
	// 1 хитбокс
	StaticHitBoxArray[1] = {
		55.0f,
		0.0f,
		{5.0f, 0.0f, 100.0f},
		&staticHitBoxesRotationMatricesArray[1],
		&invertStaticHitBoxesRotationMatricesArray[1]
	};
	// 2 хитбокс
	StaticHitBoxArray[2] = {
		0.0f,
		-55.0f,
		{100.0f, 0.0f, 5.0f},
		&staticHitBoxesRotationMatricesArray[2],
		&invertStaticHitBoxesRotationMatricesArray[2]
	};
	// 3 хитбокс
	StaticHitBoxArray[3] = {
		-55.0f,
		0.0f,
		{5.0f, 0.0f, 100.0f},
		&staticHitBoxesRotationMatricesArray[3],
		&invertStaticHitBoxesRotationMatricesArray[3]
	};

	// заполняем массивы листьев зон статических хитбоксов
	// массив 0 листа
	Leaf0Array[0] = &StaticHitBoxArray[0];
	Leaf0Array[1] = &StaticHitBoxArray[3];
	// массив 1 листа
	Leaf1Array[0] = &StaticHitBoxArray[0];
	Leaf1Array[1] = &StaticHitBoxArray[1];
	// массив 2 листа
	Leaf2Array[0] = &StaticHitBoxArray[2];
	Leaf2Array[1] = &StaticHitBoxArray[3];
	// массив 3 листа
	Leaf3Array[0] = &StaticHitBoxArray[1];
	Leaf3Array[1] = &StaticHitBoxArray[2];

	// кладем данные в массив областей статических хитбоксов
	// 0 область
	StaticHitBoxAreaArray[0] = {
		&StaticHitBoxAreaArray[2],
		&StaticHitBoxAreaArray[3],
		NULL,
		0.0f, // center
		25.0f,
		-50.0f, //lX hX
		50.0f,
		0.0f, // lZ hZ
		50.0f,
		2,
		0
	};
	// 1 область
	StaticHitBoxAreaArray[1] = {
		&StaticHitBoxAreaArray[4],
		&StaticHitBoxAreaArray[5],
		NULL,
		0.0f, // center
		-25.0f,
		-50.0f, //lX hX
		50.0f,
		-50.0f, // lZ hZ
		0.0f,
		2,
		1
	};
	// 2 область
	StaticHitBoxAreaArray[2] = {
		NULL,
		(StaticHitBoxArea*)Leaf0Array,
		&StaticHitBoxAreaArray[0],
		-25.0f, // center
		25.0f,
		-50.0f, //lX hX
		0.0f,
		0.0f, // lZ hZ
		50.0f,
		2,
		0
	};
	// 3 область
	StaticHitBoxAreaArray[3] = {
		NULL,
		(StaticHitBoxArea*)Leaf1Array,
		&StaticHitBoxAreaArray[0],
		25.0f, // center
		25.0f,
		0.0f, //lX hX
		50.0f,
		0.0f, // lZ hZ
		50.0f,
		2,
		1
	};
	// 4 область
	StaticHitBoxAreaArray[4] = {
		NULL,
		(StaticHitBoxArea*)Leaf2Array,
		&StaticHitBoxAreaArray[1],
		-25.0f, // center
		-25.0f,
		-50.0f, //lX hX
		0.0f,
		-50.0f, // lZ hZ
		0.0f,
		2,
		0
	};
	// 5 область
	StaticHitBoxAreaArray[5] = {
		NULL,
		(StaticHitBoxArea*)Leaf3Array,
		&StaticHitBoxAreaArray[1],
		25.0f, // center
		-25.0f,
		0.0f, //lX hX
		50.0f,
		-50.0f, // lZ hZ
		0.0f,
		2,
		1
	};
    
	// указатель на массив динмических хитбоксов
    DynamicHitBoxesArray = (DynamicHitBox*)(Leaf3Array + STATIC_HIT_BOX_AMOUNT_IN_STATIC_HIT_BOX_AREA_LEAF_3);
    
	//0 динмический хибокс
    DynamicHitBoxesArray[0] = {
        3.0f,
        {-25.0f, 0.0f, 25.0f},
        {2.0f, 0.0f, 2.0f},
        0.0f,
		&DynamicHitBoxesMoveVectorsArray[0],
		0.0f,
		{-25.0f, 0.0f, 25.0f}
    };
	//1 динмический хибокс
     DynamicHitBoxesArray[1] = {
        3.0f,
        {25.0f, 0.0f, 25.0f},
        {2.0f, 0.0f, 2.0f},
        0.0f,
		&DynamicHitBoxesMoveVectorsArray[1],
		0.0f,
		{25.0f, 0.0f, 25.0f}
    };
	 //2 динмический хибокс
     DynamicHitBoxesArray[2] = {
        3.0f,
        {0.0f, 0.0f, -25.0f},
        {2.0f, 0.0f, 2.0f},
        0.0f,
		&DynamicHitBoxesMoveVectorsArray[2],
		0.0f,
		{0.0f, 0.0f, -25.0f}
    };

	 //инициализация векторов движения динамических хитбоксов
	 InitMoveVectorsAndActiveCyclesAmountForDynamicHitBoxes();
};

inline void __vectorcall MoveAndRotationDynamicHitBox(FXMVECTOR moveVector, float rotationAngle, DynamicHitBox* dynamicHitBox) {
	// перемещаем хитбокс
	sseProxyRegister0 = XMLoadFloat3(&(dynamicHitBox->position));
	sseProxyRegister0 += moveVector;
	XMStoreFloat3(&(dynamicHitBox->position), sseProxyRegister0);

	// изменяем угол
	if(dynamicHitBox->angle >= XM_2PI){
		dynamicHitBox->angle += -XM_2PI + rotationAngle;
	}
	dynamicHitBox->angle += rotationAngle;
};

void DynamicHitBoxesCollisionDetection() {
	// текущаяя позиция камеры
	sseProxyRegister0 = XMLoadFloat3(&currentCameraPos);

	// обходим все динамические хитбоксы
	for (int i = 0; i < DYNAMIC_HIT_BOX_AMOUNT; ++i) {
		// позиция хитбокса
		sseProxyRegister4 = XMLoadFloat3(&DynamicHitBoxesArray[i].position);
		// координаты камеры относительно системы координат в центре хитбокса
		sseProxyRegister1 = sseProxyRegister0 - sseProxyRegister4;
		// положительные границы обертки динмаического хитбокса
		sseProxyRegister2 = XMVectorSet(DynamicHitBoxesArray[i].halfOfShellEdge, 0.0f, DynamicHitBoxesArray[i].halfOfShellEdge, 0.0f);
		// отрицательные границы обертки хитбокса
		sseProxyRegister3 = XMVectorNegate(sseProxyRegister2);

		// если камера находится внутри обертки хитбокса
		if (XMVector3LessOrEqual(sseProxyRegister1, sseProxyRegister2) and
			XMVector3GreaterOrEqual(sseProxyRegister1, sseProxyRegister3)) 
		{
			// поворот позиции камеры
			sseProxyRegister0_Matrix = XMMatrixRotationY(-DynamicHitBoxesArray[i].angle);
			sseProxyRegister1 = XMVector3Transform(sseProxyRegister1, sseProxyRegister0_Matrix);
			// координаты позиции камеры относительно системы координат, находящейся в левом нижнем углу хитбокса
			sseProxyRegister2 = XMLoadFloat3(&DynamicHitBoxesArray[i].halvesOfWidthHeightLength);
			sseProxyRegister1 += sseProxyRegister2;
			// кладем ширину высоту длину хитбокса
			sseProxyRegister2 *= 2.0f;

			// если камера находится в хитбоксе
			if (XMVector3LessOrEqual(sseProxyRegister1, sseProxyRegister2) and
				XMVector3GreaterOrEqual(sseProxyRegister1, g_XMIdentityR3))
			{
				// находим расстояние позиции камеры до правых верхних граней хитбокса
				sseProxyRegister2 = sseProxyRegister2 - sseProxyRegister1;

				// находим минимум между расстоянием позиции камеры до нижних левых граней хитбокса(это координаты позиции камеры) и 
				// расстоянием позици камеры до верхних правых граней хитбокса
				sseProxyRegister3 = XMVectorMin(sseProxyRegister1, sseProxyRegister2);

				float minX = XMVectorGetX(sseProxyRegister3);
				float minZ = XMVectorGetZ(sseProxyRegister3);

				// если минимальное расстояние до граней находится по оси X
				if (minX < minZ) {
					//если минимальное расстояние явлется минимальным расстоянием от точки до левой грани хитбокса
					if (minX == XMVectorGetX(sseProxyRegister1)) {
						sseProxyRegister1 = XMVectorSetX(sseProxyRegister1, -0.05f); // 0.0005 это расстояние на которое отодвигается камера от грани хитбокса
					}
					else { //если минимальное расстояние явлется минимальным расстоянием от точки до правой грани хитбокса
						sseProxyRegister2 = XMVectorSetX(sseProxyRegister2, 0.05f);
						sseProxyRegister3 += sseProxyRegister2;
						sseProxyRegister1 += XMVectorAndInt(sseProxyRegister3, g_XMMaskX);
					}
				}
				else {// если минимальное расстояние до граней находится по оси Z
					//если минимальное расстояние явлется минимальным расстоянием от точки до нижней грани хитбокса
					if (minZ == XMVectorGetZ(sseProxyRegister1)) {
						sseProxyRegister1 = XMVectorSetZ(sseProxyRegister1, -0.05f);
					}
					else { //если минимальное расстояние явлется минимальным расстоянием от точки до верхней грани хитбокса
						sseProxyRegister2 = XMVectorSetZ(sseProxyRegister2, 0.05f);
						sseProxyRegister3 += sseProxyRegister2;
						sseProxyRegister1 += XMVectorAndInt(sseProxyRegister3, g_XMMaskZ);
					}
				}
				// умножить на обратную матрицу
				sseProxyRegister2 = XMLoadFloat3(&DynamicHitBoxesArray[i].halvesOfWidthHeightLength);
				sseProxyRegister1 -= sseProxyRegister2;
				sseProxyRegister0_Matrix = XMMatrixRotationY(DynamicHitBoxesArray[i].angle);
				sseProxyRegister1 = XMVector3Transform(sseProxyRegister1, sseProxyRegister0_Matrix);
				sseProxyRegister2 = XMLoadFloat3(&DynamicHitBoxesArray[i].position);
				sseProxyRegister1 += sseProxyRegister2;
				// сохранить измененную позицию в currentCameraPosition
				XMStoreFloat3(&currentCameraPos, sseProxyRegister1);

				//меняем view matrix, так чтобы она описывала систему координат в новой, смещенной точке
				//находим координаты новой позиции в текущей системе координат камеры
				matricesWVP.mView = XMMatrixTranspose(matricesWVP.mView);
				sseProxyRegister1 = XMVector3Transform(sseProxyRegister1, matricesWVP.mView);
				sseProxyRegister1 = XMVectorSetW(sseProxyRegister1, 0.0f);
				//переносим систему координат камеры в новую точку
				matricesWVP.mView.r[3] -= sseProxyRegister1;
				matricesWVP.mView = XMMatrixTranspose(matricesWVP.mView);
			}
		}
	}
};

float DynamicHitBoxCyclesAmount(DynamicHitBox* hitBox) {
	// позиция хитбокса относительно системы координат, находящейся в левом нижнем углу карты
	sseProxyRegister0 = XMLoadFloat3(&hitBox->position);
	sseProxyRegister0 += halvesOfWidthHeightLengthOfMap;
	
	// узнаем какие координаты вектора движения отрицательные, и какие положительные
	sseProxyRegister1 = XMVectorAndInt(*hitBox->moveVectorPtr, g_XMNegate3);

	float xValue = XMVectorGetX(sseProxyRegister1);
	float zValue = XMVectorGetZ(sseProxyRegister1);

	// если X < 0 или X == -0
	if (*((UINT*) &xValue) == 0x80000000U) { // пиздец уродство... ну а как по другому, union или reinterpret_cast мб?
		sseProxyRegister2 = XMVectorSetX(sseProxyRegister2, 0.0f);
	}
	else {
		// если X > 0 или X == +0
		sseProxyRegister2 = XMVectorSetX(sseProxyRegister2, 100.0f); // 100 - ширина карты
	}
	
	// если Z < 0 или Z == -0
	if (*((UINT*) &zValue) == 0x80000000U) {
		sseProxyRegister2 = XMVectorSetZ(sseProxyRegister2, 0.0f);
	}
	else {
		// если Z > 0 или Z == +0
		sseProxyRegister2 = XMVectorSetZ(sseProxyRegister2, 100.0f); // 100 - длина карты
	}

	// количество moveVector-ов необходимых прибавить к позиции хитбокса, для того
	// чтобы центр хитбокса вышел за пределы карты
	// причем если какая то координата moveVector равна +0 или -0, то мы получим +inf в sseProxyRegister3 на месте этой координаты
	// это свойство хорошо используется при поиске минимума среди координат sseProxyRegister3
	sseProxyRegister3 = (sseProxyRegister2 - sseProxyRegister0) / (*hitBox->moveVectorPtr);

	// поиск минимума среди координат
	if (XMVectorGetX(sseProxyRegister3) < XMVectorGetZ(sseProxyRegister3)) {
		return XMVectorGetX(XMVectorCeiling(sseProxyRegister3));
	}
	return XMVectorGetZ(XMVectorCeiling(sseProxyRegister3));
};

void CreateNewMoveVectorForDynamicHitBox(DynamicHitBox* hitBox) {
	// кладем текущую позицию камеры
	sseProxyRegister0 = XMLoadFloat3(&currentCameraPos);
	// позиция хитбокса
	sseProxyRegister1 = XMLoadFloat3(&hitBox->position);

	// находим вектор
	sseProxyRegister0 -= sseProxyRegister1;
	sseProxyRegister0 = XMVector3NormalizeEst(sseProxyRegister0);
	sseProxyRegister0 *= DYNAMIC_HIT_BOX_MOVEVECTOR_LENGTH; 
	*hitBox->moveVectorPtr = sseProxyRegister0;
};

inline void InitMoveVectorsAndActiveCyclesAmountForDynamicHitBoxes() {
	for (int i = 0; i < DYNAMIC_HIT_BOX_AMOUNT; ++i) {
		CreateNewMoveVectorForDynamicHitBox(&DynamicHitBoxesArray[i]);
		DynamicHitBoxesArray[i].activeCyclesAmount = DynamicHitBoxCyclesAmount(&DynamicHitBoxesArray[i]);
	}
};

HRESULT InitWallsVertices(Vertex* wallsVertexArray, LPCWSTR wallsVertexShaderName, LPCWSTR wallsPixelShaderName, LPCSTR vsShaderEntryPoint, LPCSTR psShaderEntryPoint) {
	HRESULT hr;

	D3D11_BUFFER_DESC vertexBufferDesc;
	vertexBufferDesc.ByteWidth = sizeof(Vertex) * 8;
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexBufferSubresourceInitData;
	vertexBufferSubresourceInitData.pSysMem = wallsVertexArray;
	vertexBufferSubresourceInitData.SysMemPitch = 0;
	vertexBufferSubresourceInitData.SysMemSlicePitch = 0;

	hr = g_pd3dDevice->CreateBuffer(&vertexBufferDesc, &vertexBufferSubresourceInitData, &pWallsVertexBuffer);
	if (FAILED(hr)) {
		return hr;
	}

	UINT stride[] = { sizeof(Vertex) };
	UINT offset[] = { 0 };
	g_pImmediateContext->IASetVertexBuffers(0, 1, &pWallsVertexBuffer, stride, offset);

	// компиляция шейдеров
	// найти cso файлы в директории
	
	
	// Создание Input-Layout Object
	// Связывание Input-layout object с конвейером

};

void FindFilesInCurrentDir1(WCHAR* filesArray, size_t* filesNamesLengthArray){
	// найти cso файлы в директории
	WCHAR fileName [] = "\TriangleVertexShader*";
	size_t fileNameSize = sizeof(fileName);
	
	HANDLE searchHandle;
	WIN32_FIND_DATA fileData;
	
	// длина текущей директории с учетом null(все длины строк с учетом null), без учета имени искомого файла
	DWORD curDirLength = GetCurrentDirectory(0, NULL);
	// длина директории файла
	DWORD fileDirBufLength = curDirLength + fileNameSize - 1;
	// буфер хранения строки директории файла
	WCHAR fileDirBuffer [MAX_PATH];
	
	GetCurrentDirectory(fileDirBufLength, &fileDirBuffer);
	// создается полный путь до файла
	memcpy(&fileDirBuffer[curDirLength - 1], &fileName[0], fileNameSize);
	
	
	
};
// ищет первый попавшийся файл, заданный в качестве аргумента, в заданной директории
// fileName должден начининаться с '\', fileDir должен заканчиваться названием крайнего каталога или диска, без '\'
HRESULT FindFilesInCurrentDir(WCHAR* fileDir, size_t fileDirBufLength, WCHAR* fileName, size_t fileNameLength, WIN32_FIND_DATA* fileDataPtr){
	// создается полный путь до файла
	memcpy(&fileDirBuffer[fileDirBufLength - 1], &fileName[0], fileNameSize);
	
	HANDLE searchHandle = FindFirstFileEx(fileDirBuffer, FindExInfoBasic, &fileData, FindExSearchNameMatch, NULL, 0);
	// если нужный файл нашелся
	if(searchHandle != INVALID_HANDLE_VALUE){
		// ret fileDirBuffer
		FindClose(searchHandle);
		return S_OK;
	} 
	// если нужный файл не нашелся
	else{
		FindClose(searchHandle);
		
		fileDirBuffer[fileDirBufLength + 1] = L'*'; // curDirLength + 1 потому что '*' нужно поставить после '\'
		fileDirBuffer[fileDirBufLength + 2] = NULL;
		fileDirBufLength += 2; 
			
		searchHandle = FindFirstFileEx(fileDirBuffer, FindExInfoBasic, &fileData, FindExSearchNameMatch, NULL, 0);
		// если текущий каталог не пуст
		if (searchHandle != INVALID_HANDLE_VALUE) {
		do {
			// если найденый файл - каталог
			if(fileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				// изменение текущей директории
				size_t folderNameLength = FileNameLength(fileData.cFileName);
				memcpy(&fileDirBuffer[fileDirBufLength - 1], fileData.cFileName[0], folderNameLength);
				
				// если в следующей директории нашелся файл
				if (FindFilesInCurrentDir(fileDirBuffer, fileDirBufLength + folderNameLength - 2, fileName, fileNameLength) == S_OK){
					FindClose(searchHandle);
					return S_OK;
				}
			}	
		} while (FindNextFile(searchHandle, &fileData) != 0)
		}
		// если директория пуста или все каталоги в директории проверены	
		else{
			FindClose(searchHandle);
			return E_FAIL;
		}
	}
};

// с учетом null
inline size_t __vectorcall FileNameLength(WCHAR* name){
	size_t nameLength = 1;
	if(name[nameLength] != NULL){
		nameLength++;
	} 
	nameLength++;
	
	return nameLength;
};

void ReleaseObjects() {
	if (dynamicMemory != NULL) {
		VirtualFree(dynamicMemory, 0, MEM_RELEASE);
	}
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








