#pragma once
#include <functional>
#include <string_view>
#include <chrono>
#include <iostream>
#include <cstdio>
#include <cstdarg>

enum class LogLevel
{
	Verbose = 0,
	Info,
	Warning,
	Error
};

using LogFunction = std::function<void(std::string_view message, LogLevel level)>;

class Logger
{
private:
	LogFunction LogOutput = nullptr;

	LogLevel CurrentLogLevel = LogLevel::Info;

	template<typename... Args>
	std::string FormatString(const char* format, Args&&... args)
	{
		static char buffer[4096];
		snprintf(buffer, sizeof(buffer), format, std::forward<Args>(args)...);
		return std::string(buffer);
	}

public:
	Logger(LogFunction logFunction)
		: LogOutput(logFunction)
	{}

	template<typename... Args>
	inline void Log(LogLevel level, const char* format, Args&&... args)
	{
		if (LogOutput && level >= CurrentLogLevel)
		{
			std::string message = FormatString(format, std::forward<Args>(args)...);
			LogOutput(message, level);
		}
	}
};

void ConsoleLogOutput(std::string_view message, LogLevel level);