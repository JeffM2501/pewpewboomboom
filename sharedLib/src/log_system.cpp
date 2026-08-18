#include "log_system.h"

std::string_view GetLogLevelString(LogLevel level)
{
	switch (level)
	{
	case LogLevel::Info:
		return "INFO";
	case LogLevel::Warning:
		return "WARNING";
	case LogLevel::Error:
		return "ERROR";
	default:
		return "UNKNOWN";
	}
}

std::string GetCurrentTimestamp()
{
	auto now = std::chrono::system_clock::now();
	auto timeT = std::chrono::system_clock::to_time_t(now);

	char buffer[100];
	std::strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", std::localtime(&timeT));
	return std::string(buffer);
}

void ConsoleLogOutput(std::string_view message, LogLevel level)
{
	printf("[%s] %s: %s\n", GetLogLevelString(level).data(), GetCurrentTimestamp().c_str(), message.data());
}