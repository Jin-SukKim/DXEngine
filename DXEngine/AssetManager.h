#pragma once

namespace DE {
class AssetManager
{
public:
	UINT Load(std::wstring& path);
	void BindGPU();

	static AssetManager& Get() {
		static AssetManager instance;
		return instance;
	}

	UINT MaxTextureCount = 30;
private:
	std::vector<std::string> m_textures;
	std::unordered_map<std::string, UINT> m_texturesIdx;
	Texture2D m_textureArray;
};
}

