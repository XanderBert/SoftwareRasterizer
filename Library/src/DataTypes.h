#pragma once
#include "Maths.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		ColorRGB color{colors::White};
		//Vector2 uv{}; //W2
		//Vector3 normal{}; //W4
		//Vector3 tangent{}; //W4
		//Vector3 viewDirection{}; //W4
	};

	struct Vertex_Out
	{
		Vector4 position{};
		ColorRGB color{ colors::White };
		//Vector2 uv{};
		//Vector3 normal{};
		//Vector3 tangent{};
		//Vector3 viewDirection{};
	};

	enum class PrimitiveTopology
	{
		TriangleList,
		TriangleStrip
	};

	struct Mesh
	{
		std::vector<Vertex> vertices{};
		std::vector<uint32_t> indices{};
		PrimitiveTopology primitiveTopology{ PrimitiveTopology::TriangleStrip };

		std::vector<Vertex_Out> vertices_out{};
		Matrix worldMatrix{};
	};

	struct BarycentricWeights 
	{
		float w0{};
		float w1{};
		float w2{};

		BarycentricWeights(const Vector2& point, const std::vector<Vector2>& vertices)
		{
			CalculateWeights(point, vertices);	
		}
		
		void CalculateWeights(const Vector2& point, const std::vector<Vector2>& vertices)
		{
			const Vector2& v0 = vertices[0];
			const Vector2& v1 = vertices[1];
			const Vector2& v2 = vertices[2];

			const float area = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);

			w0 = ((v1.y - v2.y) * (point.x - v2.x) + (v2.x - v1.x) * (point.y - v2.y)) / area;
			w1 = ((v2.y - v0.y) * (point.x - v2.x) + (v0.x - v2.x) * (point.y - v2.y)) / area;
			w2 = 1.f - w0 - w1;
		}

		bool IsPointInsideTriangle() const
		{
			return (w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f) && (w0 + w1 + w2 <= 1.0f);
		}
	};
}
