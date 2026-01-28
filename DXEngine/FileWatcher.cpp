#include "pch.h"
#include "FileWatcher.h"

namespace DE {
	namespace fs = std::filesystem;

	FileWatcher::CallbackID FileWatcher::Register(const std::wstring& path, Callback callback)
	{
		if (m_isShuttingDown || path.empty() || !fs::exists(path)) {
			return 0;
		}

		std::wstring absPath = fs::absolute(path).wstring();
		auto it = m_watchedFiles.find(absPath);
		if (it == m_watchedFiles.end()) {
			m_watchedFiles[absPath].lastWriteTime = fs::last_write_time(absPath);
		}
		
		CallbackID id = ++m_nextID;
		m_watchedFiles[absPath].callbacks.emplace_back(CallbackEntity{ id, callback });
		return id;
	}

	void FileWatcher::Unregister(const std::wstring& path, CallbackID id)
	{
		// 종료 중이거나 ID가 0이면 무시
		if (m_isShuttingDown || id == 0 || path.empty())
			return;

		std::wstring absPath;
		try {
			absPath = fs::absolute(path).wstring();
		}
		catch (...) {
			return;  // 경로 변환 실패 시 조용히 종료
		}

		// map 접근 전 종료 상태 재확인
		if (m_isShuttingDown)
			return;

		try {
			auto it = m_watchedFiles.find(absPath);
			if (it != m_watchedFiles.end()) {
				auto& list = it->second.callbacks;
				
				auto removeIt = std::remove_if(list.begin(), list.end(), 
					[id](const CallbackEntity& entity) { return entity.id == id; });
				
				if (removeIt != list.end()) {
					list.erase(removeIt, list.end());
				}

				if (list.empty()) {
					m_watchedFiles.erase(it);
				}
			}
		}
		catch (...) {
			// 프로그램 종료 중 예외 무시
			m_isShuttingDown = true;
		}
	}

	void FileWatcher::Update()
	{
		if (m_isShuttingDown)
			return;

		for (auto& [path, file] : m_watchedFiles) {
			if (!fs::exists(path))
				continue;

			try {
				auto currentTime = fs::last_write_time(path);
				if (file.lastWriteTime < currentTime) {
					file.lastWriteTime = currentTime;

					for (auto& entity : file.callbacks) {
						if (entity.callback)
							entity.callback();
					}
				}
			} 
			catch (const std::exception& e) {
				// 파일 저장 중 에러 발생 시 다음 프레임에 재시도
			}
		}
	}
}