#pragma once

namespace DE {
class TextureManager
{
public:
	struct TextureEntity {
		std::string path;  // ���� ��� ��θ� ����
		int idx;
	};
	static TextureManager& Get() {
		static TextureManager instance;
		return instance;
	}

	void Initialize();
	TextureEntity LoadParticleTexture(const std::string& path);
	void BindParticleTextures();

	// Curl Noise 3D Texture
	void GenerateCurlNoiseTexture(UINT resolution = 64, float frequency = 4.0f);
	void BindCurlNoiseTexture(UINT slot = 26);
	void Generate2DCurlNoiseTexture(UINT resolution = 64, float frequency = 4.f);
	void Bind2DCurlNoiseTexture(UINT slot = 27);

	// �Ϲ� Texture
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

	std::unordered_map<std::string, int> m_pathToIndexMap;  // ��� ��η� �˻�
	std::unique_ptr<Texture2D> m_particleTextureArray;
	int m_nextFreeIndex = 0;

	// �Ϲ� Texture ����
	std::unordered_map<std::string, int> m_texturePathToIdx;  // ��� ��η� �˻�
	std::unordered_map<int, std::string> m_indexToPathMap;    // ������ �� ��� ��� ��ȯ
	std::vector<std::unique_ptr<Texture2D>> m_textures;

	std::string presetPath = "..\\Assets\\";  // ���� ����

	// Curl Noise 3D Texture
	ComPtr<ID3D11Texture3D> m_curlNoiseTexture;
	ComPtr<ID3D11ShaderResourceView> m_curlNoiseSRV;

	Texture2D m_curlNoise2D;
};
}

