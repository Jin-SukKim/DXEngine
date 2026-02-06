#pragma once
#include <chrono>
#include <functional>

class ScopedTimer
{
public:
	// 소멸 시점에 실행할 콜백(람다)을 받습니다.
	ScopedTimer(std::function<void(float)> onComplete)
		: m_callback(onComplete)
	{
		m_start = std::chrono::high_resolution_clock::now();
	}

	~ScopedTimer()
	{
		auto end = std::chrono::high_resolution_clock::now();
		float elapsed = std::chrono::duration<float, std::milli>(end - m_start).count();
		if (m_callback)
			m_callback(elapsed);
	}

private:
	std::chrono::high_resolution_clock::time_point m_start;
	std::function<void(float)> m_callback;
};