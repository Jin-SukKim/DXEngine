#pragma once

namespace DE {
class FileWatcher
{
public:
	using Callback = std::function<void()>;
	using CallbackID = UINT;

	static FileWatcher& Get() {
		static FileWatcher instance;
		return instance;
	}

	CallbackID Register(const std::wstring& path, Callback callback);
	void Unregister(const std::wstring& path, CallbackID id);
	void Update();

private:
	struct CallbackEntity {
		CallbackID id;
		Callback callback;
	};

	struct FileEntity {
		std::filesystem::file_time_type lastWriteTime;
		std::vector<CallbackEntity> callbacks;
	};

	// <filePath, FileEntity>
	std::unordered_map<std::wstring, FileEntity> m_watchedFiles;
	CallbackID m_nextID = 0;
};

}

