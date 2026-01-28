#pragma once

namespace DE {
	template<typename t>
	class ObjectPool
	{
	public:
		using Factory = std::function <std::unique_ptr<T>()>;

		std::unique_ptr<T> Acquire(Factory factory) {

		}

	private:
		UINT m_poolSize = 0;
	};
}

