#include "cam_options.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_BEGIN

namespace
{
	CamOptions g_camOptions;

	constexpr const char* XML_ROOT_LITERAL = "CAM_OPTION";
	constexpr const char* XML_GENERAL_LITERAL = "General";
	constexpr const char* XML_RECENT_FILE_LIST = "RecentFileList";
	constexpr size_t MAX_RECENT_FILE_COUNT = 9;

	std::filesystem::path GetOptionsFilePath()
	{
		const char* appdata = std::getenv("APPDATA");
		if (appdata && *appdata) {
			return std::filesystem::path(appdata) / "QIANJIZN" / "QJCAM" / "options.xml";
		}

		const char* user_profile = std::getenv("USERPROFILE");
		if (user_profile && *user_profile) {
			return std::filesystem::path(user_profile) / "AppData" / "Roaming" / "QIANJIZN" / "QJCAM" / "options.xml";
		}

		return std::filesystem::path("options.xml");
	}

	std::string Trim(const std::string& value)
	{
		const std::string whitespace = " \t\r\n";
		const size_t begin = value.find_first_not_of(whitespace);
		if (begin == std::string::npos) {
			return {};
		}

		const size_t end = value.find_last_not_of(whitespace);
		return value.substr(begin, end - begin + 1);
	}

	std::string EscapeXml(const std::string& value)
	{
		std::string result;
		result.reserve(value.size());
		for (char ch : value) {
			switch (ch) {
			case '&':
				result += "&amp;";
				break;
			case '<':
				result += "&lt;";
				break;
			case '>':
				result += "&gt;";
				break;
			case '"':
				result += "&quot;";
				break;
			case '\'':
				result += "&apos;";
				break;
			default:
				result.push_back(ch);
				break;
			}
		}
		return result;
	}

	std::string UnescapeXml(const std::string& value)
	{
		std::string result = value;
		const std::pair<const char*, const char*> replacements[] = {
			{ "&lt;", "<" },
			{ "&gt;", ">" },
			{ "&quot;", "\"" },
			{ "&apos;", "'" },
			{ "&amp;", "&" },
		};

		for (const auto& replacement : replacements) {
			size_t pos = 0;
			while ((pos = result.find(replacement.first, pos)) != std::string::npos) {
				result.replace(pos, std::char_traits<char>::length(replacement.first), replacement.second);
				pos += std::char_traits<char>::length(replacement.second);
			}
		}

		return result;
	}

	bool ReadFileUtf8(const std::filesystem::path& path, std::string& content)
	{
		std::ifstream input(path, std::ios::binary);
		if (!input.is_open()) {
			return false;
		}

		content.assign(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
		return true;
	}

	bool WriteFileUtf8(const std::filesystem::path& path, const std::string& content)
	{
		std::ofstream output(path, std::ios::binary | std::ios::trunc);
		if (!output.is_open()) {
			return false;
		}

		output.write(content.data(), static_cast<std::streamsize>(content.size()));
		return output.good();
	}

	bool ExtractNestedElementText(
		const std::string& xml,
		const char* root_name,
		const char* parent_name,
		const char* child_name,
		std::string& value)
	{
		const std::string root_begin = std::string("<") + root_name + ">";
		const std::string root_end = std::string("</") + root_name + ">";
		const size_t root_pos = xml.find(root_begin);
		if (root_pos == std::string::npos) {
			return false;
		}

		const size_t root_close = xml.find(root_end, root_pos + root_begin.size());
		if (root_close == std::string::npos) {
			return false;
		}

		const std::string parent_begin = std::string("<") + parent_name + ">";
		const std::string parent_end = std::string("</") + parent_name + ">";
		const size_t parent_pos = xml.find(parent_begin, root_pos + root_begin.size());
		if (parent_pos == std::string::npos) {
			return false;
		}
		if (parent_pos > root_close) {
			return false;
		}

		const size_t parent_close = xml.find(parent_end, parent_pos + parent_begin.size());
		if (parent_close == std::string::npos || parent_close > root_close) {
			return false;
		}

		const std::string child_begin = std::string("<") + child_name + ">";
		const std::string child_end = std::string("</") + child_name + ">";
		const size_t child_pos = xml.find(child_begin, parent_pos + parent_begin.size());
		if (child_pos == std::string::npos || child_pos > parent_close) {
			return false;
		}

		const size_t text_begin = child_pos + child_begin.size();
		const size_t child_close = xml.find(child_end, text_begin);
		if (child_close == std::string::npos || child_close > parent_close) {
			return false;
		}

		value = UnescapeXml(xml.substr(text_begin, child_close - text_begin));
		return true;
	}

	std::vector<std::string> SplitRecentFileList(const std::string& value)
	{
		std::vector<std::string> files;
		size_t start = 0;
		while (start <= value.size()) {
			const size_t end = value.find(';', start);
			const std::string item = Trim(value.substr(start, end == std::string::npos ? std::string::npos : end - start));
			if (!item.empty()) {
				files.push_back(item);
			}
			if (end == std::string::npos) {
				break;
			}
			start = end + 1;
		}
		return files;
	}

	std::string JoinRecentFileList(const std::vector<std::string>& files)
	{
		std::string result;
		for (size_t i = 0; i < files.size(); ++i) {
			if (i > 0) {
				result += ';';
			}
			result += files[i];
		}
		return result;
	}
}

CamOptions* CAMOptsPtr = &g_camOptions;

void CamOptions::EnsureLoaded() const
{
	if (_loaded) {
		return;
	}

	const_cast<CamOptions*>(this)->Read();
}

bool CamOptions::Read()
{
	_recentFileList.clear();
	_loaded = true;

	const std::filesystem::path options_path = GetOptionsFilePath();
	if (!std::filesystem::exists(options_path)) {
		return true;
	}

	std::string xml;
	if (!ReadFileUtf8(options_path, xml)) {
		return false;
	}

	ExtractNestedElementText(xml, XML_ROOT_LITERAL, XML_GENERAL_LITERAL, XML_RECENT_FILE_LIST, _recentFileList);
	return true;
}

bool CamOptions::Write() const
{
	const std::filesystem::path options_path = GetOptionsFilePath();
	std::error_code ec;
	std::filesystem::create_directories(options_path.parent_path(), ec);
	if (ec) {
		return false;
	}

	const std::string xml =
		std::string("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n") +
		"<" + XML_ROOT_LITERAL + ">\n" +
		"  <" + std::string(XML_GENERAL_LITERAL) + ">\n" +
		"    <" + std::string(XML_RECENT_FILE_LIST) + ">" + EscapeXml(_recentFileList) + "</" + XML_RECENT_FILE_LIST + ">\n" +
		"  </" + XML_GENERAL_LITERAL + ">\n" +
		"</" + XML_ROOT_LITERAL + ">\n";

	return WriteFileUtf8(options_path, xml);
}

std::string CamOptions::GetRecentFileList() const
{
	const_cast<CamOptions*>(this)->Read();
	return _recentFileList;
}

void CamOptions::SetRecentFileList(const std::string& recent_file_list)
{
	_recentFileList = Trim(recent_file_list);
	Write();
}

QJ_NAMESPACE_CAM_PROJECT_MANAGEMENT_END
