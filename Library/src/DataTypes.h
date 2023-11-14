#pragma once
#include "Maths.h"
#include "vector"

namespace dae
{
	struct Vertex
	{
		Vector3 position{};
		//ColorRGB color{colors::White};
		Vector2 uv{}; //W2
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

		float depth{};

		BarycentricWeights(const Vector2& point, const std::vector<Vector2>& verticesScreenSpace, const std::vector<Vertex>& verticesNDC, int currentVertexIndex)
		{
			CalculateWeights(point, verticesScreenSpace ,currentVertexIndex);
			CalculateDepth(verticesNDC, currentVertexIndex);
		}
		
		void CalculateWeights(const Vector2& point, const std::vector<Vector2>& verticesScreenSpace, int currentVertexIndex)
		{
			const Vector2& v0 = verticesScreenSpace[currentVertexIndex];
			const Vector2& v1 = verticesScreenSpace[currentVertexIndex + 1];
			const Vector2& v2 = verticesScreenSpace[currentVertexIndex + 2];
			
			
			const float area = (v1.y - v2.y) * (v0.x - v2.x) + (v2.x - v1.x) * (v0.y - v2.y);

			w0 = ((v1.y - v2.y) * (point.x - v2.x) + (v2.x - v1.x) * (point.y - v2.y)) / area;
			w1 = ((v2.y - v0.y) * (point.x - v2.x) + (v0.x - v2.x) * (point.y - v2.y)) / area;
			w2 = 1.f - w0 - w1;
		}

		bool IsPointInsideTriangle() const
		{
			return (w0 >= 0.0f) && (w1 >= 0.0f) && (w2 >= 0.0f) && (w0 + w1 + w2 <= 1.0f);
		}

		void CalculateDepth(const std::vector<Vertex>& verticesNDC, int currentVertexIndex)
		{
			//Todo: this gets called for every pixel, optimize this
			
			const float depthV0 = verticesNDC[currentVertexIndex].position.z;
			const float depthV1 = verticesNDC[currentVertexIndex + 1].position.z;
			const float depthV2 = verticesNDC[currentVertexIndex + 2].position.z;
			
			// Calculate the depth at this pixel
			depth =
			{
				1.0f /
					
					(w0 * 1.0f / depthV0 +
					w1 * 1.0f / depthV1 +
					w2 * 1.0f / depthV2)
			};
			
		}
	};
}
