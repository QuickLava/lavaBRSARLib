#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

const std::string targetBrsarName = "smashbros_sound";
const std::string tempFileDumpBaseFolder = targetBrsarName + "/";
const unsigned long fileOverwriteTestTargetFile = 0x50;
const unsigned long sawndTestsTargetGroupID = 0x11;
const std::string sawndTestsFilename = "sawnd.sawnd";

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
#define ENABLE_SAWND_EXPORT_TEST true
// Test which imports a sawnd file, then exports the .brsar.
#define ENABLE_SAWND_IMPORT_TEST true
// Test which summarizes and dumps every file in the .brsar, grouped by group.
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
#if ENABLE_SAWND_EXPORT_TEST
	testBrsar.exportSawnd(sawndTestsTargetGroupID, sawndTestsFilename);
#endif
#if ENABLE_SAWND_IMPORT_TEST
	testBrsar.importSawnd(sawndTestsFilename);
	testBrsar.exportContents(targetBrsarName + "_sawndimport.brsar");
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

int resawndzmain(int argc, char** argv)
{
	setvbuf(stdout, NULL, _IONBF, 0);
	srand(time(0));
	std::cout << "Resawndz v1.00\n";
	std::cout << "Written by QuickLava\n";
	std::cout << "Based directly on work by:\n";
	std::cout << " - Jaklub and Agoaj, as well as mstaklo, ssbbtailsfan, stickman and VILE (Sawndz, Super Sawndz)\n";
	std::cout << " - Soopercool101, as well as Kryal, BlackJax96, and libertyernie (BrawlLib, BrawlBox, BrawlCrate)\n\n";
	/*std::cout << "Provided Args:\n";
	for (int i = 0; i < argc; i++)
	{
		std::cout << "\tArgV[" << i << "]:" << argv[i] << "\n";
	}*/
	try
	{
		if (argc == 1)
		{
			std::cerr << "Please run the Sawndz GUI.\n";
			return 0;
		}
		else
		{
			if (strcmp("sawndcreate", argv[1]) == 0)
			{
				lava::brawl::brsar sourceBrsar;
				std::string activeBrsarName = targetBrsarName + ".brsar";
				if (argc == 4)
				{ 
					activeBrsarName = argv[3];
				}
				sourceBrsar.init(activeBrsarName);
				sourceBrsar.exportSawnd(std::stoi(argv[2]), "sawnd.sawnd");
				return 0;
			}
			if (strcmp("sawnd", argv[1]) == 0)
			{
				lava::brawl::brsar sourceBrsar;
				std::string activeBrsarName = targetBrsarName + ".brsar";
				if (argc == 3)
				{
					activeBrsarName = argv[2];
				}
				sourceBrsar.init(activeBrsarName);
				sourceBrsar.importSawnd("sawnd.sawnd");
				sourceBrsar.exportContents(activeBrsarName);
				return 0;
			}
			std::cerr << "Incorrect command.\n";
			return 0;
		}
	}
	catch (std::exception e)
	{
		std::cerr << "EXCEPTION: ";
		std::cerr << e.what();
	}
	return 0;
}