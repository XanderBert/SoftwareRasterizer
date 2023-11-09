//External includes
#include "SDL.h"
#include "SDL_surface.h"
#include <iostream>

//Project includes
#include "Renderer.h"
#include "Maths.h"
#include "Texture.h"
#include "Utils.h"

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

	m_pDepthBufferPixels = new float[m_Height * m_Width];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
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

	
	//Define Triangle in NDC Space
	std::vector<Vertex> vertices_ndc{};
	std::vector<Vector2> vertices_screen{};
	
	const std::vector<Vertex> vertices_world
	{
		//Triangle 0
		Vertex{{ 2.0f, 0.5f, -5.f }, colors::Red},
		{{ 1.5f, -0.5f, -5.f }, colors::Red},
		{{ 0.5f, -0.5f, -5.f }, colors::Red},  

		//Triangle 1
		{{ 1.0f, 0.5f, 5.f }},
		{{ 1.5f, -0.5f, 5.f }},
		{{ 0.5f, -0.5f, 5.f }}, 
	};
	
	
	VertexTransformationFunction(vertices_world, vertices_ndc);
	VertexTransformationToScreenSpace(vertices_ndc, vertices_screen);
	
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//RENDER LOGIC
	//For every triangle
	for(size_t vertexIndex{}; vertexIndex <  vertices_world.size(); vertexIndex += 3 )
	{
		// Get all the current vertices
		const Vector2 v0{ vertices_screen[vertexIndex] };
		const Vector2 v1{ vertices_screen[vertexIndex + 1] };
		const Vector2 v2{ vertices_screen[vertexIndex + 2] };

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

		
		
		//Render Triangle
		for (int px{startX}; px < endX; ++px)
		{
			for (int py{startY}; py < endY; ++py)
			{
		
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
				const float depthV0{ (vertices_ndc[vertexIndex].position.z) };
				const float depthV1{ (vertices_ndc[vertexIndex + 1].position.z) };
				const float depthV2{ (vertices_ndc[vertexIndex + 2].position.z) };

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

				//Update Color in Buffer
				finalColor =  vertices_world[vertexIndex].color;
				finalColor.MaxToOne();

				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));
			}
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, nullptr, m_pFrontBuffer, nullptr);
	SDL_UpdateWindowSurface(m_pWindow);
}


// function that transforms a vector of WORLD space vertices to a vector of
// NDC space vertices (or directly to SCREEN space).
void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Convert to NDC > SCREEN space
	vertices_out.resize(vertices_in.size());
	for (size_t i{}; i < vertices_in.size(); ++i)
	{
		//Transform them with a VIEW Matrix (inverse ONB)
		vertices_out[i].position = m_Camera.invViewMatrix.TransformPoint({ vertices_in[i].position, 1.0f });
		vertices_out[i].color = vertices_in[i].color;

		//Apply the Perspective Divide
		//Divide positions by old z (will probably be stored in w later on)
		vertices_out[i].position.x /= vertices_out[i].position.z;
		vertices_out[i].position.y /= vertices_out[i].position.z;
		vertices_out[i].position.z /= vertices_out[i].position.z;
	}
}

void Renderer::VertexTransformationToScreenSpace(const std::vector<Vertex>& vertices_in,
	std::vector<Vector2>& vertex_out) const
{

	vertex_out.reserve(vertices_in.size());
	for (const Vertex& ndcVertex : vertices_in)
	{
		vertex_out.emplace_back(
			m_Width * ((ndcVertex.position.x + 1) / 2.0f),
			m_Height * ((1.0f - ndcVertex.position.y) / 2.0f)
		);
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
