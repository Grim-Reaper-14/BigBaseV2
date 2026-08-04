#pragma once
#include "common.hpp"

namespace big
{
	enum class log_color : std::uint16_t
	{
		red = FOREGROUND_RED,
		green = FOREGROUND_GREEN,
		blue = FOREGROUND_BLUE,
		intensify = FOREGROUND_INTENSITY
	};

	inline log_color operator|(log_color a, log_color b)
	{
		return static_cast<log_color>(static_cast<std::underlying_type_t<log_color>>(a) | static_cast<std::underlying_type_t<log_color>>(b));
	}

	class logger;
	inline logger* g_logger{};

	class logger
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
			g_logger = nullptr;

			if (m_console_out.is_open())
				m_console_out.close();

			if (m_file_out.is_open())
				m_file_out.close();

			if (!m_did_console_exist)
				FreeConsole();
		}

		template <typename... Args>
		void raw(log_color color, const Args&... args)
		{
			std::scoped_lock lock(m_mutex);
			raw_to_console(color, args...);
			raw_to_file(args...);
		}

		template <typename... Args>
		void log(log_color color, std::string_view prefix, std::string_view format, const Args&... args)
		{
			const auto message = fmt::format(format, args...);
			const auto now = std::time(nullptr);

			std::tm local_time{};
			localtime_s(&local_time, &now);

			const auto console_timestamp = fmt::format(
				"[{:02}:{:02}:{:02}]",
				local_time.tm_hour,
				local_time.tm_min,
				local_time.tm_sec);

			const auto file_timestamp = fmt::format(
				"[{:04}-{:02}-{:02} {:02}:{:02}:{:02}]",
				local_time.tm_year + 1900,
				local_time.tm_mon + 1,
				local_time.tm_mday,
				local_time.tm_hour,
				local_time.tm_min,
				local_time.tm_sec);

			std::scoped_lock lock(m_mutex);
			raw_to_console(color, console_timestamp, " [", prefix, "] ", message, "\n");
			raw_to_file(file_timestamp, " [", prefix, "] ", message, "\n");
		}

	private:
		void initialize_file()
		{
			try
			{
				std::filesystem::path base_path;
				if (const char* appdata = std::getenv("APPDATA"); appdata != nullptr && *appdata != '\0')
					base_path = appdata;
				else
					base_path = std::filesystem::temp_directory_path();

				m_file_path = base_path / "BigBaseV2";
				std::filesystem::create_directories(m_file_path);
				m_file_path /= "BigBaseV2.log";
				m_file_out.open(m_file_path, std::ios::out | std::ios::app);
			}
			catch (const std::exception&)
			{
				m_file_out.close();
			}
		}

		void initialize_console()
		{
			m_did_console_exist = AttachConsole(ATTACH_PARENT_PROCESS) != FALSE;
			if (!m_did_console_exist && AllocConsole() == FALSE)
				return;

			m_console_handle = GetStdHandle(STD_OUTPUT_HANDLE);
			if (m_console_handle == nullptr || m_console_handle == INVALID_HANDLE_VALUE)
			{
				m_console_handle = nullptr;
				return;
			}

			SetConsoleTitleA("BigBaseV2");
			SetConsoleOutputCP(CP_UTF8);
			m_console_out.open("CONOUT$", std::ios::out | std::ios::app);
		}

		template <typename... Args>
		void raw_to_console(log_color color, const Args&... args)
		{
			if (m_console_handle != nullptr)
				SetConsoleTextAttribute(m_console_handle, static_cast<std::uint16_t>(color));

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

		std::mutex m_mutex;
		bool m_did_console_exist{};
		HANDLE m_console_handle{};
		std::ofstream m_console_out;
		std::filesystem::path m_file_path;
		std::ofstream m_file_out;
	};

	template <typename... Args>
	inline void log_info(std::string_view format, const Args&... args)
	{
		if (g_logger != nullptr)
			g_logger->log(log_color::blue | log_color::intensify, "Info", format, args...);
	}

	template <typename... Args>
	inline void log_error(std::string_view format, const Args&... args)
	{
		if (g_logger != nullptr)
			g_logger->log(log_color::red | log_color::intensify, "Error", format, args...);
	}

	template <typename... Args>
	inline void log_trace(std::string_view format, const Args&... args)
	{
		if (g_logger != nullptr)
			g_logger->log(log_color::green | log_color::intensify, "Trace", format, args...);
	}

	template <typename... Args>
	inline void log_raw(log_color color, const Args&... args)
	{
		if (g_logger != nullptr)
			g_logger->raw(color, args...);
	}

#define LOG_INFO_IMPL(format, ...) (::big::log_info(format, __VA_ARGS__))
#define LOG_INFO(format, ...) LOG_INFO_IMPL(format, __VA_ARGS__)

#define LOG_ERROR_IMPL(format, ...) (::big::log_error(format, __VA_ARGS__))
#define LOG_ERROR(format, ...) LOG_ERROR_IMPL(format, __VA_ARGS__)

#define LOG_TRACE_IMPL(format, ...) (::big::log_trace(format, __VA_ARGS__))
#define LOG_TRACE(format, ...) LOG_TRACE_IMPL(format, __VA_ARGS__)

#define LOG_RAW_IMPL(color, ...) (::big::log_raw(color, __VA_ARGS__))
#define LOG_RAW(color, ...) LOG_RAW_IMPL(color, __VA_ARGS__)
}
