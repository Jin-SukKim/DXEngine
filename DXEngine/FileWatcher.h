#pragma once

namespace DE {
class FileWatcher
{
public:
	using ReloadCallback = std::function<void()>;
	using CallbackID = UINT;

	static FileWatcher& Get() {
		static FileWatcher instance;
		return instance;
	}

	// 파일 감시 등록
	CallbackID Register(const std::wstring& path, ReloadCallback callback);
	void Unregister(const std::wstring& path, CallbackID id);

	void Update(); // 변경사항 체크

private:
	struct CallbackEntity {
		CallbackID id;
		ReloadCallback callback;
	};

	struct FileEntity {
		std::filesystem::file_time_type lastWriteTime;
		// 파일이 바뀌었을때 실행할 함수들 callback
		std::vector<CallbackEntity> callbacks; // 한 파일을 여러 곳에서 쓸 수 있으므로 배열 사용
	};
	std::unordered_map<std::wstring, FileEntity> m_watchedFiles;
	CallbackID m_nextID = -1;
};

}

