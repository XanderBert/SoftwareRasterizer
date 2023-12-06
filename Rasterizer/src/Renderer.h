#pragma once

#include <cstdint>
#include <vector>
#include "Camera.h"
#include "DataTypes.h"

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



	enum class ShadeMode
	{
		ObservedArea,
		Diffuse,
		Specular,
		Combined
	};

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
		void Render();
		bool SaveBufferToImage() const;

		void ToggleDepthBufferDisplay() { m_DisplayDepthBuffer = !m_DisplayDepthBuffer; }
		void ToggleNormalMap() { m_UseNormalMap = !m_UseNormalMap; }
		void CycleShadeMode() { m_ShadeMode = static_cast<ShadeMode>((static_cast<int>(m_ShadeMode) + 1) % 4); }
		void ToggleRotation() {m_Rotate = !m_Rotate;}
		
	private:
		void ClearBackground() const;
		void ResetDepthBuffer() const;

		//Transforms the vertices from WORLD space to NDC space
		static void VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldViewProjectionMatrix, const Matrix& meshWorldMatrix);

		//Transforms the vertices from NDC space to SCREEN space
		void VertexTransformationToScreenSpace(const std::vector<Vertex_Out>& vertices_in, std::vector<Vector2>& vertex_out) const;

		//Renders the triangle
		void RenderTriangle(const std::vector<Vector2>& verticesScreenSpace, std::vector<Vertex_Out>& verticesNDC, const std::vector<uint32_t>& verticesIndexes) const;

		//Shades the pixel
		void Shade(const Vertex_Out& vertex, ColorRGB& finalColor) const;


		//Clips the triangle
		std::vector<Vertex_Out> SutherlandHodgmanClipping(const std::vector<Vertex_Out>& inputVertices);
		std::vector<Vertex_Out> ClipAgainstPlane(const std::vector<Vertex_Out>& inputVertices, const Vector4& plane);


		
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
		bool m_UseNormalMap{ true };
		bool m_Rotate{ true };
		ShadeMode m_ShadeMode{ ShadeMode::Diffuse };

		//Todo make wrapper class for mesh with a texture and a mesh in it?
		
		std::unique_ptr<Texture> m_pTexture{};
		std::unique_ptr<Texture> m_pGlossinessTexture{};
		std::unique_ptr<Texture> m_pNormalTexture{};
		std::unique_ptr<Texture> m_pSpecularTexture{};
		const float m_Glossiness{ 25.f };
		const float m_AmbientLight{ 0.025f };

		
		std::vector<Mesh> m_MeshesWorld;



		//TODO make light struct / class
		Vector3 m_DirectionLight{.577f, -.577f, .577f};
		float m_lightIntensity{ 7.f };


		
	};
}
