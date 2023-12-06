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
#define Clipping 0



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
	m_Camera.Initialize(m_AspectRatio,60.f, { .0f,.0f,-50.f });

	//Initialize the textures
	m_pTexture = std::make_unique<Texture>(*Texture::LoadFromFile("Resources/vehicle_diffuse.png"));
	m_pGlossinessTexture = std::make_unique<Texture>(*Texture::LoadFromFile("Resources/vehicle_gloss.png"));
	m_pNormalTexture = std::make_unique<Texture>(*Texture::LoadFromFile("Resources/vehicle_normal.png"));
	m_pSpecularTexture = std::make_unique<Texture>(*Texture::LoadFromFile("Resources/vehicle_specular.png"));
	
	//Load the model
	std::vector<Vertex> vertices;
	std::vector<Uint32> indices;
	Utils::ParseOBJ("Resources/vehicle.obj", vertices, indices);	
	m_MeshesWorld.emplace_back(vertices, indices, PrimitiveTopology::TriangleList);	
}
Renderer::~Renderer()
{
	delete[] m_pDepthBufferPixels;
}

void Renderer::Update(Timer* pTimer)
{
	m_Camera.Update(pTimer);

	if(!m_Rotate) return;
	
	//Rotate the mesh at 1 Radians per second
	for(auto& mesh  : m_MeshesWorld)
	{
		const float rotationSpeed = (PI / 2.f) * pTimer->GetElapsed();
		mesh.Rotate(rotationSpeed, {0.f, 1.f, 0.f});
	}
}

void Renderer::Render()
{
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


		std::vector<Vertex_Out> clippedVertices_ndc =  vertices_ndc;//SutherlandHodgmanClipping(vertices_ndc);

		
		std::vector<Vector2> vertices_screen{};
		VertexTransformationToScreenSpace(clippedVertices_ndc, vertices_screen);
		
		
		// For each triangle
		for(mesh.m_pTriangleIterator->ResetIndex(); mesh.m_pTriangleIterator->HasNext();)
		{
			
			const std::vector<uint32_t> triangleIndices = mesh.m_pTriangleIterator->Next();
			RenderTriangle(vertices_screen, clippedVertices_ndc, triangleIndices);
		}
	}
	

	//Update SDL Surface
	SDL_UnlockSurface(m_pBackBuffer);
	SDL_BlitSurface(m_pBackBuffer, nullptr, m_pFrontBuffer, nullptr);
	SDL_UpdateWindowSurface(m_pWindow);
}


// function that transforms a vector of WORLD space vertices to a vector of NDC space vertices
void Renderer::VertexTransformationFunction(const std::vector<Vertex>& vertices_in, std::vector<Vertex_Out>& vertices_out, const Matrix& worldViewProjectionMatrix, const Matrix& meshWorldMatrix)
{
	vertices_out.resize(vertices_in.size());

	for (size_t i{}; i < vertices_in.size(); ++i)
	{		
		// Transform with VIEW matrix (inverse ONB)
		vertices_out[i].position = worldViewProjectionMatrix.TransformPoint({vertices_in[i].position, 1.0f});

		//Safe the vertex in world space (for specular shading)
		vertices_out[i].viewDirection = vertices_out[i].position;


		//Normals and are transformed with the world matrix
		vertices_out[i].normal = meshWorldMatrix.TransformVector(vertices_in[i].normal);
		vertices_out[i].tangent = meshWorldMatrix.TransformVector(vertices_in[i].tangent);


		//UV is passed through
		vertices_out[i].uv = vertices_in[i].uv;
		
		// Apply Perspective Divide to the vertices
		vertices_out[i].position.x /= vertices_out[i].position.w;
		vertices_out[i].position.y /= vertices_out[i].position.w;
		vertices_out[i].position.z /= vertices_out[i].position.w;		
	}
}


// function that transforms a vector of NDC space vertices to a vector of SCREEN space vertices
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
                              std::vector<Vertex_Out>& verticesNDC, const std::vector<uint32_t>& verticesIndexes) const
{
	//RENDER LOGIC
	//Calculate the current vertex index
	//Get index of last vertex,
	// +2
	// -1
	// +2
	
	const uint32_t vertexIndex0{ verticesIndexes[0] };
	const uint32_t vertexIndex1{ verticesIndexes[1] };
	const uint32_t vertexIndex2{ verticesIndexes[2] };
	
	//If A triangle has the same vertex, skip it
	if (vertexIndex0 == vertexIndex1 || vertexIndex1 == vertexIndex2 || vertexIndex0 == vertexIndex2) return;


	
#if Clipping
	//Should i resize the vector to 3?
	std::vector<uint32_t> verticesOutOfFrustum{};
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex0].position))
	{
		verticesOutOfFrustum.emplace_back(0);
	}
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex1].position))
	{
		verticesOutOfFrustum.push_back(1);
	}
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex2].position))
	{
		verticesOutOfFrustum.push_back(2);
	}

	if(verticesOutOfFrustum.size() == 3) return;

	
	//1. 2 Vertices are outside the frustum
	//We Should Add 2 new vertices to the triangle where the line gets clipped off, and render it as 1 new triangle
	if(verticesOutOfFrustum.size() == 2)
	{
		return;
	}

	//2. Only 1 Vertex is outside the frustum
	//We should add 2 new vertices to the triangle where the line gets clipped off,  and render it as 2 triangles
	if(verticesOutOfFrustum.size() == 1)
	{
		

		
		const auto index0 = verticesIndexes[verticesOutOfFrustum[0]];
		Vertex_Out vertexOutOfFrustum =  verticesNDC[index0];
    
		const auto index1 = verticesIndexes[((verticesOutOfFrustum[0] + 1) % 3)];
		const auto index2 =verticesIndexes[(verticesOutOfFrustum[0] + 2) % 3];
		Vertex_Out vertexInsideFrustum1 = verticesNDC[index1];
		Vertex_Out vertexInsideFrustum2 = verticesNDC[index2];

		Vector4 dir1 = vertexInsideFrustum1.position - vertexOutOfFrustum.position;
		Vector4 dir2 = vertexInsideFrustum2.position - vertexOutOfFrustum.position;

		// Calculate t for each axis and direction
		float t1[6], t2[6];
		for(int i = 0; i < 3; ++i)
		{
			t1[i*2] = (-1.0f - vertexOutOfFrustum.position[i]) / dir1[i]; // min
			t1[i*2+1] = (1.0f - vertexOutOfFrustum.position[i]) / dir1[i]; // max
			t2[i*2] = (-1.0f - vertexOutOfFrustum.position[i]) / dir2[i]; // min
			t2[i*2+1] = (1.0f - vertexOutOfFrustum.position[i]) / dir2[i]; // max
		}

		// Find valid t values that are within the range [0, 1]
		std::vector<float> validT1, validT2;
		for(int i = 0; i < 6; ++i)
		{
			if(t1[i] >= 0.0f && t1[i] <= 1.0f) validT1.push_back(t1[i]);
			if(t2[i] >= 0.0f && t2[i] <= 1.0f) validT2.push_back(t2[i]);
		}

		// Calculate the intersection points
		Vector4 intersectionPoint1 = vertexOutOfFrustum.position + dir1 * (*std::ranges::min_element(validT1));
		Vector4 intersectionPoint2 = vertexOutOfFrustum.position + dir2 * (*std::ranges::min_element(validT2));

		// Replace the vertex outside the frustum with the intersection point
		vertexOutOfFrustum.position = intersectionPoint1;

		// Create a new vertex with the second intersection point
		// Vertex_Out newVertex;
		//
		// newVertex.position = intersectionPoint2;
		// newVertex.normal = -vertexOutOfFrustum.normal;
		// newVertex.uv = vertexOutOfFrustum.uv;
		// newVertex.tangent = vertexOutOfFrustum.tangent;

		
		//verticesNDC.push_back(newVertex);
		// verticesNDC.push_back(vertexInsideFrustum1);
		// verticesNDC.push_back(vertexInsideFrustum2);
		//
		//
		// std::vector<Vector2> extraVerticesScreenSpace{};
		// VertexTransformationToScreenSpace({newVertex, vertexInsideFrustum1, vertexInsideFrustum2}, extraVerticesScreenSpace);
		//
		// verticesScreenSpace.push_back(extraVerticesScreenSpace[0]);
		// verticesScreenSpace.push_back(extraVerticesScreenSpace[1]);
		// verticesScreenSpace.push_back(extraVerticesScreenSpace[2]);
		//AND THIS WILL ONLY WORK WHEN ITS NOT A TRIANGLSTRIPPPPPPPPPPPPPP
		
	}

	
#else
	//If a vertex is out of the frustum, don't render the triangle.
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex0].position)) return;
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex1].position))  return;
	if(Camera::IsOutsideFrustum(verticesNDC[vertexIndex2].position))  return;
#endif
	
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
			//Calculate the Z depth
			const float depthV0{ (verticesNDC[vertexIndex0].position.z) };
			const float depthV1{ (verticesNDC[vertexIndex1].position.z) };
			const float depthV2{ (verticesNDC[vertexIndex2].position.z) };

			// Calculate the depth at this pixel
			const float interpolatedDepth
			{
				1.0f /
					(weightV0  / depthV0 +
					weightV1  / depthV1 +
					weightV2 / depthV2)
			};

			// If this pixel hit is further away then a previous pixel hit, continue to the next pixel
			if (m_pDepthBufferPixels[pixelIdx] < interpolatedDepth) continue;

			// Save the new depth
			m_pDepthBufferPixels[pixelIdx] = interpolatedDepth;

//-------------------------------------------------------------------------------

			if(m_DisplayDepthBuffer)
			{
				//Display the depth buffer when needed
				//Remap the interpolated depthColor to a range between 0 and 1
				// Min and Max values of the original range
				constexpr float minValue = 0.92f;
				constexpr float maxValue = 1.f;

				// Remap the value to the range [0, 1]
				float remappedValue = (interpolatedDepth - minValue) / (maxValue - minValue);
				remappedValue = std::clamp(remappedValue, 0.f, 1.f);
				
				finalColor =  ColorRGB{remappedValue, remappedValue, remappedValue};
				finalColor.MaxToOne();
				
				
				m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
					static_cast<uint8_t>(finalColor.r * 255),
					static_cast<uint8_t>(finalColor.g * 255),
					static_cast<uint8_t>(finalColor.b * 255));


				//I only calculate the first pixel of every vertex...
				continue;
			}

			

			//Calculate W the depth
			const float WdepthV0{ (verticesNDC[vertexIndex0].position.w) };
			const float WdepthV1{ (verticesNDC[vertexIndex1].position.w) };
			const float WdepthV2{ (verticesNDC[vertexIndex2].position.w) };
			
			
			// Calculate the depth at this pixel -> Linear [0,1]
			const float interpolatedWDepth
			{
				1.0f /
					(weightV0  / WdepthV0 +
					weightV1  / WdepthV1 +
					weightV2 / WdepthV2)
			};
			


			//Interpolate the needed values for shading
			Vertex_Out shadePixel{};
			
			//Calculate the UV
			shadePixel.uv =
			{
				(weightV0 * verticesNDC[vertexIndex0].uv / WdepthV0 +
				weightV1 * verticesNDC[vertexIndex1].uv / WdepthV1 +
				weightV2 * verticesNDC[vertexIndex2].uv / WdepthV2) * interpolatedWDepth
			};
			
			
			#if TextureTiling
			// Wrap UV coordinates to the [0, 1] range
			shadePixel.uv.x = fmod(shadePixel.uv.x, 1.0f);
			shadePixel.uv.y = fmod(shadePixel.uv.y, 1.0f);
			if (shadePixel.uv.x < 0.0f) shadePixel.uv.x += 1.0f;
			if (shadePixel.uv.y < 0.0f) shadePixel.uv.y += 1.0f;
			#else
			
			// Clamp UV coordinates to the [0, 1] range
			shadePixel.uv.x = std::clamp(shadePixel.uv.x, 0.0f, 1.0f);
			shadePixel.uv.y = std::clamp(shadePixel.uv.y, 0.0f, 1.0f);
			#endif

			//Calculate the normal
			shadePixel.normal =
			{
				((weightV0 * verticesNDC[vertexIndex0].normal / WdepthV0 +
				weightV1 * verticesNDC[vertexIndex1].normal / WdepthV1 +
				weightV2 * verticesNDC[vertexIndex2].normal / WdepthV2) * interpolatedWDepth).Normalized()
			};

			//Calculate the tangent
			shadePixel.tangent =
			{
				((weightV0 * verticesNDC[vertexIndex0].tangent / WdepthV0 +
				weightV1 * verticesNDC[vertexIndex1].tangent / WdepthV1 +
				weightV2 * verticesNDC[vertexIndex2].tangent / WdepthV2) * interpolatedWDepth).Normalized()
			};


			//Calculate the view direction
			shadePixel.viewDirection = 
			{
				((weightV0 * verticesNDC[vertexIndex0].viewDirection / WdepthV0 +
				weightV1 * verticesNDC[vertexIndex1].viewDirection / WdepthV1 +
				weightV2 * verticesNDC[vertexIndex2].viewDirection / WdepthV2) * interpolatedWDepth).Normalized()
			};

			
			Shade(shadePixel, finalColor);
			
			finalColor.MaxToOne();
			
			m_pBackBufferPixels[px + (py * m_Width)] = SDL_MapRGB(m_pBackBuffer->format,
				static_cast<uint8_t>(finalColor.r * 255),
				static_cast<uint8_t>(finalColor.g * 255),
				static_cast<uint8_t>(finalColor.b * 255));
		}
	}
}






void Renderer::Shade(const Vertex_Out& vertex, ColorRGB& finalColor) const
{

	assert(m_pTexture);
	assert(m_pGlossinessTexture);
	assert(m_pNormalTexture);
	assert(m_pSpecularTexture);

	
	Vector3 normal{ vertex.normal };

	//Use the normal map when enabled and the texture is loaded correctly
	if(m_UseNormalMap && m_pNormalTexture)
	{
		//First we would need the normal in tangent space
		//Calculate the biNormal
		const Vector3 biNormal{ Vector3::Cross(vertex.normal, vertex.tangent) };
		const Matrix tangentSpaceAxis{vertex.tangent, biNormal, vertex.normal, Vector3::Zero};

		//Sample the normal map
		ColorRGB normalSample = m_pNormalTexture->Sample(vertex.uv);
		
		//bring the normal map from [0, 1] to [-1, 1]
		normalSample = (normalSample * 2.f) - ColorRGB{ 1.f, 1.f, 1.f };
		
		const Vector3 normalSampled =  {normalSample.r, normalSample.g, normalSample.b};
		normal = tangentSpaceAxis.TransformVector(normalSampled);
	}
	
	
	
	const float observedArea{ std::max(Vector3::Dot(normal.Normalized(), -m_DirectionLight.Normalized()), 0.0f) };


	
	switch (m_ShadeMode)
	{
		case ShadeMode::ObservedArea:
		{
			finalColor = ColorRGB{ observedArea, observedArea, observedArea };
			return;
		}
		
		case ShadeMode::Diffuse:
		{
			// cd * (kd) / PI
			const ColorRGB lambert{ m_pTexture->Sample(vertex.uv) / PI };
			finalColor = ColorRGB(m_lightIntensity * observedArea * lambert);
			return;
		}

		case ShadeMode::Specular:
		{
				const auto reflectedLight{ Vector3::Reflect(-m_DirectionLight, normal) };
				const auto reflectedViewDot{ std::max(Vector3::Dot(reflectedLight, vertex.viewDirection), 0.0f) };
				

				const float phongExponent{ m_Glossiness * (m_pGlossinessTexture->Sample(vertex.uv).r / 255.f) };
				const auto phong = std::powf(reflectedViewDot, phongExponent);
				const ColorRGB phongColor{ phong, phong, phong };
				const ColorRGB specularColor{ m_pSpecularTexture->Sample(vertex.uv)  * phongColor };


				finalColor = m_lightIntensity * specularColor * observedArea;
				return;
		}
		
		case ShadeMode::Combined:
		{
				const auto reflectedLight{ Vector3::Reflect(-m_DirectionLight, normal) };
				const auto reflectedViewDot{ std::max(Vector3::Dot(reflectedLight, vertex.viewDirection), 0.0f) };
				

				const float phongExponent{ m_Glossiness * (m_pGlossinessTexture->Sample(vertex.uv).r / 255.f) };
				const auto phong = std::powf(reflectedViewDot, phongExponent);
				const ColorRGB phongColor{ phong, phong, phong };
				const ColorRGB specularColor{ m_pSpecularTexture->Sample(vertex.uv)  * phongColor };
				
				const ColorRGB lambert{ m_pTexture->Sample(vertex.uv) / PI };
				const ColorRGB ambient{ m_AmbientLight, m_AmbientLight, m_AmbientLight} ;
				
				finalColor = (m_lightIntensity * lambert + specularColor + ambient) * observedArea;
				return;
		}
	}
}

std::vector<Vertex_Out> dae::Renderer::ClipAgainstPlane(const std::vector<Vertex_Out>& inputVertices,
	const Vector4& plane)
{
	std::vector<Vertex_Out> outputVertices;

	// Loop through each edge of the polygon
	const size_t numVertices = inputVertices.size();
	for (size_t i = 0; i < numVertices; ++i)
	{
		// Current and next vertex in the input list
		const Vertex_Out& currentVertex = inputVertices[i];
		const Vertex_Out& nextVertex = inputVertices[(i + 1) % numVertices];

		// Calculate distances from the plane for the current and next vertices

		const float d1 = Vector4::Dot(plane, currentVertex.position) + plane.w;
		const float d2 = Vector4::Dot(plane, nextVertex.position)+ plane.w;


		// Check if the vertices are inside or outside the clipping plane
		if (d1 >= 0)
		{
			outputVertices.push_back(currentVertex);
		}

		// Check for intersection and add the intersection point
		if(d1 * d2 < 0)
		//if((d1 >= 0 && d2 < 0) || (d1 < 0 && d2 >= 0))
		{
			const float t = d1 / (d1 - d2);
			const Vector4 intersectionPoint = currentVertex.position + (nextVertex.position - currentVertex.position) * t;
			
			Vertex_Out newVertex = currentVertex;
			newVertex.position = intersectionPoint;
			outputVertices.push_back(newVertex);
		}
	}

	return outputVertices;
}

std::vector<Vertex_Out> Renderer::SutherlandHodgmanClipping(const std::vector<Vertex_Out>& inputVertices)
{
	//std::vector<Vertex_Out> outputVertices = inputVertices;
	// if(inputVertices.size() < 3) return outputVertices;
	//
	// // Define the clipping cube in NDC space
	// const float xmin = -1.0f, xmax = 1.0f;
	// const float ymin = -1.0f, ymax = 1.0f;
	// const float zmin = -1.0f, zmax = 1.0f;
	//
	// // Clip against each of the six planes of the clipping cube
	// outputVertices = ClipAgainstPlane(outputVertices, { 1, 0, 0, -xmin });
	// outputVertices = ClipAgainstPlane(outputVertices, { -1, 0, 0, xmax });
	// outputVertices = ClipAgainstPlane(outputVertices, { 0, 1, 0, -ymin });
	// outputVertices = ClipAgainstPlane(outputVertices, { 0, -1, 0, ymax });
	// outputVertices = ClipAgainstPlane(outputVertices, { 0, 0, 1, -zmin });
	// outputVertices = ClipAgainstPlane(outputVertices, { 0, 0, -1, zmax });

	std::vector<Vertex_Out> outputVertices;
	outputVertices.reserve(inputVertices.size());
	
	for(const auto& vertex : inputVertices)
	{
		if(Camera::IsOutsideFrustum(vertex.position)) continue;
		outputVertices.push_back(vertex);
	}
		
	outputVertices.shrink_to_fit();
	return outputVertices;
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
