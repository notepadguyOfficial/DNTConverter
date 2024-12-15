#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include "libs/tinyxml2.h"

namespace filesystem = std::filesystem;

class Header {
public:
	int16_t columnCount;
	int32_t rowCount;

	void readHeader(std::ifstream& input) {
		input.read(reinterpret_cast<char*>(&rowCount), sizeof(rowCount));
		input.read(reinterpret_cast<char*>(&columnCount), sizeof(columnCount));
		input.read(reinterpret_cast<char*>(&rowCount), sizeof(rowCount));
	}
};

class Column {
public:
	std::vector<int16_t> columnTitleLengths;
	std::vector<std::string> columnTitles;
	std::vector<int8_t> columnTypes;

	void readColumn(std::ifstream& input, const Header& header) {
		for (int i = 0; i < header.columnCount; ++i) {
			int16_t titleLength;
			char buffer[4096];
			int8_t type;

			input.read(reinterpret_cast<char*>(&titleLength), sizeof(titleLength));
			if (titleLength <= 0 || titleLength >= 4096) {
				throw std::runtime_error("Invalid title length encountered.");
			}

			input.read(buffer, titleLength);
			buffer[titleLength] = '\0';
			input.read(reinterpret_cast<char*>(&type), sizeof(type));

			columnTitleLengths.push_back(titleLength);
			columnTitles.emplace_back(buffer);
			columnTypes.push_back(type);
		}
	}
};

class Body {
public:
	void readData(std::ifstream& input, const Header& header, const Column& column, std::ofstream& csv) {
		for (int i = 0; i < header.rowCount; ++i) {
			int32_t _ID;
			input.read(reinterpret_cast<char*>(&_ID), sizeof(_ID));

			csv << _ID;

			for (int j = 0; j < header.columnCount; ++j) {
				csv << ",";

				switch (column.columnTypes[j]) {
				case 1: {
					int16_t length;
					char buffer[4096];

					input.read(reinterpret_cast<char*>(&length), sizeof(length));
					if (length <= 0 || length >= 4096) {
						input.seekg(length, std::ios::cur);
						break;
					}
					input.read(buffer, length);
					buffer[length] = '\0';

					for (int k = 0; k < length; ++k)
						if (buffer[k] == ',')
							buffer[k] = '^';

					csv << buffer;
					break;
				}
				case 2: {
					int32_t value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));
					csv << value;
					break;
				}
				case 3: {
					int32_t value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));
					csv << value;
					break;
				}
				case 4: {
					float value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));
					csv << value;
					break;
				}
				case 5: {
					float value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));
					csv << value;
					break;
				}
				default:
					csv << "[Unknown Type]";
				}
			}
			csv << "\n";
		}
	}
};

void processDntFile(const std::string& path, const std::string& outputDir) {
	std::ifstream _file(path, std::ios::binary);

	if (!_file.is_open()) {
		std::cerr << "Failed to open input file: " << path << std::endl;
		return;
	}

	Header header;
	Column column;
	Body body;

	try {
		header.readHeader(_file);
		std::cout << "Rows: " << header.rowCount << ", Columns: " << header.columnCount << "\n";

		column.readColumn(_file, header);
		for (size_t i = 0; i < column.columnTitles.size(); ++i) {
			std::cout << "Column " << i << ": " << column.columnTitles[i]
				<< " (Type: " << static_cast<int>(column.columnTypes[i]) << ")\n";
		}

		std::string output = outputDir + "/" + filesystem::path(path).stem().string() + ".csv";
		std::ofstream csv(output);

		csv << "_ID";
		for (size_t i = 0; i < column.columnTitles.size(); ++i) {
			csv << "," << column.columnTitles[i];
		}
		csv << "\n";

		body.readData(_file, header, column, csv);

		csv.close();
		std::cout << "Processed: " << path << " -> " << output << std::endl;
	}
	catch (const std::exception& ex) {
		std::cerr << "Error processing file: " << path << " - " << ex.what() << std::endl;
	}

	_file.close();
}

void processFolder(const std::string& folder, const std::string& outputDir) {
	for (const auto& entry : filesystem::directory_iterator(folder)) {
		if (entry.is_regular_file() && entry.path().extension() == ".dnt") {
			processDntFile(entry.path().string(), outputDir);
		}
	}
}

int main(int argc, const char* argv[]) {
	try {
		wchar_t buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);
		std::string currentDir = filesystem::path(buffer).parent_path().string();
		std::string dntFolder = currentDir + "/dnt";
		std::string outputFolder = currentDir + "/output";

		if (!filesystem::exists(xmlFolder)) {
			std::cerr << "Error: 'dnt' folder not found in the current directory." << std::endl;
			return 1;
		}

		if (!filesystem::exists(dntFolder)) {
			std::cerr << "Error: 'dnt' folder not found in the current directory." << std::endl;
			return 1;
		}

		if (!filesystem::exists(outputFolder)) {
			filesystem::create_directory(outputFolder);
		}

		processFolder(dntFolder, outputFolder);

		std::cout << "Processing completed. Output saved in 'output' folder." << std::endl;
	}
	catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}
