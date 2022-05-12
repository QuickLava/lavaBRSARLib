#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

int main()
{
	lava::brawl::brsar testBrsar;
	testBrsar.init(lava::brawl::targetBrsarName + ".brsar");
	testBrsar.exportContents(lava::brawl::targetBrsarName + "_rebuilt.brsar");
	/*std::ofstream infoDumpTest(lava::brawl::targetBrsarName + "_infodump.dat", std::ios_base::out | std::ios_base::binary);
	if (infoDumpTest.is_open())
	{
		testBrsar.infoSection.exportContents(infoDumpTest);
		infoDumpTest.close();
	}
	std::ofstream symbDumpTest(lava::brawl::targetBrsarName + "_symbdump.dat", std::ios_base::out | std::ios_base::binary);
	if (symbDumpTest.is_open())
	{
		testBrsar.symbSection.exportContents(symbDumpTest);
		symbDumpTest.close();
	}
	std::ofstream fileDumpTest(lava::brawl::targetBrsarName + "_filedump.dat", std::ios_base::out | std::ios_base::binary);
	if (fileDumpTest.is_open())
	{
		testBrsar.fileSection.exportContents(fileDumpTest);
		fileDumpTest.close();
	}*/
	//testBrsar.doFileDump(lava::brawl::tempFileDumpBaseFolder, 0);
	/*std::ofstream stringDumpTest(lava::brawl::targetBrsarName + "_symbStrings.txt", std::ios_base::out | std::ios_base::binary);
	if (stringDumpTest.is_open())
	{
		testBrsar.summarizeSymbStringData(stringDumpTest);
		stringDumpTest.close();
	}*/
	/*std::ofstream infoFileDataTest(lava::brawl::targetBrsarName + "_infoFileData.txt", std::ios_base::out | std::ios_base::binary);
	if (infoFileDataTest.is_open())
	{
		testBrsar.infoSection.summarizeFileEntryData(infoFileDataTest);
		infoFileDataTest.close();
	}*/
	return 0;
}