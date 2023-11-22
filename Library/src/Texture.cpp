#include "Texture.h"

#include <cassert>

#include "Vector2.h"
#include <SDL_image.h>

namespace dae
{
	Texture::Texture(SDL_Surface* pSurface) :
		m_pSurface{ pSurface },
		m_pSurfacePixels{ static_cast<uint32_t*>(pSurface->pixels) }
	{
	}

	Texture::~Texture()
	{
		if (m_pSurface)
		{
			SDL_FreeSurface(m_pSurface);
			m_pSurface = nullptr;
		}
	}

	Texture* Texture::LoadFromFile(const std::string& path)
	{
		SDL_Surface* pSurface = IMG_Load(path.c_str());


		//Todo implement own assertion library?
		assert(pSurface != nullptr && IMG_GetError());
		
		if(!pSurface) return nullptr;

		
		return new Texture(pSurface);
	}

	ColorRGB Texture::Sample(const Vector2& uv) const
	{
		//assert if u or v of uv are out of range [0,1]
		assert(uv.x >= 0.f && uv.x <= 1.f && uv.y >= 0.f && uv.y <= 1.f && "uv out of range [0,1]");

		
		Uint8 r{};
		Uint8 g{};
		Uint8 b{};

		// Assuming m_pSurface->w and m_pSurface->h don't change
		const int textureWidth = m_pSurface->w;
		const int textureHeight = m_pSurface->h;
		
		// Round to the nearest integer for pixel coordinates		
		const int xCord = static_cast<int>(std::lround(static_cast<float>(textureWidth) * uv.x + 0.5f));
		const int yCord = static_cast<int>(std::lround(static_cast<float>(textureHeight) * uv.y + 0.5f));

		// Get the pixel index
		const auto surfacePixelIndex = xCord + yCord * textureWidth;
		const Uint32 pixelIndex{ m_pSurfacePixels[surfacePixelIndex] };

		SDL_GetRGB(pixelIndex, m_pSurface->format, &r, &g, &b);

		return {r / 255.f, g / 255.f, b / 255.f};
	}
}