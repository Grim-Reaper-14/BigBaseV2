#pragma once

#include <SDKDDKVer.h>
#include <Windows.h>
#include <D3D11.h>
#include <D3D12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

#include <algorithm>
#include <any>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cinttypes>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <StackWalker.h>

#include "logger.hpp"

namespace big
{
	using namespace std::chrono_literals;

	template <typename T>
	using comptr = Microsoft::WRL::ComPtr<T>;

	inline HMODULE g_hmodule{};
	inline HANDLE g_main_thread{};
	inline DWORD g_main_thread_id{};
	inline std::atomic_bool g_running{true};

	struct stackwalker final : StackWalker
	{
		using StackWalker::StackWalker;

		void OnOutput(LPCSTR text) override
		{
			if (g_logger && text)
				g_logger->raw(log_color::red | log_color::intensify, text);
			else if (text)
				OutputDebugStringA(text);
		}
	};

	inline stackwalker g_stackwalker;
}
