#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

int main()
{
	lava::brawl::brsarFile testBrsar;
	testBrsar.init("smashbros_sound.brsar");
	//testBrsar.exportSawnd(0x08, "mario.sawnd");
	lava::brawl::brsarSymbPTrieNode res = testBrsar.symbSection.tries.begin()->findString("snd_se_mario_ouen");
	std::string temp = testBrsar.symbSection.getString(res.stringID);
	return 0;
}