#ifndef LAVA_BRSAR_LIBRARY_V1
#define LAVA_BRSAR_LIBRARY_V1

#include <unordered_map>
#include <filesystem>
#include <sstream>
#include <array>
#include "lavaDSP.h"
#include "lavaByteArray.h"
#include "lavaUtility.h"
#include "md5.h"

/* -- About & Credits --
lavaBRSARLib is a work-in-progress library to facilitate the manipulation of BRSAR files.
This library is based heavily off of the work of other people. Special thanks to:
- Jaklub and Agoaj, as well as mstaklo, ssbbtailsfan, stickman and VILE (Sawndz, Super Sawndz)
- Soopercool101, as well as Kryal, BlackJax96, and libertyernie (BrawlLib, BrawlBox, BrawlCrate)
- Gota7 and kitlith (RhythmRevolution Documentation)
- Halley's Comet Software (VGMStream Source & Documentation, along with a varietey of other resources)
Additionally, this library directly makes use of the following code written by other people:
- VGAudioCli by Alex Barney (used for conversions between GC-ADPCM .dsp files and standard .wav files)
- Portable C++ Hashing Library by Stephan Brumme (used to provide md5 hashes in file export summaries)
*/

namespace lava
{
	namespace brawl
	{
		const std::string version = "v1.2.1";
		const std::string VGAudioPath = "./VGAudio/";
		const std::string VGAudioMainExeName = "VGAudioCli.exe";
		const std::string VGAudioMainExePath = VGAudioPath + VGAudioMainExeName;
		const std::string VGAudioTempConvFilename = "__tempfile.dsp";
		std::string generateVGAudioWavToDSPCommand(std::string wavFilePath, std::string outputFilePath);
		std::string generateVGAudioDSPToWavCommand(std::string dspFilePath, std::string outputFilePath);

		enum brsarHexTags
		{
			// BRSAR Sections
			bht_RSAR = 0x52534152,
			bht_SYMB = 0x53594D42,
			bht_INFO = 0x494E464F,
			bht_FILE = 0x46494C45,
			// FILE Section Types
			bht_RWSD = 0x52575344,
			bht_RBNK = 0x52424E4B,
			bht_RSEQ = 0x52534551,
			// Subfile Subsections
			bht_SUBF_DATA = 0x44415441,
			bht_SUBF_WAVE = 0x57415645,
			bht_SUBF_RWAR = 0x52574152,
			bht_SUBF_LABL = 0x4C41424C,
		};
		enum brsarHexTagType
		{
			bhtt_RSAR_SECTION = 1,
			bhtt_FILE_SECTION, 
			bhtt_SUBFILE_SUBSECTION
		};
		enum brsarAddressConsts
		{
			bac_NOT_IN_FILE,
		};
		enum brsarErrorReturnCodes
		{
			berc_OVERWRITE_SOUND_SHARED_WAVE = 0xFFA00000,
		};
		enum soundInfoTypes
		{
			sit_SEQUENCE = 0x01,
			sit_STREAM,
			sit_WAVE,
		};

		constexpr unsigned long _EMPTY_SOUND_TOTAL_LENGTH = 0x20;
		constexpr unsigned long _EMPTY_SOUND_SOUND_LENGTH = 0x02;
		constexpr unsigned long _EMPTY_SOUND_PADDING_LENGTH = _EMPTY_SOUND_TOTAL_LENGTH - _EMPTY_SOUND_SOUND_LENGTH;

		/* Misc. */

		unsigned long validateHexTag(unsigned long tagIn);
		bool detectHexTags(const byteArray& bodyIn, unsigned long startingAddress = 0x00);
		bool adjustOffset(unsigned long relativeBaseOffset, unsigned long& offsetIn, signed long adjustmentAmount, unsigned long startingAddress);
		int padLengthTo(unsigned long lengthIn, unsigned long padTo, bool allowZeroPaddingLength = 1);

		/* Misc. */


		/* Brawl Reference */

		struct brawlReference
		{
			unsigned long addressType = ULONG_MAX;
			unsigned long address = ULONG_MAX;

			static constexpr unsigned long size();
			brawlReference(unsigned long long valueIn = ULONG_MAX);

			bool isOffset() const;
			unsigned long getAddress(unsigned long relativeToIn = ULONG_MAX) const;
			std::string getAddressString(unsigned long relativeToIn = ULONG_MAX) const;
			unsigned long long getHex() const;
		};
		struct brawlReferenceVector
		{
			std::vector<brawlReference> refs{};

			unsigned long size() const;
			bool populate(const lava::byteArray& bodyIn, std::size_t addressIn = SIZE_MAX);
			std::vector<unsigned long> getHex() const;

			bool exportContents(std::ostream& destinationStream) const;
		};
		unsigned long calcRefVecSize(unsigned long entryCount);

		/* Brawl Reference */


		/*Sound Data Structs*/

		struct wavePacket
		{
			bool populated = 0;

			unsigned long address = ULONG_MAX;

			std::vector<unsigned char> body{};
			std::vector<unsigned char> padding{};

			bool populate(const lava::byteArray& bodyIn, unsigned long addressIn, unsigned long dataLengthIn, unsigned long paddingLengthIn);
		};
		struct waveInfo
		{
			unsigned long address = ULONG_MAX;

			unsigned char encoding = UCHAR_MAX;
			unsigned char looped = UCHAR_MAX;
			unsigned char channels = UCHAR_MAX;
			unsigned char sampleRate24 = UCHAR_MAX;
			unsigned short sampleRate = USHRT_MAX;
			unsigned char dataLocationType = UCHAR_MAX;
			unsigned char pad = UCHAR_MAX;
			unsigned long loopStartSample = ULONG_MAX;
			unsigned long nibbles = ULONG_MAX;
			unsigned long channelInfoTableOffset = ULONG_MAX;
			unsigned long dataLocation = ULONG_MAX;
			unsigned long reserved = ULONG_MAX;
			std::vector<unsigned long> channelInfoTable{};

			std::vector<channelInfo> channelInfoEntries;
			std::vector<adpcmInfo> adpcmInfoEntries;

			wavePacket packetContents;

			unsigned long size() const;
			unsigned long getAudioLengthInBytes() const;

			void copyOverWaveInfoProperties(const waveInfo& sourceInfo);
			void hollowOut();

			bool populate(const lava::byteArray& bodyIn, unsigned long addressIn);
			bool exportContents(std::ostream& destinationStream);
		};
		struct dataInfo
		{
			unsigned long address = ULONG_MAX;

			// References
			brawlReference wsdInfo;
			brawlReference trackTable;
			brawlReference noteTable;

			// "WSD" Data
			float wsdPitch = FLT_MAX;
			unsigned char wsdPan = UCHAR_MAX;
			unsigned char wsdSurroundPan = UCHAR_MAX;
			unsigned char wsdFxSendA = UCHAR_MAX;
			unsigned char wsdFxSendB = UCHAR_MAX;
			unsigned char wsdFxSendC = UCHAR_MAX;
			unsigned char wsdMainSend = UCHAR_MAX;
			unsigned char wsdPad1 = UCHAR_MAX;
			unsigned char wsdPad2 = UCHAR_MAX;
			brawlReference wsdGraphEnvTableRef = ULLONG_MAX;
			brawlReference wsdRandomizerTableRef = ULLONG_MAX;
			unsigned long wsdPadding = ULONG_MAX;

			// "Track Table" Data
			brawlReferenceVector ttReferenceList1;
			brawlReference ttIntermediateReference = ULLONG_MAX;
			brawlReferenceVector ttReferenceList2;
			float ttPosition = FLT_MAX;
			float ttLength = FLT_MAX;
			unsigned long ttNoteIndex = ULONG_MAX;
			unsigned long ttReserved = ULONG_MAX;

			// "Note Table" Data
			brawlReferenceVector ntReferenceList; // Appears to always have one entry that points to the below info
			unsigned long ntWaveIndex = ULONG_MAX;
			unsigned char ntAttack = UCHAR_MAX;
			unsigned char ntDecay = UCHAR_MAX;
			unsigned char ntSustain = UCHAR_MAX;
			unsigned char ntRelease = UCHAR_MAX;
			unsigned char ntHold = UCHAR_MAX;
			unsigned char ntPad1 = UCHAR_MAX;
			unsigned char ntPad2 = UCHAR_MAX;
			unsigned char ntPad3 = UCHAR_MAX;
			unsigned char ntOriginalKey = UCHAR_MAX;
			unsigned char ntVolume = UCHAR_MAX;
			unsigned char ntPan = UCHAR_MAX;
			unsigned char ntSurroundPan = UCHAR_MAX;
			float ntPitch = FLT_MAX;
			brawlReference ntIfoTableRef = ULLONG_MAX;
			brawlReference ntGraphEnvTableRef = ULLONG_MAX;
			brawlReference ntRandomizerTableRef = ULLONG_MAX;
			unsigned long ntReserved = ULONG_MAX;

			unsigned long size() const;
			bool populate(const lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream) const;
			void copyOverDataInfoProperties(const dataInfo& sourceInfo);
		};

		/*Sound Data Structs*/

		struct brsar; //Brsar Forward Decl.

		/* BRSAR Symb Section */

		struct brsarSymbPTrieNode
		{
			unsigned long address = ULONG_MAX;

			unsigned short isLeaf = USHRT_MAX;
			unsigned short posAndBit = USHRT_MAX;
			unsigned long leftID = ULONG_MAX;
			unsigned long rightID = ULONG_MAX;
			unsigned long stringID = ULONG_MAX;
			unsigned long infoID = ULONG_MAX;

			static constexpr unsigned long size();
			bool populate(lava::byteArray& bodyIn, unsigned long addressIn);

			bool exportContents(std::ostream& destinationStream) const;

			unsigned long getBit() const;
			unsigned long getPos() const;
			bool compareCharAndBit(char charIn) const;

			
		};
		struct brsarSymbPTrie
		{
			unsigned long address = ULONG_MAX;

			unsigned long rootID = ULONG_MAX;
			unsigned long numEntries = ULONG_MAX;

			std::vector<brsarSymbPTrieNode> entries{};

			unsigned long size() const;
			bool populate(lava::byteArray& bodyIn, unsigned long addressIn);
			bool exportContents(std::ostream& destinationStream) const;

			brsarSymbPTrieNode findString(std::string stringIn) const;

		};
		struct brsarSymbSection
		{
			brsar* parent = nullptr;

			unsigned long address = ULONG_MAX;

			unsigned long stringListOffset = ULONG_MAX;

			unsigned long soundTrieOffset;
			unsigned long playerTrieOffset;
			unsigned long groupTrieOffset;
			unsigned long bankTrieOffset;

			std::vector<unsigned long> stringEntryOffsets{};

			brsarSymbPTrie soundTrie;
			brsarSymbPTrie playerTrie;
			brsarSymbPTrie groupTrie;
			brsarSymbPTrie bankTrie;

			std::vector<unsigned char> stringBlock{};

			unsigned long size() const;
			unsigned long paddedSize(unsigned long padTo = 0x20) const;
			unsigned long getAddress() const;
			bool populate(brsar& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream) const;

			std::string getString(std::size_t idIn) const;
			bool dumpTrieStrings(std::ostream& destinationStream, const brsarSymbPTrie& sourceTrie) const;
			bool dumpStrings(std::ostream& destinationStream) const;
		};

		/* BRSAR Symb Section */



		/* BRSAR Info Section */

		struct brsarInfoSection; // Info Section Forward Decl.

		struct brsarInfoSoundEntry; // Info Sound Struct Forward Decl.
		struct brsarInfo3DSoundInfo
		{
			const brsarInfoSoundEntry* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long flags = ULONG_MAX;
			unsigned char decayCurve = UCHAR_MAX;
			unsigned char decayRatio = UCHAR_MAX;
			unsigned char dopplerFactor = UCHAR_MAX;
			unsigned char padding = UCHAR_MAX;
			unsigned long reserved = ULONG_MAX;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		};
		struct brsarInfoSequenceSoundInfo
		{
			const brsarInfoSoundEntry* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned int dataID = ULONG_MAX;
			unsigned int bankID = ULONG_MAX;
			unsigned int allocTrack = ULONG_MAX;
			unsigned char channelPriority = UCHAR_MAX;
			unsigned char releasePriorityFix = UCHAR_MAX;
			unsigned char pad1 = UCHAR_MAX;
			unsigned char pad2 = UCHAR_MAX;
			unsigned long reserved = ULONG_MAX;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		}; 
		struct brsarInfoStreamSoundInfo
		{
			const brsarInfoSoundEntry* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long startPosition = ULONG_MAX;
			unsigned short allocChannelCount = USHRT_MAX;
			unsigned short allocTrackFlag = USHRT_MAX;
			unsigned long reserved = ULONG_MAX;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		}; 
		struct brsarInfoWaveSoundInfo
		{
			const brsarInfoSoundEntry* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long soundIndex = ULONG_MAX;
			unsigned long allocTrack = ULONG_MAX;
			unsigned char channelPriority = UCHAR_MAX;
			unsigned char releasePriorityFix = UCHAR_MAX;
			unsigned char pad1 = UCHAR_MAX;
			unsigned char pad2 = UCHAR_MAX;
			unsigned long reserved = ULONG_MAX;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		};
		struct brsarInfoSoundEntry
		{
			const brsarInfoSection* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long stringID = ULONG_MAX;
			unsigned long fileID = ULONG_MAX;
			unsigned long playerID = ULONG_MAX;
			brawlReference param3DRefOffset = ULLONG_MAX;
			unsigned char volume = UCHAR_MAX;
			unsigned char playerPriority = UCHAR_MAX;
			unsigned char soundType = UCHAR_MAX;
			unsigned char remoteFilter = UCHAR_MAX;
			brawlReference soundInfoRef = ULLONG_MAX;
			unsigned long userParam1 = ULONG_MAX;
			unsigned long userParam2 = ULONG_MAX;
			unsigned char panMode = UCHAR_MAX;
			unsigned char panCurve = UCHAR_MAX;
			unsigned char actorPlayerID = UCHAR_MAX;
			unsigned char reserved = UCHAR_MAX;

			brsarInfo3DSoundInfo sound3DInfo{};
			union
			{
				brsarInfoSequenceSoundInfo seqSoundInfo{};
				brsarInfoWaveSoundInfo waveSoundInfo;
				brsarInfoStreamSoundInfo streamSoundInfo;
			};

			unsigned long size() const;
			unsigned long getAddress() const;
			bool populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);

			void updateSound3DInfoOffsetValue();
			void updateSpecificSoundOffsetValue();
		};

		struct brsarInfoBankEntry
		{
			const brsarInfoSection* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long stringID = ULONG_MAX;
			unsigned long fileID = ULONG_MAX;
			unsigned long padding = ULONG_MAX;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		};
		struct brsarInfoPlayerEntry
		{
			const brsarInfoSection* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long stringID = ULONG_MAX;
			unsigned char playableSoundCount = UCHAR_MAX;
			unsigned char padding = UCHAR_MAX;
			unsigned short padding2 = USHRT_MAX;
			unsigned long heapSize = ULONG_MAX;
			unsigned long reserved = ULONG_MAX;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		};
		struct brsarInfoFileHeader; // Info File Header Struct Forward Decl.
		struct brsarFileFileContents
		{
			std::vector<unsigned char> header{};
			std::vector<unsigned char> data{};

			unsigned long size() const;
			unsigned long getFileType() const;
			std::string getFileTypeString() const;

			bool populateFromFile(std::string filePathIn);

			bool dumpToStream(std::ostream& output);
			bool dumpHeaderToStream(std::ostream& output);
			bool dumpDataToStream(std::ostream& output);
			bool dumpToFile(std::string filePath);
			bool dumpHeaderToFile(std::string filePath);
			bool dumpDataToFile(std::string filePath);
		};
		struct brsarInfoFileEntry
		{
			const brsarInfoFileHeader* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long groupID = ULONG_MAX;
			unsigned long index = ULONG_MAX;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoFileHeader& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		};
		struct brsarInfoFileHeader
		{
			const brsarInfoSection* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long entryNumber = ULONG_MAX;
			brawlReference stringOffset = ULLONG_MAX;
			brawlReference listOffset = ULLONG_MAX;
			std::vector<unsigned char> stringContent{};
			std::vector<brsarInfoFileEntry> entries{};

			unsigned long originalFileHeaderLength = ULONG_MAX;
			unsigned long originalFileDataLength = ULONG_MAX;
			brsarFileFileContents fileContents;

			unsigned long size() const;
			unsigned long getAddress() const;
			bool populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);

			bool writeFileEntryRefVec(std::ostream& destinationStream) const;
			void updateFileEntryOffsetValues();
		};
		struct brsarInfoGroupHeader; // Info Group Header Forward Decl.
		struct brsarInfoGroupEntry
		{
			const brsarInfoGroupHeader* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long fileID = ULONG_MAX;
			unsigned long headerOffset = ULONG_MAX;
			unsigned long headerLength = ULONG_MAX;
			unsigned long dataOffset = ULONG_MAX;
			unsigned long dataLength = ULONG_MAX;
			unsigned long reserved = 0x00;

			static constexpr unsigned long size();
			unsigned long getAddress() const;
			bool populate(const brsarInfoGroupHeader& parentIn, lava::byteArray& bodyIn, std::size_t address);
			bool exportContents(std::ostream& destinationStream);
		};
		struct brsarInfoGroupHeader
		{
			const brsarInfoSection* parent = nullptr;
			unsigned long parentRelativeOffset = ULONG_MAX;

			unsigned long originalAddress = ULONG_MAX;

			unsigned long groupID = ULONG_MAX;
			unsigned long entryNum = ULONG_MAX;
			brawlReference extFilePathRef = ULLONG_MAX; // This is always zero?
			unsigned long headerAddress = ULONG_MAX;
			unsigned long headerLength = ULONG_MAX;
			unsigned long dataAddress = ULONG_MAX;
			unsigned long dataLength = ULONG_MAX;
			brawlReference listOffset = ULLONG_MAX;
			std::vector<brsarInfoGroupEntry> entries{};

			unsigned long size() const;
			unsigned long getAddress() const;
			bool populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t address);
			bool exportContents(std::ostream& destinationStream);

			bool writeGroupEntryRefVec(std::ostream& destinationStream) const;
			void updateGroupEntryOffsetValues();
			unsigned long getSynonymFileID(std::size_t headerLengthIn = SIZE_MAX) const;
			bool usesFileID(unsigned long fileIDIn = ULONG_MAX) const;
		};

		struct brsarInfoSection
		{
			enum class infoSectionLandmark
			{
				iSL_Header = 0,
				iSL_VecReferences,
				iSL_SoundEntries,
				iSL_BankEntries,
				iSL_PlayerEntries,
				iSL_FileHeaders,
				iSL_GroupHeaders,
				iSL_Footer,
			};

			brsar* parent = nullptr;

			unsigned long address = ULONG_MAX;
			std::unordered_map<unsigned long, std::vector<std::size_t>> fileIDsToGroupInfoIndecesThatUseThem{};

			brawlReference soundsSectionReference = ULLONG_MAX;
			brawlReference banksSectionReference = ULLONG_MAX;
			brawlReference playerSectionReference = ULLONG_MAX;
			brawlReference filesSectionReference = ULLONG_MAX;
			brawlReference groupsSectionReference = ULLONG_MAX;
			brawlReference footerReference = ULLONG_MAX;

			std::vector<std::unique_ptr<brsarInfoSoundEntry>> soundEntries{};
			std::vector<brsarInfoBankEntry> bankEntries;
			std::vector<brsarInfoPlayerEntry> playerEntries;
			std::vector<std::unique_ptr<brsarInfoFileHeader>> fileHeaders{};
			std::vector<std::unique_ptr<brsarInfoGroupHeader>> groupHeaders{};

			// Footer
			unsigned short sequenceMax = USHRT_MAX;
			unsigned short sequenceTrackMax = USHRT_MAX;
			unsigned short streamMax = USHRT_MAX;
			unsigned short streamTrackMax = USHRT_MAX;
			unsigned short streamChannelsMax = USHRT_MAX;
			unsigned short waveMax = USHRT_MAX;
			unsigned short waveTrackMax = USHRT_MAX;
			unsigned short padding = USHRT_MAX;
			unsigned long reserved = ULONG_MAX;
			// Footer

			unsigned long size(infoSectionLandmark tallyUpTo = infoSectionLandmark::iSL_Footer) const;
			unsigned long paddedSize(unsigned long padTo = 0x20) const;
			unsigned long getAddress() const;
			bool populate(brsar& parentIn, lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);

			bool writeSoundRefVec(std::ostream& destinationStream) const;
			bool writeBankRefVec(std::ostream& destinationStream) const;
			bool writePlayerRefVec(std::ostream& destinationStream) const;
			bool writeFileRefVec(std::ostream& destinationStream) const;
			bool writeGroupRefVec(std::ostream& destinationStream) const;

			void updateChildStructOffsetValues(infoSectionLandmark startFrom = infoSectionLandmark::iSL_Header);

			unsigned long virutalFileSectionSize() const;
			bool updateGroupEntryAddressValues();

			unsigned long addNewFileEntry();
			bool linkFileEntryToGroup(unsigned long fileID, unsigned long groupInfoIndexToLinkTo);

			brsarInfoGroupHeader* getGroupWithID(unsigned long groupIDIn);
			brsarInfoGroupHeader* getGroupWithInfoIndex(unsigned long infoIndexIn);
			std::vector<brsarInfoFileHeader*> getFilesWithGroupID(unsigned long groupIDIn);
			brsarInfoFileHeader* getFileHeaderPointer(unsigned long fileID);
			bool summarizeFileEntryData(std::ostream& output);
		};

		/* BRSAR Info Section */



		/* BRSAR File Section */

		struct rwsdDataSection
		{
			unsigned long address = ULONG_MAX;

			unsigned long originalLength = ULONG_MAX;

			brawlReferenceVector entryReferences;
			std::vector<dataInfo> entries{};

			bool hasExclusiveWave(unsigned long dataSectionIndex);
			bool isFirstToUseWave(unsigned long dataSectionIndex);

			unsigned long size() const;
			unsigned long paddedSize(unsigned long padTo = 0x20) const;

			bool populate(const lava::byteArray& bodyIn, std::size_t address);
			bool exportContents(std::ostream& destinationStream);
		};
		struct rwsdWaveSection
		{
			unsigned long address = ULONG_MAX;

			unsigned long originalLength = ULONG_MAX;
			
			std::vector<waveInfo> entries{};

			void waveEntryPushBack(const waveInfo& sourceWave);
			void waveEntryPushFront(const waveInfo& sourceWave);

			unsigned long size() const;
			std::vector<unsigned long> calculateOffsetVector() const;
			unsigned long paddedSize(unsigned long padTo = 0x20) const;
			bool populate(const lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
		};

		/*struct rwsdHeader
		{
			unsigned long address = ULONG_MAX;

			unsigned short endianType = USHRT_MAX;
			unsigned short version = USHRT_MAX;
			unsigned long headerLength = ULONG_MAX;
			unsigned short entriesOffset = USHRT_MAX;
			unsigned short entriesCount = USHRT_MAX;

			unsigned long dataOffset = ULONG_MAX;
			unsigned long dataLength = ULONG_MAX;
			unsigned long waveOffset = ULONG_MAX;
			unsigned long waveLength = ULONG_MAX;

			bool populate(const lava::byteArray& bodyIn, std::size_t addressIn);
			bool exportContents(std::ostream& destinationStream);
			static constexpr unsigned long size();
		};*/

		struct rwsd
		{
			unsigned long address = ULONG_MAX;

			unsigned short endianType;
			unsigned short versionNumber;
			rwsdDataSection dataSection;
			rwsdWaveSection waveSection;

		private:
			unsigned long dataSectionCachedSize = ULONG_MAX;
			unsigned long waveSectionCachedSize = ULONG_MAX;
		public:

			// Size + Offset Functions

			unsigned long size();
			void signalDATASectionSizeChange();
			void signalWAVESectionSizeChange();
			unsigned long getDATASectionSize();
			unsigned long getWAVESectionSize();
			unsigned long getDATASectionOffset();
			unsigned long getWAVESectionOffset();

			// Populate Funcs

			bool populateWavePacket(const lava::byteArray& bodyIn, unsigned long waveIndex, unsigned long specificDataAddressIn, unsigned long specificDataMaxLengthIn);
			bool populateWavePackets(const lava::byteArray& bodyIn, unsigned long waveDataAddressIn, unsigned long waveDataLengthIn);
			bool populate(const byteArray& fileBodyIn, unsigned long fileBodyAddressIn, const byteArray& rawDataIn, unsigned long rawDataAddressIn, unsigned long rawDataLengthIn);
			bool populate(const brsarFileFileContents& fileContentsIn);
			bool populate(std::string filePathIn);

			// Export Funcs

			bool exportFileSection(std::ostream& destinationStream);
			bool exportRawDataSection(std::ostream& destinationStream);
			std::vector<unsigned char> fileSectionToVec();
			std::vector<unsigned char> rawDataSectionToVec();
			dsp exportWaveRawDataToDSP(unsigned long waveSectionIndex);
			bool exportWaveRawDataToWAV(unsigned long waveSectionIndex, std::string wavOutputPath);

			// Utility + Maintenance Funcs

			bool updateWaveEntryDataLocations();
			waveInfo* getWaveInfoAssociatedWithDataInfo(unsigned long dataSectionIndex);
			bool summarize(std::ostream& output);
			bool summarize(std::string filepath);


			// Edit Funcs
			bool overwriteDataWSDInfo(unsigned long dataSectionIndex, const dataInfo& dataInfoIn);
			bool overwriteDataTrackInfo(unsigned long dataSectionIndex, const dataInfo& dataInfoIn);
			bool overwriteDataNoteInfo(unsigned long dataSectionIndex, const dataInfo& dataInfoIn);
			bool overwriteWave(unsigned long waveSectionIndex, const waveInfo& waveInfoIn);
			bool overwriteAllWaves(const waveInfo waveInfoIn);
			bool overwriteWaveRawData(unsigned long waveSectionIndex, const std::vector<unsigned char>& rawDataIn);
			bool overwriteWaveRawDataWithDSP(unsigned long waveSectionIndex, const dsp& dspIn);
			bool overwriteWaveRawDataWithDSP(unsigned long waveSectionIndex, std::string dspPathIn);
			bool overwriteWaveRawDataWithWAV(unsigned long waveSectionIndex, std::string wavPathIn);

			bool createNewWaveEntry(const waveInfo& sourceWave, bool pushFront = 0);
			bool createNewWaveEntries(const waveInfo sourceWave, unsigned char cloneCount, bool pushFront = 0);
			bool grantDataEntryUniqueWave(unsigned long dataSectionIndex, const waveInfo& sourceWave, bool pushFront = 0);

			bool hollowOutWAVEEntry(unsigned long waveID);
			bool cutDownWaveSection(unsigned long remainingWaveCount = 1, bool zeroOutWaveContent = 1);
		};

		/* BRSAR File Section */



		/* BRSAR */

		struct brsar
		{
			unsigned short byteOrderMarker = USHRT_MAX;
			unsigned short version = USHRT_MAX;
			unsigned short headerLength = USHRT_MAX;
			unsigned short sectionCount = USHRT_MAX;

			brsarSymbSection symbSection;
			brsarInfoSection infoSection;

		private:
			unsigned long symbSectionCachedSize = ULONG_MAX;
			unsigned long infoSectionCachedSize = ULONG_MAX;
			unsigned long virtualFileSectionCachedSize = ULONG_MAX;
		public:

			unsigned long size();
			bool init(std::string filePathIn);
			bool exportVirtualFileSection(std::ostream& destinationStream);
			bool exportContents(std::ostream& destinationStream);
			bool exportContents(std::string outputFilename);

			void signalSYMBSectionSizeChange();
			void signalINFOSectionSizeChange();
			void signalVirtualFILESectionSizeChange();

			unsigned long getSYMBSectionSize();
			unsigned long getINFOSectionSize();
			unsigned long getVirtualFILESectionSize();

			unsigned long getSYMBSectionAddress();
			unsigned long getINFOSectionAddress();
			unsigned long getVirtualFILESectionAddress();

			std::string getSymbString(unsigned long indexIn);
			unsigned long getGroupOffset(unsigned long groupIDIn);

			bool overwriteFile(const std::vector<unsigned char>& headerIn, const std::vector<unsigned char>& dataIn, unsigned long fileIDIn);
			bool cloneFile(unsigned long fileIDToClone, unsigned long groupToLink);

			bool summarizeSymbStringData(std::ostream& output = std::cout);
			bool outputConsecutiveSoundEntryStringsWithSameFileID(unsigned long startingIndex, std::ostream& output = std::cout);
			bool doFileDump(std::string dumpRootFolder, bool joinHeaderAndData = 0, bool doSummaryOnly = 0);
		};

		/* BRSAR */

	}
}

#endif