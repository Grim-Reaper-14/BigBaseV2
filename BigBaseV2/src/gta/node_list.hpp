#pragma once

#include "base.hpp"

namespace rage
{
	template <typename T, typename Base = datBase>
	class atDNode : public Base
	{
	public:
		T m_data{};
		void* m_unknown{};
		atDNode<T, Base>* m_next{};
	};

	template <typename Node>
	class atDList
	{
	public:
		[[nodiscard]] bool empty() const noexcept
		{
			return m_head == nullptr;
		}

		[[nodiscard]] Node* front() noexcept
		{
			return m_head;
		}

		[[nodiscard]] const Node* front() const noexcept
		{
			return m_head;
		}

		Node* m_head{};
		Node* m_tail{};
	};

	static_assert(sizeof(atDList<atDNode<void*>>) == 0x10);
}
