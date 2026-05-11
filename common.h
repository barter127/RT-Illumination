#pragma once

#include "DXSample.h"


#include "DXRApp.h"



using namespace DirectX;

struct Vertex { // IMPORTANT - the hlsl version of this is STriVertex
    XMFLOAT3 position;
	XMFLOAT4 normal;
	XMFLOAT2 texcoord;
};

struct AccelerationStructureBuffers {
	ComPtr<ID3D12Resource> pScratch;      // Scratch memory for AS builder
	ComPtr<ID3D12Resource> pResult;       // Where the AS is
	ComPtr<ID3D12Resource> pInstanceDesc; // Hold the matrices of the instances
};

struct LightData
{
	XMFLOAT4 position;
	XMFLOAT4 ambientColour;
	XMFLOAT4 diffuseColour;
	XMFLOAT4 specularColour;

	float shininess;
	float attenuationRadius;
	int type;
	int padding;
};

struct LightBuffer
{
	LightData lightArray[3];
};

struct DebugBuffer
{
	int shadowSampleCount;
	float materialAlbedo;
	float materialRoughness;
	float materialMetalness;
};
