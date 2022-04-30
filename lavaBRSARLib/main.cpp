#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

int main()
{
	lava::brawl::brsarFile testBrsar;
	testBrsar.init("smashbros_sound.brsar");
	testBrsar.exportSawnd(0x08, "mario.sawnd");
	return 0;
}