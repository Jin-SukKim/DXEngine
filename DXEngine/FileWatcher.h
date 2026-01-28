#pragma once
#include <functional>
#include <unordered_map>
#include <filesystem>

namespace DE {

class FileWatcher
{
public:
	using Callback = std::function<void()>;

	using CallbackID = size_t;

	static FileWatcher& Get() {
		static FileWatcher instance;
		return instance;
	}

	// 명시적 종료
	void Shutdown() {
		m_isShuttingDown = true;
		m_watchedFiles.clear();
	}

	CallbackID Register(const std::wstring& path, Callback callback);
	void Unregister(const std::wstring& path, CallbackID id);
	void Update();

private:
	FileWatcher() = default;
	~FileWatcher() {
		m_isShuttingDown = true;
		m_watchedFiles.clear();
	}

	struct CallbackEntity {
		CallbackID id;
		Callback callback;
	};

	struct FileEntity {
		std::filesystem::file_time_type lastWriteTime;
		std::vector<CallbackEntity> callbacks;
	};

	std::unordered_map<std::wstring, FileEntity> m_watchedFiles;
	CallbackID m_nextID = 0;
	bool m_isShuttingDown = false;  // 종료 플래그
};

}

