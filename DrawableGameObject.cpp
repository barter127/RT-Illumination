#include "stdafx.h"
#include "DrawableGameObject.h"
#include <string>

using namespace std;

//#define NUM_VERTICES 36

DrawableGameObject::DrawableGameObject()
{
	//m_pVertexBuffer = nullptr;
	//m_pIndexBuffer = nullptr;
	//m_pTextureResourceView = nullptr;
	//m_pSamplerLinear = nullptr;

	// Initialize the world matrix
	XMStoreFloat4x4(&m_World, XMMatrixIdentity());
	m_position = XMFLOAT3(0, 0, 0);
}


DrawableGameObject::~DrawableGameObject()
{
	cleanup();
}

void DrawableGameObject::cleanup()
{
	// TODO
	//if (m_pVertexBuffer)
	//	m_pVertexBuffer->Release();
	//m_pVertexBuffer = nullptr;

	//if (m_pIndexBuffer)
	//	m_pIndexBuffer->Release();
	//m_pIndexBuffer = nullptr;

	//if (m_pTextureResourceView)
	//	m_pTextureResourceView->Release();
	//m_pTextureResourceView = nullptr;

	//if (m_pSamplerLinear)
	//	m_pSamplerLinear->Release();
	//m_pSamplerLinear = nullptr;

	//if (m_pMaterialConstantBuffer)
	//	m_pMaterialConstantBuffer->Release();
	//m_pMaterialConstantBuffer = nullptr;
}

HRESULT DrawableGameObject::initMeshFromPath(ComPtr<ID3D12Device5> device, string path)
{
	m_meshData = OBJLoader::Load((char*)path.c_str(), device.Get(), false);

	return S_OK;
}

HRESULT DrawableGameObject::initTriMesh(ComPtr<ID3D12Device5> device)
{
	//// Create the vertex buffer.
	//{

	//	// Define the geometry for a triangle.
	//	Vertex vertices[] = {
	//		{{0.0f, 0.25f * 1.0f, 0.0f}, {0.0f,0.0f,1.0f,0.0f}, {0.0f, 0.0f}},
	//		{{0.25f, -0.25f * 1.0f, 0.0f}, {0.0f,0.0f,1.0f,0.0f}, {0.5f, 1.0f}},
	//		{{-0.25f, -0.25f * 1.0f, 0.0f}, {0.0f,0.0f,1.0f,0.0f}, {1.0f, 1.0f}}
	//	};

	//	m_vertexCount = 3;
	//	
	//	const UINT vertexBufferSize = sizeof(vertices);

	//	// Note: using upload heaps to transfer static data like vert buffers is not
	//	// recommended. Every time the GPU needs it, the upload heap will be
	//	// marshalled over. Please read up on Default Heap usage. An upload heap is
	//	// used here for code simplicity and because there are very few verts to
	//	// actually transfer.
	//	ThrowIfFailed(device->CreateCommittedResource(
	//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
	//		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
	//		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
	//		IID_PPV_ARGS(&m_vertexBuffer)));

	//	// Copy the triangle data to the vertex buffer.
	//	UINT8* pVertexDataBegin;
	//	CD3DX12_RANGE readRange(
	//		0, 0); // We do not intend to read from this resource on the CPU.
	//	ThrowIfFailed(m_vertexBuffer->Map(
	//		0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	//	memcpy(pVertexDataBegin, vertices, sizeof(vertices));
	//	m_vertexBuffer->Unmap(0, nullptr);
	//}


	//// create the index buffer
	//{
	//	// indices.
	//	UINT indices[] =
	//	{
	//		0,1,2
	//	};

	//	m_indexCount = sizeof(indices) / sizeof(UINT);

	//	const UINT indexBufferSize = sizeof(indices);
	//	CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
	//	ThrowIfFailed(device->CreateCommittedResource(
	//		&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
	//		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer)));

	//	// Copy the triangle data to the index buffer.
	//	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.

	//	UINT8* pIndexDataBegin;
	//	ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	//	memcpy(pIndexDataBegin, indices, indexBufferSize);
	//	m_indexBuffer->Unmap(0, nullptr);
	//}

	return S_OK;
}

HRESULT DrawableGameObject::initPlaneMesh(ComPtr<ID3D12Device5> device)
{
	//// Create the vertex buffer.
	//{
	//	float diameter = 0.5f;

	//	Vertex planeVertices[] = {
	//		{XMFLOAT3(-diameter, diameter, 0), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}, // 0 
	//		{XMFLOAT3(-diameter, -diameter, -0), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}, // 1
	//		{ XMFLOAT3(diameter, diameter, 0), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}, // 2
	//		{ XMFLOAT3(diameter, -diameter, -0), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)}  // 3

	//	};
	//	m_vertexCount = 4;

	//	const UINT vertexBufferSize = sizeof(planeVertices);

	//	// Note: using upload heaps to transfer static data like vert buffers is not
	//	// recommended. Every time the GPU needs it, the upload heap will be
	//	// marshalled over. Please read up on Default Heap usage. An upload heap is
	//	// used here for code simplicity and because there are very few verts to
	//	// actually transfer.
	//	ThrowIfFailed(device->CreateCommittedResource(
	//		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), D3D12_HEAP_FLAG_NONE,
	//		&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
	//		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
	//		IID_PPV_ARGS(&m_vertexBuffer)));

	//	// Copy the triangle data to the vertex buffer.
	//	UINT8* pVertexDataBegin;
	//	CD3DX12_RANGE readRange(
	//		0, 0); // We do not intend to read from this resource on the CPU.
	//	ThrowIfFailed(m_vertexBuffer->Map(
	//		0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	//	memcpy(pVertexDataBegin, planeVertices, sizeof(planeVertices));
	//	m_vertexBuffer->Unmap(0, nullptr);
	//}

	//// create the index buffer
	//{
	//	// indices.
	//	UINT indices[] =
	//	{
	//		0,1,2,
	//		2,1,3,
	//	};

	//	m_indexCount = sizeof(indices) / sizeof(UINT);

	//	const UINT indexBufferSize = sizeof(indices);
	//	CD3DX12_HEAP_PROPERTIES heapProperty = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	//	CD3DX12_RESOURCE_DESC bufferResource = CD3DX12_RESOURCE_DESC::Buffer(indexBufferSize);
	//	ThrowIfFailed(device->CreateCommittedResource(
	//		&heapProperty, D3D12_HEAP_FLAG_NONE, &bufferResource, //
	//		D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_indexBuffer)));

	//	// Copy the triangle data to the index buffer.
	//	CD3DX12_RANGE readRange(0, 0); // We do not intend to read from this resource on the CPU.

	//	UINT8* pIndexDataBegin;
	//	ThrowIfFailed(m_indexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pIndexDataBegin)));
	//	memcpy(pIndexDataBegin, indices, indexBufferSize);
	//	m_indexBuffer->Unmap(0, nullptr);
	//}


	return S_OK;
}

DrawableGameObject* DrawableGameObject::createCopy()
{
	DrawableGameObject* pobj = new DrawableGameObject();
	*pobj = *this;
	return pobj;
}

void DrawableGameObject::setPosition(XMFLOAT3 position)
{
	m_position = position;
}

void DrawableGameObject::setEulerRotation(XMFLOAT3 euler)
{
	m_eulerRotation = euler;
}

void DrawableGameObject::setScale(XMFLOAT3 scale)
{
	m_scale = scale;
}

void DrawableGameObject::update(float t)
{
	static float cummulativeTime = 0;
	cummulativeTime += t;

	// Cube:  Rotate around origin
	XMMATRIX mSpin = XMMatrixRotationY(cummulativeTime);

	XMMATRIX mTranslate = XMMatrixTranslation(m_position.x, m_position.y, m_position.z);
	XMMATRIX mRotation = XMMatrixRotationRollPitchYaw(XMConvertToRadians(m_eulerRotation.x), XMConvertToRadians(m_eulerRotation.y), XMConvertToRadians(m_eulerRotation.z));
	XMMATRIX mScale = XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z);

	XMMATRIX world = mScale * mRotation * mTranslate;
	XMStoreFloat4x4(&m_World, world);
}
