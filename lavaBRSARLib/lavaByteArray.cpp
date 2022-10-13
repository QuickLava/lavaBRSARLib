#include "lavaByteArray.h"

namespace lava
{
	byteArray::byteArray(std::size_t lengthIn, char defaultChar)
	{
		populate(lengthIn, defaultChar);
	};
	byteArray::byteArray(const char* dataIn, std::size_t lengthIn)
	{
		populate(dataIn, lengthIn);
	}
	byteArray::byteArray(const unsigned char* dataIn, std::size_t lengthIn)
	{
		populate(dataIn, lengthIn);
	}
	byteArray::byteArray(const std::vector<char>& sourceVec)
	{
		populate(sourceVec);
	}
	byteArray::byteArray(const std::vector<unsigned char>& sourceVec)
	{
		populate(sourceVec);
	}
	byteArray::byteArray(const byteArray& sourceArray, std::size_t startIndex, std::size_t endIndex)
	{
		populate(sourceArray, startIndex, endIndex);
	}
	byteArray::byteArray(std::istream& sourceStream)
	{
		populate(sourceStream);
	}
	byteArray::byteArray(std::string sourceFilePath)
	{
		populate(sourceFilePath);
	}

	void byteArray::populate(std::size_t lengthIn, char defaultChar)
	{
		if (_populated)
		{
			body.clear();
		}
		_populated = 1;
		body.resize(lengthIn, defaultChar);
	}
	void byteArray::populate(const char* sourceData, std::size_t lengthIn)
	{
		if (sourceData != nullptr)
		{
			_populated = 1;
			body = std::vector<char>(sourceData, sourceData + lengthIn);
		}
	}
	void byteArray::populate(const unsigned char* sourceData, std::size_t lengthIn)
	{
		populate((const char*)sourceData, lengthIn);
	}
	void byteArray::populate(const std::vector<char>& sourceVec)
	{
		populate(sourceVec.data(), sourceVec.size());
	}
	void byteArray::populate(const std::vector<unsigned char>& sourceVec)
	{
		populate((const char*)sourceVec.data(), sourceVec.size());
	}
	void byteArray::populate(const byteArray& sourceArray, std::size_t startIndex, std::size_t endIndex)
	{
		if (sourceArray.populated() && sourceArray.size() > startIndex)
		{
			if (endIndex >= startIndex)
			{
				if (endIndex > body.size())
				{
					endIndex = body.size();
				}
				populate(sourceArray.data(), endIndex - startIndex);
			}
		}
	}
	void byteArray::populate(std::istream& sourceStream)
	{
		if (_populated)
		{
			body.clear();
		}
		_populated = 1;
		sourceStream.seekg(0, sourceStream.end);
		std::size_t sourceSize(sourceStream.tellg());
		sourceStream.seekg(0, sourceStream.beg);
		body.resize(sourceSize);
		sourceStream.read(body.data(), sourceSize);
	}
	void byteArray::populate(std::string sourceFilePath)
	{
		if (std::filesystem::is_regular_file(sourceFilePath))
		{
			std::ifstream fileStreamIn(sourceFilePath, std::ios_base::in | std::ios_base::binary);
			populate(fileStreamIn);
		}
	}
	bool byteArray::populated() const
	{
		return _populated;
	}

	const char* byteArray::data() const
	{
		return body.data();
	}
	std::size_t byteArray::size() const
	{
		return body.size();
	}
	const std::vector<char>::const_iterator byteArray::begin() const
	{
		return body.begin();
	}
	const std::vector<char>::const_iterator byteArray::end() const
	{
		return body.end();
	}
	const char& byteArray::front() const
	{
		return body.front();
	}
	const char& byteArray::back() const
	{
		return body.back();
	}

	void byteArray::setDefaultEndian(endType endianIn)
	{
		if (endianIn != endType::et_NULL)
		{
			defaultEndian = endianIn;
		}
	}
	endType byteArray::getDefaultEndian()
	{
		return defaultEndian;
	}

	std::vector<unsigned char> byteArray::getBytes(std::size_t numToGet, std::size_t startIndex, std::size_t* nextIndexOut) const
	{
		if (startIndex < body.size())
		{
			if (numToGet >= body.size() || (startIndex + numToGet) >= body.size())
			{
				numToGet = body.size() - startIndex;
			}
			if (nextIndexOut != nullptr)
			{
				*nextIndexOut = startIndex + numToGet;
			}
			return std::vector<unsigned char>(body.begin() + startIndex, body.begin() + startIndex + numToGet);
		}
		else
		{
			if (nextIndexOut != nullptr)
			{
				*nextIndexOut = SIZE_MAX;
			}
			std::cerr << "[ERROR] Requested region startpoint was invalid. Specified index was [" << startIndex << "], max valid index is [" << body.size() - 1 << "].\n";
		}
		return std::vector<unsigned char>();
	}
	unsigned long long int byteArray::getLLong(std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn) const
	{
		return getFundamental<unsigned long long int>(startIndex, nextIndexOut, endianIn);
	}
	unsigned long int byteArray::getLong(std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn) const
	{
		return getFundamental<unsigned long int>(startIndex, nextIndexOut, endianIn);
	}
	unsigned short int byteArray::getShort(std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn) const
	{
		return getFundamental<unsigned short int>(startIndex, nextIndexOut, endianIn);
	}
	unsigned char byteArray::getChar(std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn) const
	{
		return getFundamental<unsigned char>(startIndex, nextIndexOut, endianIn);
	}
	double byteArray::getDouble(std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn) const
	{
		unsigned long long tempRes = getLLong(startIndex, nextIndexOut, endianIn);
		return *(double*)(&tempRes);
	}
	float byteArray::getFloat(std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn) const
	{
		unsigned long tempRes = getLong(startIndex, nextIndexOut, endianIn);
		return *(float*)(&tempRes);
	}

	bool byteArray::setBytes(const std::vector<unsigned char>& bytesIn, std::size_t atIndex, std::size_t* nextIndexOut)
	{
		bool result = 0;
		if ((atIndex + bytesIn.size()) >= atIndex && atIndex + bytesIn.size() <= body.size())
		{
			/*int tempInt = 0;
			char* tempPtr = body.data() + atIndex;
			std::cout << "Original Value: " << std::hex;
			for (int i = 0; i < bytesIn.size(); i++)
			{
				tempInt = *(tempPtr + i);
				tempInt &= 0x000000FF;
				std::cout << ((tempInt < 0x10) ? "0" : "") << tempInt;
			}
			std::cout << "\n" << std::dec;*/
			std::memcpy(body.data() + atIndex, bytesIn.data(), bytesIn.size());
			if (nextIndexOut != nullptr)
			{
				*nextIndexOut += bytesIn.size();
			}
			/*tempInt = 0;
			tempPtr = body.data() + atIndex;
			std::cout << "Modified Value: " << std::hex;
			for (int i = 0; i < bytesIn.size(); i++)
			{
				tempInt = *(tempPtr + i);
				tempInt &= 0x000000FF;
				std::cout << ((tempInt < 0x10) ? "0" : "") << tempInt;
			}
			std::cout << "\n" << std::dec;*/
			result = 1;
		}
		else
		{
			if (nextIndexOut != nullptr)
			{
				*nextIndexOut = SIZE_MAX;
			}
		}

		return result;
	}
	bool byteArray::setLLong(unsigned long long int valueIn, std::size_t atIndex, std::size_t* nextIndexOut, endType endianIn)
	{
		return setFundamental<unsigned long long int>(valueIn, atIndex, nextIndexOut, endianIn);
	}
	bool byteArray::setLong(unsigned long int valueIn, std::size_t atIndex, std::size_t* nextIndexOut, endType endianIn)
	{
		return setFundamental<unsigned long int>(valueIn, atIndex, nextIndexOut, endianIn);
	}
	bool byteArray::setShort(unsigned short int valueIn, std::size_t atIndex, std::size_t* nextIndexOut, endType endianIn)
	{
		return setFundamental<unsigned short int>(valueIn, atIndex, nextIndexOut, endianIn);
	}
	bool byteArray::setChar(unsigned char valueIn, std::size_t atIndex, std::size_t* nextIndexOut, endType endianIn)
	{
		return setFundamental<unsigned char>(valueIn, atIndex, nextIndexOut, endianIn);
	}
	bool byteArray::setDouble(double valueIn, std::size_t atIndex, std::size_t* nextIndexOut, endType endianIn)
	{
		unsigned long long tempVal = *(unsigned long long*)(&valueIn);
		return setFundamental<unsigned long long>(tempVal, atIndex, nextIndexOut, endianIn);
	}
	bool byteArray::setFloat(float valueIn, std::size_t atIndex, std::size_t* nextIndexOut, endType endianIn)
	{
		unsigned long tempVal = *((unsigned long*)(&valueIn));
		return setFundamental<unsigned long>(tempVal, atIndex, nextIndexOut, endianIn);
	}

	bool byteArray::insertBytes(const std::vector<unsigned char>& bytesIn, std::size_t atIndex)
	{
		bool result = 0;
		if (atIndex < body.size())
		{
			result = 1;
			body.insert(body.begin() + atIndex, bytesIn.begin(), bytesIn.end());
		}
		return result;
	}
	bool byteArray::insertLLong(unsigned long long int valueIn, std::size_t atIndex, endType endianIn)
	{
		return insertFundamental<unsigned long long int>(valueIn, atIndex, endianIn);
	}
	bool byteArray::insertLong(unsigned long int valueIn, std::size_t atIndex, endType endianIn)
	{
		return insertFundamental<unsigned long int>(valueIn, atIndex, endianIn);
	}
	bool byteArray::insertShort(unsigned short int valueIn, std::size_t atIndex, endType endianIn)
	{
		return insertFundamental<unsigned short int>(valueIn, atIndex, endianIn);
	}
	bool byteArray::insertChar(unsigned char valueIn, std::size_t atIndex, endType endianIn)
	{
		return insertFundamental<unsigned char>(valueIn, atIndex, endianIn);
	}
	bool byteArray::insertDouble(double valueIn, std::size_t atIndex, endType endianIn)
	{
		unsigned long long tempVal = *(unsigned long long*)(&valueIn);
		return insertFundamental<unsigned long long>(tempVal, atIndex, endianIn);
	}
	bool byteArray::insertFloat(float valueIn, std::size_t atIndex, endType endianIn)
	{
		unsigned long tempVal = *(unsigned long*)(&valueIn);
		return insertFundamental<unsigned long>(tempVal, atIndex, endianIn);
	}

	std::size_t byteArray::search(const std::vector<unsigned char>& searchCriteria, std::size_t startItr, std::size_t endItr) const
	{
		std::vector<char>::const_iterator itr = body.end();
		std::vector<char>* searchCriteriaSigned = ((std::vector<char>*) & searchCriteria);
		if (endItr < startItr)
		{
			endItr = SIZE_MAX;
		}
		if (endItr > body.size())
		{
			endItr = body.size();
		}
		if (body.size() && startItr < body.size() && searchCriteria.size())
		{
			itr = std::search(body.begin() + startItr, body.begin() + endItr, searchCriteriaSigned->begin(), searchCriteriaSigned->end());
		}
		return (itr != body.end()) ? itr - body.begin() : SIZE_MAX;
	}
	std::size_t byteArray::searchLLong(unsigned long long int searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamental<unsigned long long int>(searchCriteria, startItr, endItr, endianIn);
	}
	std::size_t byteArray::searchLong(unsigned long int searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamental<unsigned long int>(searchCriteria, startItr, endItr, endianIn);
	}
	std::size_t byteArray::searchShort(unsigned short int searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamental<unsigned short>(searchCriteria, startItr, endItr, endianIn);
	}
	std::size_t byteArray::searchChar(unsigned char searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamental<unsigned char>(searchCriteria, startItr, endItr, endianIn);
	}
	std::size_t byteArray::searchDouble(double searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamental<double>(searchCriteria, startItr, endItr, endianIn);
	}
	std::size_t byteArray::searchFloat(float searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamental<float>(searchCriteria, startItr, endItr, endianIn);
	}

	std::vector<std::size_t> byteArray::searchMultiple(const std::vector<unsigned char>& searchCriteria, std::size_t startItr, std::size_t endItr) const
	{
		std::size_t cursor = startItr;
		std::vector<std::size_t> result;
		std::size_t critSize = searchCriteria.size();
		bool done = 0;
		while (!done && cursor <= endItr)
		{
			cursor = search(searchCriteria, cursor);
			if (cursor != SIZE_MAX)
			{
				result.push_back(cursor);
				if ((cursor + critSize) > cursor)
				{
					cursor += critSize;
				}
			}
			else
			{
				done = 1;
			}
		}
		return result;
	}
	std::vector<std::size_t> byteArray::searchMultipleLLong(unsigned long long int searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamentalMultiple<unsigned long long int>(searchCriteria, startItr, endItr, endianIn);
	}
	std::vector<std::size_t> byteArray::searchMultipleLong(unsigned long int searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamentalMultiple<unsigned long int>(searchCriteria, startItr, endItr, endianIn);
	}
	std::vector<std::size_t> byteArray::searchMultipleShort(unsigned short int searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamentalMultiple<unsigned short>(searchCriteria, startItr, endItr, endianIn);
	}
	std::vector<std::size_t> byteArray::searchMultipleChar(unsigned char searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamentalMultiple<unsigned char>(searchCriteria, startItr, endItr, endianIn);
	}
	std::vector<std::size_t> byteArray::searchMultipleDouble(double searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamentalMultiple<double>(searchCriteria, startItr, endItr, endianIn);
	}
	std::vector<std::size_t> byteArray::searchMultipleFloat(float searchCriteria, std::size_t startItr, std::size_t endItr, endType endianIn) const
	{
		return findFundamentalMultiple<float>(searchCriteria, startItr, endItr, endianIn);
	}

	bool byteArray::dumpToStream(std::ostream& outputStream, std::size_t startIndex, std::size_t endIndex) const
	{
		bool result = 0;
		if (outputStream.good())
		{
			if (startIndex < body.size())
			{
				if (endIndex > startIndex)
				{
					if (endIndex > body.size())
					{
						endIndex = body.size();
					}
					outputStream.write(body.data() + startIndex, endIndex - startIndex);
					result = outputStream.good();
				}
			}
		}
		return result;
	}
	bool byteArray::dumpToFile(std::string targetPath, std::size_t startIndex, std::size_t endIndex) const
	{
		bool result = 0;
		std::ofstream output;
		output.open(targetPath, std::ios_base::binary | std::ios_base::out);
		if (output.is_open())
		{
			result = dumpToStream(output, startIndex, endIndex);
			std::cout << "Dumped body to \"" << targetPath << "\".\n";
		}
		return result;
	}
}