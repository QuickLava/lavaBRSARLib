#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

const std::string targetBrsarName = "smashbros_sound";
const std::string tempFileDumpBaseFolder = "./Junk/" + targetBrsarName + "/";
const unsigned long fileOverwriteTestTargetFile = 0x50;
const unsigned long dspTestTargetFileID = 0x275;
const std::string dspTestFileName = "sawnd000";
const unsigned long dspTestExportWaveIndex = 0x01;
const unsigned long dspTestImportWaveIndex = 0x00;
const unsigned long multiWaveExportTestInitialGroupID = 7;
const unsigned long multiWaveExportTestGroupsCound = 3;
const unsigned long multiRWSDPushWAVCount = 0x7F;
const std::string multiWaveExportOutputDIrectory = "./WAVE_TEST/";
const std::string testFileName = "testFile";
const std::string testFileSuffix = ".dat";
const std::string testFilePath = testFileName + testFileSuffix;
const std::string testFileOutputPath = testFileName + "_edit" + testFileSuffix;

// Test which overwrites File 0x06 with itself, shouldn't actually change anything.
constexpr bool ENABLE_FILE_OVERWRITE_TEST_1 = false;
// Test which overwrites File 0x06's header and data with zeroed-out 0x20 byte vectors.
constexpr bool ENABLE_FILE_OVERWRITE_TEST_2 = false;
// Test which clones a specified file, handling all necessary INFO and FILE section changes.
constexpr bool ENABLE_FILE_CLONE_TEST = false;
// Test which exports the entire .brsar.
constexpr bool ENABLE_BRSAR_EXPORT_TEST = true;
// Test which exports the full SYMB section.
constexpr bool ENABLE_SYMB_SECTION_EXPORT_TEST = false;
// Test which exports the full INFO section.
constexpr bool ENABLE_INFO_SECTION_EXPORT_TEST	= false;
// Test which exports the full FILE section.
constexpr bool ENABLE_FILE_SECTION_EXPORT_TEST	= false;
// Test which summarizes and dumps every file in the .brsar, grouped by group.
constexpr bool ENABLE_FILE_DUMP_TEST = false;
// Test which dumps all the strings in the SYMB section.
constexpr bool ENABLE_STRING_DUMP_TEST = false;
// Test which summarizes data for every brsarInfoFileHeader in the .brsar.
constexpr bool ENABLE_FILE_INFO_SUMMARY_TEST = false;
// Tests the .spt to .dsp header conversion system (see "lavaDSP.h").
constexpr bool ENABLE_SPT_TO_DSP_HEADER_TEST = false;
// Tests summarizing and exporting an RWSD's data.
constexpr bool ENABLE_RWSD_SUMMARIZE_TEST = false;
// Tests taking apart and reconstructing every RWSD in the BRSAR.
constexpr bool ENABLE_RWSD_RECONSTRUCTION_TEST = false; constexpr bool DUMP_BAD_RECONSTRUCTIONS = false;
// Tests exporting an RWSD Wave Info entry into a DSP.
constexpr bool ENABLE_WAVE_INFO_TO_DSP_TEST = false;
// Tests importing a DSP into an RWSD Wave Info entry.
constexpr bool ENABLE_DSP_TO_WAVE_INFO_TEST = false;
// Tests exporting an RWSD Wave Info entry into a WAV.
constexpr bool ENABLE_WAVE_INFO_TO_WAV_TEST = false;
// Tests exporting all of the Wave Info entries in the specified groups to WAVs.
constexpr bool ENABLE_MULTI_WAVE_INFO_TO_WAV_TEST = false;
// Tests importing a WAV into an RWSD Wave Info entry.
constexpr bool ENABLE_WAV_TO_WAVE_INFO_TEST = false;
// Tests adding new WAV entries to an RWSD
constexpr bool ENABLE_PUSH_RWSD_WAV_ENTRY_TEST = false;
// Pushes multiple WAVE Entries to the specified RWSD
constexpr bool ENABLE_MULTI_PUSH_RWSD_WAV_ENTRY_TEST = false;
// Fills all WAVE entries with a given WAV
constexpr bool ENABLE_FILL_RWSD_WITH_WAV_TEST = false;
// Fills all WAVE entries with a given WAV
constexpr bool ENABLE_CUT_DOWN_RWSD_TEST = false;
// Tests lossiness of the dsp-to-wav conversion process
constexpr bool ENABLE_CONV_LOSS_TEST = false;
// Tests lavaByteArray's Operations for errors.
constexpr bool ENABLE_BYTE_ARRAY_TEST = false;

int main()
{
	lava::brawl::brsar testBrsar;
	testBrsar.init(targetBrsarName + ".brsar");
	if (ENABLE_FILE_OVERWRITE_TEST_1)
	{
		lava::brawl::brsarInfoFileHeader* targetFile = testBrsar.infoSection.getFileHeaderPointer(fileOverwriteTestTargetFile);
		if (targetFile != nullptr)
		{
			testBrsar.overwriteFile(targetFile->fileContents.header, targetFile->fileContents.data, fileOverwriteTestTargetFile);
		}
		else
		{
			std::cerr << "ENABLE_FILE_OVERWRITE_TEST_1 Test failed! Specified File ID (" 
				<< fileOverwriteTestTargetFile << " / 0x" << lava::numToHexStringWithPadding(fileOverwriteTestTargetFile, 0x03)
				<< ") doesn't exist in this BRSAR!\n";
		}
	}
	if (ENABLE_FILE_OVERWRITE_TEST_2)
	{
		std::vector<unsigned char> fakeHeader(0x20, 0x00);
		std::vector<unsigned char> fakeData(0x20, 0x00);
		testBrsar.overwriteFile(fakeHeader, fakeData, fileOverwriteTestTargetFile);
	}
	if (ENABLE_FILE_CLONE_TEST)
	{
		testBrsar.cloneFile(0x32D, 0x01);
	}
	if (ENABLE_BRSAR_EXPORT_TEST)
	{
		testBrsar.exportContents(targetBrsarName + "_rebuilt.brsar");
	}
	if (ENABLE_SYMB_SECTION_EXPORT_TEST)
	{
		std::ofstream symbDumpTest(targetBrsarName + "_symbdump.dat", std::ios_base::out | std::ios_base::binary);
		if (symbDumpTest.is_open())
		{
			testBrsar.symbSection.exportContents(symbDumpTest);
			symbDumpTest.close();
		}
	}
	if (ENABLE_INFO_SECTION_EXPORT_TEST)
	{
		std::ofstream infoDumpTest(targetBrsarName + "_infodump.dat", std::ios_base::out | std::ios_base::binary);
		if (infoDumpTest.is_open())
		{
			testBrsar.infoSection.exportContents(infoDumpTest);
			infoDumpTest.close();
		}
	}
	if (ENABLE_FILE_SECTION_EXPORT_TEST)
	{
		std::ofstream fileDumpTest(targetBrsarName + "_filedump.dat", std::ios_base::out | std::ios_base::binary);
		if (fileDumpTest.is_open())
		{
			testBrsar.exportVirtualFileSection(fileDumpTest);
			fileDumpTest.close();
		}
	}
	if (ENABLE_FILE_DUMP_TEST)
	{
		testBrsar.doFileDump(tempFileDumpBaseFolder, 0);
	}
	if (ENABLE_STRING_DUMP_TEST)
	{
		std::ofstream stringDumpTest(targetBrsarName + "_symbStrings.txt", std::ios_base::out | std::ios_base::binary);
		if (stringDumpTest.is_open())
		{
			testBrsar.summarizeSymbStringData(stringDumpTest);
			stringDumpTest.close();
		}
	}
	if (ENABLE_FILE_INFO_SUMMARY_TEST)
	{
		std::ofstream infoFileDataTest(targetBrsarName + "_infoFileData.txt", std::ios_base::out | std::ios_base::binary);
		if (infoFileDataTest.is_open())
		{
			testBrsar.infoSection.summarizeFileEntryData(infoFileDataTest);
			infoFileDataTest.close();
		}
	}
	if (ENABLE_SPT_TO_DSP_HEADER_TEST)
	{
		lava::brawl::spt testSPT;
		testSPT.populate("sawnd.spt", 0x04);
		lava::brawl::dsp testDSP = lava::brawl::sptToDSPHeader(testSPT);
		std::ofstream dspFileOut("sawnd_new.dsp", std::ios_base::out | std::ios_base::binary);
		if (dspFileOut.is_open())
		{
			testDSP.exportContents(dspFileOut);
			std::ifstream testSPD("sawnd.spd", std::ios_base::in | std::ios_base::binary);
			if (testSPD.is_open())
			{
				dspFileOut << testSPD.rdbuf();
			}
			dspFileOut.close();
		}
	}
	if (ENABLE_RWSD_SUMMARIZE_TEST)
	{
		lava::brawl::rwsd testExportRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (relevantFileHeader->fileContents.getFileType() == lava::brawl::brsarHexTags::bht_RWSD)
			{
				if (testExportRWSD.populate(relevantFileHeader->fileContents))
				{
					std::string baseName = targetBrsarName + "_" + lava::numToHexStringWithPadding(dspTestTargetFileID, 0x03) + "_RWSD";
					std::ofstream fileDumpOut(baseName + ".dat", std::ios_base::out | std::ios_base::binary);
					std::ofstream summaryOut(baseName + ".txt", std::ios_base::out | std::ios_base::binary);
					testExportRWSD.exportFileSection(fileDumpOut);
					testExportRWSD.summarize(summaryOut);
				}
			}
		}
	}
	if (ENABLE_RWSD_RECONSTRUCTION_TEST)
	{
		MD5 md5Object;
		for (int i = 0; i < testBrsar.infoSection.fileHeaders.size(); i++)
		{
			lava::brawl::brsarInfoFileHeader* currHeader = testBrsar.infoSection.fileHeaders[i].get();
			if (currHeader != nullptr && currHeader->fileContents.getFileType() == lava::brawl::brsarHexTags::bht_RWSD)
			{
				std::string originalHeaderHash = md5Object((char*)currHeader->fileContents.header.data(), currHeader->fileContents.header.size());

				lava::brawl::rwsd testRWSD;
				if (testRWSD.populate(currHeader->fileContents))
				{
					lava::brawl::brsarFileFileContents backupFileContents = currHeader->fileContents;

					testBrsar.overwriteFile(testRWSD.fileSectionToVec(), testRWSD.rawDataSectionToVec(), i);
					std::string newHeaderHash = md5Object((char*)currHeader->fileContents.header.data(), currHeader->fileContents.header.size());
					if (originalHeaderHash != newHeaderHash)
					{
						std::string baseFilename = "bad_" + lava::numToHexStringWithPadding(i, 3);
						std::cerr << "File ID 0x" << lava::numToHexStringWithPadding(i, 0x03) << ": RWSD parsing error, input and output don't match!\n";
						if (DUMP_BAD_RECONSTRUCTIONS)
						{
							backupFileContents.dumpToFile(baseFilename + "_bak.brwsd");
							currHeader->fileContents.dumpToFile(baseFilename + ".brwsd");
						}
					}
				}
			}
		}
		testBrsar.exportContents(targetBrsarName + "_RWSD_rebuild.brsar");
	}
	if (ENABLE_WAVE_INFO_TO_DSP_TEST)
	{
		lava::brawl::rwsd testExportRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (testExportRWSD.populate(relevantFileHeader->fileContents))
			{
				lava::brawl::dsp tempDSP = testExportRWSD.exportWaveRawDataToDSP(dspTestExportWaveIndex);
				std::ofstream dspOut(dspTestFileName + ".dsp", std::ios_base::out | std::ios_base::binary);
				tempDSP.exportContents(dspOut);
			}
		}
	}
	if (ENABLE_DSP_TO_WAVE_INFO_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (tempRWSD.populate(relevantFileHeader->fileContents))
			{
				if (tempRWSD.overwriteWaveRawDataWithDSP(dspTestImportWaveIndex, dspTestFileName + ".dsp"))
				{
					if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
					{
						testBrsar.exportContents(targetBrsarName + "_dsp.brsar");
					}
				}
				lava::brawl::dsp tempDSP = tempRWSD.exportWaveRawDataToDSP(dspTestExportWaveIndex);
				std::ofstream dspOut(dspTestFileName + ".dsp", std::ios_base::out | std::ios_base::binary);
				tempDSP.exportContents(dspOut);
			}
		}
	}
	if (ENABLE_WAVE_INFO_TO_WAV_TEST)
	{
		lava::brawl::rwsd testExportRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (testExportRWSD.populate(relevantFileHeader->fileContents))
			{
				testExportRWSD.exportWaveRawDataToWAV(dspTestExportWaveIndex, dspTestFileName + ".wav");
			}
		}
	}
	if (ENABLE_MULTI_WAVE_INFO_TO_WAV_TEST)
	{
		for (unsigned long u = multiWaveExportTestInitialGroupID; u < multiWaveExportTestInitialGroupID + multiWaveExportTestGroupsCound; u++)
		{
			lava::brawl::rwsd testExportRWSD;
			lava::brawl::brsarInfoGroupHeader* currGroupHead = testBrsar.infoSection.getGroupWithID(u);
			if (currGroupHead != nullptr)
			{
				std::string targetDirectory = multiWaveExportOutputDIrectory + "Group_" + lava::numToDecStringWithPadding(u, 3) + "/";
				for (unsigned long y = 0; y < currGroupHead->entries.size(); y++)
				{
					lava::brawl::brsarInfoGroupEntry* currFile = &currGroupHead->entries[y];
					lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(currFile->fileID);
					lava::brawl::brsarFileFileContents* currFileContents = &relevantFileHeader->fileContents;
					if (currFileContents->header.size() >= 4)
					{
						if (lava::bytesToFundamental<unsigned int>(currFileContents->header.data()) == lava::brawl::brsarHexTags::bht_RWSD)
						{
							if (testExportRWSD.populate(*currFileContents))
							{
								if (std::filesystem::create_directories(targetDirectory) || std::filesystem::is_directory(targetDirectory))
								{
									for (unsigned long i = 0; i < testExportRWSD.waveSection.entries.size(); i++)
									{
										std::string targetFile = "file_" + lava::numToDecStringWithPadding(currFile->fileID, 3) + "_wav_" + lava::numToDecStringWithPadding(i, 3) + ".wav";
										std::cout << "Exporting " << targetDirectory + targetFile << "... ";
										if (testExportRWSD.exportWaveRawDataToWAV(i, targetDirectory + targetFile))
										{
											std::cout << "Success!\n";
										}
										else
										{
											std::cerr << "Failure!\n";
										}
									}
								}
								else
								{
									std::cerr << "Unable to create output folder (\"" << targetDirectory << "\"), make sure it's a valid location.\n";
								}
							}
						}
					}
				}
			}
		}
	}
	if (ENABLE_WAV_TO_WAVE_INFO_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (tempRWSD.populate(relevantFileHeader->fileContents))
			{
				if (tempRWSD.overwriteWaveRawDataWithWAV(dspTestImportWaveIndex, dspTestFileName + ".wav"))
				{
					if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
					{
						testBrsar.exportContents(targetBrsarName + "_wav.brsar");
					}
				}
			}
		}
	}
	if (ENABLE_PUSH_RWSD_WAV_ENTRY_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (tempRWSD.populate(relevantFileHeader->fileContents))
			{
				tempRWSD.summarize("tempRWSDSummary.txt");
				relevantFileHeader->fileContents.dumpToFile("tempRWSDDump.dat");
				if (tempRWSD.grantDataEntryUniqueWave(0x25, tempRWSD.waveSection.entries[0x24]))
				{
					//tempRWSD.overwriteWaveRawDataWithWAV(0, "sawnd000.wav");
					tempRWSD.summarize("tempRWSDSummary_edit.txt");
					if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
					{
						relevantFileHeader->fileContents.dumpToFile("tempRWSDDump_edit.dat");
						testBrsar.exportContents(targetBrsarName + "_newwav.brsar");
					}
				}
			}
		}
	}
	if (ENABLE_MULTI_PUSH_RWSD_WAV_ENTRY_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (tempRWSD.populate(relevantFileHeader->fileContents))
			{
				tempRWSD.summarize("tempMRWSDSummary.txt");
				relevantFileHeader->fileContents.dumpToFile("tempMRWSDDump.dat");
				tempRWSD.createNewWaveEntries(tempRWSD.waveSection.entries[0x00], multiRWSDPushWAVCount, 0);
				//tempRWSD.overwriteWaveRawDataWithWAV(0, "sawnd000.wav");
				tempRWSD.summarize("tempMRWSDSummary_edit.txt");
				if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
				{
					relevantFileHeader->fileContents.dumpToFile("tempMRWSDDump_edit.dat");
					testBrsar.exportContents(targetBrsarName + "_mnewwav.brsar");
				}
			}
		}
	}
	if (ENABLE_FILL_RWSD_WITH_WAV_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (tempRWSD.populate(relevantFileHeader->fileContents))
			{
				tempRWSD.summarize("tempORWSDSummary.txt");
				relevantFileHeader->fileContents.dumpToFile("tempORWSDDump.dat");
				tempRWSD.overwriteWaveRawDataWithWAV(0, "sawnd000.wav");
				tempRWSD.overwriteAllWaves(tempRWSD.waveSection.entries[0]);
				tempRWSD.summarize("tempORWSDSummary_edit.txt");
				if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
				{
					relevantFileHeader->fileContents.dumpToFile("tempORWSDDump_edit.dat");
					testBrsar.exportContents(targetBrsarName + "_overwrite.brsar");
				}
			}
		}
	}
	if (ENABLE_CUT_DOWN_RWSD_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (tempRWSD.populate(relevantFileHeader->fileContents))
			{
				tempRWSD.summarize("tempShrRWSDSummary.txt");
				relevantFileHeader->fileContents.dumpToFile("tempShrRWSDDump.dat");
				tempRWSD.cutDownWaveSection();
				tempRWSD.summarize("tempShrRWSDSummary_edit.txt");
				if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
				{
					relevantFileHeader->fileContents.dumpToFile("tempShrRWSDDump_edit.dat");
					testBrsar.exportContents(targetBrsarName + "_shr.brsar");
				}
			}
		}
	}
	if (ENABLE_CONV_LOSS_TEST)
	{
		lava::brawl::rwsd testExportRWSD;
		lava::brawl::brsarInfoFileHeader* relevantFileHeader = testBrsar.infoSection.getFileHeaderPointer(dspTestTargetFileID);
		if (relevantFileHeader != nullptr)
		{
			if (testExportRWSD.populate(relevantFileHeader->fileContents))
			{
				lava::brawl::dsp tempDSP = testExportRWSD.exportWaveRawDataToDSP(dspTestExportWaveIndex);
				std::ofstream dspOut(dspTestFileName + ".dsp", std::ios_base::out | std::ios_base::binary);
				tempDSP.exportContents(dspOut);
				dspOut.close();
				testExportRWSD.overwriteWaveRawDataWithDSP(dspTestExportWaveIndex, dspTestFileName + ".dsp");
				for (unsigned long i = 0; i < 30; i++)
				{
					lava::brawl::dsp testDSP = testExportRWSD.exportWaveRawDataToDSP(dspTestExportWaveIndex);
					std::ofstream testDSPOut(dspTestFileName + "_edit.dsp", std::ios_base::out | std::ios_base::binary);
					testDSP.exportContents(testDSPOut);
					testDSPOut.close();
					testExportRWSD.overwriteWaveRawDataWithDSP(dspTestExportWaveIndex, dspTestFileName + "_edit.dsp");
				}
			}
		}
	}
	if (ENABLE_BYTE_ARRAY_TEST)
	{
		std::ofstream testFileOut(testFilePath, std::ios_base::out | std::ios_base::binary);
		if (testFileOut.is_open())
		{
			for (std::size_t i = 0; i < USHRT_MAX; i++)
			{
				lava::writeRawDataToStream(testFileOut, i | (i << 0x10));
			}
			lava::byteArray testArr;
			std::ifstream testFileIn(testFilePath, std::ios_base::in | std::ios_base::binary);
			testArr.populate(testFileIn);
			testFileIn.close();
			std::size_t cursor = 0x00;
			while (cursor < testArr.size())
			{
				long temp = testArr.getLong(cursor, nullptr);
				testArr.setLong(temp, cursor, &cursor);
			}
			testArr.dumpToFile(testFileOutputPath);
		}
	}
	return 0;
}
