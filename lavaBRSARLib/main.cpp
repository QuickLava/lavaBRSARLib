#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

int main()
{
	lava::brawl::brsarFile testBrsar;
	testBrsar.init(lava::brawl::targetBrsarName + ".brsar");
	/*std::ofstream infoDumpTest("infodump.dat", std::ios_base::out | std::ios_base::binary);
	if (infoDumpTest.is_open())
	{
		testBrsar.infoSection.exportContents(infoDumpTest);
		infoDumpTest.close();
	}
	std::ofstream symbDumpTest("symbdump.dat", std::ios_base::out | std::ios_base::binary);
	if (symbDumpTest.is_open())
	{
		testBrsar.symbSection.exportContents(symbDumpTest);
		symbDumpTest.close();
	}*/
	testBrsar.doFileDump(lava::brawl::tempFileDumpBaseFolder, 0);
	return 0;
}