#ifndef LAVA_UTILITY_V1_H
#define LAVA_UTILITY_V1_H

#include <string>
#include <vector>
#include <cctype>
#include <sstream>

namespace lava
{
	std::string stringToUpper(const std::string& stringIn);
	std::string numToHexStringWithPadding(unsigned long numIn, unsigned long paddingLength);
	std::string numToDecStringWithPadding(unsigned long numIn, unsigned long paddingLength);
	std::string doubleToStringWithPadding(double dblIn, unsigned long paddingLength, unsigned long precisionIn = 2);
	std::string floatToStringWithPadding(float fltIn, unsigned long paddingLength, unsigned long precisionIn = 2);
	std::string numberToOrdinal(unsigned int numberIn);
	std::string pruneFileExtension(std::string filepathIn);

	enum byteLevels
	{
		BYT = 0,
		KILO,
		MEGA,
		GIGA,
		TERA,
		PETA,
		EXA,
		ZETTA,
		YOTTA,
		_count
	};
	std::string bytesToFileSizeString(long long int bytesIn, char byteLevel = CHAR_MAX, bool abbrv = 0);
}

#endif