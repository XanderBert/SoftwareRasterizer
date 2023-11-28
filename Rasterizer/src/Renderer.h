#pragma once

#include <cstdint>
#include <vector>

#include "Camera.h"

struct SDL_Window;
struct SDL_Surface;

namespace dae
{
	class Texture;
	struct Mesh;
	struct Vertex;
	struct Vertex_Out;
	class Timer;
	class Scene;

	class Renderer final
	{
	public:
		Renderer(SDL_Window* pWindow);
		~Renderer();

		Renderer(const Renderer&) = delete;
		Renderer(Renderer&&) noexcept = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer& operator=(Renderer&&) noexcept = delete;

		void Update(Timer* pTimer);
		void Render() const;
		bool SaveBufferToImage() const;

		void ToggleDepthBufferDisplay() { m_DisplayDepthBuffer = !m_DisplayDepthBuffer; }
		
	private:
		void ClearBackground() const;
		void ResetDepthBuffer() const;

		static void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldViewProjectionMatrix, const Matrix& meshWorldMatrix);
		void VertexTransformationToScreenSpace(const std::vector<Vertex_Out>& vertices_in, std::vector<Vector2>& vertex_out) const;
		
		void RenderTriangle(const std::vector<Vector2>& verticesScreenSpace, const std::vector<Vertex_Out>& verticesNDC, const std::vector<uint32_t>& verticesIndexes) const;
		
		SDL_Window* m_pWindow{};

		SDL_Surface* m_pFrontBuffer{ nullptr };
		SDL_Surface* m_pBackBuffer{ nullptr };
		uint32_t* m_pBackBufferPixels{};
		float* m_pDepthBufferPixels{};

		Camera m_Camera{};
		int m_Width{};
		int m_Height{};
		float m_AspectRatio{};

		bool m_DisplayDepthBuffer{ false };

		//Todo make wrapper class for mesh with a texture and a mesh in it?
		// Add bool to choose between texture wraping or clamping
		Texture* m_pTexture{};


		std::vector<Mesh> m_MeshesWorld;
	};
}
