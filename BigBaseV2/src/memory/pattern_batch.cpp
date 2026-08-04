#include "../common.hpp"
#include "../logger.hpp"
#include "pattern_batch.hpp"

namespace memory
{
	void pattern_batch::add(std::string name, pattern signature, std::function<void(handle)> callback)
	{
		if (name.empty())
			throw std::invalid_argument("Pattern batch entries require a name.");
		if (!callback)
			throw std::invalid_argument("Pattern batch entries require a callback.");

		m_entries.push_back({std::move(name), std::move(signature), std::move(callback)});
	}

	pattern_batch::run_result pattern_batch::run(const range& region, bool throw_on_failure)
	{
		run_result result{};
		result.total = m_entries.size();

		for (auto& entry : m_entries)
		{
			const auto address = region.scan(entry.signature);
			if (!address)
			{
				result.missing.push_back(entry.name);
				LOG_ERROR("Failed to find '{}'.", entry.name);
				continue;
			}

			try
			{
				entry.callback(address);
				++result.found;
				LOG_INFO("Found '{}'.", entry.name);
			}
			catch (const std::exception& exception)
			{
				result.missing.push_back(entry.name);
				LOG_ERROR("Pattern callback '{}' failed: {}", entry.name, exception.what());
			}
			catch (...)
			{
				result.missing.push_back(entry.name);
				LOG_ERROR("Pattern callback '{}' failed with an unknown exception.", entry.name);
			}
		}

		clear();

		if (throw_on_failure && !result.success())
		{
			std::string message = "Failed to resolve required patterns:";
			for (const auto& name : result.missing)
				message += " " + name;
			throw std::runtime_error(message);
		}

		return result;
	}

	void pattern_batch::clear() noexcept
	{
		m_entries.clear();
	}
}
