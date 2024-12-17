#include <iostream>
#include "Logs/Logs.h"
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include "libs/tinyxml2.h"

namespace filesystem = std::filesystem;


class XML {
public:
	struct Message {
		int mid;
		std::string text;
	};

	std::vector<Message> Data;

	bool loadXML(const std::string& path) {
		std::string xml = path + "\\uistring.xml";
		tinyxml2::XMLDocument document;
		tinyxml2::XMLError error = document.LoadFile(xml.c_str());

		if (error != tinyxml2::XML_SUCCESS) {
			std::cerr << "Error loading XML file: " << xml << std::endl;
			return false;
		}
		
		LOG_INFO("Successfully loaded XML file: " + xml);

		tinyxml2::XMLElement* root = document.RootElement();

		if (root) {
			LOG_INFO("Root element found: " + std::string(root->Name()));
		}
		else {
			std::cerr << "No root element found in XML file." << std::endl;
			return false;
		}

		tinyxml2::XMLElement* messageElement = root->FirstChildElement("message");
		int _tCount = 0;

		while (messageElement) {
			_tCount++;
			messageElement = messageElement->NextSiblingElement("message");
		}
		
		LOG_INFO("XML First Check Line Count: " + std::to_string(_tCount));

		messageElement = root->FirstChildElement("message");

		int _mCount = 0;

		while (messageElement) {
			const char* midAttr = messageElement->Attribute("mid");

			const char* messageText = nullptr;
			tinyxml2::XMLNode* cdataNode = messageElement->FirstChild();

			if (cdataNode && cdataNode->ToText()) {
				messageText = cdataNode->ToText()->Value();
			}

			if (midAttr && messageText) {
				Message message;
				message.mid = std::stoi(midAttr);
				message.text = messageText;
				Data.push_back(message);
				_mCount++;
			}

			messageElement = messageElement->NextSiblingElement("message");
		}

		LOG_INFO("XML Line Count: " + std::to_string(_mCount));

		LOG_INFO("Finished processing " + std::to_string(_mCount) + "out of " + std::to_string(_tCount));

		return true;
	}

	std::string translateIDtoString(int mid) {
		for (const auto& message : Data) {
/* Only for extreme checking in details

#ifdef _DEBUG
			LOG_INFO("Checking MID: " + std::to_string(message.mid));
#endif
*/
			if (message.mid == mid) {
#ifdef _DEBUG
				LOG_INFO("Translating MID: " + std::to_string(mid) + " | Text: " + message.text);
#endif
				return message.text;
			}
		}
		
		return "";
	}
};

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
	void readData(std::ifstream& input, const Header& header, const Column& column, std::ofstream& csv, XML& xml) {
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
#ifdef _DEBUG
					std::string _temp = "Title: " + std::string(column.columnTitles[j]) + " | Type: 1 | Value: " + buffer;
					LOG_INFO(_temp);
#endif

					break;
				}
				case 2: {
					int32_t value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));
					csv << value;
#ifdef _DEBUG
					std::string _temp = "Title: " + std::string(column.columnTitles[j]) + " | Type: 2 | Value: " + std::to_string(value);
					LOG_INFO(_temp);
#endif
					break;
				}
				case 3: {
					int32_t value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));

					if (column.columnTitles[j] == "_NameID") {
#ifdef _DEBUG
						LOG_INFO("Translating! Please wait.");
#endif
						std::string translatedString = xml.translateIDtoString(value);
						csv << translatedString;

#ifdef _DEBUG
						std::string _temp = "Title: " + std::string(column.columnTitles[j]) + " | Type: 3 | Value: " + std::to_string(value);
						LOG_INFO(_temp);
#endif
/* Not Practical to Translate Description
					}

					else if (column.columnTitles[j] == "_DescriptionID") {
#ifdef _DEBUG
						LOG_INFO("Translating! Please wait.");
#endif
						std::string translatedString = xml.translateIDtoString(value);
						csv << translatedString;

#ifdef _DEBUG
						std::string _temp = "Title: " + std::string(column.columnTitles[j]) + " | Type: 3 | Value: " + std::to_string(value);
						LOG_INFO(_temp);
#endif
*/
					}
					else {
						csv << value;

#ifdef _DEBUG
						std::string _temp = "Title: " + std::string(column.columnTitles[j]) + " | Type: 3 | Value: " + std::to_string(value);
						LOG_INFO(_temp);
#endif
					}

					break;
				}
				case 4: {
					float value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));
					csv << value;
#ifdef _DEBUG
					std::string _temp = "Title: " + std::string(column.columnTitles[j]) + " | Type: 4 | Value: " + std::to_string(value);
					LOG_INFO(_temp);
#endif
					break;
				}
				case 5: {
					float value;
					input.read(reinterpret_cast<char*>(&value), sizeof(value));
					csv << value;
#ifdef _DEBUG
					std::string _temp = "Title: " + std::string(column.columnTitles[j]) + " | Type: 5 | Value: " + std::to_string(value);
					LOG_INFO(_temp);
#endif
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

void processDntFile(const std::string& path, const std::string& outputDir, XML& xml) {
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

		body.readData(_file, header, column, csv, xml);

		csv.close();
		std::cout << "Processed: " << path << " -> " << output << std::endl;
	}
	catch (const std::exception& ex) {
		std::cerr << "Error processing file: " << path << " - " << ex.what() << std::endl;
	}

	_file.close();
}

void processFolder(const std::string& folder, const std::string& outputDir, XML& xml) {
	for (const auto& entry : filesystem::directory_iterator(folder)) {
		if (entry.is_regular_file() && entry.path().extension() == ".dnt") {
			processDntFile(entry.path().string(), outputDir, xml);
		}
	}
}

int main(int argc, const char* argv[]) {
	try {
		Log.Initialized(argv[0]);
		XML xml;

		wchar_t buffer[MAX_PATH];
		GetModuleFileName(NULL, buffer, MAX_PATH);
		std::string currentDir = filesystem::path(buffer).parent_path().string();

		std::string xmlFolder = currentDir + "\\uistring";
		std::string dntFolder = currentDir + "\\dnt";
		std::string outputFolder = currentDir + "\\output";

		if (!filesystem::exists(xmlFolder)) {
			std::cerr << "Error: 'dnt' folder not found in the current directory." << std::endl;
			return 1;
		}

		if (!filesystem::exists(dntFolder)) {
			std::cerr << "Error: 'dnt' folder not found in the current directory." << std::endl;
			return 1;
		}

		if (!filesystem::exists(outputFolder))
			filesystem::create_directory(outputFolder);

		if (!xml.loadXML(xmlFolder))
			return 1;

		processFolder(dntFolder, outputFolder, xml);

		std::cout << "Processing completed. Output saved in 'output' folder." << std::endl;
	}
	catch (const std::exception& ex) {
		std::cerr << "Error: " << ex.what() << std::endl;
		return 1;
	}

#ifdef _DEBUG
	system("pause");
#endif

	return 0;
}
