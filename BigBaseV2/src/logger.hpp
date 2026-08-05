#pragma once

#include <Windows.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

#include <fmt/format.h>

namespace big
{
	enum class log_color : std::uint16_t
	{
		red = FOREGROUND_RED,
		green = FOREGROUND_GREEN,
		blue = FOREGROUND_BLUE,
		yellow = FOREGROUND_RED | FOREGROUND_GREEN,
		white = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE,
		intensify = FOREGROUND_INTENSITY
	};

	[[nodiscard]] inline constexpr log_color operator|(log_color left, log_color right) noexcept
	{
		using underlying = std::underlying_type_t<log_color>;
		return static_cast<log_color>(static_cast<underlying>(left) | static_cast<underlying>(right));
	}

	class logger;
	inline logger* g_logger{};

	class logger final
	{
	public:
		logger()
		{
			initialize_file();
			initialize_console();
			g_logger = this;
		}

		~logger()
		{
			std::scoped_lock lock(m_mutex);
			if (g_logger == this)
				g_logger = nullptr;

			m_console_out.close();
			m_file_out.close();

			if (!m_console_preexisted && m_console_allocated)
				FreeConsole();
		}

		logger(const logger&) = delete;
		logger(logger&&) = delete;
		logger& operator=(const logger&) = delete;
		logger& operator=(logger&&) = delete;

		template <typename... Args>
		void raw(log_color color, const Args&... args)
		{
			std::scoped_lock lock(m_mutex);
			raw_to_console(color, args...);
			raw_to_file(args...);
		}

		template <typename... Args>
		void log(log_color color, std::string_view prefix, fmt::string_view format, Args&&... args)
		{
			const auto message = fmt::format(format, std::forward<Args>(args)...);
			const auto timestamps = make_timestamps();

			std::scoped_lock lock(m_mutex);
			raw_to_console(color, timestamps.console, " [", prefix, "] ", message, "\n");
			raw_to_file(timestamps.file, " [", prefix, "] ", message, "\n");
		}

		[[nodiscard]] const std::filesystem::path& file_path() const noexcept
		{
			return m_file_path;
		}

	private:
		struct timestamp_pair final
		{
			std::string console;
			std::string file;
		};

		static timestamp_pair make_timestamps()
		{
			const auto now = std::chrono::system_clock::now();
			const auto time = std::chrono::system_clock::to_time_t(now);
			std::tm local{};
			localtime_s(&local, &time);

			return {
				fmt::format("[{:02}:{:02}:{:02}]", local.tm_hour, local.tm_min, local.tm_sec),
				fmt::format("[{:04}-{:02}-{:02} {:02}:{:02}:{:02}]",
					local.tm_year + 1900,
					local.tm_mon + 1,
					local.tm_mday,
					local.tm_hour,
					local.tm_min,
					local.tm_sec)
			};
		}

		void initialize_file() noexcept
		{
			try
			{
				const char* appdata = std::getenv("APPDATA");
				m_file_path = appdata && *appdata
					? std::filesystem::path(appdata) / "BigBaseV2"
					: std::filesystem::temp_directory_path() / "BigBaseV2";

				std::error_code error;
				std::filesystem::create_directories(m_file_path, error);
				if (error)
					return;

				m_file_path /= "BigBaseV2.log";
				m_file_out.open(m_file_path, std::ios::out | std::ios::app);
			}
			catch (...)
			{
				m_file_path.clear();
			}
		}

		void initialize_console() noexcept
		{
			m_console_preexisted = AttachConsole(ATTACH_PARENT_PROCESS) != FALSE;
			if (!m_console_preexisted)
				m_console_allocated = AllocConsole() != FALSE;

			m_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
			if (!m_console_handle || m_console_handle == INVALID_HANDLE_VALUE)
			{
				m_console_handle = nullptr;
				return;
			}

			SetConsoleTitleA("BigBaseV2 Enhanced");
			SetConsoleOutputCP(CP_UTF8);
			m_console_out.open("CONOUT$", std::ios::out | std::ios::app);
		}

		template <typename... Args>
		void raw_to_console(log_color color, const Args&... args)
		{
			if (m_console_handle)
				SetConsoleTextAttribute(m_console_handle, static_cast<WORD>(color));

			if (m_console_out)
			{
				((m_console_out << args), ...);
				m_console_out.flush();
			}
		}

		template <typename... Args>
		void raw_to_file(const Args&... args)
		{
			if (m_file_out)
			{
				((m_file_out << args), ...);
				m_file_out.flush();
			}
		}

		mutable std::mutex m_mutex;
		bool m_console_preexisted{};
		bool m_console_allocated{};
		HANDLE m_console_handle{};
		std::ofstream m_console_out;
		std::filesystem::path m_file_path;
		std::ofstream m_file_out;
	};

	template <typename... Args>
	inline void log_info(fmt::string_view format, Args&&... args)
	{
		if (g_logger)
			g_logger->log(log_color::blue | log_color::green | log_color::intensify, "Info", format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline void log_warning(fmt::string_view format, Args&&... args)
	{
		if (g_logger)
			g_logger->log(log_color::yellow | log_color::intensify, "Warning", format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline void log_error(fmt::string_view format, Args&&... args)
	{
		if (g_logger)
			g_logger->log(log_color::red | log_color::intensify, "Error", format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline void log_trace(fmt::string_view format, Args&&... args)
	{
		if (g_logger)
			g_logger->log(log_color::white, "Trace", format, std::forward<Args>(args)...);
	}

	template <typename... Args>
	inline void log_raw(log_color color, const Args&... args)
	{
		if (g_logger)
			g_logger->raw(color, args...);
	}
}

#define LOG_INFO(...) (::big::log_info(__VA_ARGS__))
#define LOG_WARNING(...) (::big::log_warning(__VA_ARGS__))
#define LOG_ERROR(...) (::big::log_error(__VA_ARGS__))
#define LOG_TRACE(...) (::big::log_trace(__VA_ARGS__))
#define LOG_RAW(...) (::big::log_raw(__VA_ARGS__))
