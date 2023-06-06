#include "SimpleConfig.h"

namespace azely
{

	SimpleConfig::SimpleConfig(const string &filePath)
	{
		if (filePath.empty()) return;

		ifstream file(filePath);

		if (!file.is_open()) return;
		
		string line;
		string divider = "=";
		while (getline(file, line))
		{
			if (line.find(divider) == string::npos) continue;
			string key = line.substr(0, line.find(divider));
			string value = line.substr(line.find(divider) + divider.length(), line.length());
			settingMap[key] = value;
		}

		file.close();
	}

	SimpleConfig::~SimpleConfig() = default;

	BOOL SimpleConfig::IsConfigLoaded() const
	{
		return settingMap.size() > 0;
	}

	BOOL SimpleConfig::GetBoolean(const string &key, BOOL *outValue) const
	{
		if (key.empty() || outValue == nullptr) return false;

		const auto foundIterator = settingMap.find(key);
		if (foundIterator == settingMap.end()) return false;

		if (foundIterator->second[0] == 'T' || foundIterator->second[0] == 't')
		{
			*outValue = true;
		} else
		{
			*outValue = false;
		}
		return true;
	}

	BOOL SimpleConfig::GetString(const string &key, string *outString) const
	{
		if (key.empty() || outString == nullptr) return false;

		const auto foundIterator = settingMap.find(key);
		if (foundIterator == settingMap.end()) return false;

		if (foundIterator->second.find("\"") != string::npos)
		{
			*outString = foundIterator->second.substr(1, foundIterator->second.length() - 2);
		} else
		{
			*outString = foundIterator->second;
		}
		return true;
	}
	
	BOOL SimpleConfig::GetInt(const string &key, INT32 *outInt) const
	{
		if (key.empty() || outInt == nullptr) return false;

		const auto foundIterator = settingMap.find(key);
		if (foundIterator == settingMap.end()) return false;

		*outInt = stoi(foundIterator->second);
		return true;
	}

	BOOL SimpleConfig::GetDouble(const string &key, DOUBLE *outDouble) const
	{
		if (key.empty() || outDouble == nullptr) return false;

		const auto foundIterator = settingMap.find(key);
		if (foundIterator == settingMap.end()) return false;

		*outDouble = stod(foundIterator->second);
		return true;
	}

	BOOL SimpleConfig::GetChar(const string &key, CHAR *outChar) const
	{
		if (key.empty() || outChar == nullptr) return false;

		const auto foundIterator = settingMap.find(key);
		if (foundIterator == settingMap.end()) return false;

		*outChar = foundIterator->second[0];
		return true;
	}

}