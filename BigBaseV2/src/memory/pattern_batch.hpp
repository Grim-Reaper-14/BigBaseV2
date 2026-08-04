#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "pattern.hpp"
#include "range.hpp"

namespace memory
{
	class pattern_batch final
	{
	public:
		struct run_result final
		{
			std::size_t total{};
			std::size_t found{};
			std::vector<std::string> missing;

			[[nodiscard]] bool success() const noexcept
			{
				return missing.empty() && found == total;
			}
		};

		struct entry final
		{
			std::string name;
			pattern signature;
			std::function<void(handle)> callback;
		};

		pattern_batch() = default;
		~pattern_batch() = default;

		pattern_batch(const pattern_batch&) = delete;
		pattern_batch(pattern_batch&&) noexcept = default;
		pattern_batch& operator=(const pattern_batch&) = delete;
		pattern_batch& operator=(pattern_batch&&) noexcept = default;

		void add(std::string name, pattern signature, std::function<void(handle)> callback);
		[[nodiscard]] run_result run(const range& region, bool throw_on_failure = true);
		void clear() noexcept;

		[[nodiscard]] std::size_t size() const noexcept
		{
			return m_entries.size();
		}

		[[nodiscard]] bool empty() const noexcept
		{
			return m_entries.empty();
		}

	private:
		std::vector<entry> m_entries;
	};
}
