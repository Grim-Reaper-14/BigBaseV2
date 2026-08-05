#pragma once

#include <filesystem>
#include <fstream>
#include <mutex>
#include <string_view>

namespace reaper::core
{
    class logger final
    {
    public:
        static logger& instance();

        bool initialize();
        void shutdown();
        void write(std::string_view level, std::string_view message);

        logger(const logger&) = delete;
        logger& operator=(const logger&) = delete;

    private:
        logger() = default;
        ~logger() = default;

        std::mutex m_mutex;
        std::ofstream m_file;
        std::filesystem::path m_path;
        bool m_console_owned{};
    };
}
