#pragma once

namespace DE {
// The max shader resource slots is 128, and for feature level 11 the max texture array size is 2048.
class AssetManager
{
public:
	struct TextureEntity {
		std::string path;
		UINT idx;
	};
	static AssetManager& Get() {
		static AssetManager instance;
		return instance;
	}

	void Initialize();
	TextureEntity LoadParticleTexture(const std::string& path);
	void BindParticleTextures();
private:
	static const UINT PARTICLE_TEXTURE_WIDTH = 512;
	static const UINT PARTICLE_TEXTURE_HEIGHT = 512;
	static const UINT MAX_PARTICLE_TEXTURES = 64;
	static const bool particleSRGB = true;

	std::unordered_map<std::string, UINT> m_pathToIndexMap;
	std::unique_ptr<Texture2D> m_particleTextureArray;
	UINT m_nextFreeIndex = 0;
	std::string presetPath = "..\\Assets\\";
};
}

