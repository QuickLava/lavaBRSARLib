#include "../lavaBRSARLib/lavaByteArray.h"
#include "../lavaBRSARLib/lavaBRSARLib.h"

int stringToNum(const std::string& stringIn, bool allowNeg, int defaultVal)
{
	int result = defaultVal;
	std::string manipStr = stringIn;
	int base = (manipStr.find("0x") == 0) ? 16 : 10;
	char* res = nullptr;
	result = std::strtoul(manipStr.c_str(), &res, base);
	if (res != (manipStr.c_str() + manipStr.size()))
	{
		result = defaultVal;
	}
	if (result < 0 && !allowNeg)
	{
		result = defaultVal;
	}
	return result;
}

const std::string targetBrsarName = "smashbros_sound";

// Default Argument Constants
const std::string brsarDefaultFilename = targetBrsarName + ".brsar";
const std::string brsarDumpDefaultPath = "./Dump/";
const std::string nullArgumentString = "-";
bool isNullArg(const char* argIn)
{
	return (argIn != nullptr) ? (strcmp(argIn, nullArgumentString.c_str()) == 0) : 0;
}

int main(int argc, char** argv)
{
	srand(time(0));
	std::cout << "lavaBRSARLib " << lava::brawl::version << " CLI\n";
	std::cout << "Written by QuickLava\n";
	std::cout << "Based directly on work by:\n";
	std::cout << " - Jaklub and Agoaj, as well as mstaklo, ssbbtailsfan, stickman and VILE (Sawndz, Super Sawndz)\n";
	std::cout << " - Soopercool101, as well as Kryal, BlackJax96, and libertyernie (BrawlLib, BrawlBox, BrawlCrate)\n\n";
	try
	{
		if (argc >= 2)
		{
			if (strcmp("dumpRSAR", argv[1]) == 0 && argc >= 3)
			{
				std::cout << "Operation: Dump BRSAR\n";
				bool result = 1;
				lava::brawl::brsar sourceBrsar;
				std::string targetBRSARPath = argv[2];
				bool folderArgumentProvided = 0;
				bool joinHeaderAndData = 0;
				bool summaryOnly = 0;
				std::string targetFolder = brsarDumpDefaultPath;
				if (argc >= 4 && !isNullArg(argv[3]))
				{
					targetFolder = argv[3];
					folderArgumentProvided = 1;
				}
				if (argc >= 5 && !isNullArg(argv[4]))
				{
					if (strcmp("true", argv[4]) == 0)
					{
						summaryOnly = 1;
					}
					else
					{
						unsigned long value = stringToNum(argv[4], 0, ULONG_MAX);
						summaryOnly = value == 1;
					}
				}
				if (argc >= 6 && !isNullArg(argv[5]))
				{
					if (strcmp("true", argv[5]) == 0)
					{
						joinHeaderAndData = 1;
					}
					else
					{
						unsigned long value = stringToNum(argv[5], 0, ULONG_MAX);
						joinHeaderAndData = value == 1;
					}
				}
				if (std::filesystem::exists(targetBRSARPath))
				{
					if (folderArgumentProvided)
					{
						if (targetFolder.back() != '/' && targetFolder.back() != '\\')
						{
							targetFolder += "/";
						}
						std::filesystem::path temppath = targetFolder;
						if (!std::filesystem::exists(temppath))
						{
							if (folderArgumentProvided)
							{
								std::cerr << "[ERROR] Specified dump path doesn't exist (\"" << temppath << "\").\n";
								result = 0;
							}
						}
					}
					else
					{
						std::filesystem::path temppath = targetBRSARPath;
						targetFolder += lava::pruneFileExtension(temppath.filename().string()) + "/";
						if (!std::filesystem::exists(targetFolder))
						{
							std::filesystem::create_directories(targetFolder);
							if (!std::filesystem::exists(targetFolder))
							{
								std::cerr << "[ERROR] Unable to create default dump folder (\"" << targetFolder << "\").\n";
								result = 0;
							}
						}
					}
				}
				else
				{
					std::cerr << "[ERROR] Specified BRSAR (\"" << targetBRSARPath << "\") does not exist.\n";
					result = 0;
				}
				if (result)
				{
					lava::brawl::brsar testBrsar;
					if (testBrsar.init(targetBRSARPath))
					{
						std::cout << "Success!\n";
						std::cout << "Dumping to \"" << targetFolder << "\"...\n";
						testBrsar.doFileDump(targetFolder, joinHeaderAndData, summaryOnly);
					}
				}
				return 0;
			}
		}
		std::cout << "Invalid operation argument set supplied:\n";
		for (unsigned long i = 0; i < argc; i++)
		{
			std::cout << "\tArgv[" << i << "]: " << argv[i] << "\n";
		}
		std::cout << "Please provide one of the following sets of arguments!\n";
		std::cout << "To dump and summarize the files contained in a brsar file:\n";
		std::cout << "\tdumpRSAR {BRSAR_PATH} {OUTPATH_PATH, optional} {DO_FILE_SUMMARY_ONLY, optional} {JOIN_HEADERS_AND_DATA, optional}\n";
		std::cout << "Note: Default OUTPUT_PATH is \"" << brsarDumpDefaultPath << "{BRSAR_NAME}/\"\n";
		std::cout << "Note: {DO_FILE_SUMMARY_ONLY} {JOIN_HEADERS_AND_DATA} are both set to false by default.\n";
		std::cout << "Note: To explicitly use one of the above defaults, specify \"" << nullArgumentString << "\" for that argument.\n";
	}
	catch (std::exception e)
	{
		std::cerr << "EXCEPTION: ";
		std::cerr << e.what();
	}
	return 0;
}