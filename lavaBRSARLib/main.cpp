#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

const std::string targetBrsarName = "smashbros_sound";
const std::string tempFileDumpBaseFolder = targetBrsarName + "/";
const unsigned long fileOverwriteTestTargetFile = 0x50;
const unsigned long dspTestTargetFileID = 0x32D;
const std::string dspTestFileName = "sawnd000";
const unsigned long dspTestExportWaveIndex = 0x01;
const unsigned long dspTestImportWaveIndex = 0x00;
const unsigned long multiWaveExportTestInitialGroupID = 7;
const unsigned long multiWaveExportTestGroupsCound = 3;
const std::string multiWaveExportOutputDIrectory = "./WAVE_TEST/";
const std::string testFileName = "testFile";
const std::string testFileSuffix = ".dat";
const std::string testFilePath = testFileName + testFileSuffix;
const std::string testFileOutputPath = testFileName + "_edit" + testFileSuffix;

// Test which overwrites File 0x06 with itself, shouldn't actually change anything.
constexpr bool ENABLE_FILE_OVERWRITE_TEST_1 = false;
// Test which overwrites File 0x06's header and data with zeroed-out 0x20 byte vectors.
constexpr bool ENABLE_FILE_OVERWRITE_TEST_2 = false;
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
// Tests addign new WAV entries to an RWSD
constexpr bool ENABLE_PUSH_RWSD_WAV_ENTRY_TEST = true;
// Tests lossiness of the dsp-to-wav conversion process
constexpr bool ENABLE_CONV_LOSS_TEST = true;
// Tests lavaByteArray's Operations for errors.
constexpr bool ENABLE_BYTE_ARRAY_TEST = false;

int main()
{
	lava::brawl::brsar testBrsar;
	testBrsar.init(targetBrsarName + ".brsar");
	if (ENABLE_FILE_OVERWRITE_TEST_1)
	{
		lava::brawl::brsarFileFileContents temp = *testBrsar.fileSection.getFileContentsPointer(fileOverwriteTestTargetFile);
		testBrsar.overwriteFile(temp.header, temp.data, fileOverwriteTestTargetFile);
	}
	if (ENABLE_FILE_OVERWRITE_TEST_2)
	{
		std::vector<unsigned char> fakeHeader(0x20, 0x00);
		std::vector<unsigned char> fakeData(0x20, 0x00);
		testBrsar.overwriteFile(fakeHeader, fakeData, fileOverwriteTestTargetFile);
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
			testBrsar.fileSection.exportContents(fileDumpTest);
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
	if (ENABLE_WAVE_INFO_TO_DSP_TEST)
	{
		lava::brawl::rwsd testExportRWSD;
		if (testExportRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(dspTestTargetFileID)))
		{
			lava::brawl::dsp tempDSP = testExportRWSD.exportWaveRawDataToDSP(dspTestExportWaveIndex);
			std::ofstream dspOut(dspTestFileName + ".dsp", std::ios_base::out | std::ios_base::binary);
			tempDSP.exportContents(dspOut);
		}
	}
	if (ENABLE_DSP_TO_WAVE_INFO_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		if (tempRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(dspTestTargetFileID)))
		{
			if (tempRWSD.overwriteWaveRawDataWithDSP(dspTestImportWaveIndex, dspTestFileName + ".dsp"))
			{
				if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
				{
					testBrsar.exportContents(targetBrsarName + "_dsp.brsar");
				}
			}
		}
	}
	if (ENABLE_WAVE_INFO_TO_WAV_TEST)
	{
		lava::brawl::rwsd testExportRWSD;
		if (testExportRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(dspTestTargetFileID)))
		{
			testExportRWSD.exportWaveRawDataToWAV(dspTestExportWaveIndex, dspTestFileName + ".wav");
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
					lava::brawl::brsarFileFileContents* currFileContents = testBrsar.fileSection.getFileContentsPointer(currFile->fileID, u);
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
		if (tempRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(dspTestTargetFileID)))
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
	if (ENABLE_PUSH_RWSD_WAV_ENTRY_TEST)
	{
		lava::brawl::rwsd tempRWSD;
		if (tempRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(799)))
		{
			if (tempRWSD.grantDataEntryUniqueWave(1, tempRWSD.waveSection.entries[1]))
			{
				if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), 799))
				{
					testBrsar.exportContents(targetBrsarName + "_newwav.brsar");
				}
			}
		}
	}
	if (ENABLE_CONV_LOSS_TEST)
	{
		lava::brawl::rwsd testExportRWSD;
		if (testExportRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(dspTestTargetFileID)))
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
