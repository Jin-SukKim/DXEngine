#include "pch.h"
#include "FileWatcher.h"

namespace DE {
	namespace fs = std::filesystem;

	FileWatcher::CallbackID FileWatcher::Register(const std::wstring& path, Callback callback)
	{
		if (!fs::exists(path)) {
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
		if (!fs::exists(path))
			return;

		std::wstring absPath = fs::absolute(path).wstring();

		auto it = m_watchedFiles.find(absPath);
		if (it != m_watchedFiles.end()) {
			auto& list = it->second.callbacks;
			list.erase(std::remove_if(list.begin(), list.end(), 
					[id](const CallbackEntity& entity) { return entity.id == id; }), list.end());

			// callback이 비어있다면 unordered_map의 element를 삭제
			if (list.empty())
				m_watchedFiles.erase(it);
		}
	}

	void FileWatcher::Update()
	{
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
				// file이 저장될때 못 읽어와서 에러가 발생할 수 있음
				// 이때 다음 프레임에 다시 시도
			}
		}
	}

}