#include "pch.h"
#include "FileWatcher.h"

namespace DE {
	namespace fs = std::filesystem;

	FileWatcher::CallbackID FileWatcher::Register(const std::wstring& path, ReloadCallback callback)
	{
		if (!fs::exists(path)) {
			// 파일이 없다면 
			std::wcout << L"[FileWatcher] Warning: File not found: " << path << std::endl;
			return -1;
		}

		// 같은 파일 이름이라도 다른 폴더라면 등록 가능하도록 절대 경로 사용
		std::wstring absPath = fs::absolute(path).wstring();
		if (m_watchedFiles.find(absPath) == m_watchedFiles.end()) {
			// 처음 등록되는 파일이라면 현재 수정 시간 저장
			m_watchedFiles[absPath].lastWriteTime = fs::last_write_time(absPath);
		}

		CallbackID newID = ++m_nextID;
		m_watchedFiles[absPath].callbacks.push_back({ newID, callback });

		return newID;
	}

	void FileWatcher::Unregister(const std::wstring& path, CallbackID id)
	{
		if (!fs::exists(path)) return;
		std::wstring absPath = fs::absolute(path).wstring();

		auto it = m_watchedFiles.find(absPath);
		if (it != m_watchedFiles.end()) {
			auto& list = it->second.callbacks;

			// ID가 일치하는 콜백 제거
			list.erase(
				std::remove_if(list.begin(), list.end(),
					[id](const CallbackEntity& entry) { return entry.id == id; }),
				list.end()
			);

			// 더 이상 감시할 callback이 없으면 제거
			if (list.empty()) {
				m_watchedFiles.erase(it);
			}
		}
	}

	void FileWatcher::Update()
	{
		for (auto& [path, entity] : m_watchedFiles) {
			if (!fs::exists(path))
				continue;

			try {
				auto currentWriteTime = fs::last_write_time(path);

				if (entity.lastWriteTime < currentWriteTime) {
					entity.lastWriteTime = currentWriteTime;
					std::wcout << L"[FileWatcher] Detected Change: " << path << std::endl;

					// 등록된 모든 함수 실행
					for (auto& cbEntity : entity.callbacks)
						if (cbEntity.callback)
							cbEntity.callback();
				}
			}
			catch (const std::exception& e) {
				// 파일 저장 직후 읽기를 실패할 수 있는데 무시하고 다음 프레임에 재시도
			}
		}
	}
}