#include <iostream>
#include <windows.h>
//#include <D3DX10math.h>
//#include <xnamath.h>
//#include <directxmath.h>
#include <DirectXMath.h>
#include <cstdlib>

using namespace DirectX; 
void CreateAndWriteTest() {
	char charArray[3] = {'b', 'b', 'b'};
	HANDLE errorsFileHandle = NULL;
	DWORD bytesWritten = NULL;
		
	errorsFileHandle = CreateFile(L"shaderErrors.txt", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			
	WriteFile(errorsFileHandle, charArray, sizeof(charArray), &bytesWritten, NULL);

	CloseHandle(errorsFileHandle);
};

struct strStr {
	char a;
	short b;
};

int func(int a) {
	return ++a;
}

void movS(strStr& burg, strStr& vic) {
	burg.a = vic.a;
	burg.b = vic.b;
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
	return XMVectorSet(1.0f, 0.0f, 0.0f, XMVectorGetByIndex(vector, 3));
};

void RotationAroundAxis(XMVECTOR yAxis, XMVECTOR point, FLOAT angle, XMMATRIX* outputMatrix) {
	//yAxis = XMVectorSetByIndex(XMVector3Normalize(yAxis), 0.0f, 3);
	yAxis = XMVectorPermute<XM_PERMUTE_0X, XM_PERMUTE_0Y, XM_PERMUTE_0Z, XM_PERMUTE_1W>(XMVector3Normalize(yAxis), g_XMZero);
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


template <typename T, size_t N>
void tempFunc( T (&peremen_t)[N])
{
	for (size_t i = 0; i < N; i++)
	{
		++peremen_t[i];
	}
}

int main() {
	
	int intA[] = {1, 2, 3};
	//XMVECTOR z1 = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f);
	//XMVECTOR y2 = XMVectorSet(XMScalarSin(XM_PI / 4.0f), XMScalarCos(XM_PI / 4.0f), 0.0f, 1.0f);
	//XMVECTOR eye = XMVectorSet(2.0f, 1.0f, 0.0f, 1.0f);
	float vecAngle = -XM_PIDIV4;
	XMVECTOR eye = XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f); // ������ �������
	//XMVECTOR z1 = XMVectorSet(0.0f, XMScalarSinEst(vecAngle), XMScalarCosEst(vecAngle), 1.0f); // ���� �������
	//XMVECTOR y2 = XMVectorSet(0.0f, XMScalarCosEst(XM_PIDIV4), XMScalarSinEst(XM_PIDIV4), 1.0f); // ������� � ����, ���� �������

	XMVECTOR z1 = XMVectorSet(0.0f, 0.0f, 1.0f, 1.0f); // ���� �������. 
	XMVECTOR y2 = XMVectorSet(0.0f, 1.0f, 0.0f, 1.0f); // ������� � ����, ���� �������

	// ���� RotationAroundAxis
	XMMATRIX m;
	XMFLOAT4X4 m2;

	XMVECTOR yAxis = XMVectorSet(8.21f, 200.0f, 28.0f, 0.0032f);
	XMVECTOR pointYAxis = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	float vecAngle2 = -XM_PIDIV4;

	RotationAroundAxis(yAxis, pointYAxis, vecAngle2, &m);
	XMStoreFloat4x4(&m2, m);

	for (int i = 0; i < 4; ++i) {
		for (int i2 = 0; i2 < 4; ++i2) {
			std::cout <<  m2.m[i][i2] << ' ';
		}
		std::cout << '\n';
	}

	std::cout << '\n';

	XMMATRIX M = XMMatrixRotationAxis(yAxis, vecAngle2);
	//M.r[3] = _mm_mul_ps(eye, XMVectorSet(-1.0f, -1.0f, -1.0f, -1.0f));
	XMStoreFloat4x4(&m2, M);

	for (int i = 0; i < 4; ++i) {
		for (int i2 = 0; i2 < 4; ++i2) {
			std::cout << m2.m[i][i2] << ' ';
		}
		std::cout << '\n';
	}
	
	

	/*
	PVOID memAddress = NULL;

	SYSTEM_INFO systemInfo;
	GetSystemInfo(&systemInfo);

	std::cout << systemInfo.dwAllocationGranularity;
	std::cout << '\n';

	std::cout << systemInfo.dwPageSize;
	std::cout << '\n';
	
	LPVOID allocatedMemory = VirtualAlloc(NULL, 400, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	int allocatedMemory[4] = { 1, 2, 3, 4 };
	for (int i = 0; i < 4; ++i) {
		std::cout << ((int*) allocatedMemory + i) ;
		std::cout << '\n';
	}
	*/
	
	XMVECTOR negVal = XMVECTORF32{ 10.0f, -10.0f, -10.0f, -10.0f };
	XMVECTOR negZero = XMVECTORF32{ -0.0f, -0.0f, -0.0f, -0.0f };
	XMVECTOR resVec = negVal / negZero;
	float resX = XMVectorGetX(resVec);
	std::cout << resX;
	std::cout << '\n';

	if (resX > -10.0f) {
		std::cout << 1;
	}
	else {
		std::cout << 0;
	}

	/*
	strStr myStruct = { 10 , 40 };
	std::cout << (int*)&myStruct.a;
	std::cout << '\n';
	std::cout << &myStruct.b;
	std::cout << '\n';
	*/
	return 0;
}

