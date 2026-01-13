#pragma once

namespace DE {
class FileWatcher
{
public:
	using Callback = std::function<void()>;

	static FileWatcher& Get() {
		static FileWatcher instance;
		return instance;
	}

	void Register(const std::wstring& path, Callback callback);
	void Update();

private:
	struct FileEntity {
		std::filesystem::file_time_type lastWriteTime;
		std::vector<Callback> callbacks;
	};

	// <filePath, FileEntity>
	std::unordered_map<std::wstring, FileEntity> m_watchedFiles;
};

}

