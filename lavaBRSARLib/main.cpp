#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

const std::string targetBrsarName = "smashbros_sound";
const std::string tempFileDumpBaseFolder = targetBrsarName + "/";
const unsigned long fileOverwriteTestTargetFile = 0x50;

// Test which overwrites File 0x06 with itself, shouldn't actually change anything.
#define ENABLE_FILE_OVERWRITE_TEST_1 true
// Test which overwrites File 0x06's header and data with zeroes-out 0x20 byte vectors.
#define ENABLE_FILE_OVERWRITE_TEST_2 false
// Test which exports the entire .brsar.
#define ENABLE_BRSAR_EXPORT_TEST true
// Test which exports the full SYMB section.
#define ENABLE_SYMB_SECTION_EXPORT_TEST false
// Test which exports the full INFO section.
#define ENABLE_INFO_SECTION_EXPORT_TEST	false
// Test which exports the full FILE section.
#define ENABLE_FILE_SECTION_EXPORT_TEST	false
// Test which exports a sawnd file of the specified name for the specified group.
#define ENABLE_FILE_DUMP_TEST false
// Test which dumps all the strings in the SYMB section.
#define ENABLE_STRING_DUMP_TEST false
// Test which summarizes data for every brsarInfoFileHeader in the .brsar.
#define ENABLE_FILE_INFO_SUMMARY_TEST false

int main()
{
	lava::brawl::brsar testBrsar;
	testBrsar.init(targetBrsarName + ".brsar");
#if ENABLE_FILE_OVERWRITE_TEST_1
	lava::brawl::brsarFileFileContents temp = *testBrsar.fileSection.getFileContentsPointer(fileOverwriteTestTargetFile);
	testBrsar.overwriteFile(temp.header, temp.data, fileOverwriteTestTargetFile);
#endif
#if ENABLE_FILE_OVERWRITE_TEST_2
	std::vector<unsigned char> fakeHeader(0x20, 0x00);
	std::vector<unsigned char> fakeData(0x20, 0x00);
	testBrsar.overwriteFile(fakeHeader, fakeData, fileOverwriteTestTargetFile);
#endif
#if ENABLE_BRSAR_EXPORT_TEST
	testBrsar.exportContents(targetBrsarName + "_rebuilt.brsar");
#endif
#if ENABLE_SYMB_SECTION_EXPORT_TEST
	std::ofstream symbDumpTest(targetBrsarName + "_symbdump.dat", std::ios_base::out | std::ios_base::binary);
	if (symbDumpTest.is_open())
	{
		testBrsar.symbSection.exportContents(symbDumpTest);
		symbDumpTest.close();
	}
#endif
#if ENABLE_INFO_SECTION_EXPORT_TEST
	std::ofstream infoDumpTest(targetBrsarName + "_infodump.dat", std::ios_base::out | std::ios_base::binary);
	if (infoDumpTest.is_open())
	{
		testBrsar.infoSection.exportContents(infoDumpTest);
		infoDumpTest.close();
	}
#endif
#if ENABLE_FILE_SECTION_EXPORT_TEST
	std::ofstream fileDumpTest(targetBrsarName + "_filedump.dat", std::ios_base::out | std::ios_base::binary);
	if (fileDumpTest.is_open())
	{
		testBrsar.fileSection.exportContents(fileDumpTest);
		fileDumpTest.close();
	}
#endif
#if ENABLE_FILE_DUMP_TEST
	testBrsar.doFileDump(tempFileDumpBaseFolder, 0);
#endif
#if ENABLE_STRING_DUMP_TEST
	std::ofstream stringDumpTest(targetBrsarName + "_symbStrings.txt", std::ios_base::out | std::ios_base::binary);
	if (stringDumpTest.is_open())
	{
		testBrsar.summarizeSymbStringData(stringDumpTest);
		stringDumpTest.close();
	}
#endif
#if ENABLE_FILE_INFO_SUMMARY_TEST
	std::ofstream infoFileDataTest(targetBrsarName + "_infoFileData.txt", std::ios_base::out | std::ios_base::binary);
	if (infoFileDataTest.is_open())
	{
		testBrsar.infoSection.summarizeFileEntryData(infoFileDataTest);
		infoFileDataTest.close();
	}
#endif
	return 0;
}