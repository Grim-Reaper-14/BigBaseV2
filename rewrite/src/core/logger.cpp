#include "core/logger.hpp"

#include <Windows.h>

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>

namespace reaper::core
{
    logger& logger::instance()
    {
        static logger value;
        return value;
    }

    bool logger::initialize()
    {
        std::scoped_lock lock(m_mutex);

        if (!GetConsoleWindow())
        {
            m_console_owned = AllocConsole() != FALSE;
            if (m_console_owned)
            {
                FILE* stream{};
                freopen_s(&stream, "CONOUT$", "w", stdout);
                freopen_s(&stream, "CONOUT$", "w", stderr);
                SetConsoleTitleW(L"ReaperCore");
            }
        }

        const char* appdata = std::getenv("APPDATA");
        m_path = appdata && *appdata
            ? std::filesystem::path(appdata) / "ReaperCore"
            : std::filesystem::temp_directory_path() / "ReaperCore";

        std::error_code error;
        std::filesystem::create_directories(m_path, error);
        if (error)
            return false;

        m_path /= "ReaperCore.log";
        m_file.open(m_path, std::ios::out | std::ios::app);
        return m_file.is_open();
    }

    void logger::shutdown()
    {
        std::scoped_lock lock(m_mutex);
        if (m_file.is_open())
            m_file.close();

        if (m_console_owned)
        {
            FreeConsole();
            m_console_owned = false;
        }
    }

    void logger::write(std::string_view level, std::string_view message)
    {
        const auto now = std::chrono::system_clock::now();
        const auto value = std::chrono::system_clock::to_time_t(now);
        std::tm local{};
        localtime_s(&local, &value);

        std::ostringstream line;
        line << '[' << std::put_time(&local, "%Y-%m-%d %H:%M:%S") << "] ["
             << level << "] " << message << '\n';

        std::scoped_lock lock(m_mutex);
        std::cout << line.str();
        if (m_file.is_open())
        {
            m_file << line.str();
            m_file.flush();
        }
    }
}
