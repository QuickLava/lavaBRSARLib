#include "../lavaBRSARLib/lavaByteArray.h"
#include "../lavaBRSARLib/lavaBRSARLib.h"

// CLI Constants
const std::string cliVersion = "v0.2.0";

// Default Argument Constants
const std::string brsarDumpDefaultPath = "./Dump/";
const std::string nullArgumentString = "-";


int _globalArgC = INT_MAX;
char** _globalArgV = nullptr;
bool isNullArg(const char* argIn)
{
	return (argIn != nullptr) ? (strcmp(argIn, nullArgumentString.c_str()) == 0) : 0;
}
bool argProvided(unsigned int argIndex)
{
	return _globalArgC > (argIndex) && !isNullArg(_globalArgV[argIndex]);
}
int stringToNum(const std::string& stringIn, bool allowNeg, int defaultVal)
{
	int result = defaultVal;

	std::size_t firstNonWhitespaceCharFound = SIZE_MAX;
	for (std::size_t i = 0; i < stringIn.size() && firstNonWhitespaceCharFound == SIZE_MAX; i++)
	{
		if (!isblank(stringIn[i]))
		{
			firstNonWhitespaceCharFound = i;
		}
	}
	if (firstNonWhitespaceCharFound != SIZE_MAX)
	{
		int base = (stringIn.find("0x") == firstNonWhitespaceCharFound) ? 16 : 10;
		char* res = nullptr;
		result = std::strtoul(stringIn.c_str(), &res, base);
		if (res != (stringIn.c_str() + stringIn.size()))
		{
			result = defaultVal;
		}
		if (result < 0 && !allowNeg)
		{
			result = defaultVal;
		}
	}

	return result;
}
bool processBoolArgument(const char* argIn)
{
	bool result = 0;

	unsigned long value = stringToNum(argIn, 0, ULONG_MAX);
	if (value != ULONG_MAX)
	{
		result = value == 1;
	}
	else
	{
		std::string argStr = argIn;
		argStr = lava::stringToUpper(argStr);
		if (argStr == "T" || "TRUE" || argStr == "Y" || argStr == "YES")
		{
			result = 1;
		}
	}

	return result;
}


const std::string commentChars = "/\\#";
bool lineIsCommented(std::string lineIn)
{
	bool result = 1;

	if (!lineIn.empty())
	{
		result = commentChars.find(lineIn.front()) != std::string::npos;
	}

	return result;
}
std::string scrubPathString(std::string stringIn)
{
	std::string manipStr = "";
	manipStr.reserve(stringIn.size());
	bool inQuote = 0;
	bool doEscapeChar = 0;
	for (std::size_t i = 0; i < stringIn.size(); i++)
	{
		if (stringIn[i] == '\"' && !doEscapeChar)
		{
			inQuote = !inQuote;
		}
		else if (stringIn[i] == '\\')
		{
			doEscapeChar = 1;
		}
		else if (inQuote || !std::isspace(stringIn[i]))
		{
			doEscapeChar = 0;
			manipStr += stringIn[i];
		}
	}
	return manipStr;
}
std::string suffixFilename(std::string filepathIn, std::string suffixIn)
{
	std::filesystem::path tempPath(filepathIn); // We use this to decompose the passed in path arg.
	std::string finalDirectory = tempPath.parent_path().string();
	std::string finalFilename = tempPath.stem().string() + suffixIn + tempPath.extension().string();

	return finalDirectory + finalFilename;
}
std::vector<unsigned long> parseIDListDocument(std::string documentPath)
{
	std::vector<unsigned long> result{};

	if (std::filesystem::is_regular_file(documentPath))
	{
		std::ifstream fileIn(documentPath);
		std::string currentLine = "";
		while (std::getline(fileIn, currentLine))
		{
			if (!lineIsCommented(currentLine))
			{
				constexpr char delimChar = ',';
				std::vector<std::string> lineSegments{};

				std::size_t delimLoc = 0;
				while (delimLoc != std::string::npos)
				{
					delimLoc = currentLine.find(delimChar, delimLoc);
					lineSegments.push_back(currentLine.substr(0, delimLoc));
					if ((delimLoc + 1) < currentLine.size())
					{
						currentLine = currentLine.substr(delimLoc + 1, std::string::npos);
					}
				}

				for (std::size_t i = 0; i < lineSegments.size(); i++)
				{
					unsigned long retrievedID = stringToNum(lineSegments[i], 0, ULONG_MAX);
					if (retrievedID != ULONG_MAX)
					{
						result.push_back(retrievedID);
					}
				}
			}
		}
	}

	return result;
}
std::vector<unsigned long> handleLiteralNumvsNumListPathOverload(char* argument)
{
	std::vector<unsigned long> result{};

	unsigned long literalNum = stringToNum(argument, 0, ULONG_MAX);
	if (literalNum != ULONG_MAX)
	{
		result.push_back(literalNum);
	}
	else
	{
		if (std::filesystem::is_regular_file(argument))
		{
			result = parseIDListDocument(argument);
		}
	}

	return result;
}

bool doCloneWAVEs(lava::brawl::brsar& targetBRSAR, std::vector<unsigned long> fileIDList, unsigned long cloneCount, unsigned long sourceWAVEID)
{
	bool result = 0;

	for (unsigned long i = 0; i < fileIDList.size(); i++)
	{
		std::cout << "Adding to: RWSD 0x" << lava::numToHexStringWithPadding(fileIDList[i], 0x03) << "...\n";
		lava::brawl::rwsd tempRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = targetBRSAR.infoSection.getFileHeaderPointer(fileIDList[i]);
		if (relevantFileHeader != nullptr)
		{
			if (tempRWSD.populate(relevantFileHeader->fileContents))
			{
				bool invalidSource = 0;
				lava::brawl::waveInfo sourceWAVEObj;
				if (sourceWAVEID != ULONG_MAX)
				{
					if (sourceWAVEID < tempRWSD.waveSection.entries.size())
					{
						sourceWAVEObj = tempRWSD.waveSection.entries[sourceWAVEID];
					}
					else
					{
						invalidSource = 1;
					}
				}
				else
				{
					sourceWAVEObj = tempRWSD.waveSection.entries[0];
					sourceWAVEObj.hollowOut();
				}

				if (!invalidSource)
				{
					if (tempRWSD.createNewWaveEntries(sourceWAVEObj, cloneCount, 0))
					{
						if (targetBRSAR.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), fileIDList[i]))
						{
							result = 1;
							std::cout << "[SUCCESS] Modified and reimported RWSD 0x" << lava::numToHexStringWithPadding(fileIDList[i], 3) << "!\n";
						}
						else
						{
							std::cerr << "[ERROR] Failed while writing edited RWSD back into BRSAR!\n";
						}
					}
					else
					{
						std::cerr << "[ERROR] Failed while cloning WAVE entry!\n";
					}
				}
				else
				{
					std::cerr << "[ERROR] Specified RWSD has no WAVE with ID " <<
						lava::numToDecStringWithPadding(sourceWAVEID, 0) <<
						" (0x" << lava::numToHexStringWithPadding(sourceWAVEID, 2) << ")!\n";
					std::cerr << "\tHighest WAVE ID in the file is " <<
						lava::numToDecStringWithPadding(tempRWSD.waveSection.entries.size(), 0) <<
						" (0x" << lava::numToHexStringWithPadding(tempRWSD.waveSection.entries.size(), 2) << ")!\n";
				}
			}
			else
			{
				std::cerr << "[ERROR] Specified File ID does not belong to a valid RWSD!\n";
			}
		}
		else
		{
			std::cerr << "[ERROR] Invalid File ID Specified!\n";
		}
	}

	return result;
}

int main(int argc, char** argv)
{
	_globalArgC = argc;
	_globalArgV = argv;

	parseIDListDocument("idtest.txt");

	srand(time(0));
	std::cout << "lavaBRSARLibCLI (Library " << lava::brawl::version << ", CLI " << cliVersion << ")\n";
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
				bool splitHeaderAndData = 0;
				bool summaryOnly = 0;
				std::string targetFolder = brsarDumpDefaultPath;
				if (argc >= 4 && !isNullArg(argv[3]))
				{
					targetFolder = argv[3];
					folderArgumentProvided = 1;
				}
				if (argc >= 5 && !isNullArg(argv[4]))
				{
					summaryOnly = processBoolArgument(argv[4]);
					if (summaryOnly)
					{
						std::cout << "[C.Arg] Skipping file dump, summarizing contents only.\n";
					}
				}
				if (argc >= 6 && !isNullArg(argv[5]))
				{
					splitHeaderAndData = processBoolArgument(argv[5]);
					if (splitHeaderAndData && !summaryOnly)
					{
						std::cout << "[C.Arg] Exported files will be split into header and data \".dat\" files.\n";
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
								std::cerr << "[ERROR] Specified dump folder doesn't exist (\"" << temppath << "\").\n";
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
						testBrsar.doFileDump(targetFolder, !splitHeaderAndData, summaryOnly);
					}
				}
				return 0;
			}
			else if (strcmp("summarizeRWSDs", argv[1]) == 0 && argc >= 3)
			{
				std::cout << "Operation: Summarize RWSDs\n";
				bool result = 1;
				std::string rwsdListPath = argv[2];
				bool folderArgumentProvided = 0;
				std::string targetFolder = "";
				if (argc >= 4 && !isNullArg(argv[3]))
				{
					targetFolder = argv[3];
					std::cout << "[C.Arg] Produced summaries will be written to folder \"" << targetFolder << "\".\n";
					folderArgumentProvided = 1;
				}
				if (std::filesystem::is_regular_file(rwsdListPath))
				{
					if (folderArgumentProvided)
					{
						if (targetFolder.back() != '/' && targetFolder.back() != '\\')
						{
							targetFolder += "/";
						}
						std::filesystem::path temppath = targetFolder;
						if (!std::filesystem::is_directory(temppath))
						{
							if (folderArgumentProvided)
							{
								std::cerr << "[ERROR] Specified output folder doesn't exist (\"" << temppath << "\").\n";
								result = 0;
							}
						}
					}
				}
				else
				{
					std::cerr << "[ERROR] Specified RWSD Path List document (\"" << rwsdListPath << "\") does not exist.\n";
					result = 0;
				}
				if (result)
				{
					std::vector<std::filesystem::path> collectedPaths{};
					std::ifstream pathsIn(rwsdListPath, std::ios_base::in);
					std::string currentLine = "";
					while (std::getline(pathsIn, currentLine))
					{
						if (!currentLine.empty() && !lineIsCommented(currentLine))
						{
							std::filesystem::path linePath = scrubPathString(currentLine);
							if (std::filesystem::is_regular_file(linePath))
							{
								collectedPaths.push_back(linePath);
							}
							else
							{
								std::cout << "[WARNING] Skipping file \"" << linePath.string() << "\". It doesn't appear to exist.\n";
							}
						}
					}
					for (unsigned long i = 0; i < collectedPaths.size(); i++)
					{
						lava::brawl::rwsd tempRWSD;
						if (tempRWSD.populate(collectedPaths[i].string()))
						{
							std::filesystem::path summaryPath;
							if (folderArgumentProvided)
							{
								summaryPath = targetFolder + lava::pruneFileExtension(collectedPaths[i].filename().string()) + "_meta.txt";
							}
							else
							{
								summaryPath = lava::pruneFileExtension(collectedPaths[i].string()) + "_meta.txt";
							}
							std::cout << "Parsed \"" << collectedPaths[i].string() << "\", writing summary...\n";
							if (tempRWSD.summarize(summaryPath.string()))
							{
								std::cout << "\t[SUCCESS] Summary written to \"" << summaryPath.string() << "\".\n";
							}
							else
							{
								std::cerr << "\t[ERROR] Failure! Unable to write to \"" << summaryPath.string() << "\".\n";
							}
						}
						else
						{
							std::cerr << "[ERROR] Unable to parse file \"" << collectedPaths[i].string() << "\", It doesn't appear to be a valid RWSD file.\n";
						}
					}
				}
				return 0;
			}
			else if (strcmp("createWAVEs", argv[1]) == 0 && argc >= 5)
			{
				std::cout << "Operation: Create New WAVE Entries\n";
				bool result = 1;
				lava::brawl::brsar sourceBrsar;
				std::string targetBRSARPath = argv[2];

				std::vector<unsigned long> fileIDList = handleLiteralNumvsNumListPathOverload(argv[3]);

				unsigned char cloneCount = (unsigned char)stringToNum(argv[4], 0, UCHAR_MAX);

				unsigned long sourceWaveID = ULONG_MAX;
				if (argProvided(5))
				{
					sourceWaveID = stringToNum(argv[5], 0, ULONG_MAX);
				}

				if (std::filesystem::exists(targetBRSARPath))
				{
					sourceBrsar.init(targetBRSARPath);
					if (doCloneWAVEs(sourceBrsar, fileIDList, cloneCount, sourceWaveID))
					{
						std::string exportPath = suffixFilename(targetBRSARPath, "_edit");
						if (sourceBrsar.exportContents(exportPath))
						{
							std::cout << "[SUCCESS] Exported modified BRSAR to \"" << exportPath << "\"!\n";
						}
						else
						{
							std::cerr << "[ERROR] Failed to export modified BRSAR to \"" << exportPath << "\"!\n";
						}
					}
					else
					{
						std::cerr << "[ERROR] Failed to create WAVEs!\n";
					}
				}
				else
				{
					std::cerr << "[ERROR] Specified BRSAR (\"" << targetBRSARPath << "\") does not exist.\n";
					result = 0;
				}
				return 0;
			}
			else if (strcmp("zeroOutRWSD", argv[1]) == 0 && argc >= 4)
			{
				std::cout << "Operation: Zero Out WAVE Data in RWSD\n";
				bool result = 1;
				lava::brawl::brsar sourceBrsar;
				std::string targetBRSARPath = argv[2];

				unsigned long targetFileID = stringToNum(argv[3], 0, ULONG_MAX);

				unsigned long remainingWAVEEntries = 1;
				if (argProvided(4))
				{
					remainingWAVEEntries = stringToNum(argv[4], 0, ULONG_MAX);
				}

				bool zeroOutRemainingEntries = 1;
				if (argProvided(5))
				{
					zeroOutRemainingEntries = processBoolArgument(argv[5]);
				}

				if (std::filesystem::exists(targetBRSARPath))
				{
					sourceBrsar.init(targetBRSARPath);
					lava::brawl::rwsd tempRWSD;
					lava::brawl::brsarInfoFileHeader* relevantFileHeader = sourceBrsar.infoSection.getFileHeaderPointer(targetFileID);
					if (relevantFileHeader != nullptr)
					{
						if (tempRWSD.populate(relevantFileHeader->fileContents))
						{
							if (remainingWAVEEntries == 0)
							{
								remainingWAVEEntries = tempRWSD.waveSection.entries.size();
							}
							if (remainingWAVEEntries <= tempRWSD.waveSection.entries.size())
							{
								if (tempRWSD.cutDownWaveSection(remainingWAVEEntries, zeroOutRemainingEntries))
								{
									if (sourceBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), targetFileID))
									{
										std::filesystem::path tempPath(targetBRSARPath); // We use this to decompose the passed in path arg.
										std::string exportDirectory = tempPath.parent_path().string();
										std::string exportFilename = tempPath.stem().string() + "_edit.brsar";
										if (sourceBrsar.exportContents(exportDirectory + exportFilename))
										{
											std::cout << "[SUCCESS] Exporting modified BRSAR to \""
												<< exportDirectory << exportFilename << "\"!\n";
										}
										else
										{
											std::cerr << "[ERROR] Failed to export edited BRSAR to \""
												<< exportDirectory << exportFilename << "\"!\n";
										}
									}
									else
									{
										std::cerr << "[ERROR] Failed while writing edited RWSD back into BRSAR!\n";
									}
								}
								else
								{
									std::cerr << "[ERROR] Failed while cutting down RWSD!\n";
								}
							}
							else
							{
								std::cerr << "[ERROR] Unable to reduce RWSD down to " 
									<< lava::numToDecStringWithPadding(remainingWAVEEntries, 0) << " WAVE entries:\n";
								std::cerr << "\tThere are only "
									<< lava::numToDecStringWithPadding(tempRWSD.waveSection.entries.size(), 0) << " entries in the RWSD.\n";
							}
						}
						else
						{
							std::cerr << "[ERROR] Specified File ID does not belong to a valid RWSD!\n";
						}
					}
					else
					{
						std::cerr << "[ERROR] Invalid File ID Specified!\n";
					}
				}
				else
				{
					std::cerr << "[ERROR] Specified BRSAR (\"" << targetBRSARPath << "\") does not exist.\n";
					result = 0;
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
		// DumpRSAR Info
		{
			std::cout << "To dump and summarize the files contained in a brsar file:\n";
			std::cout << "\tdumpRSAR {BRSAR_PATH} {OUTPUT_DIR, optional} {DO_FILE_SUMMARY_ONLY, optional} {SPLIT_HEADERS_AND_DATA, opt}\n";
			std::cout << "\tNote: Default OUTPUT_DIRECTORY is \"" << brsarDumpDefaultPath << "{BRSAR_NAME}/\"\n";
			std::cout << "\tNote: DO_FILE_SUMMARY_ONLY and SPLIT_HEADERS_AND_DATA are boolean arguments, both are set to false by default.\n";
		}
		// SummarizeRWSDs Info
		{
			std::cout << "To summarize the contents of dumped RWSD files:\n";
			std::cout << "\tsummarizeRWSDs {RWSD_PATHS_LIST_PATH} {OUTPUT_DIRECTORY, optional}\n";
			std::cout << "\tNote: RWSD paths list file should contain a list of paths to the RWSDs to summarize.\n";
			std::cout << "\tNote: By default, summaries will be placed in the same folder as their source file, wherever they are.\n";
			std::cout << "\t  Specifying your own output directory will instead force all summaries to be placed in the specified folder.\n";
		}
		// CreateWAVEs Info
		{
			std::cout << "To add WAVE entries to an RWSD:\n";
			std::cout << "\tcreateWAVEs {BRSAR_PATH} {FILE_ID} {COPY_COUNT} {SOURCE_WAVE_ID, optional}, OR\n";
			std::cout << "\tcreateWAVEs {BRSAR_PATH} {FILE_ID_LIST_PATH} {COPY_COUNT} {SOURCE_WAVE_ID, optional}\n";
			std::cout << "\tNote: FILE_ID_LIST_PATH should point to a file which lists the IDs of RWSDs to add to.\n";
		}
		// zeroOutRWSD
		{
			std::cout << "To minimize the size of an RWSD by removing or zeroing out WAVE entries:\n";
			std::cout << "\tzeroOutRWSD {BRSAR_PATH} {FILE_ID} {REMAINING_WAVES, optional} {ZERO_OUT_WAVES, opt}\n";
			std::cout << "\tNote: REMAINING_WAVES is the number WAVE entries to leave in the RWSD.\n";
			std::cout << "\t  Default value is 1. Specify 0 to avoid removing any WAVE entries, maintaining the original count.\n";
			std::cout << "\tNote: ZERO_OUT_WAVES is a boolean argument, decides wether to zero out the audio data for remaining entries.\n";
			std::cout << "\t  This is a boolean argument, default vaule is true.\n";
		}
		
		std::cout << "Note: To explicitly use one of the above defaults, specify \"" << nullArgumentString << "\" for that argument.\n";
	}
	catch (std::exception e)
	{
		std::cerr << "EXCEPTION: ";
		std::cerr << e.what();
	}
	return 0;
}