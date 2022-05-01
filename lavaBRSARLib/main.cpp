#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

int main()
{
	lava::brawl::brsarFile testBrsar;
	//testBrsar.init("revo_kart.brsar");
	testBrsar.init("smashbros_sound.brsar");
	std::ofstream infoDumpTest("infodump.dat", std::ios_base::out | std::ios_base::binary);
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
	}
	//testBrsar.exportSawnd(0x08, "mario.sawnd");
	lava::brawl::brsarSymbPTrieNode res = testBrsar.symbSection.tries.begin()->findString("snd_se_mario_ouen");
	std::string temp = testBrsar.symbSection.getString(res.stringID);
	return 0;
}