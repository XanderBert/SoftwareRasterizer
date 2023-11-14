//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include <iostream>

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

#define TextureTiling 0

using namespace dae;

Renderer::Renderer(SDL_Window* pWindow) :
	m_pWindow(pWindow)
{
	//Initialize
	SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

	//Create Buffers
	m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
	m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
	m_pBackBufferPixels = static_cast<uint32_t*>(m_pBackBuffer->pixels);

	m_pDepthBufferPixels = new float[static_cast<int>(m_Width * m_Height)];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
	m_AspectRatio = (float)m_Width / (float)m_Height;

	//Initialize the texture
	m_pTexture = Texture::LoadFromFile("Resources/uv_grid_2.png");

	
	//Initialize the mesh
	//Todo: is there an option to optimize the vertices? (remove doubles)
	m_MeshesWorld.emplace_back(
		Mesh{
					{
						Vertex{{ -3.f, 3.f, -2.f }, { 0.0f, 0.0f }},
						Vertex{{ 0.f, 3.f, -2.f }, { 0.5f, 0.0f }},
						Vertex{{ 3.f, 3.f, -2.f }, { 1.0f, 0.0f }},
						Vertex{{ -3.f, 0.f, -2.f }, { 0.0f, 0.5f }},
						Vertex{{ 0.f, 0.f, -2.f }, { 0.5f, 0.5f }},
						Vertex{{ 3.f, 0.f, -2.f }, { 1.0f, 0.5f }},
						Vertex{{ -3.f, -3.f, -2.f }, { 0.0f, 1.0f }},
						Vertex{{ 0.f, -3.f, -2.f }, { 0.5f, 1.0f }},
						Vertex{{ 3.f, -3.f, -2.f }, { 1.0f, 1.0f }},
					},

		{
						3,0,4,1,5,2,
						2,6,
						6,3,7,4,8,5
					},
		
					PrimitiveTopology::TriangleStrip });
}


Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
	delete m_pTexture;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render() const
{
	//@START
	ClearBackground();
	ResetDepthBuffer()
	;
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);


	//for each mesh
	for(const auto& mesh : m_MeshesWorld)
	{
		//Define Triangle in NDC Space
		std::vector<Vertex> vertices_ndc{};
		std::vector<Vector2> vertices_screen{};
			
		VertexTransformationFunction(mesh.vertices, vertices_ndc);
		VertexTransformationToScreenSpace(vertices_ndc, vertices_screen);

		//Frustum Culling
		const auto distance = (mesh.worldMatrix.GetTranslation() - m_Camera.origin).SqrMagnitude();
		if (distance <= m_Camera.nearPlane || distance >= m_Camera.farPlane) continue;

		
		// For each triangle
		const int meshIndicesSize = static_cast<int>(mesh.indices.size());
		for (int curStartVertexIdx{}; curStartVertexIdx < meshIndicesSize - 2; ++curStartVertexIdx)
		{
			RenderTriangle(mesh, vertices_screen, vertices_ndc, curStartVertexIdx, curStartVertexIdx % 2);
		}
	}

	
	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, nullptr, m_pFrontBuffer, nullptr);
	SDL_UpdateWindowSurface(m_pWindow);
}


// function that transforms a vector of WORLD space vertices to a vector of
// NDC space vertices.
void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	vertices_out.resize(vertices_in.size());

	// Precompute reciprocal values
	const float invAspectRatioFov = 1.0f / (m_AspectRatio * m_Camera.fov);
	const float invFov = 1.0f / m_Camera.fov;

	for (size_t i{}; i < vertices_in.size(); ++i)
	{
		// Transform with VIEW matrix (inverse ONB)
		vertices_out[i].position = m_Camera.invViewMatrix.TransformPoint(vertices_in[i].position);

		// Apply Perspective Divide
		const float invZ = 1.0f / vertices_out[i].position.z;
		
		vertices_out[i].position.x *= invAspectRatioFov * invZ;
		vertices_out[i].position.y *= invFov * invZ;
	}
}

void Renderer::VertexTransformationToScreenSpace(const std::vector<Vertex>& vertices_in,
	std::vector<Vector2>& vertex_out) const
{

	vertex_out.reserve(vertices_in.size());
	
	const float fWidth{ static_cast<float>(m_Width) };
	const float fHeight{ static_cast<float>(m_Height) };
	
	for (const Vertex& ndcVertex : vertices_in)
	{
		vertex_out.emplace_back(
			fWidth * ((ndcVertex.position.x + 1) / 2.0f),
			fHeight * ((1.0f - ndcVertex.position.y) / 2.0f)
		);
	}
}

void Renderer::RenderTriangle(const Mesh& mesh, const std::vector<Vector2>& verticesScreenSpace,
	const std::vector<Vertex>& verticesNDC, int currentVertexIndex, bool doSwapVertices) const
{
	//RENDER LOGIC
	//Calculate the current vertex index
	const uint32_t vertexIndex0{ mesh.indices[currentVertexIndex] };
	const uint32_t vertexIndex1{ mesh.indices[currentVertexIndex + 1 * !doSwapVertices + 2 * doSwapVertices] };
	const uint32_t vertexIndex2{ mesh.indices[currentVertexIndex + 2 * !doSwapVertices + 1 * doSwapVertices] };


	//If A triangle has the same vertex, skip it
	if (vertexIndex0 == vertexIndex1 || vertexIndex1 == vertexIndex2 || vertexIndex0 == vertexIndex2) return;

	
	// Get all the current vertices
	const Vector2 v0{ verticesScreenSpace[vertexIndex0] };
	const Vector2 v1{ verticesScreenSpace[vertexIndex1] };
	const Vector2 v2{ verticesScreenSpace[vertexIndex2] };

	// Calculate the edges of the current triangle
	const Vector2 edge01{ v1 - v0 };
	const Vector2 edge12{ v2 - v1 };
	const Vector2 edge20{ v0 - v2 };

	// Calculate the area of the current triangle
	const float fullTriangleArea{ Vector2::Cross(edge01, edge12) };
	
	// Calculate the bounding box of this triangle
	const Vector2 minBoundingBox{ Vector2::Min(v0, Vector2::Min(v1, v2)) };
	const Vector2 maxBoundingBox{ Vector2::Max(v0, Vector2::Max(v1, v2)) };

	// A margin that enlarges the bounding box, makes sure that some pixels do no get ignored
	constexpr int margin{ 1 };

	// Calculate the start and end pixel bounds of this triangle
	const int startX{ std::clamp(static_cast<int>(minBoundingBox.x - margin), 0, m_Width) };
	const int startY{ std::clamp(static_cast<int>(minBoundingBox.y - margin), 0, m_Height) };
	const int endX{ std::clamp(static_cast<int>(maxBoundingBox.x + margin), 0, m_Width) };
	const int endY{ std::clamp(static_cast<int>(maxBoundingBox.y + margin), 0, m_Height) };

		
		
	// For each pixel
	for (int px{startX}; px < endX; ++px)
	{
		for (int py{startY}; py < endY; ++py)
		{

			//Reset final color
			ColorRGB finalColor{ 0, 0, 0 };
		
			// Calculate the pixel index and create a Vector2 of the current pixel
			const int pixelIdx{ px + py * m_Width };
			const Vector2 curPixel{ static_cast<float>(px), static_cast<float>(py) };

			// Calculate the vector between the first vertex and the point
			const Vector2 v0ToPoint{ curPixel - v0 };
			const Vector2 v1ToPoint{ curPixel - v1 };
			const Vector2 v2ToPoint{ curPixel - v2 };

			// Calculate cross product from edge to start to point
			const float edge01PointCross{ Vector2::Cross(edge01, v0ToPoint) };
			const float edge12PointCross{ Vector2::Cross(edge12, v1ToPoint) };
			const float edge20PointCross{ Vector2::Cross(edge20, v2ToPoint) };

			// Check if pixel is inside triangle, if not continue to the next pixel
			if (!(edge01PointCross > 0 && edge12PointCross > 0 && edge20PointCross > 0)) continue;

			// Calculate the barycentric weights
			const float weightV0{ edge12PointCross / fullTriangleArea };
			const float weightV1{ edge20PointCross / fullTriangleArea };
			const float weightV2{ edge01PointCross / fullTriangleArea };


			//Calculate the depth
			const float depthV0{ (verticesNDC[vertexIndex0].position.z) };
			const float depthV1{ (verticesNDC[vertexIndex1].position.z) };
			const float depthV2{ (verticesNDC[vertexIndex2].position.z) };

			// Calculate the depth at this pixel
			const float interpolatedDepth
			{
				1.0f /
					(weightV0 * 1.0f / depthV0 +
					weightV1 * 1.0f / depthV1 +
					weightV2 * 1.0f / depthV2)
			};

			// If this pixel hit is further away then a previous pixel hit, continue to the next pixel
			if (m_pDepthBufferPixels[pixelIdx] < interpolatedDepth) continue;

			// Save the new depth
			m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;


			//Calculate the UV
			Vector2 curPixelUV
			{
				(weightV0 * mesh.vertices[vertexIndex0].uv / depthV0 +
				weightV1 * mesh.vertices[vertexIndex1].uv / depthV1 +
				weightV2 * mesh.vertices[vertexIndex2].uv / depthV2) * interpolatedDepth
			};


			#if TextureTiling
			// Wrap UV coordinates to the [0, 1] range
			curPixelUV.x = fmod(curPixelUV.x, 1.0f);
			curPixelUV.y = fmod(curPixelUV.y, 1.0f);
			if (curPixelUV.x < 0.0f) curPixelUV.x += 1.0f;
			if (curPixelUV.y < 0.0f) curPixelUV.y += 1.0f;

			#else
			
			// Clamp UV coordinates to the [0, 1] range
			curPixelUV.x = std::max(0.0f, std::min(1.0f, curPixelUV.x));
			curPixelUV.y = std::max(0.0f, std::min(1.0f, curPixelUV.y));
			#endif

			
			//Update Color in Buffer
			finalColor =  m_pTexture->Sample(curPixelUV);
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}




bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}


void Renderer::ClearBackground() const
{
	SDL_FillRect(m_pBackBuffer, nullptr, SDL_MapRGB(m_pBackBuffer->format, 100, 100, 100));
}

void Renderer::ResetDepthBuffer() const
{
	const int nrPixels{ m_Width * m_Height };
	std::fill_n(m_pDepthBufferPixels, nrPixels, FLT_MAX);
}
