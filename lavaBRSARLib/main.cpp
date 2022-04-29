#include "lavaByteArray.h"
#include <iostream>
const std::string testFileName = "testFIle";
const std::string testFileSuffix = ".dat";
const std::string testFilePath = testFileName + testFileSuffix;
const std::string testFileOutputPath = testFileName + "_edit" + testFileSuffix;

int makeTestFile()
{
	std::ofstream testFileOut(testFilePath, std::ios_base::out | std::ios_base::binary);
	for (std::size_t i = 0; i < 0x1000000; i++)
	{
		char temp = (1 << (i % 8));
		testFileOut.write(&temp, 1);
	}
	return 0;
}

int main()
{
	makeTestFile();
	lava::byteArray testFile;
	std::ifstream testFileIn(testFilePath, std::ios_base::in | std::ios_base::binary);
	testFile.populate(testFileIn);
	testFileIn.close();

	std::size_t cursor = 0x00;
	while (cursor < testFile.body.size())
	{
		float temp = testFile.getFloat(cursor, nullptr);
		testFile.setFloat(temp, cursor, &cursor);
	}
	testFile.dumpToFile(testFileOutputPath);
	return 0;
}