#pragma once
#include "Object.h"
#include "VertexBuffer.h"
#include "IndexBuffer.h"
#include "Texture.h"

class SkyBox : public Object
{
private:
	std::vector<VERTEX_3D> m_Vertices;
	std::vector<unsigned int> m_Indices;

	VertexBuffer<VERTEX_3D> m_VertexBuffer;
	IndexBuffer m_IndexBuffer;

	// 6ñáÅi+X,-X,+Y,-Y,+Z,-ZÅj
	Texture m_Textures[6];

	float m_Size = 500.0f;

public:
	void Init() override;
	void Update() override;
	void Draw(Camera* cam) override;
	void Uninit() override;
};