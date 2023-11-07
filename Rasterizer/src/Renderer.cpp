//External includes
#include "SDL.h"
#include "SDL_surface.h"

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
	m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

	//m_pDepthBufferPixels = new float[m_Width * m_Height];

	//Initialize Camera
	m_Camera.Initialize(60.f, { .0f,.0f,-10.f });
}

Renderer::~Renderer()
{
	//delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);
}

void Renderer::Render()
{
	//@START

	//Define Triangle in NDC Space
	std::vector<Vertex> vertices_ndc{};
	std::vector<Vector2> vertices_screen{};
	
	const std::vector<Vertex> vertices_world
	{
		{{ 0.0f, 0.5f, 1.f }},
		{{ 0.5f, -0.5f, 1.f }},
		{{ -0.5f, -0.5f, 1.f }}
	};
	
	VertexTransformationFunction(vertices_world, vertices_ndc);
	VertexTransformationToScreenSpace(vertices_ndc, vertices_screen);	
	//Lock BackBuffer
	SDL_LockSurface(m_pBackBuffer);

	//RENDER LOGIC
	for (int px{}; px < m_Width; ++px)
	{
		for (int py{}; py < m_Height; ++py)
		{
			ColorRGB finalColor{ 0, 0, 0 };
			
			const Vector2 curPixel{ static_cast<float>(px), static_cast<float>(py) };
			const BarycentricWeights weights{ curPixel, vertices_screen };

			if(weights.IsPointInsideTriangle())
			{
				finalColor = ColorRGB{ 255, 0, 0 } * weights.w0 + ColorRGB{ 0, 255, 0 } * weights.w1 + ColorRGB{ 0, 0, 255 } * weights.w2;
			}
			
			
			
			//Update Color in Buffer
			finalColor.MaxToOne();

			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}

	//@END
	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, 0, m_pFrontBuffer, 0);
	SDL_UpdateWindowSurface(m_pWindow);
}


// function that transforms a vector of WORLD space vertices to a vector of
// NDC space vertices (or directly to SCREEN space).
void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex>& vertices_out) const
{
	//Todo > W1 Projection Stage
	
	//Convert to NDC > SCREEN space
	vertices_out.resize(vertices_in.size());
	for (size_t i{}; i < vertices_in.size(); ++i)
	{
		//Transform them with a VIEW Matrix (inverse ONB)
		vertices_out[i].position = m_Camera.invViewMatrix.TransformPoint({ vertices_in[i].position, 1.0f });
		vertices_out[i].color = vertices_in[i].color;

		//Apply the Perspective Divide
		//Divide positions by old z (stored in w)?
		vertices_out[i].position.x /= vertices_out[i].position.z;
		vertices_out[i].position.y /= vertices_out[i].position.z;
		vertices_out[i].position.z /= vertices_out[i].position.z;
	}
}

void Renderer::VertexTransformationToScreenSpace(const std::vector<Vertex>& vertices_in,
	std::vector<Vector2>& vertex_out) const
{

	vertex_out.reserve(vertices_in.size());
	for (const Vertex& ndcVertec : vertices_in)
	{
		vertex_out.emplace_back(
			m_Width * ((ndcVertec.position.x + 1) / 2.0f),
			m_Height * ((1.0f - ndcVertec.position.y) / 2.0f)
		);
	}
}

bool Renderer::SaveBufferToImage() const
{
	return SDL_SaveBMP(m_pBackBuffer, "Rasterizer_ColorBuffer.bmp");
}
