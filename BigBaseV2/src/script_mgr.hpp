#pragma once
#include "common.hpp"
#include "script.hpp"

namespace big
{
	class script_mgr final
	{
	public:
		script_mgr() = default;
		~script_mgr() = default;

		script_mgr(const script_mgr&) = delete;
		script_mgr(script_mgr&&) = delete;
		script_mgr& operator=(const script_mgr&) = delete;
		script_mgr& operator=(script_mgr&&) = delete;

		void add_script(std::unique_ptr<script> instance);
		void remove_all_scripts();
		void tick();

		[[nodiscard]] std::size_t size() const;

	private:
		void tick_internal();
		void ensure_runtime_ready();

		mutable std::recursive_mutex m_mutex;
		std::vector<std::unique_ptr<script>> m_scripts;
		bool m_runtime_ready{};
	};

	inline script_mgr g_script_mgr;
}
