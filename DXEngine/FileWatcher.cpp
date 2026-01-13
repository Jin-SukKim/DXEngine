#include "pch.h"
#include "FileWatcher.h"

namespace DE {
	namespace fs = std::filesystem;

	void FileWatcher::Register(const std::wstring& path, Callback callback)
	{
		if (!fs::exists(path)) {
			std::wcout << L"[FileWatcher] Warning: File not found: " << path << std::endl;
			return;
		}

		std::wstring absPath = fs::absolute(path).wstring();
		auto it = m_watchedFiles.find(absPath);
		if (it == m_watchedFiles.end()) {
			m_watchedFiles[absPath].lastWriteTime = fs::last_write_time(absPath);
		}
		
		m_watchedFiles[absPath].callbacks.emplace_back(callback);
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
					std::wcout << L"[FileWatcher] Detect Change : " << path << std::endl;

					for (auto& callback : file.callbacks) {
						if (callback)
							callback();
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