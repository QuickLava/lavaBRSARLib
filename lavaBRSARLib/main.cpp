#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

int main()
{
	lava::brawl::brsar testBrsar;
	testBrsar.init(lava::brawl::targetBrsarName + ".brsar");
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
	}*/
	//testBrsar.doFileDump(lava::brawl::tempFileDumpBaseFolder, 0);
	std::ofstream stringDumpTest(lava::brawl::targetBrsarName + "_symbStrings.txt", std::ios_base::out | std::ios_base::binary);
	if (stringDumpTest.is_open())
	{
		testBrsar.summarizeSymbStringData(stringDumpTest);
		stringDumpTest.close();
	}
	return 0;
}