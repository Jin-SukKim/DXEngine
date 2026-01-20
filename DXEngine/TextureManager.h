#pragma once

namespace DE {
// The max shader resource slots is 128, and for feature level 11 the max texture array size is 2048.
class TextureManager
{
public:
	struct TextureEntity {
		std::string path;
		int idx;
	};
	static TextureManager& Get() {
		static TextureManager instance;
		return instance;
	}

	void Initialize();
	TextureEntity LoadParticleTexture(const std::string& path);
	void BindParticleTextures();

	// 老馆 Texture
	int LoadTexture(const std::string& path, bool isSRGB);
	int LoadMetallicRoughnessTexture(const std::string& metallicPath, const std::string& roughnessPath);
	ID3D11ShaderResourceView* GetTextureSRV(int index);

private:
	static const UINT PARTICLE_TEXTURE_WIDTH = 512;
	static const UINT PARTICLE_TEXTURE_HEIGHT = 512;
	static const UINT MAX_PARTICLE_TEXTURES = 64;
	static const bool particleSRGB = true;

	std::unordered_map<std::string, int> m_pathToIndexMap;
	std::unique_ptr<Texture2D> m_particleTextureArray;
	int m_nextFreeIndex = 0;

	// 老馆 Texture 包府
	std::unordered_map<std::string, int> m_texturePathToIdx;
	std::vector<std::unique_ptr<Texture2D>> m_textures;

	std::string presetPath = "..\\Assets\\";
};
}

