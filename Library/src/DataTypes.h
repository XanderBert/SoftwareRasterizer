#pragma once
#include "Maths.h"
#include "vector"
#include "../Misc/ITriangleIndicesIterator.h"
#include <memory>

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		//ColorRGB color{colors::White};
		Vector2 uv{}; //W2
		Vector3 normal{}; //W4
		Vector3 tangent{}; //W4
		Vector3 viewDirection{}; //W4
	};

	struct Vertex_Out
	{
		Vector4 position{};
		//ColorRGB color{ colors::White };
		Vector2 uv{};
		Vector3 normal{};
		Vector3 tangent{};
		Vector3 viewDirection{};
	};


	struct BoundingBox
	{
		Vector3 min{};
		Vector3 max{};
	};
	
	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};
	
	struct Mesh
	{

		Mesh(const  std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList) :
			vertices{ (vertices) },
			indices{ (indices) },
			primitiveTopology{ primitiveTopology }
		{
			if(primitiveTopology == PrimitiveTopology::TriangleList)
			{
				m_pTriangleIterator = std::make_unique<TriangleListIterator>((this->indices));
			}
			else if(primitiveTopology == PrimitiveTopology::TriangleStrip)
			{
				m_pTriangleIterator = std::make_unique<TriangleStripIterator>((this->indices));
			}
		}



		void Rotate(float angle, const Vector3& axis)
		{
			worldMatrix = Matrix::CreateRotation(axis * angle) * worldMatrix;				
		}
		
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};

		std::unique_ptr<ITriangleIndicesIterator> m_pTriangleIterator{};
	};
}
