#ifndef LAVA_BYTE_ARRAY_H_V3
#define LAVA_BYTE_ARRAY_H_V3

#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "lavaBytes.h"

namespace lava
{
	struct byteArray
	{
	private:
		bool _populated = 0;
		std::vector<char> body = {};
		endType defaultEndian = endType::et_BIG_ENDIAN;
	private:

		template<typename objectType>
		objectType getFundamental(std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL) const
		{
			if (endianIn == endType::et_NULL)
			{
				endianIn = defaultEndian;
			}

			objectType result = ULLONG_MAX;
			if ((startIndex + sizeof(objectType)) <= body.size())
			{
				result = bytesToFundamental<objectType>(((unsigned char*)body.data()) + startIndex, endianIn);
				if (nextIndexOut != nullptr)
				{
					*nextIndexOut = startIndex + sizeof(objectType);
				}
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
		template<>
		unsigned char getFundamental<unsigned char>(std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn) const
		{
			if (endianIn == endType::et_NULL)
			{
				endianIn = defaultEndian;
			}

			unsigned char result = UCHAR_MAX;
			if (startIndex < body.size())
			{
				result = body[startIndex];
				if (nextIndexOut != nullptr)
				{
					*nextIndexOut = startIndex + sizeof(unsigned char);
				}
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

		template<typename objectType>
		bool setFundamental(const objectType& objectIn, std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL)
		{
			if (endianIn == endType::et_NULL)
			{
				endianIn = defaultEndian;
			}

			bool result = 0;
			if (startIndex + sizeof(objectType) <= body.size())
			{
				result = 1;
				unsigned char* startingPtr = ((unsigned char*)body.data()) + startIndex;
				writeFundamentalToBuffer<objectType>(objectIn, startingPtr, endianIn);
				if (nextIndexOut != nullptr)
				{
					*nextIndexOut = startIndex + sizeof(objectType);
				}
			}
			else
			{
				if (nextIndexOut != nullptr)
				{
					*nextIndexOut = body.size();
				}
			}
			return result;
		}
		template<>
		bool setFundamental<unsigned char>(const unsigned char& objectIn, std::size_t startIndex, std::size_t* nextIndexOut, endType endianIn)
		{
			if (endianIn == endType::et_NULL)
			{
				endianIn = defaultEndian;
			}

			bool result = 0;
			if (startIndex < body.size())
			{
				result = 1;
				body[startIndex] = objectIn;
				if (nextIndexOut != nullptr)
				{
					*nextIndexOut = startIndex + sizeof(unsigned char);
				}
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

		template<typename objectType>
		bool insertFundamental(const objectType& objectIn, std::size_t startIndex, endType endianIn = endType::et_NULL)
		{
			if (endianIn == endType::et_NULL)
			{
				endianIn = defaultEndian;
			}

			bool result = 0;
			if (startIndex < body.size())
			{
				body.insert(body.begin() + startIndex, sizeof(objectIn), 0x00);
				result = setFundamental<objectType>(objectIn, startIndex, nullptr, endianIn);
			}
			return result;
		}

		template<typename objectType>
		std::size_t findFundamental(const objectType& objectIn, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const
		{
			if (endianIn == endType::et_NULL)
			{
				endianIn = defaultEndian;
			}

			std::size_t result = SIZE_MAX;
			if (startItr < body.size())
			{
				result = search(lava::fundamentalToBytes(objectIn, endianIn), startItr, endItr);
			}
			return result;
		}

		template<typename objectType>
		std::vector<std::size_t> findFundamentalMultiple(const objectType& objectIn, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const
		{
			if (endianIn == endType::et_NULL)
			{
				endianIn = defaultEndian;
			}

			std::vector<std::size_t> result{};
			if (startItr < body.size())
			{
				result = searchMultiple(lava::fundamentalToBytes(objectIn, endianIn), startItr, endItr);
			}
			return result;
		}

	public:
		byteArray(std::size_t lengthIn = 0x00, char defaultChar = 0x00);
		byteArray(const char* dataIn, std::size_t lengthIn);
		byteArray(const unsigned char* dataIn, std::size_t lengthIn);
		byteArray(std::vector<char>& sourceVec);
		byteArray(std::vector<unsigned char>& sourceVec);
		byteArray(std::istream& sourceStream);

		void populate(std::size_t lengthIn, char defaultChar = 0x00);
		void populate(const char* sourceData, std::size_t lengthIn);
		void populate(const unsigned char* sourceData, std::size_t lengthIn);
		void populate(std::vector<char>& sourceVec);
		void populate(std::vector<unsigned char>& sourceVec);
		void populate(std::istream& sourceStream);
		bool populated() const;

		const char* data() const;
		std::size_t size() const;
		const std::vector<char>::const_iterator begin() const;
		const std::vector<char>::const_iterator end() const;
		const char& front() const;
		const char& back() const;

		void setDefaultEndian(endType endianIn);
		endType getDefaultEndian();

		std::vector<unsigned char> getBytes(std::size_t numToGet, std::size_t startIndex, std::size_t* nextIndexOut = nullptr) const;
		unsigned long long int getLLong(std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL) const;
		unsigned long int getLong(std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL) const;
		unsigned short int getShort(std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL) const;
		unsigned char getChar(std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL) const;
		double getDouble(std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL) const;
		float getFloat(std::size_t startIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL) const;

		bool setBytes(const std::vector<unsigned char>& bytesIn, std::size_t atIndex, std::size_t* nextIndexOut = nullptr);
		bool setLLong(unsigned long long int valueIn, std::size_t atIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL);
		bool setLong(unsigned long int valueIn, std::size_t atIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL);
		bool setShort(unsigned short int valueIn, std::size_t atIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL);
		bool setChar(unsigned char valueIn, std::size_t atIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL);
		bool setDouble(double valueIn, std::size_t atIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL);
		bool setFloat(float valueIn, std::size_t atIndex, std::size_t* nextIndexOut = nullptr, endType endianIn = endType::et_NULL);

		bool insertBytes(const std::vector<unsigned char>& bytesIn, std::size_t atIndex);
		bool insertLLong(unsigned long long int valueIn, std::size_t atIndex, endType endianIn = endType::et_NULL);
		bool insertLong(unsigned long int valueIn, std::size_t atIndex, endType endianIn = endType::et_NULL);
		bool insertShort(unsigned short int valueIn, std::size_t atIndex, endType endianIn = endType::et_NULL);
		bool insertChar(unsigned char valueIn, std::size_t atIndex, endType endianIn = endType::et_NULL);
		bool insertDouble(double valueIn, std::size_t atIndex, endType endianIn = endType::et_NULL);
		bool insertFloat(float valueIn, std::size_t atIndex, endType endianIn = endType::et_NULL);

		std::size_t search(const std::vector<unsigned char>& searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX) const;
		std::size_t searchLLong(unsigned long long int searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::size_t searchLong(unsigned long int searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::size_t searchShort(unsigned short int searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::size_t searchChar(unsigned char searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::size_t searchDouble(double searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::size_t searchFloat(float searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;

		std::vector<std::size_t> searchMultiple(const std::vector<unsigned char>& searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX) const;
		std::vector<std::size_t> searchMultipleLLong(unsigned long long int searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::vector<std::size_t> searchMultipleLong(unsigned long int searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::vector<std::size_t> searchMultipleShort(unsigned short int searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::vector<std::size_t> searchMultipleChar(unsigned char searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::vector<std::size_t> searchMultipleDouble(double searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;
		std::vector<std::size_t> searchMultipleFloat(float searchCriteria, std::size_t startItr = 0, std::size_t endItr = SIZE_MAX, endType endianIn = endType::et_NULL) const;

		bool dumpToStream(std::ostream& outputStream, std::size_t startIndex = 0x00, std::size_t endIndex = SIZE_MAX) const;
		bool dumpToFile(std::string targetPath, std::size_t startIndex = 0x00, std::size_t endIndex = SIZE_MAX) const;
	};

	// Old Testing Stuff
	/*const std::string testFileName = "testFile";
	const std::string testFileSuffix = ".dat";
	const std::string testFilePath = testFileName + testFileSuffix;
	const std::string testFileOutputPath = testFileName + "_edit" + testFileSuffix;
	int makeTestFile()
	{
		std::ofstream testFileOut(testFilePath, std::ios_base::out | std::ios_base::binary);
		for (std::size_t i = 0; i < USHRT_MAX; i++)
		{
			lava::writeRawDataToStream(testFileOut, i | (i << 0x10));
		}
		return 0;
	}
	int unitTest()
	{
		makeTestFile();
		lava::byteArray testFile;
		std::ifstream testFileIn(testFilePath, std::ios_base::in | std::ios_base::binary);
		testFile.populate(testFileIn);
		testFileIn.close();

		std::size_t cursor = 0x00;
		while (cursor < testFile.body.size())
		{
			double temp = testFile.getDouble(cursor, nullptr);
			testFile.setDouble(temp, cursor, &cursor);
		}
		testFile.dumpToFile(testFileOutputPath);
		return 0;
	}*/
}

#endif