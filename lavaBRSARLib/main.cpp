#include "lavaByteArray.h"
#include "lavaBRSARLib.h"

const std::string targetBrsarName = "smashbros_sound";
const std::string tempFileDumpBaseFolder = targetBrsarName + "/";
const unsigned long fileOverwriteTestTargetFile = 0x50;
const std::string dspTestFileName = "sawnd000";
const unsigned long dspTestTargetFileID = 0x32D;
const unsigned long dspTestTargetWaveIndex = 0x01;

// Test which overwrites File 0x06 with itself, shouldn't actually change anything.
#define ENABLE_FILE_OVERWRITE_TEST_1 false
// Test which overwrites File 0x06's header and data with zeroed-out 0x20 byte vectors.
#define ENABLE_FILE_OVERWRITE_TEST_2 false
// Test which exports the entire .brsar.
#define ENABLE_BRSAR_EXPORT_TEST false
// Test which exports the full SYMB section.
#define ENABLE_SYMB_SECTION_EXPORT_TEST false
// Test which exports the full INFO section.
#define ENABLE_INFO_SECTION_EXPORT_TEST	false
// Test which exports the full FILE section.
#define ENABLE_FILE_SECTION_EXPORT_TEST	false
// Test which summarizes and dumps every file in the .brsar, grouped by group.
#define ENABLE_FILE_DUMP_TEST false
// Test which dumps all the strings in the SYMB section.
#define ENABLE_STRING_DUMP_TEST false
// Test which summarizes data for every brsarInfoFileHeader in the .brsar.
#define ENABLE_FILE_INFO_SUMMARY_TEST false
// Tests the .spt to .dsp header conversion system (see "lavaDSP.h").
#define ENABLE_SPT_TO_DSP_HEADER_TEST false
// Tests the DSP import function.
#define ENABLE_DSP_TO_WAVE_INFO_TEST true
// Tests the DSP export function.
#define ENABLE_WAVE_INFO_TO_DSP_TEST true

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
#if ENABLE_SPT_TO_DSP_HEADER_TEST
	lava::brawl::spt testSPT;
	testSPT.populate("sawnd.spt", 0x04);
	lava::brawl::dspHeader testDSP = lava::brawl::sptToDSPHeader(testSPT);
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
#endif
#if ENABLE_WAVE_INFO_TO_DSP_TEST
	lava::brawl::rwsd testExportRWSD;
	if (testExportRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(dspTestTargetFileID)))
	{
		lava::brawl::dsp tempDSP = testExportRWSD.exportWaveRawDataToDSP(dspTestTargetWaveIndex);
		std::ofstream dspOut(dspTestFileName + ".dsp", std::ios_base::out | std::ios_base::binary);
		tempDSP.exportContents(dspOut);
	}
#endif
#if ENABLE_DSP_TO_WAVE_INFO_TEST
	lava::brawl::dsp testImportDSP;
	if (testImportDSP.populate(dspTestFileName + ".dsp", 0x00))
	{
		lava::brawl::rwsd tempRWSD;
		tempRWSD.populate(*testBrsar.fileSection.getFileContentsPointer(dspTestTargetFileID));
		tempRWSD.overwriteWaveRawDataWithDSP(dspTestTargetWaveIndex, testImportDSP);
		if (testBrsar.overwriteFile(tempRWSD.fileSectionToVec(), tempRWSD.rawDataSectionToVec(), dspTestTargetFileID))
		{
			testBrsar.exportContents(targetBrsarName + "_dsp.brsar");
		}
	}
#endif
	return 0;
}