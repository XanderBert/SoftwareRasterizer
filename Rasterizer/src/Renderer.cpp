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
	m_AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
	m_Camera.Initialize(m_AspectRatio,60.f, { .0f,.0f,-10.f });

	//Initialize the texture
	m_pTexture = Texture::LoadFromFile("Resources/tuktuk.png");
	
	
	std::vector<Vertex> vertices;
	std::vector<Uint32> indices;
	Utils::ParseOBJ("Resources/tuktuk.obj", vertices, indices);	
	m_MeshesWorld.emplace_back(vertices, indices, PrimitiveTopology::TriangleList);

	
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
	ResetDepthBuffer();
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);


	//for each mesh
	for(const auto& mesh : m_MeshesWorld)
	{
		//Define Triangle in NDC Space		
		const auto worldViewProjectionMatrix = mesh.worldMatrix * m_Camera.viewMatrix * m_Camera.projectionMatrix;
		std::vector<Vertex_Out> vertices_ndc{};
		VertexTransformationFunction(mesh.vertices, vertices_ndc, worldViewProjectionMatrix, mesh.worldMatrix);


		//TODO Frustum culling and clipping should happen here to be more performant


		
		std::vector<Vector2> vertices_screen{};
		VertexTransformationToScreenSpace(vertices_ndc, vertices_screen);
		
		// For each triangle
		for(mesh.m_pTriangleIterator->ResetIndex(); mesh.m_pTriangleIterator->HasNext();)
		{
			const std::vector<uint32_t> triangleIndices = mesh.m_pTriangleIterator->Next();
			RenderTriangle(vertices_screen, vertices_ndc, triangleIndices);
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
void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldViewProjectionMatrix, const Matrix& meshWorldMatrix)
{
	vertices_out.resize(vertices_in.size());

	for (size_t i{}; i < vertices_in.size(); ++i)
	{		
		// Transform with VIEW matrix (inverse ONB)
		vertices_out[i].position = worldViewProjectionMatrix.TransformPoint({vertices_in[i].position, 1.0f});
		vertices_out[i].normal = meshWorldMatrix.TransformVector(vertices_in[i].normal);
		vertices_out[i].tangent = meshWorldMatrix.TransformVector(vertices_in[i].tangent);

		vertices_out[i].uv = vertices_in[i].uv;
		
		// Apply Perspective Divide
		vertices_out[i].position.x /= vertices_out[i].position.w;
		vertices_out[i].position.y /= vertices_out[i].position.w;
		vertices_out[i].position.z /= vertices_out[i].position.w;		
		//vertices_out[i].uv /= vertices_out[i].position.w;	
	}
}

void Renderer::VertexTransformationToScreenSpace(const std::vector<Vertex_Out>& vertices_in,
	std::vector<Vector2>& vertex_out) const
{

	vertex_out.reserve(vertices_in.size());
	
	const float fWidth{ static_cast<float>(m_Width) };
		const float fHeight{ static_cast<float>(m_Height) };
	
	for (const Vertex_Out& ndcVertex : vertices_in)
	{
		vertex_out.emplace_back(
			fWidth * ((ndcVertex.position.x + 1) / 2.0f),
			fHeight * ((1.0f - ndcVertex.position.y) / 2.0f)
		);
	}
}

void Renderer::RenderTriangle(const std::vector<Vector2>& verticesScreenSpace,
	const std::vector<Vertex_Out>& verticesNDC, const std::vector<uint32_t>& verticesIndexes) const
{
	//RENDER LOGIC
	//Calculate the current vertex index
	const uint32_t vertexIndex0{ verticesIndexes[0] };
	const uint32_t vertexIndex1{ verticesIndexes[1] };
	const uint32_t vertexIndex2{ verticesIndexes[2] };
	
	//If A triangle has the same vertex, skip it
	if (vertexIndex0 == vertexIndex1 || vertexIndex1 == vertexIndex2 || vertexIndex0 == vertexIndex2) return;

	
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex0].position)) return;
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex1].position)) return;
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex2].position)) return;

	
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

//-------------------------------------------------------------------------------
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

//-------------------------------------------------------------------------------


			//Calculate the depth
			const float WdepthV0{ (verticesNDC[vertexIndex0].position.w) };
			const float WdepthV1{ (verticesNDC[vertexIndex1].position.w) };
			const float WdepthV2{ (verticesNDC[vertexIndex2].position.w) };

			// Calculate the depth at this pixel
			const float interpolatedWDepth
			{
				1.0f /
					(weightV0 * 1.0f / WdepthV0 +
					weightV1 * 1.0f / WdepthV1 +
					weightV2 * 1.0f / WdepthV2)
			};


			if(!m_DisplayDepthBuffer)
			{
				//Calculate the UV
				Vector2 curPixelUV
				{
					(weightV0 * verticesNDC[vertexIndex0].uv / WdepthV0 +
					weightV1 * verticesNDC[vertexIndex1].uv / WdepthV1 +
					weightV2 * verticesNDC[vertexIndex2].uv / WdepthV2) * interpolatedWDepth
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
			}else
			{
				//Display the depth buffer when needed
				//Remap the interpolated depthColor to a range between 0 and 1
				// Min and Max values of the original range
				constexpr float minValue = 1.0;
				constexpr float maxValue = 34.0;

				// Remap the value to the range [0, 1]
				const float remappedValue = (interpolatedWDepth - minValue) / (maxValue - minValue);
				finalColor =  ColorRGB{remappedValue, remappedValue, remappedValue};
			}
			
			
			
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
