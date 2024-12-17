#include "Logs.h"
#include <mutex>

Logs& Logs::GetInstance()
{
	static Logs instance;
	return instance;
}

void Logs::SetLogFile(const std::string& filename)
{
	std::lock_guard<std::mutex> guard(mutex_);
	filestream_.open(filename, std::ios_base::app | std::ios_base::out);

	if (!filestream_)
	{
		std::cerr << "Error opening the file!" << std::endl;
	}
}

void Logs::SetLogLevel(LOG_LEVEL level)
{
	LogLevel_ = level;
}

std::string Logs::GetCurrentTime()
{
	std::time_t now = std::time(nullptr);
	char buffer[100];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
	return std::string(buffer);
}

std::string Logs::FormatFileName(const std::string _name)
{
	std::time_t now = std::time(nullptr);
	char buffer[100];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d_%H-%M-%S", std::localtime(&now));
	std::filesystem::path programPath(_name);
	return programPath.filename().generic_string() + "_"+ std::string(buffer) + ".log";
}

std::string Logs::LogLevelToString(LOG_LEVEL level)
{
	switch (level)
	{
	case LOG_LEVEL::DEBUG: return "DEBUG";
	case LOG_LEVEL::INFO: return "INFO";
	case LOG_LEVEL::WARNING: return "WARNING";
	case LOG_LEVEL::ERROR: return "ERROR";
	default: return "UKNOWN";
	}
}

std::string Logs::FormatMessage(const std::string& message, LOG_LEVEL level)
{
	std::ostringstream ss;
	ss << "[" << GetCurrentTime() << "] [" << LogLevelToString(level) << "] : " << message;
	return ss.str();
}

void Logs::Print(const std::string& message, LOG_LEVEL level)
{
	if (level >= LogLevel_)
	{
		std::lock_guard<std::mutex> guard(mutex_);
		std::string output = FormatMessage(message, level);

		std::cout << output << std::endl;

		if (filestream_.is_open())
		{
			filestream_ << output << std::endl;
		}
	}
}

void Logs::Input(std::string& input, const std::string& prompt)
{
	std::cout << prompt;
	std::cin >> input;

	std::string temp = prompt + " " + input;

	LOG_INFO(temp);
}

void Logs::Initialized(const std::string& _n)
{
	GetInstance();
	SetLogFile(FormatFileName(_n));
	SetLogLevel(LOG_LEVEL::INFO);
}

Logs& Log = Logs::GetInstance();