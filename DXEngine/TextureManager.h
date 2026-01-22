#pragma once

namespace DE {
class TextureManager
{
public:
	struct TextureEntity {
		std::string path;  // 이제 상대 경로만 저장
		int idx;
	};
	static TextureManager& Get() {
		static TextureManager instance;
		return instance;
	}

	void Initialize();
	TextureEntity LoadParticleTexture(const std::string& path);
	void BindParticleTextures();

	// 일반 Texture
	int LoadTexture(const std::string& path, bool isSRGB);
	int LoadMetallicRoughnessTexture(const std::string& metallicPath, const std::string& roughnessPath);
	std::pair<int, int> LoadMetallicRoughnessTexture(const std::string& path);
	ID3D11ShaderResourceView* GetTextureSRV(int index);
	std::string GetTexturePath(int index);
private:
	static const UINT PARTICLE_TEXTURE_WIDTH = 512;
	static const UINT PARTICLE_TEXTURE_HEIGHT = 512;
	static const UINT MAX_PARTICLE_TEXTURES = 64;
	static const bool particleSRGB = true;

	std::unordered_map<std::string, int> m_pathToIndexMap;  // 상대 경로로 검색
	std::unique_ptr<Texture2D> m_particleTextureArray;
	int m_nextFreeIndex = 0;

	// 일반 Texture 관리
	std::unordered_map<std::string, int> m_texturePathToIdx;  // 상대 경로로 검색
	std::unordered_map<int, std::string> m_indexToPathMap;    // 역참조 시 상대 경로 반환
	std::vector<std::unique_ptr<Texture2D>> m_textures;

	std::string presetPath = "..\\Assets\\";  // 내부 전용
};
}

