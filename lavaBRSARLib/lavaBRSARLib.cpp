#include "lavaBRSARLib.h"

namespace lava
{
	namespace brawl
	{
		std::string generateVGAudioWavToDSPCommand(std::string wavFilePath, std::string outputFilePath)
		{
			return "\"\"" + VGAudioMainExePath + "\" -c -i:0 \"" + wavFilePath + "\" -o \"" + outputFilePath + "\"\"";
		}
		std::string generateVGAudioDSPToWavCommand(std::string dspFilePath, std::string outputFilePath)
		{
			return "\"\"" + VGAudioMainExePath + "\" -c -i \"" + dspFilePath + "\" -o \"" + outputFilePath + "\"\"";
		}

		/* Misc. */

		unsigned long validateHexTag(unsigned long tagIn)
		{
			unsigned long result = 0;

			switch (tagIn)
			{
				case brsarHexTags::bht_RSAR:
				case brsarHexTags::bht_SYMB:
				case brsarHexTags::bht_INFO:
				case brsarHexTags::bht_FILE:
				{
					result = brsarHexTagType::bhtt_RSAR_SECTION;
					break;
				}
				case brsarHexTags::bht_RWSD:
				case brsarHexTags::bht_RBNK:
				case brsarHexTags::bht_RSEQ:
				case brsarHexTags::bht_RWAR:
				{
					result = brsarHexTagType::bhtt_FILE_SECTION;
					break;
				}
			}

			return result;
		}
		bool detectHexTags(const byteArray& bodyIn, unsigned long startingAddress)
		{
			bool result = 0;

			if (startingAddress < bodyIn.size())
			{
				std::vector<std::size_t> findResults = bodyIn.searchMultipleChar('R', startingAddress);
				if (!findResults.empty())
				{
					for (std::size_t i = 0; !result && i < findResults.size(); i++)
					{
						unsigned long harvestedLong = bodyIn.getLong(findResults[i]);
						result = validateHexTag(harvestedLong);
					}
				}
			}

			return result;
		}
		bool adjustOffset(unsigned long relativeBaseOffset, unsigned long& offsetIn, signed long adjustmentAmount, unsigned long startingAddress)
		{
			bool result = 0;

			if (startingAddress < relativeBaseOffset + offsetIn)
			{
				offsetIn += adjustmentAmount;
				result = 1;
			}

			return result;
		}
		int padLengthTo(unsigned long lengthIn, unsigned long padTo)
		{
			unsigned long padLength = padTo - (lengthIn % 0x10);
			return lengthIn + padLength;
		}

		/* Misc. */



		/* Brawl Reference */

		constexpr unsigned long brawlReference::size()
		{
			return sizeof(addressType) + sizeof(address);
		}
		brawlReference::brawlReference(unsigned long long valueIn)
		{
			addressType = valueIn >> 0x20;
			address = valueIn & 0x00000000FFFFFFFF;
		}
		bool brawlReference::isOffset() const
		{
			return addressType & 0x01000000;
		}
		unsigned long brawlReference::getAddress(unsigned long relativeToIn) const
		{
			unsigned long result = address;
			if (isOffset())
			{
				if (relativeToIn != ULONG_MAX)
				{
					result += relativeToIn;
				}
			}
			return result;
		}
		std::string brawlReference::getAddressString(unsigned long relativeToIn) const
		{
			return numToHexStringWithPadding(getAddress(relativeToIn), 8);
		}
		unsigned long long brawlReference::getHex() const
		{
			unsigned long long result = address | (unsigned long long(addressType) << 0x20);
			return result;
		}

		unsigned long brawlReferenceVector::size() const
		{
			return sizeof(unsigned long) + (refs.size() * brawlReference::size());
		}
		bool brawlReferenceVector::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated() && (addressIn < bodyIn.size()))
			{
				std::size_t count = bodyIn.getLong(addressIn);
				refs.resize(count);
				unsigned long cursor = addressIn + 0x04;
				for (std::size_t i = 0; i < count; i++)
				{
					refs[i] = brawlReference(bodyIn.getLLong(cursor));
					cursor += 0x08;
				}
				result = 1;
			}

			return result;
		}
		std::vector<unsigned long> brawlReferenceVector::getHex() const
		{
			std::vector<unsigned long> result{};

			result.push_back(refs.size());
			for (std::size_t i = 0; i < refs.size(); i++)
			{
				unsigned long long temp = refs[i].getHex();
				result.push_back(temp >> 0x20);
				result.push_back(temp);
			}

			return result;
		}
		bool brawlReferenceVector::exportContents(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				std::vector<unsigned long> hexBufferVector = getHex();
				for (std::size_t i = 0; i < hexBufferVector.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, hexBufferVector[i]);
				}
				result = destinationStream.good();
			}

			return result;
		}

		unsigned long calcRefVecSize(unsigned long entryCount)
		{
			unsigned long result = 0;

			result += sizeof(unsigned long); // Count Value
			result += sizeof(unsigned long long) * entryCount;

			return result;
		}

		/* Brawl Reference */



		/*Sound Data Structs*/

		bool wavePacket::populate(const lava::byteArray& bodyIn, unsigned long addressIn, unsigned long dataLengthIn, unsigned long paddingLengthIn)
		{
			bool result = 0;

			if (bodyIn.populated() && (addressIn + dataLengthIn + paddingLengthIn) <= bodyIn.size())
			{
				address = addressIn;

				result = 1;
				if (dataLengthIn != 0)
				{
					body = bodyIn.getBytes(dataLengthIn, addressIn);
					result &= body.size() == dataLengthIn;
				}
				if (paddingLengthIn != 0 && result)
				{
					padding = bodyIn.getBytes(paddingLengthIn, addressIn + dataLengthIn);
					result &= padding.size() == paddingLengthIn;
				}

				populated = result;
				if (!result)
				{
					address = ULONG_MAX;
					body.clear();
				}
			}

			return result;
		}

		unsigned long waveInfo::size() const
		{
			unsigned long result = 0;

			result += sizeof(encoding);
			result += sizeof(looped);
			result += sizeof(channels);
			result += sizeof(sampleRate24);
			result += sizeof(sampleRate);
			result += sizeof(dataLocationType);
			result += sizeof(pad);
			result += sizeof(loopStartSample);
			result += sizeof(nibbles);
			result += sizeof(channelInfoTableOffset);
			result += sizeof(dataLocation);
			result += sizeof(reserved);
			result += channelInfoTable.size() * sizeof(unsigned long);
			result += channelInfoEntries.size() * channelInfo::size();
			result += adpcmInfoEntries.size() * adpcmInfo::size();

			return result;
		}
		bool waveInfo::populate(const lava::byteArray& bodyIn, unsigned long addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				std::size_t cursor = address;
				encoding = bodyIn.getChar(cursor, &cursor);
				looped = bodyIn.getChar(cursor, &cursor);
				channels = bodyIn.getChar(cursor, &cursor);
				sampleRate24 = bodyIn.getChar(cursor, &cursor);
				sampleRate = bodyIn.getShort(cursor, &cursor);
				dataLocationType = bodyIn.getChar(cursor, &cursor);
				pad = bodyIn.getChar(cursor, &cursor);
				loopStartSample = bodyIn.getLong(cursor, &cursor);
				nibbles = bodyIn.getLong(cursor, &cursor);
				channelInfoTableOffset = bodyIn.getLong(cursor, &cursor);
				dataLocation = bodyIn.getLong(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);

				for (unsigned long i = 0; i < channels; i++)
				{
					channelInfoTable.push_back(bodyIn.getLong(cursor, &cursor));
					channelInfoEntries.push_back(channelInfo());
					unsigned long infoAddress = address + channelInfoTable.back();
					channelInfoEntries.back().populate(bodyIn, infoAddress);
					if (encoding == 2)
					{
						adpcmInfoEntries.push_back(adpcmInfo());
						adpcmInfoEntries.back().populate(bodyIn, infoAddress + 0x1C);
					}
				}
				result = 1;
			}

			return result;
		}
		bool waveInfo::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, encoding);
				lava::writeRawDataToStream(destinationStream, looped);
				lava::writeRawDataToStream(destinationStream, channels);
				lava::writeRawDataToStream(destinationStream, sampleRate24);
				lava::writeRawDataToStream(destinationStream, sampleRate);
				lava::writeRawDataToStream(destinationStream, dataLocationType);
				lava::writeRawDataToStream(destinationStream, pad);
				lava::writeRawDataToStream(destinationStream, loopStartSample);
				lava::writeRawDataToStream(destinationStream, nibbles);
				lava::writeRawDataToStream(destinationStream, channelInfoTableOffset);
				lava::writeRawDataToStream(destinationStream, dataLocation);
				lava::writeRawDataToStream(destinationStream, reserved);

				for (unsigned long i = 0x0; i < channelInfoTable.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, channelInfoTable[i]);
				}
				for (unsigned long i = 0x0; i < channelInfoEntries.size(); i++)
				{
					channelInfoEntries[i].exportContents(destinationStream);
					if (encoding == 2)
					{
						adpcmInfoEntries[i].exportContents(destinationStream);
					}
				}

				result = destinationStream.good();
			}
			return result;
		}
		unsigned long waveInfo::getAudioLengthInBytes() const
		{
			unsigned long result = ULONG_MAX;

			if (address != ULONG_MAX)
			{
				result = lava::brawl::nibblesToBytes(nibbles);
			}

			return result;
		}
		void waveInfo::copyOverWaveInfoProperties(const waveInfo& sourceInfo)
		{
			address = brsarAddressConsts::bac_NOT_IN_FILE;

			encoding = sourceInfo.encoding;
			looped = sourceInfo.looped;
			channels = sourceInfo.channels;
			sampleRate24 = sourceInfo.sampleRate24;
			sampleRate = sourceInfo.sampleRate;
			pad = sourceInfo.pad;
			loopStartSample = sourceInfo.loopStartSample;
			nibbles = sourceInfo.nibbles;
			channelInfoTableOffset = sourceInfo.channelInfoTableOffset;
			reserved = sourceInfo.reserved;
			channelInfoTable = sourceInfo.channelInfoTable;
			channelInfoEntries = sourceInfo.channelInfoEntries;
			adpcmInfoEntries = sourceInfo.adpcmInfoEntries;
		}

		unsigned long dataInfo::size() const
		{
			unsigned long result = 0;

			result += wsdInfo.size();
			result += trackTable.size();
			result += noteTable.size();
			result += sizeof(wsdPitch);
			result += sizeof(wsdPan);
			result += sizeof(wsdSurroundPan);
			result += sizeof(wsdFxSendA);
			result += sizeof(wsdFxSendB);
			result += sizeof(wsdFxSendC);
			result += sizeof(wsdMainSend);
			result += sizeof(wsdPad1);
			result += sizeof(wsdPad2);
			result += wsdGraphEnvTableRef.size();
			result += wsdRandomizerTableRef.size();
			result += sizeof(wsdPadding);

			result += ttReferenceList1.size();
			result += ttIntermediateReference.size();
			result += ttReferenceList2.size();
			result += sizeof(ttPosition);
			result += sizeof(ttLength);
			result += sizeof(ttNoteIndex);
			result += sizeof(ttReserved);

			result += ntReferenceList.size();
			result += sizeof(ntWaveIndex);
			result += sizeof(ntAttack);
			result += sizeof(ntDecay);
			result += sizeof(ntSustain);
			result += sizeof(ntRelease);
			result += sizeof(ntHold);
			result += sizeof(ntPad1);
			result += sizeof(ntPad2);
			result += sizeof(ntPad3);
			result += sizeof(ntOriginalKey);
			result += sizeof(ntVolume);
			result += sizeof(ntPan);
			result += sizeof(ntSurroundPan);
			result += sizeof(ntPitch);
			result += ntIfoTableRef.size();
			result += ntGraphEnvTableRef.size();
			result += ntRandomizerTableRef.size();
			result += sizeof(ntReserved);

			return result;
		}
		bool dataInfo::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;
				wsdInfo = brawlReference(bodyIn.getLLong(address));
				trackTable = brawlReference(bodyIn.getLLong(address + 0x08));
				noteTable = brawlReference(bodyIn.getLLong(address + 0x10));

				wsdPitch = bodyIn.getFloat(address + 0x18);
				wsdPan = bodyIn.getChar(address + 0x1C);
				wsdSurroundPan = bodyIn.getChar(address + 0x1D);
				wsdFxSendA = bodyIn.getChar(address + 0x1E);
				wsdFxSendB = bodyIn.getChar(address + 0x1F);
				wsdFxSendC = bodyIn.getChar(address + 0x20);
				wsdMainSend = bodyIn.getChar(address + 0x21);
				wsdPad1 = bodyIn.getChar(address + 0x22);
				wsdPad2 = bodyIn.getChar(address + 0x23);
				wsdGraphEnvTableRef = brawlReference(bodyIn.getLLong(address + 0x24));
				wsdRandomizerTableRef = brawlReference(bodyIn.getLLong(address + 0x2C));
				wsdPadding = bodyIn.getLong(address + 0x34);

				ttReferenceList1.populate(bodyIn, address + 0x38);
				ttIntermediateReference = brawlReference(bodyIn.getLLong(address + 0x44));
				ttReferenceList2.populate(bodyIn, address + 0x4C);
				ttPosition = bodyIn.getFloat(address + 0x58);
				ttLength = bodyIn.getFloat(address + 0x5C);
				ttNoteIndex = bodyIn.getLong(address + 0x60);
				ttReserved = bodyIn.getLong(address + 0x64);

				ntReferenceList.populate(bodyIn, address + 0x68);
				ntWaveIndex = bodyIn.getLong(address + 0x74);
				ntAttack = bodyIn.getChar(address + 0x78);
				ntDecay = bodyIn.getChar(address + 0x79);
				ntSustain = bodyIn.getChar(address + 0x7A);
				ntRelease = bodyIn.getChar(address + 0x7B);
				ntHold = bodyIn.getChar(address + 0x7C);
				ntPad1 = bodyIn.getChar(address + 0x7D);
				ntPad2 = bodyIn.getChar(address + 0x7E);
				ntPad3 = bodyIn.getChar(address + 0x7F);
				ntOriginalKey = bodyIn.getChar(address + 0x80);
				ntVolume = bodyIn.getChar(address + 0x81);
				ntPan = bodyIn.getChar(address + 0x82);
				ntSurroundPan = bodyIn.getChar(address + 0x83);
				ntPitch = bodyIn.getFloat(address + 0x84);
				ntIfoTableRef = brawlReference(bodyIn.getLLong(address + 0x88));
				ntGraphEnvTableRef = brawlReference(bodyIn.getLLong(address + 0x90));
				ntRandomizerTableRef = brawlReference(bodyIn.getLLong(address + 0x98));
				ntReserved = bodyIn.getLong(address + 0xA0);

				result = 1;
			}

			return result;
		}
		bool dataInfo::exportContents(std::ostream& destinationStream) const
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, wsdInfo.getHex());
				lava::writeRawDataToStream(destinationStream, trackTable.getHex());
				lava::writeRawDataToStream(destinationStream, noteTable.getHex());
				lava::writeRawDataToStream(destinationStream, wsdPitch);
				lava::writeRawDataToStream(destinationStream, wsdPan);
				lava::writeRawDataToStream(destinationStream, wsdSurroundPan);
				lava::writeRawDataToStream(destinationStream, wsdFxSendA);
				lava::writeRawDataToStream(destinationStream, wsdFxSendB);
				lava::writeRawDataToStream(destinationStream, wsdFxSendC);
				lava::writeRawDataToStream(destinationStream, wsdMainSend);
				lava::writeRawDataToStream(destinationStream, wsdPad1);
				lava::writeRawDataToStream(destinationStream, wsdPad2);
				lava::writeRawDataToStream(destinationStream, wsdGraphEnvTableRef.getHex());
				lava::writeRawDataToStream(destinationStream, wsdRandomizerTableRef.getHex());
				lava::writeRawDataToStream(destinationStream, wsdPadding);

				ttReferenceList1.exportContents(destinationStream);
				lava::writeRawDataToStream(destinationStream, ttIntermediateReference.getHex());
				ttReferenceList2.exportContents(destinationStream);
				lava::writeRawDataToStream(destinationStream, ttPosition);
				lava::writeRawDataToStream(destinationStream, ttLength);
				lava::writeRawDataToStream(destinationStream, ttNoteIndex);
				lava::writeRawDataToStream(destinationStream, ttReserved);

				ntReferenceList.exportContents(destinationStream);
				lava::writeRawDataToStream(destinationStream, ntWaveIndex);
				lava::writeRawDataToStream(destinationStream, ntAttack);
				lava::writeRawDataToStream(destinationStream, ntDecay);
				lava::writeRawDataToStream(destinationStream, ntSustain);
				lava::writeRawDataToStream(destinationStream, ntRelease);
				lava::writeRawDataToStream(destinationStream, ntHold);
				lava::writeRawDataToStream(destinationStream, ntPad1);
				lava::writeRawDataToStream(destinationStream, ntPad2);
				lava::writeRawDataToStream(destinationStream, ntPad3);
				lava::writeRawDataToStream(destinationStream, ntOriginalKey);
				lava::writeRawDataToStream(destinationStream, ntVolume);
				lava::writeRawDataToStream(destinationStream, ntPan);
				lava::writeRawDataToStream(destinationStream, ntSurroundPan);
				lava::writeRawDataToStream(destinationStream, ntPitch);
				lava::writeRawDataToStream(destinationStream, ntIfoTableRef.getHex());
				lava::writeRawDataToStream(destinationStream, ntGraphEnvTableRef.getHex());
				lava::writeRawDataToStream(destinationStream, ntRandomizerTableRef.getHex());
				lava::writeRawDataToStream(destinationStream, ntReserved);

				result = destinationStream.good();
			}
			return result;
		}
		void dataInfo::copyOverDataInfoProperties(const dataInfo& sourceInfo)
		{
			address = brsarAddressConsts::bac_NOT_IN_FILE;

			wsdPitch = sourceInfo.wsdPitch;
			wsdPan = sourceInfo.wsdPan;
			wsdSurroundPan = sourceInfo.wsdSurroundPan;
			wsdFxSendA = sourceInfo.wsdFxSendA;
			wsdFxSendB = sourceInfo.wsdFxSendB;
			wsdFxSendC = sourceInfo.wsdFxSendC;
			wsdMainSend = sourceInfo.wsdMainSend;
			wsdPad1 = sourceInfo.wsdPad1;
			wsdPad2 = sourceInfo.wsdPad2;

			ttPosition = sourceInfo.ttPosition;
			ttLength = sourceInfo.ttLength;
			ttNoteIndex = sourceInfo.ttNoteIndex;

			ntAttack = sourceInfo.ntAttack;
			ntDecay = sourceInfo.ntDecay;
			ntSustain = sourceInfo.ntSustain;
			ntRelease = sourceInfo.ntRelease;
			ntHold = sourceInfo.ntHold;
			ntPad1 = sourceInfo.ntPad1;
			ntPad2 = sourceInfo.ntPad2;
			ntPad3 = sourceInfo.ntPad3;
			ntOriginalKey = sourceInfo.ntOriginalKey;
			ntVolume = sourceInfo.ntVolume;
			ntPan = sourceInfo.ntPan;
			ntSurroundPan = sourceInfo.ntSurroundPan;
			ntPitch = sourceInfo.ntPitch;
		}

		/*Sound Data Structs*/



		/* BRSAR Symb Section */

		constexpr unsigned long brsarSymbPTrieNode::size()
		{
			return 0x14;
		}
		bool brsarSymbPTrieNode::populate(lava::byteArray& bodyIn, unsigned long addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				isLeaf = bodyIn.getShort(addressIn);
				posAndBit = bodyIn.getShort(addressIn + 0x02);
				leftID = bodyIn.getLong(address + 0x04);
				rightID = bodyIn.getLong(address + 0x08);
				stringID = bodyIn.getLong(address + 0x0C);
				infoID = bodyIn.getLong(address + 0x10);

				result = 1;
			}

			return result;
		}
		bool brsarSymbPTrieNode::exportContents(std::ostream& destinationStream) const
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, isLeaf);
				lava::writeRawDataToStream(destinationStream, posAndBit);
				lava::writeRawDataToStream(destinationStream, leftID);
				lava::writeRawDataToStream(destinationStream, rightID);
				lava::writeRawDataToStream(destinationStream, stringID);
				lava::writeRawDataToStream(destinationStream, infoID);

				result = destinationStream.good();
			}
			return result;
		}
		unsigned long brsarSymbPTrieNode::getBit() const
		{
			return posAndBit & 0b00000111;
		}
		unsigned long brsarSymbPTrieNode::getPos() const
		{
			return posAndBit >> 3;
		}
		bool brsarSymbPTrieNode::compareCharAndBit(char charIn) const
		{
			unsigned char comparisonTerm = 1 << (7 - getBit());
			return charIn & comparisonTerm;
		}

		unsigned long brsarSymbPTrie::size() const
		{
			return 0x08 + (entries.size() * brsarSymbPTrieNode::size());
		}
		bool brsarSymbPTrie::populate(lava::byteArray& bodyIn, unsigned long addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				rootID = bodyIn.getLong(address);
				numEntries = bodyIn.getLong(address + 0x04);

				unsigned long cursor = address + 0x08;
				unsigned long cursorMax = cursor + (0x14 * numEntries);

				std::size_t i = 0;
				entries.resize(numEntries);

				while (cursor < cursorMax)
				{
					entries[i].populate(bodyIn, cursor);
					cursor += 0x14;
					i++;
				}

				result = 1;
			}

			return result;
		}
		bool brsarSymbPTrie::exportContents(std::ostream& destinationStream) const
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, rootID);
				lava::writeRawDataToStream(destinationStream, numEntries);

				for (std::size_t i = 0; i < numEntries; i++)
				{
					entries[i].exportContents(destinationStream);
				}

				result = destinationStream.good();
			}
			return result;
		}
		brsarSymbPTrieNode brsarSymbPTrie::findString(std::string stringIn) const
		{
			if (rootID < numEntries)
			{
				const brsarSymbPTrieNode* currentNode = &entries[rootID];

				while (!currentNode->isLeaf)
				{
					int pos = currentNode->getPos();
					int bit = currentNode->getBit();
					if (pos < stringIn.size() && currentNode->compareCharAndBit(stringIn[pos]))
					{
						currentNode = &entries[currentNode->rightID];
					}
					else
					{
						currentNode = &entries[currentNode->leftID];
					}
				}

				return *currentNode;
			}
			return brsarSymbPTrieNode();
		}

		unsigned long brsarSymbSection::size() const
		{
			unsigned long result = 0;

			result += 0x04; // SYMB Tag
			result += sizeof(unsigned long); // Length
			result += sizeof(stringListOffset);
			result += sizeof(soundTrieOffset);
			result += sizeof(playerTrieOffset);
			result += sizeof(groupTrieOffset);
			result += sizeof(bankTrieOffset);
			result += sizeof(unsigned long); // String Entry Count
			result += stringEntryOffsets.size() * sizeof(unsigned long); // Size of OffsetVec
			result += soundTrie.size() + playerTrie.size() + groupTrie.size() + bankTrie.size(); // Collective Size of Tries
			result += stringBlock.size() * sizeof(unsigned char); // Size of String Blocks

			return result;
		}
		unsigned long brsarSymbSection::paddedSize(unsigned long padTo) const
		{
			return padLengthTo(size(), padTo);
		}
		unsigned long brsarSymbSection::getAddress() const
		{
			return (parent != nullptr) ? parent->getSYMBSectionAddress() : ULONG_MAX;
		}
		bool brsarSymbSection::populate(brsar& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated() && bodyIn.getLong(addressIn) == brsarHexTags::bht_SYMB)
			{
				result = 1;
				address = addressIn;
				parent = &parentIn;

				std::size_t cursor1 = address + 0x04;

				bodyIn.getLong(cursor1, &cursor1); // Move Past Length Entry
				stringListOffset = bodyIn.getLong(cursor1, &cursor1);

				soundTrieOffset = bodyIn.getLong(cursor1, &cursor1);
				playerTrieOffset = bodyIn.getLong(cursor1, &cursor1);
				groupTrieOffset = bodyIn.getLong(cursor1, &cursor1);
				bankTrieOffset = bodyIn.getLong(cursor1, &cursor1);

				result &= soundTrie.populate(bodyIn, address + 0x08 + soundTrieOffset);
				result &= playerTrie.populate(bodyIn, address + 0x08 + playerTrieOffset);
				result &= groupTrie.populate(bodyIn, address + 0x08 + groupTrieOffset);
				result &= bankTrie.populate(bodyIn, address + 0x08 + bankTrieOffset);

				std::size_t cursor = address + 0x08 + stringListOffset;
				stringEntryOffsets.resize(bodyIn.getLong(cursor), ULONG_MAX);
				cursor += 0x04;
				for (std::size_t i = 0; i < stringEntryOffsets.size(); i++)
				{
					stringEntryOffsets[i] = bodyIn.getLong(cursor);
					cursor += 0x04;
				}
				if (stringEntryOffsets.size())
				{
					unsigned long stringBlockStartAddr = address + 0x08 + stringEntryOffsets.front();
					unsigned long stringBlockEndAddr = address + 0x08 + soundTrieOffset;
					if (stringBlockStartAddr < stringBlockEndAddr)
					{
						stringBlock = bodyIn.getBytes(stringBlockEndAddr - stringBlockStartAddr, stringBlockStartAddr);
					}
				}

				parent->signalSYMBSectionSizeChange();
			}

			return result;
		}
		bool brsarSymbSection::exportContents(std::ostream& destinationStream) const
		{
			bool result = 0;
			if (destinationStream.good())
			{
				result = 1;
				unsigned long initialStreamPos = destinationStream.tellp();

				lava::writeRawDataToStream(destinationStream, brsarHexTags::bht_SYMB);
				lava::writeRawDataToStream(destinationStream, paddedSize());
				lava::writeRawDataToStream(destinationStream, stringListOffset);
				lava::writeRawDataToStream(destinationStream, soundTrieOffset);
				lava::writeRawDataToStream(destinationStream, playerTrieOffset);
				lava::writeRawDataToStream(destinationStream, groupTrieOffset);
				lava::writeRawDataToStream(destinationStream, bankTrieOffset);

				/*for (std::size_t i = 0; i < trieOffsets.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, trieOffsets[i]);
				}*/

				lava::writeRawDataToStream(destinationStream, stringEntryOffsets.size());
				for (std::size_t i = 0; i < stringEntryOffsets.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, stringEntryOffsets[i]);
				}
				destinationStream.write((char*)stringBlock.data(), stringBlock.size());

				soundTrie.exportContents(destinationStream);
				playerTrie.exportContents(destinationStream);
				groupTrie.exportContents(destinationStream);
				bankTrie.exportContents(destinationStream);

				/*for (std::size_t i = 0; i < tries.size(); i++)
				{
					result &= tries[i].exportContents(destinationStream);
				}*/

				unsigned long finalStreamPos = destinationStream.tellp();
				unsigned long amountWritten = finalStreamPos - initialStreamPos;
				unsigned long expectedLength = paddedSize();
				if (expectedLength > amountWritten)
				{
					std::vector<char> finalPadding(expectedLength - amountWritten, 0x00);
					destinationStream.write(finalPadding.data(), finalPadding.size());
				}

				result &= destinationStream.good();
			}
			return result;
		}
		std::string brsarSymbSection::getString(std::size_t idIn) const
		{
			std::string result = "";
			if (idIn < stringEntryOffsets.size())
			{
				unsigned long stringAddr = (stringEntryOffsets[idIn] - stringEntryOffsets.front());
				result = (char*)(stringBlock.data() + stringAddr);
			}
			return result;
		}
		bool brsarSymbSection::dumpTrieStrings(std::ostream& destinationStream, const brsarSymbPTrie& sourceTrie) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				for (std::size_t i = 0; i < sourceTrie.entries.size(); i++)
				{
					const brsarSymbPTrieNode* currNode = &sourceTrie.entries[i];
					if (currNode->isLeaf)
					{
						destinationStream << "[StrID: 0x" << numToHexStringWithPadding(currNode->stringID, 0x04) << ", InfoID: 0x" << numToHexStringWithPadding(currNode->infoID, 0x04) << "] " << getString(currNode->stringID) << "\n";
					}
				}
				result = destinationStream.good();
			}

			return result;
		}
		bool brsarSymbSection::dumpStrings(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				dumpTrieStrings(destinationStream, playerTrie);
				dumpTrieStrings(destinationStream, groupTrie);
				dumpTrieStrings(destinationStream, bankTrie);
				dumpTrieStrings(destinationStream, soundTrie);
				result = destinationStream.good();
			}

			return result;
		}

		/* BRSAR Symb Section */



		/* BRSAR Info Section */

		constexpr unsigned long brsarInfo3DSoundInfo::size()
		{
			unsigned long result = 0;

			result += sizeof(flags);
			result += sizeof(decayCurve);
			result += sizeof(decayRatio);
			result += sizeof(dopplerFactor);
			result += sizeof(padding);
			result += sizeof(reserved);

			return result;
		}
		unsigned long brsarInfo3DSoundInfo::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfo3DSoundInfo::populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				flags = bodyIn.getLong(cursor, &cursor);
				decayCurve = bodyIn.getChar(cursor, &cursor);
				decayRatio = bodyIn.getChar(cursor, &cursor);
				dopplerFactor = bodyIn.getChar(cursor, &cursor);
				padding = bodyIn.getChar(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);
			}

			return result;
		}
		bool brsarInfo3DSoundInfo::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, flags);
				lava::writeRawDataToStream(destinationStream, decayCurve);
				lava::writeRawDataToStream(destinationStream, decayRatio);
				lava::writeRawDataToStream(destinationStream, dopplerFactor);
				lava::writeRawDataToStream(destinationStream, padding);
				lava::writeRawDataToStream(destinationStream, reserved);

				result = destinationStream.good();
			}
			return result;
		}

		constexpr unsigned long brsarInfoSequenceSoundInfo::size()
		{
			unsigned long result = 0;

			result += sizeof(dataID);
			result += sizeof(bankID);
			result += sizeof(allocTrack);
			result += sizeof(channelPriority);
			result += sizeof(releasePriorityFix);
			result += sizeof(pad1);
			result += sizeof(pad2);
			result += sizeof(reserved);

			return result;
		}
		unsigned long brsarInfoSequenceSoundInfo::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoSequenceSoundInfo::populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				dataID = bodyIn.getLong(cursor, &cursor);
				bankID = bodyIn.getLong(cursor, &cursor);
				allocTrack = bodyIn.getLong(cursor, &cursor);
				channelPriority = bodyIn.getChar(cursor, &cursor);
				releasePriorityFix = bodyIn.getChar(cursor, &cursor);
				pad1 = bodyIn.getChar(cursor, &cursor);
				pad2 = bodyIn.getChar(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);
			}

			return result;
		}
		bool brsarInfoSequenceSoundInfo::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, dataID);
				lava::writeRawDataToStream(destinationStream, bankID);
				lava::writeRawDataToStream(destinationStream, allocTrack);
				lava::writeRawDataToStream(destinationStream, channelPriority);
				lava::writeRawDataToStream(destinationStream, releasePriorityFix);
				lava::writeRawDataToStream(destinationStream, pad1);
				lava::writeRawDataToStream(destinationStream, pad2);
				lava::writeRawDataToStream(destinationStream, reserved);

				result = destinationStream.good();
			}
			return result;
		}

		constexpr unsigned long brsarInfoStreamSoundInfo::size()
		{
			unsigned long result = 0;

			result += sizeof(startPosition);
			result += sizeof(allocChannelCount);
			result += sizeof(allocTrackFlag);
			result += sizeof(reserved);

			return result;
		}
		unsigned long brsarInfoStreamSoundInfo::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoStreamSoundInfo::populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				startPosition = bodyIn.getLong(cursor, &cursor);
				allocChannelCount = bodyIn.getShort(cursor, &cursor);
				allocTrackFlag = bodyIn.getShort(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);
			}

			return result;
		}
		bool brsarInfoStreamSoundInfo::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, startPosition);
				lava::writeRawDataToStream(destinationStream, allocChannelCount);
				lava::writeRawDataToStream(destinationStream, allocTrackFlag);
				lava::writeRawDataToStream(destinationStream, reserved);

				result = destinationStream.good();
			}
			return result;
		}

		constexpr unsigned long brsarInfoWaveSoundInfo::size()
		{
			unsigned long result = 0;

			result += sizeof(soundIndex);
			result += sizeof(allocTrack);
			result += sizeof(channelPriority);
			result += sizeof(releasePriorityFix);
			result += sizeof(pad1);
			result += sizeof(pad2);
			result += sizeof(reserved);

			return result;
		}
		unsigned long brsarInfoWaveSoundInfo::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoWaveSoundInfo::populate(const brsarInfoSoundEntry& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				soundIndex = bodyIn.getLong(cursor, &cursor);
				allocTrack = bodyIn.getLong(cursor, &cursor);
				channelPriority = bodyIn.getChar(cursor, &cursor);
				releasePriorityFix = bodyIn.getChar(cursor, &cursor);
				pad1 = bodyIn.getChar(cursor, &cursor);
				pad2 = bodyIn.getChar(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);
			}

			return result;
		}
		bool brsarInfoWaveSoundInfo::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, soundIndex);
				lava::writeRawDataToStream(destinationStream, allocTrack);
				lava::writeRawDataToStream(destinationStream, channelPriority);
				lava::writeRawDataToStream(destinationStream, releasePriorityFix);
				lava::writeRawDataToStream(destinationStream, pad1);
				lava::writeRawDataToStream(destinationStream, pad2);
				lava::writeRawDataToStream(destinationStream, reserved);

				result = destinationStream.good();
			}
			return result;
		}

		unsigned long brsarInfoSoundEntry::size() const
		{
			unsigned long result = 0;

			result += sizeof(stringID);
			result += sizeof(fileID);
			result += sizeof(playerID);
			result += param3DRefOffset.size();
			result += sizeof(volume);
			result += sizeof(playerPriority);
			result += sizeof(soundType);
			result += sizeof(remoteFilter);
			result += soundInfoRef.size();
			result += sizeof(userParam1);
			result += sizeof(userParam2);
			result += sizeof(panMode);
			result += sizeof(panCurve);
			result += sizeof(actorPlayerID);
			result += sizeof(reserved);
			switch (soundType)
			{
				case sit_SEQUENCE:
				{
					result += seqSoundInfo.size();
					break;
				}
				case sit_STREAM:
				{
					result += streamSoundInfo.size();
					break;
				}
				case sit_WAVE:
				{
					result += waveSoundInfo.size();
					break;
				}
				default:
				{
					break;
				}
			}
			result += sound3DInfo.size();

			return result;
		}
		unsigned long brsarInfoSoundEntry::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoSoundEntry::populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				stringID = bodyIn.getLong(cursor, &cursor);
				fileID = bodyIn.getLong(cursor, &cursor);
				playerID = bodyIn.getLong(cursor, &cursor);
				param3DRefOffset = brawlReference(bodyIn.getLLong(cursor, &cursor));
				volume = bodyIn.getChar(cursor, &cursor);
				playerPriority = bodyIn.getChar(cursor, &cursor);
				soundType = bodyIn.getChar(cursor, &cursor);
				remoteFilter = bodyIn.getChar(cursor, &cursor);
				soundInfoRef = brawlReference(bodyIn.getLLong(cursor, &cursor));
				userParam1 = bodyIn.getLong(cursor, &cursor);
				userParam2 = bodyIn.getLong(cursor, &cursor);
				panMode = bodyIn.getChar(cursor, &cursor);
				panCurve = bodyIn.getChar(cursor, &cursor);
				actorPlayerID = bodyIn.getChar(cursor, &cursor);
				reserved = bodyIn.getChar(cursor, &cursor);

				result &= sound3DInfo.populate(*this, bodyIn, param3DRefOffset.getAddress(parent->getAddress() + 0x08));
				switch (soundType)
				{
					case sit_SEQUENCE:
					{
						result &= seqSoundInfo.populate(*this, bodyIn, soundInfoRef.getAddress(parent->getAddress() + 0x08));
						break;
					}
					case sit_STREAM:
					{
						result &= streamSoundInfo.populate(*this, bodyIn, soundInfoRef.getAddress(parent->getAddress() + 0x08));
						break;
					}
					case sit_WAVE:
					{
						result &= waveSoundInfo.populate(*this, bodyIn, soundInfoRef.getAddress(parent->getAddress() + 0x08));
						break;
					}
					default:
					{
						break;
					}
				}
			}

			return result;
		}
		bool brsarInfoSoundEntry::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, stringID);
				lava::writeRawDataToStream(destinationStream, fileID);
				lava::writeRawDataToStream(destinationStream, playerID);
				lava::writeRawDataToStream(destinationStream, param3DRefOffset.getHex());
				lava::writeRawDataToStream(destinationStream, volume);
				lava::writeRawDataToStream(destinationStream, playerPriority);
				lava::writeRawDataToStream(destinationStream, soundType);
				lava::writeRawDataToStream(destinationStream, remoteFilter);
				lava::writeRawDataToStream(destinationStream, soundInfoRef.getHex());
				lava::writeRawDataToStream(destinationStream, userParam1);
				lava::writeRawDataToStream(destinationStream, userParam2);
				lava::writeRawDataToStream(destinationStream, panMode);
				lava::writeRawDataToStream(destinationStream, panCurve);
				lava::writeRawDataToStream(destinationStream, actorPlayerID);
				lava::writeRawDataToStream(destinationStream, reserved);
				switch (soundType)
				{
					case sit_SEQUENCE:
					{
						seqSoundInfo.exportContents(destinationStream);
						break;
					}
					case sit_STREAM:
					{
						streamSoundInfo.exportContents(destinationStream);
						break;
					}
					case sit_WAVE:
					{
						waveSoundInfo.exportContents(destinationStream);
						break;
					}
					default:
					{
						break;
					}
				}
				sound3DInfo.exportContents(destinationStream);
				result = destinationStream.good();
			}
			return result;
		}
		void brsarInfoSoundEntry::updateSound3DInfoOffsetValue()
		{
			unsigned long relativeOffset = 0;
			relativeOffset += sizeof(stringID);
			relativeOffset += sizeof(fileID);
			relativeOffset += sizeof(playerID);
			relativeOffset += param3DRefOffset.size();
			relativeOffset += sizeof(volume);
			relativeOffset += sizeof(playerPriority);
			relativeOffset += sizeof(soundType);
			relativeOffset += sizeof(remoteFilter);
			relativeOffset += soundInfoRef.size();
			relativeOffset += sizeof(userParam1);
			relativeOffset += sizeof(userParam2);
			relativeOffset += sizeof(panMode);
			relativeOffset += sizeof(panCurve);
			relativeOffset += sizeof(actorPlayerID);
			relativeOffset += sizeof(reserved);
			switch (soundType)
			{
				case sit_SEQUENCE:
				{
					relativeOffset += seqSoundInfo.size();
					break;
				}
				case sit_STREAM:
				{
					relativeOffset += streamSoundInfo.size();
					break;
				}
				case sit_WAVE:
				{
					relativeOffset += waveSoundInfo.size();
					break;
				}
				default:
				{
					break;
				}
			}
			sound3DInfo.parentRelativeOffset = relativeOffset;
		}
		void brsarInfoSoundEntry::updateSpecificSoundOffsetValue()
		{
			unsigned long relativeOffset = 0;
			relativeOffset += sizeof(stringID);
			relativeOffset += sizeof(fileID);
			relativeOffset += sizeof(playerID);
			relativeOffset += param3DRefOffset.size();
			relativeOffset += sizeof(volume);
			relativeOffset += sizeof(playerPriority);
			relativeOffset += sizeof(soundType);
			relativeOffset += sizeof(remoteFilter);
			relativeOffset += soundInfoRef.size();
			relativeOffset += sizeof(userParam1);
			relativeOffset += sizeof(userParam2);
			relativeOffset += sizeof(panMode);
			relativeOffset += sizeof(panCurve);
			relativeOffset += sizeof(actorPlayerID);
			relativeOffset += sizeof(reserved);
			switch (soundType)
			{
				case sit_SEQUENCE:
				{
					seqSoundInfo.parentRelativeOffset = relativeOffset;
					break;
				}
				case sit_STREAM:
				{
					streamSoundInfo.parentRelativeOffset = relativeOffset;
					break;
				}
				case sit_WAVE:
				{
					waveSoundInfo.parentRelativeOffset = relativeOffset;
					break;
				}
				default:
				{
					break;
				}
			}
		}

		constexpr unsigned long brsarInfoBankEntry::size()
		{
			unsigned long result = 0;

			result += sizeof(stringID);
			result += sizeof(fileID);
			result += sizeof(padding);

			return result;
		}
		unsigned long brsarInfoBankEntry::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoBankEntry::populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;
				
				std::size_t cursor = originalAddress;
				stringID = bodyIn.getLong(cursor, &cursor);
				fileID = bodyIn.getLong(cursor, &cursor);
				padding = bodyIn.getLong(cursor, &cursor);
			}

			return result;
		}
		bool brsarInfoBankEntry::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, stringID);
				lava::writeRawDataToStream(destinationStream, fileID);
				lava::writeRawDataToStream(destinationStream, padding);

				result = destinationStream.good();
			}
			return result;
		}

		constexpr unsigned long brsarInfoPlayerEntry::size()
		{
			unsigned long result = 0;

			result += sizeof(stringID);
			result += sizeof(playableSoundCount);
			result += sizeof(padding);
			result += sizeof(padding2);
			result += sizeof(heapSize);
			result += sizeof(reserved);

			return result;
		}
		unsigned long brsarInfoPlayerEntry::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoPlayerEntry::populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				stringID = bodyIn.getLong(cursor, &cursor);
				playableSoundCount = bodyIn.getChar(cursor, &cursor);
				padding = bodyIn.getChar(cursor, &cursor);
				padding2 = bodyIn.getShort(cursor, &cursor);
				heapSize = bodyIn.getLong(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);
			}

			return result;
		}
		bool brsarInfoPlayerEntry::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, stringID);
				lava::writeRawDataToStream(destinationStream, playableSoundCount);
				lava::writeRawDataToStream(destinationStream, padding);
				lava::writeRawDataToStream(destinationStream, padding2);
				lava::writeRawDataToStream(destinationStream, heapSize);
				lava::writeRawDataToStream(destinationStream, reserved);

				result = destinationStream.good();
			}
			return result;
		}

		constexpr unsigned long brsarInfoFileEntry::size()
		{
			unsigned long result = 0;

			result += sizeof(groupID);
			result += sizeof(index);

			return result;
		}
		unsigned long brsarInfoFileEntry::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoFileEntry::populate(const brsarInfoFileHeader& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				groupID = bodyIn.getLong(cursor, &cursor);
				index = bodyIn.getLong(cursor, &cursor);
			}

			return result;
		}
		bool brsarInfoFileEntry::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, groupID);
				lava::writeRawDataToStream(destinationStream, index);

				result = destinationStream.good();
			}
			return result;
		}

		unsigned long brsarInfoFileHeader::size() const
		{
			unsigned result = 0;

			result += sizeof(headerLength);
			result += sizeof(dataLength);
			result += sizeof(entryNumber);
			result += stringOffset.size();
			result += listOffset.size();
			result += stringContent.size() * sizeof(unsigned char);
			result += calcRefVecSize(entries.size()); // Size of the Entry List We'll Be Generating
			result += entries.size() * brsarInfoFileEntry::size();

			return result;
		}
		unsigned long brsarInfoFileHeader::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoFileHeader::populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				headerLength = bodyIn.getLong(cursor, &cursor);
				dataLength = bodyIn.getLong(cursor, &cursor);
				entryNumber = bodyIn.getLong(cursor, &cursor);
				stringOffset = brawlReference(bodyIn.getLLong(cursor, &cursor));
				listOffset = brawlReference(bodyIn.getLLong(cursor, &cursor));

				if (stringOffset.getAddress() != 0x00)
				{
					std::string measurementString = bodyIn.data() + (stringOffset.getAddress(parent->getAddress() + 0x08));
					std::size_t sizePrescription = measurementString.size() + (0x04 - (measurementString.size() % 0x04));
					stringContent = bodyIn.getBytes(sizePrescription, stringOffset.getAddress(parent->getAddress() + 0x08));
					result &= stringContent.size() == sizePrescription;
				}
				if (listOffset.getAddress() != 0x00)
				{
					lava::brawl::brawlReferenceVector entryReferenceList;
					result &= entryReferenceList.populate(bodyIn, listOffset.getAddress(parent->getAddress() + 0x08));
					for (std::size_t u = 0; u < entryReferenceList.refs.size(); u++)
					{
						entries.push_back(brsarInfoFileEntry());
						result &= entries.back().populate(*this, bodyIn, entryReferenceList.refs[u].getAddress(parent->getAddress() + 0x08));
					}
				}
			}

			return result;
		}
		bool brsarInfoFileHeader::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				result = 1;
				lava::writeRawDataToStream(destinationStream, headerLength);
				lava::writeRawDataToStream(destinationStream, dataLength);
				lava::writeRawDataToStream(destinationStream, entryNumber);
				lava::writeRawDataToStream(destinationStream, stringOffset.getHex());
				lava::writeRawDataToStream(destinationStream, listOffset.getHex());
				destinationStream.write((char*)stringContent.data(), stringContent.size());
				// Write BrawlRef Vec into stream
				result &= writeFileEntryRefVec(destinationStream);
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					result &= entries[i].exportContents(destinationStream);
				}
				result &= destinationStream.good();
			}
			return result;
		}
		bool brsarInfoFileHeader::writeFileEntryRefVec(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, unsigned long(entries.size()));
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, unsigned long(0x01000000));
					// Offset into section is gonna be:
					// The stuct's pRO + the child's pRO - 0x08 to uncount the INFO Block's Tag and Size Field
					lava::writeRawDataToStream(destinationStream, (parentRelativeOffset + entries[i].parentRelativeOffset) - 0x08);
				}
				result = destinationStream.good();
			}

			return result;
		}
		void brsarInfoFileHeader::updateFileEntryOffsetValues()
		{
			unsigned long relativeOffset = 0;
			relativeOffset += sizeof(headerLength);
			relativeOffset += sizeof(dataLength);
			relativeOffset += sizeof(entryNumber);

			// If there's actual string content, calculate the new offset address for it
			if (stringContent.size() > 0x00)
			{
				stringOffset.addressType = 0x01000000;
				// Update the offset stored in stringOffset
				// This needs to point to after listOffset (which comes right after this)
				// Offset of this File Header into the Info Section
				// + The offset of listOffset into this Group Header
				// + The size of this reference (so that we point to after it)
				// + The size of listReference (so we also point to after that)
				// - 0x08 (to "uncount" the INFO Section Tag and Size fields' length)
				stringOffset.address = (parentRelativeOffset + relativeOffset + stringOffset.size() + listOffset.size()) - 0x08;
			}
			// Otherwise, make sure the offset value is nulled out
			else
			{
				stringOffset.addressType = 0x00000000;
				stringOffset.address = 0x00000000;
			}
			relativeOffset += stringOffset.size();
			// Ensure that the address is recorded
			listOffset.addressType = 0x01000000;
			// Also update the offset stored in listOffset
			// Should point to after the string content (which comes right after this)
			// Offset of this File Header into the Info Section
			// + The offset of listOffset into this Group Header
			// + The size of this reference (so that we point to after it)
			// + The length of this File Header's string content
			// - 0x08 (to "uncount" the INFO Section Tag and Size fields' length)
			listOffset.address = (parentRelativeOffset + relativeOffset + listOffset.size() + (stringContent.size() * sizeof(unsigned char))) - 0x08;
			relativeOffset += listOffset.size();
			relativeOffset += stringContent.size() * sizeof(unsigned char);
			relativeOffset += calcRefVecSize(entries.size()); // Size of the Entry List We'll Be Generating
			for (std::size_t i = 0; i < entries.size(); i++)
			{
				entries[i].parentRelativeOffset = relativeOffset;
				relativeOffset += entries[i].size();
			}
		}

		constexpr unsigned long brsarInfoGroupEntry::size()
		{
			unsigned long result = 0;

			result += sizeof(fileID);
			result += sizeof(headerOffset);
			result += sizeof(headerLength);
			result += sizeof(dataOffset);
			result += sizeof(dataLength);
			result += sizeof(reserved);

			return result;
		}
		unsigned long brsarInfoGroupEntry::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoGroupEntry::populate(const brsarInfoGroupHeader& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;
				fileID = bodyIn.getLong(cursor, &cursor);
				headerOffset = bodyIn.getLong(cursor, &cursor);
				headerLength = bodyIn.getLong(cursor, &cursor);
				dataOffset = bodyIn.getLong(cursor, &cursor);
				dataLength = bodyIn.getLong(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);

				result = 1;
			}

			return result;
		}
		bool brsarInfoGroupEntry::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, fileID);
				lava::writeRawDataToStream(destinationStream, headerOffset);
				lava::writeRawDataToStream(destinationStream, headerLength);
				lava::writeRawDataToStream(destinationStream, dataOffset);
				lava::writeRawDataToStream(destinationStream, dataLength);
				lava::writeRawDataToStream(destinationStream, reserved);

				result = destinationStream.good();
			}
			return result;
		}

		unsigned long brsarInfoGroupHeader::size() const
		{
			unsigned long result = 0;

			result += sizeof(groupID);
			result += sizeof(entryNum);
			result += extFilePathRef.size();
			result += sizeof(headerAddress);
			result += sizeof(headerLength);
			result += sizeof(dataAddress);
			result += sizeof(dataLength);
			result += listOffset.size();
			result += calcRefVecSize(entries.size()); // Size of the Entry List We'll Be Generating
			result += entries.size() * brsarInfoGroupEntry::size();

			return result;
		}
		unsigned long brsarInfoGroupHeader::getAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeOffset : ULONG_MAX;
		}
		bool brsarInfoGroupHeader::populate(const brsarInfoSection& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				originalAddress = addressIn;
				parent = &parentIn;

				std::size_t cursor = originalAddress;

				groupID = bodyIn.getLong(cursor, &cursor);
				entryNum = bodyIn.getLong(cursor, &cursor);
				extFilePathRef = brawlReference(bodyIn.getLLong(cursor, &cursor));
				headerAddress = bodyIn.getLong(cursor, &cursor);
				headerLength = bodyIn.getLong(cursor, &cursor);
				dataAddress = bodyIn.getLong(cursor, &cursor);
				dataLength = bodyIn.getLong(cursor, &cursor);
				listOffset = brawlReference(bodyIn.getLLong(cursor, &cursor));
				lava::brawl::brawlReferenceVector entryReferenceList;
				result &= entryReferenceList.populate(bodyIn, listOffset.getAddress(parent->getAddress() + 0x08));
				for (std::size_t u = 0; u < entryReferenceList.refs.size(); u++)
				{
					entries.push_back(brsarInfoGroupEntry());
					result &= entries.back().populate(*this, bodyIn, entryReferenceList.refs[u].getAddress(parent->getAddress() + 0x08));
				}
			}

			return result;
		}
		bool brsarInfoGroupHeader::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				result = 1;
				lava::writeRawDataToStream(destinationStream, groupID);
				lava::writeRawDataToStream(destinationStream, entryNum);
				lava::writeRawDataToStream(destinationStream, extFilePathRef.getHex());
				lava::writeRawDataToStream(destinationStream, headerAddress);
				lava::writeRawDataToStream(destinationStream, headerLength);
				lava::writeRawDataToStream(destinationStream, dataAddress);
				lava::writeRawDataToStream(destinationStream, dataLength);
				lava::writeRawDataToStream(destinationStream, listOffset.getHex());
				// Write BrawlRef Vec into stream
				result &= writeGroupEntryRefVec(destinationStream);
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					result &= entries[i].exportContents(destinationStream);
				}
				result &= destinationStream.good();
			}
			return result;
		}

		bool brsarInfoGroupHeader::writeGroupEntryRefVec(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, unsigned long(entries.size()));
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, unsigned long(0x01000000));
					// Offset into section is gonna be:
					// The stuct's pRO + the child's pRO - 0x08 to uncount the INFO Block's Tag and Size Field
					lava::writeRawDataToStream(destinationStream, (parentRelativeOffset + entries[i].parentRelativeOffset) - 0x08);
				}
				result = destinationStream.good();
			}

			return result;
		}
		void brsarInfoGroupHeader::updateGroupEntryOffsetValues()
		{
			unsigned long relativeOffset = 0;
			relativeOffset += sizeof(groupID);
			relativeOffset += sizeof(entryNum);
			relativeOffset += extFilePathRef.size();
			relativeOffset += sizeof(headerAddress);
			relativeOffset += sizeof(headerLength);
			relativeOffset += sizeof(dataAddress);
			relativeOffset += sizeof(dataLength);
			listOffset.addressType = 0x01000000;
			// Update the offset stored in listOffset
			// This needs to point to right after itself, so it'll be:
			// Offset of this Group Header into the Info Section
			// + The offset of listOffset into this Group Header
			// + The size of this reference (so that we point to after it)
			// - 0x08 (to "uncount" the INFO Section Tag and Size fields' length)
			listOffset.address = (parentRelativeOffset + relativeOffset + listOffset.size()) - 0x08;
			relativeOffset += listOffset.size();
			relativeOffset += calcRefVecSize(entries.size()); // Size of the Entry List We'll Be Generating
			for (int i = 0; i < entries.size(); i++)
			{
				entries[i].parentRelativeOffset = relativeOffset;
				relativeOffset += entries[i].size();
			}
		}
		unsigned long brsarInfoGroupHeader::getSynonymFileID(std::size_t headerLengthIn) const
		{
			unsigned long result = ULONG_MAX;

			for (unsigned long i = 0; result == ULONG_MAX && i < entries.size(); i++)
			{
				if (headerLengthIn == entries[i].headerLength)
				{
					result = entries[i].fileID;
				}
			}

			return result;
		}
		bool brsarInfoGroupHeader::usesFileID(unsigned long fileIDIn) const
		{
			bool result = 0;

			for (unsigned long i = 0; !result && i < entries.size(); i++)
			{
				result = fileIDIn == entries[i].fileID;
			}

			return result;
		}

		unsigned long brsarInfoSection::size(infoSectionLandmark tallyUpTo) const
		{
			unsigned long result = 0;

			result += 0x04; // INFO Tag
			result += sizeof(unsigned long); // Length
			if (tallyUpTo <= infoSectionLandmark::iSL_Header)
			{
				return result;
			}

			result += soundsSectionReference.size();
			result += banksSectionReference.size();
			result += playerSectionReference.size();
			result += filesSectionReference.size();
			result += groupsSectionReference.size();
			result += footerReference.size();
			if (tallyUpTo <= infoSectionLandmark::iSL_VecReferences)
			{
				return result;
			}

			result += calcRefVecSize(soundEntries.size());
			for (std::size_t i = 0; i < soundEntries.size(); i++)
			{
				result += soundEntries[i]->size();
			}
			if (tallyUpTo <= infoSectionLandmark::iSL_SoundEntries)
			{
				return result;
			}

			result += calcRefVecSize(bankEntries.size());
			result += bankEntries.size() * brsarInfoBankEntry::size();
			if (tallyUpTo <= infoSectionLandmark::iSL_BankEntries)
			{
				return result;
			}

			result += calcRefVecSize(playerEntries.size());
			result += playerEntries.size() * brsarInfoPlayerEntry::size();
			if (tallyUpTo <= infoSectionLandmark::iSL_PlayerEntries)
			{
				return result;
			}

			result += calcRefVecSize(fileHeaders.size());
			for (std::size_t i = 0; i < fileHeaders.size(); i++)
			{
				result += fileHeaders[i]->size();
			}
			if (tallyUpTo <= infoSectionLandmark::iSL_FileHeaders)
			{
				return result;
			}

			result += calcRefVecSize(groupHeaders.size());
			for (std::size_t i = 0; i < groupHeaders.size(); i++)
			{
				result += groupHeaders[i]->size();
			}
			if (tallyUpTo <= infoSectionLandmark::iSL_GroupHeaders)
			{
				return result;
			}

			result += sizeof(sequenceMax);
			result += sizeof(sequenceTrackMax);
			result += sizeof(streamMax);
			result += sizeof(streamTrackMax);
			result += sizeof(streamChannelsMax);
			result += sizeof(waveMax);
			result += sizeof(waveTrackMax);
			result += sizeof(padding);
			result += sizeof(reserved);

			return result;
		}
		unsigned long brsarInfoSection::paddedSize(unsigned long padTo) const
		{
			return padLengthTo(size(), padTo);
		}
		unsigned long brsarInfoSection::getAddress() const
		{
			return (parent != nullptr) ? parent->getINFOSectionAddress() : ULONG_MAX;
		}
		bool brsarInfoSection::populate(brsar& parentIn, lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated() && bodyIn.getLong(addressIn) == brsarHexTags::bht_INFO)
			{
				result = 1;
				parent = &parentIn;
				address = addressIn;

				//length = bodyIn.getLong(address + 0x04); No longer needed!
				soundsSectionReference = brawlReference(bodyIn.getLLong(address + 0x08));
				banksSectionReference = brawlReference(bodyIn.getLLong(address + 0x10));
				playerSectionReference = brawlReference(bodyIn.getLLong(address + 0x18));
				filesSectionReference = brawlReference(bodyIn.getLLong(address + 0x20));
				groupsSectionReference = brawlReference(bodyIn.getLLong(address + 0x28));
				footerReference = brawlReference(bodyIn.getLLong(address + 0x30));

				brawlReferenceVector soundsSection;
				brawlReferenceVector banksSection;
				brawlReferenceVector playerSection;
				brawlReferenceVector filesSection;
				brawlReferenceVector groupsSection;

				result &= soundsSection.populate(bodyIn, soundsSectionReference.getAddress(address + 0x08));
				result &= banksSection.populate(bodyIn, banksSectionReference.getAddress(address + 0x08));
				result &= playerSection.populate(bodyIn, playerSectionReference.getAddress(address + 0x08));
				result &= filesSection.populate(bodyIn, filesSectionReference.getAddress(address + 0x08));
				result &= groupsSection.populate(bodyIn, groupsSectionReference.getAddress(address + 0x08));

				brsarInfoSoundEntry* currSoundEntry = nullptr;
				soundEntries.resize(soundsSection.refs.size());
				for (std::size_t i = 0; i < soundsSection.refs.size(); i++)
				{
					soundEntries[i] = std::make_unique<brsarInfoSoundEntry>();
					currSoundEntry = soundEntries[i].get();
					result &= currSoundEntry->populate(*this, bodyIn, soundsSection.refs[i].getAddress(address + 0x08));
				}

				bankEntries.resize(banksSection.refs.size());
				for (std::size_t i = 0; i < banksSection.refs.size(); i++)
				{
					result &= bankEntries[i].populate(*this, bodyIn, banksSection.refs[i].getAddress(address + 0x08));
				}

				playerEntries.resize(playerSection.refs.size());
				for (std::size_t i = 0; i < playerSection.refs.size(); i++)
				{
					result &= playerEntries[i].populate(*this, bodyIn, playerSection.refs[i].getAddress(address + 0x08));
				}

				fileHeaders.resize(filesSection.refs.size());
				brsarInfoFileHeader* currFileHeader = nullptr;
				for (std::size_t i = 0; i < filesSection.refs.size(); i++)
				{
					fileHeaders[i] = std::make_unique<brsarInfoFileHeader>();
					currFileHeader = fileHeaders[i].get();
					currFileHeader->populate(*this, bodyIn, filesSection.refs[i].getAddress(address + 0x08));
				}

				groupHeaders.resize(groupsSection.refs.size());
				brsarInfoGroupHeader* currGroupHeader = nullptr;
				for (std::size_t i = 0; i < groupsSection.refs.size(); i++)
				{
					groupHeaders[i] = std::make_unique<brsarInfoGroupHeader>();
					currGroupHeader = groupHeaders[i].get();
					currGroupHeader->populate(*this, bodyIn, groupsSection.refs[i].getAddress(address + 0x08));
				}
				
				std::size_t cursor = footerReference.getAddress(address + 0x08);
				sequenceMax = bodyIn.getShort(cursor, &cursor);
				sequenceTrackMax = bodyIn.getShort(cursor, &cursor);
				streamMax = bodyIn.getShort(cursor, &cursor);
				streamTrackMax = bodyIn.getShort(cursor, &cursor);
				streamChannelsMax = bodyIn.getShort(cursor, &cursor);
				waveMax = bodyIn.getShort(cursor, &cursor);
				waveTrackMax = bodyIn.getShort(cursor, &cursor);
				padding = bodyIn.getShort(cursor, &cursor);
				reserved = bodyIn.getLong(cursor, &cursor);

				updateChildStructOffsetValues();

				parent->signalINFOSectionSizeChange();
			}

			return result;
		}
		bool brsarInfoSection::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				result = 1;
				unsigned long initialStreamPos = destinationStream.tellp();

				lava::writeRawDataToStream(destinationStream, brsarHexTags::bht_INFO);
				lava::writeRawDataToStream(destinationStream, paddedSize());
				lava::writeRawDataToStream(destinationStream, soundsSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, banksSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, playerSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, filesSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, groupsSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, footerReference.getHex());

				writeSoundRefVec(destinationStream);
				for (std::size_t i = 0; i < soundEntries.size(); i++)
				{
					soundEntries[i]->exportContents(destinationStream);
				}

				writeBankRefVec(destinationStream);
				for (std::size_t i = 0; i < bankEntries.size(); i++)
				{
					bankEntries[i].exportContents(destinationStream);
				}

				writePlayerRefVec(destinationStream);
				for (std::size_t i = 0; i < playerEntries.size(); i++)
				{
					playerEntries[i].exportContents(destinationStream);
				}

				writeFileRefVec(destinationStream);
				for (std::size_t i = 0; i < fileHeaders.size(); i++)
				{
					fileHeaders[i]->exportContents(destinationStream);
				}

				writeGroupRefVec(destinationStream);
				for (std::size_t i = 0; i < groupHeaders.size(); i++)
				{
					groupHeaders[i]->exportContents(destinationStream);
				}

				unsigned long pos = destinationStream.tellp();

				lava::writeRawDataToStream(destinationStream, sequenceMax);
				lava::writeRawDataToStream(destinationStream, sequenceTrackMax);
				lava::writeRawDataToStream(destinationStream, streamMax);
				lava::writeRawDataToStream(destinationStream, streamTrackMax);
				lava::writeRawDataToStream(destinationStream, streamChannelsMax);
				lava::writeRawDataToStream(destinationStream, waveMax);
				lava::writeRawDataToStream(destinationStream, waveTrackMax);
				lava::writeRawDataToStream(destinationStream, padding);
				lava::writeRawDataToStream(destinationStream, reserved);

				unsigned long finalStreamPos = destinationStream.tellp();
				unsigned long amountWritten = finalStreamPos - initialStreamPos;
				unsigned long expectedLength = paddedSize();
				if (expectedLength > amountWritten)
				{
					std::vector<char> finalPadding(expectedLength - amountWritten, 0x00);
					destinationStream.write(finalPadding.data(), finalPadding.size());
				}

				result = destinationStream.good();
			}
			return result;
		}

		
		bool brsarInfoSection::writeSoundRefVec(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, unsigned long(soundEntries.size()));
				for (std::size_t i = 0; i < soundEntries.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, unsigned long(0x01000000));
					lava::writeRawDataToStream(destinationStream, soundEntries[i]->parentRelativeOffset - 0x08);
				}
				result = destinationStream.good();
			}

			return result;
		}
		bool brsarInfoSection::writeBankRefVec(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, unsigned long(bankEntries.size()));
				for (std::size_t i = 0; i < bankEntries.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, unsigned long(0x01000000));
					lava::writeRawDataToStream(destinationStream, bankEntries[i].parentRelativeOffset - 0x08);
				}
				result = destinationStream.good();
			}

			return result;
		}
		bool brsarInfoSection::writePlayerRefVec(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, unsigned long(playerEntries.size()));
				for (std::size_t i = 0; i < playerEntries.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, unsigned long(0x01000000));
					lava::writeRawDataToStream(destinationStream, playerEntries[i].parentRelativeOffset - 0x08);
				}
				result = destinationStream.good();
			}

			return result;
		}
		bool brsarInfoSection::writeFileRefVec(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, unsigned long(fileHeaders.size()));
				for (std::size_t i = 0; i < fileHeaders.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, unsigned long(0x01000000));
					lava::writeRawDataToStream(destinationStream, fileHeaders[i]->parentRelativeOffset - 0x08);
				}
				result = destinationStream.good();
			}

			return result;
		}
		bool brsarInfoSection::writeGroupRefVec(std::ostream& destinationStream) const
		{
			bool result = 0;

			if (destinationStream.good())
			{
				lava::writeRawDataToStream(destinationStream, unsigned long(groupHeaders.size()));
				for (std::size_t i = 0; i < groupHeaders.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, unsigned long(0x01000000));
					lava::writeRawDataToStream(destinationStream, groupHeaders[i]->parentRelativeOffset - 0x08);
				}
				result = destinationStream.good();
			}

			return result;
		}

		void brsarInfoSection::updateChildStructOffsetValues(infoSectionLandmark startFrom)
		{
			unsigned long relativeOffset = ULONG_MAX;
			if (startFrom <= infoSectionLandmark::iSL_SoundEntries)
			{
				if (relativeOffset == ULONG_MAX)
				{
					relativeOffset = size(infoSectionLandmark::iSL_VecReferences); // Get size up to and including the vector references.
				}
				soundsSectionReference.address = relativeOffset - 0x08;
				relativeOffset += calcRefVecSize(soundEntries.size());
				for (std::size_t i = 0; i < soundEntries.size(); i++)
				{
					soundEntries[i]->parentRelativeOffset = relativeOffset;
					if (soundEntries[i]->getAddress() != soundEntries[i]->originalAddress)
					{
						int ruhroh = 0;
					}
					soundEntries[i]->updateSpecificSoundOffsetValue();
					soundEntries[i]->updateSound3DInfoOffsetValue();
					relativeOffset += soundEntries[i]->size();
				}
			}
			if (startFrom <= infoSectionLandmark::iSL_BankEntries)
			{
				if (relativeOffset == ULONG_MAX)
				{
					relativeOffset = size(infoSectionLandmark::iSL_SoundEntries); // Get size up to and including the sound entries.
				}
				banksSectionReference.address = relativeOffset - 0x08;
				relativeOffset += calcRefVecSize(bankEntries.size());
				for (std::size_t i = 0; i < bankEntries.size(); i++)
				{
					bankEntries[i].parentRelativeOffset = relativeOffset;
					relativeOffset += bankEntries[i].size();
				}
			}
			if (startFrom <= infoSectionLandmark::iSL_PlayerEntries)
			{
				if (relativeOffset == ULONG_MAX)
				{
					relativeOffset = size(infoSectionLandmark::iSL_BankEntries); // Get size up to and including the bank entries.
				}
				playerSectionReference.address = relativeOffset - 0x08;
				relativeOffset += calcRefVecSize(playerEntries.size());
				for (std::size_t i = 0; i < playerEntries.size(); i++)
				{
					playerEntries[i].parentRelativeOffset = relativeOffset;
					relativeOffset += playerEntries[i].size();
				}
			}
			if (startFrom <= infoSectionLandmark::iSL_FileHeaders)
			{
				if (relativeOffset == ULONG_MAX)
				{
					relativeOffset = size(infoSectionLandmark::iSL_PlayerEntries); // Get size up to and including the player entries.
				}
				filesSectionReference.address = relativeOffset - 0x08;
				relativeOffset += calcRefVecSize(fileHeaders.size());
				for (std::size_t i = 0; i < fileHeaders.size(); i++)
				{
					fileHeaders[i]->parentRelativeOffset = relativeOffset;
					fileHeaders[i]->updateFileEntryOffsetValues();
					relativeOffset += fileHeaders[i]->size();
				}
			}
			if (startFrom <= infoSectionLandmark::iSL_GroupHeaders)
			{
				if (relativeOffset == ULONG_MAX)
				{
					relativeOffset = size(infoSectionLandmark::iSL_FileHeaders); // Get size up to and including the file headers.
				}
				groupsSectionReference.address = relativeOffset - 0x08;
				relativeOffset += calcRefVecSize(groupHeaders.size());
				for (std::size_t i = 0; i < groupHeaders.size(); i++)
				{
					groupHeaders[i]->parentRelativeOffset = relativeOffset;
					groupHeaders[i]->updateGroupEntryOffsetValues();
					relativeOffset += groupHeaders[i]->size();
				}
			}
			if (startFrom <= infoSectionLandmark::iSL_Footer)
			{
				relativeOffset = size(infoSectionLandmark::iSL_GroupHeaders); // Get size up to and including the file headers.
				footerReference.address = relativeOffset - 0x08;
			}
		}

		brsarInfoGroupHeader* brsarInfoSection::getGroupWithID(unsigned long groupIDIn)
		{
			brsarInfoGroupHeader* result = nullptr;

			unsigned long i = 0;

			while (result == nullptr && i < groupHeaders.size())
			{
				if (groupIDIn == groupHeaders[i]->groupID)
				{
					result = groupHeaders[i].get();
				}
				i++;
			}

			return result;
		}
		brsarInfoGroupHeader* brsarInfoSection::getGroupWithInfoIndex(unsigned long infoIndexIn)
		{
			brsarInfoGroupHeader* result = nullptr;

			if (infoIndexIn < groupHeaders.size())
			{
				result = groupHeaders[infoIndexIn].get();
			}

			return result;
		}
		std::vector<brsarInfoFileHeader*> brsarInfoSection::getFilesWithGroupID(unsigned long groupIDIn)
		{
			std::vector<brsarInfoFileHeader*> result{};

			brsarInfoGroupHeader* targetGroupHeader = getGroupWithID(groupIDIn);
			if (targetGroupHeader != nullptr)
			{
				for (unsigned long i = 0; i < targetGroupHeader->entries.size(); i++)
				{
					brsarInfoGroupEntry* currentGroupEntry = &targetGroupHeader->entries[i];
					if (currentGroupEntry->fileID < fileHeaders.size())
					{
						result.push_back( fileHeaders[currentGroupEntry->fileID].get());
					}
				}
			}

			return result;
		}
		brsarInfoFileHeader* brsarInfoSection::getFileHeaderPointer(unsigned long fileID)
		{
			brsarInfoFileHeader* result = nullptr;

			if (fileID < fileHeaders.size())
			{
				result = fileHeaders[fileID].get();
			}

			return result;
		}
		bool brsarInfoSection::summarizeFileEntryData(std::ostream& output)
		{
			bool result = 0;
			if (output.good())
			{
				output << "There are " << fileHeaders.size() << " File Info Entries(s) in this BRSAR:\n";
				for (unsigned long i = 0; i < fileHeaders.size(); i++)
				{
					brsarInfoFileHeader* currHeaderPtr = fileHeaders[i].get();
					output << "File Info #" << i << " (@ 0x" << numToHexStringWithPadding(currHeaderPtr->getAddress(), 0x08) << "):\n";
					output << "\tFile String: \"";
					if (!currHeaderPtr->stringContent.empty())
					{
						output << std::string((char*)currHeaderPtr->stringContent.data());
					}
					output << "\"\n";
					output << "\tFile Header Length: " << currHeaderPtr->headerLength << " byte(s)\n";
					output << "\tFile Data Length: " << currHeaderPtr->dataLength << " byte(s)\n";
					output << "\tFile Entry Number: " << currHeaderPtr->entryNumber << "\n";
					output << "\tFile Entries:\n";
					for (unsigned long u = 0; u < currHeaderPtr->entries.size(); u++)
					{
						brsarInfoFileEntry* currEntryPtr = &currHeaderPtr->entries[u];
						output << "\tEntry #" << u << " (@ 0x" << numToHexStringWithPadding(currEntryPtr->getAddress(), 0x08) << "):\n";
						output << "\t\tEntry Index: " << currEntryPtr->index << "\n";
						output << "\t\tGroup ID: " << currEntryPtr->groupID << "\n";
					}
				}
				result = output.good();
			}
			return result;
		}
		
		/* BRSAR Info Section */



		/* BRSAR File Section */

		unsigned long brsarFileFileContents::size() const
		{
			return header.size() + data.size();
		}
		unsigned long brsarFileFileContents::getHeaderAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeHeaderOffset : ULONG_MAX;
		}
		unsigned long brsarFileFileContents::getDataAddress() const
		{
			return (parent != nullptr) ? parent->getAddress() + parentRelativeDataOffset : ULONG_MAX;
		}
		bool brsarFileFileContents::dumpToStream(std::ostream& output)
		{
			bool result = 0;

			if (output.good())
			{
				result = dumpHeaderToStream(output);
				result &= dumpDataToStream(output);
			}

			return result;
		}
		bool brsarFileFileContents::dumpHeaderToStream(std::ostream& output)
		{
			bool result = 0;

			if (output.good())
			{
				output.write((char*)header.data(), header.size());
				result = output.good();
			}

			return result;
		}
		bool brsarFileFileContents::dumpDataToStream(std::ostream& output)
		{
			bool result = 0;

			if (output.good())
			{
				output.write((char*)data.data(), data.size());
				result = output.good();
			}

			return result;
		}
		bool brsarFileFileContents::dumpToFile(std::string filePath)
		{
			bool result = 0;

			std::ofstream output(filePath, std::ios_base::binary | std::ios_base::out);
			if (output.is_open())
			{
				result = dumpToStream(output);
			}

			return result;
		}
		bool brsarFileFileContents::dumpHeaderToFile(std::string filePath)
		{
			bool result = 0;

			std::ofstream output(filePath, std::ios_base::binary | std::ios_base::out);
			if (output.is_open())
			{
				result = dumpHeaderToStream(output);
			}

			return result;
		}
		bool brsarFileFileContents::dumpDataToFile(std::string filePath)
		{
			bool result = 0;

			std::ofstream output(filePath, std::ios_base::binary | std::ios_base::out);
			if (output.is_open())
			{
				result = dumpDataToStream(output);
			}

			return result;
		}

		unsigned long brsarFileSection::size() const
		{
			unsigned long result = 0;

			result += 0x04; // FILE Tag
			result += sizeof(unsigned long); // Length
			result += 0x18; // Padding
			for (unsigned long i = 0; i < fileContents.size(); i++)
			{
				result += fileContents[i].size();
			}

			return result;
		}
		unsigned long brsarFileSection::getAddress() const
		{
			return (parent != nullptr) ? parent->getFILESectionAddress() : ULONG_MAX;
		}
		bool brsarFileSection::populate(brsar& parentIn, lava::byteArray& bodyIn, std::size_t addressIn, brsarInfoSection& infoSectionIn)
		{
			bool result = 0;

			if (bodyIn.populated() && bodyIn.getLong(addressIn) == brsarHexTags::bht_FILE)
			{
				result = 1;
				parent = &parentIn;
				originalAddress = addressIn;

				//length = bodyIn.getLong(address + 0x04); No longer needed!
				brsarInfoGroupHeader* currHeader = nullptr;
				brsarInfoGroupEntry* currEntry = nullptr;
				for (std::size_t i = 0; i < infoSectionIn.groupHeaders.size(); i++)
				{
					currHeader = infoSectionIn.groupHeaders[i].get();
					for (std::size_t u = 0; u < currHeader->entries.size(); u++)
					{
						currEntry = &currHeader->entries[u];
						fileContents.push_back(brsarFileFileContents());
						fileContents.back().fileID = currEntry->fileID;
						fileContents.back().groupInfoIndex = i;
						fileContents.back().groupID = currHeader->groupID;
						fileContents.back().originalHeaderAddress = currHeader->headerAddress + currEntry->headerOffset;
						fileContents.back().header = bodyIn.getBytes(currEntry->headerLength, currHeader->headerAddress + currEntry->headerOffset);
						fileContents.back().originalDataAddress = currHeader->dataAddress + currEntry->dataOffset;
						fileContents.back().data = bodyIn.getBytes(currEntry->dataLength, currHeader->dataAddress + currEntry->dataOffset);

						fileContents.back().parent = this;
						fileContents.back().parentRelativeHeaderOffset = fileContents.back().originalHeaderAddress - getAddress();
						fileContents.back().parentRelativeDataOffset = fileContents.back().originalDataAddress - getAddress();

						fileIDToIndex[currEntry->fileID].push_back(fileContents.size() - 1);
					}
				}

				parent->signalFILESectionSizeChange();
			}

			return result;
		}
		bool brsarFileSection::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;

			if (destinationStream.good())
			{
				byteArray tempArray(size(), 0x00);
				tempArray.setLong(brsarHexTags::bht_FILE, 0x00);
				tempArray.setLong(size(), 0x04);
				for (unsigned long i = 0; i < fileContents.size(); i++)
				{
					brsarFileFileContents* currFilePtr = &fileContents[i];
					if (currFilePtr->getHeaderAddress() > getAddress())
					{
						tempArray.setBytes(currFilePtr->header, currFilePtr->getHeaderAddress() - getAddress());
					}
					else
					{
						std::cerr << "Invalid header address!\n";
					}
					if (currFilePtr->getDataAddress() > getAddress())
					{
						tempArray.setBytes(currFilePtr->data, currFilePtr->getDataAddress() - getAddress());
					}
					else
					{
						std::cerr << "Invalid data address!\n";
					}
				}
				tempArray.dumpToStream(destinationStream);
				result = destinationStream.good();
			}

			return result;
		}
		std::vector<brsarFileFileContents*> brsarFileSection::getFileContentsPointerVector(unsigned long fileID)
		{
			std::vector<brsarFileFileContents*> result{};

			auto findRes = fileIDToIndex.find(fileID);
			if (findRes != fileIDToIndex.end())
			{
				std::vector<std::size_t>* indexVecPtr = &findRes->second;
				for (unsigned long i = 0; i < indexVecPtr->size(); i++)
				{
					brsarFileFileContents* currFileContentsPtr = &fileContents[(*indexVecPtr)[i]];
					result.push_back(currFileContentsPtr);
				}
			}

			return result;
		}
		brsarFileFileContents* brsarFileSection::getFileContentsPointer(unsigned long fileID, unsigned long groupID)
		{
			brsarFileFileContents* result = nullptr;

			std::vector<brsarFileFileContents*> pointerVec = getFileContentsPointerVector(fileID);
			unsigned long i = 0;
			while (result == nullptr && i < pointerVec.size())
			{
				if (groupID == ULONG_MAX || pointerVec[i]->groupID == groupID)
				{
					result = pointerVec[i];
				}
				i++;
			}

			return result;
		}
		bool brsarFileSection::propogateFileLengthChange(signed long changeAmount, unsigned long pastThisAddress)
		{
			bool result = 1;

			for (unsigned long i = 0; i < fileContents.size(); i++)
			{
				brsarFileFileContents* currFile = &fileContents[i];
				adjustOffset(getAddress(), currFile->parentRelativeHeaderOffset, changeAmount, pastThisAddress);
				adjustOffset(getAddress(), currFile->parentRelativeDataOffset, changeAmount, pastThisAddress);
			}

			return result;
		}
		

		/* RWSD */

		void rwsdWaveSection::waveEntryPushBack(const waveInfo& sourceWave)
		{
			if (!entryOffsets.empty())
			{
				for (unsigned long i = 0; i < entryOffsets.size(); i++)
				{
					entryOffsets[i] += 0x04;
				}

				entryOffsets.push_back(entryOffsets.back() + entries.back().size());
			}
			else
			{
				entryOffsets.push_back(0x10);
			}

			entries.push_back(sourceWave);

			length += 0x04 + sourceWave.size();
		}
		void rwsdWaveSection::waveEntryPushFront(const waveInfo& sourceWave)
		{
			if (!entryOffsets.empty())
			{
				unsigned long initialEntryAddress = entryOffsets.front();
				for (unsigned long i = 0; i < entryOffsets.size(); i++)
				{
					entryOffsets[i] += 0x04 + sourceWave.size();
				}
				entryOffsets.push_back(initialEntryAddress + 0x04);
			}
			else
			{
				entryOffsets.push_back(0x10);
			}

			entries.insert(entries.begin(), sourceWave);

			length += 0x04 + sourceWave.size();
		}

		bool rwsdWaveSection::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				length = bodyIn.getLong(addressIn + 0x04);
				unsigned long entryCount = bodyIn.getLong(addressIn + 0x08);

				entryOffsets.clear();
				entries.clear();

				for (unsigned long cursor = 0x0; cursor < (entryCount * 4); cursor += 0x04)
				{
					entryOffsets.push_back(bodyIn.getLong(address + 0x0C + cursor));
					entries.push_back(waveInfo());
					entries.back().populate(bodyIn, address + entryOffsets.back());
				}

				result = 1;
			}

			return result;
		}
		bool rwsdWaveSection::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				unsigned long initialStreamPos = destinationStream.tellp();

				destinationStream << "WAVE";
				lava::writeRawDataToStream(destinationStream, length);
				lava::writeRawDataToStream<unsigned long>(destinationStream, entries.size());

				for (unsigned long i = 0x0; i < entryOffsets.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, entryOffsets[i]);
				}
				for (unsigned long i = 0x0; i < entries.size(); i++)
				{
					entries[i].exportContents(destinationStream);
				}

				unsigned long finalStreamPos = destinationStream.tellp();
				unsigned long lengthOfExport = finalStreamPos - initialStreamPos;
				if (lengthOfExport != length)
				{
					if (lengthOfExport < length)
					{
						std::vector<char> padding{};
						padding.resize(length - lengthOfExport, 0x00);
						destinationStream.write(padding.data(), padding.size());
					}
				}

				result = destinationStream.good();
			}
			return result;
		}

		bool rwsdDataSection::hasExclusiveWave(unsigned long dataSectionIndex)
		{
			bool result = 1;
			if (dataSectionIndex < entries.size())
			{
				dataInfo* dataInfoEntry = &entries[dataSectionIndex];
				unsigned long dataWaveIndex = dataInfoEntry->ntWaveIndex;

				std::size_t i = 0;
				while (result && i < entries.size())
				{
					if (i != dataSectionIndex)
					{
						result = dataWaveIndex != entries[i].ntWaveIndex;
					}
					i++;
				}
			}
			return result;
		}
		bool rwsdDataSection::isFirstToUseWave(unsigned long dataSectionIndex)
		{
			bool result = 1;
			if (dataSectionIndex < entries.size())
			{
				dataInfo* dataInfoEntry = &entries[dataSectionIndex];
				unsigned long dataWaveIndex = dataInfoEntry->ntWaveIndex;

				std::size_t i = 0;
				while (result && i < dataSectionIndex)
				{
					result = dataWaveIndex != entries[i].ntWaveIndex;
					i++;
				}
			}
			return result;
		}
		bool rwsdDataSection::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;
				length = bodyIn.getLong(addressIn + 0x04);
				entryReferences.populate(bodyIn, addressIn + 0x08);
				std::vector<brawlReference>* refVecPtr = &entryReferences.refs;
				std::size_t entryCount = refVecPtr->size();
				unsigned long entryTargetAddress = ULONG_MAX;
				entries.resize(entryCount);
				for (std::size_t i = 0; i < entryCount; i++)
				{
					entryTargetAddress = address + 0x08 + refVecPtr->at(i).getAddress();
					if (bodyIn.getLong(entryTargetAddress) == 0x01000000)
					{
						entries[i].populate(bodyIn, entryTargetAddress);
					}
					else
					{
						std::cerr << "Skipping Data Entry @ " << entryTargetAddress << "\n";
					}
				}

				result = 1;
			}

			return result;
		}
		bool rwsdDataSection::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				unsigned long initialStreamPos = destinationStream.tellp();
				destinationStream << "DATA";
				lava::writeRawDataToStream(destinationStream, length);
				entryReferences.exportContents(destinationStream);
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					entries[i].exportContents(destinationStream);
				}
				unsigned long finalStreamPos = destinationStream.tellp();
				unsigned long lengthOfExport = finalStreamPos - initialStreamPos;
				if (lengthOfExport != length)
				{
					if (lengthOfExport < length)
					{
						std::vector<char> padding{};
						padding.resize(length - lengthOfExport, 0x00);
						destinationStream.write(padding.data(), padding.size());
					}
				}
				result = destinationStream.good();
			}
			return result;
		}

		bool rwsdHeader::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;
				endianType = bodyIn.getShort(addressIn + 0x04);
				version = bodyIn.getShort(addressIn + 0x06);
				headerLength = bodyIn.getLong(addressIn + 0x08);
				entriesOffset = bodyIn.getShort(addressIn + 0x0C);
				entriesCount = bodyIn.getShort(addressIn + 0x0E);

				dataOffset = bodyIn.getLong(addressIn + 0x10);
				dataLength = bodyIn.getLong(addressIn + 0x14);
				waveOffset = bodyIn.getLong(addressIn + 0x18);
				waveLength = bodyIn.getLong(addressIn + 0x1C);
				result = 1;
			}

			return result;
		}
		bool rwsdHeader::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				destinationStream << "RWSD";
				lava::writeRawDataToStream(destinationStream, endianType);
				lava::writeRawDataToStream(destinationStream, version);
				lava::writeRawDataToStream(destinationStream, headerLength);
				lava::writeRawDataToStream(destinationStream, entriesOffset);
				lava::writeRawDataToStream(destinationStream, entriesCount);
				lava::writeRawDataToStream(destinationStream, dataOffset);
				lava::writeRawDataToStream(destinationStream, dataLength);
				lava::writeRawDataToStream(destinationStream, waveOffset);
				lava::writeRawDataToStream(destinationStream, waveLength);

				result = destinationStream.good();
			}
			return result;
		}

		bool rwsd::updateWaveEntryDataLocations()
		{
			bool result = 1;

			unsigned long positionAccumulator = 0x00;
			for (unsigned long i = 0; i < waveSection.entries.size(); i++)
			{
				waveInfo* currEntry = &waveSection.entries[i];
				currEntry->dataLocation = positionAccumulator;
				positionAccumulator += currEntry->packetContents.body.size() + currEntry->packetContents.padding.size();
			}

			return result;
		}
		waveInfo* rwsd::getWaveInfoAssociatedWithDataInfo(unsigned long dataSectionIndex)
		{
			waveInfo* result = nullptr;

			if (dataSectionIndex < dataSection.entries.size())
			{
				unsigned long waveIndex = dataSection.entries[dataSectionIndex].ntWaveIndex;
				if (waveIndex < waveSection.entries.size())
				{
					result = &waveSection.entries[waveIndex];
				}
			}

			return result;
		}
		bool rwsd::summarize(std::ostream& output)
		{
			bool result = 0;

			if (output.good() && header.address != UINT_MAX)
			{
				output << "\tAddress:\t0x" << lava::numToHexStringWithPadding(header.address, 0x08) << "\n";
				output << "\tTotal Length/End:\t0x" << lava::numToHexStringWithPadding(header.headerLength, 0x08) << " / 0x" << lava::numToHexStringWithPadding(header.headerLength + header.address, 0x08) << "\n";
				output << "\tData Section Offset/Address:\t0x" << lava::numToHexStringWithPadding(header.dataOffset, 0x08) << " / 0x" << lava::numToHexStringWithPadding(dataSection.address, 0x08) << "\n";
				output << "\tData Section Length/End:\t0x" << lava::numToHexStringWithPadding(header.dataLength, 0x08) << " / 0x" << lava::numToHexStringWithPadding(dataSection.address + header.dataLength, 0x08) << "\n";
				output << "\tData Entry Count:\t\t0x" << lava::numToHexStringWithPadding(dataSection.entries.size(), 0x04) << "\n";
				output << "\tWave Section Offset/Address:\t0x" << lava::numToHexStringWithPadding(header.waveOffset, 0x08) << " / 0x" << lava::numToHexStringWithPadding(waveSection.address, 0x08) << "\n";
				output << "\tWave Section Length/End:\t0x" << lava::numToHexStringWithPadding(header.waveLength, 0x08) << " / 0x" << lava::numToHexStringWithPadding(waveSection.address + header.waveLength, 0x08) << "\n";
				output << "\tWave Entry Count:\t\t0x" << lava::numToHexStringWithPadding(waveSection.entries.size(), 0x04) << "\n";

				std::unordered_map<unsigned long, std::vector<unsigned long>> waveIndecesToReferrerDataIndeces{};

				std::vector<dataInfo>* dataVecPtr = &dataSection.entries;
				std::vector<waveInfo>* waveVecPtr = &waveSection.entries;
				brawlReferenceVector* dataRefVecPtr = &dataSection.entryReferences;
				std::vector<unsigned long>* waveOffVecPtr = &waveSection.entryOffsets;

				unsigned long waveDataBaseAddress = header.waveOffset + header.waveLength;

				for (unsigned long u = 0; u < dataVecPtr->size(); u++)
				{
					dataInfo* currDataEntry = &(*dataVecPtr)[u];
					output << "\tData Entry 0x" << lava::numToHexStringWithPadding(u, 0x04) << "\n";
					output << "\t\tOffset / Address:\t0x" << lava::numToHexStringWithPadding(dataRefVecPtr->refs[u].address, 0x08) << " / 0x" << lava::numToHexStringWithPadding(currDataEntry->address, 0x08) << "\n";
					output << "\t\tAssociated Wave ID:\t0x" << lava::numToHexStringWithPadding(currDataEntry->ntWaveIndex, 0x04) << "\n";
					if (currDataEntry->ntWaveIndex != ULONG_MAX)
					{
						auto emplaceResult = waveIndecesToReferrerDataIndeces.emplace(std::make_pair(currDataEntry->ntWaveIndex, std::vector<unsigned long>()));
						if (emplaceResult.second)
						{
							waveInfo* currWaveEntry = &(*waveVecPtr)[currDataEntry->ntWaveIndex];
							output << "\t\tWave Entry Offset / Address:\t0x" << lava::numToHexStringWithPadding((*waveOffVecPtr)[currDataEntry->ntWaveIndex], 0x08) << " / 0x" << lava::numToHexStringWithPadding(currWaveEntry->address, 0x08) << "\n";
							output << "\t\tWave Contents Offset / Address:\t0x" << lava::numToHexStringWithPadding(currWaveEntry->dataLocation, 0x08) << " / 0x" << lava::numToHexStringWithPadding(waveDataBaseAddress + currWaveEntry->dataLocation, 0x08) << "\n";
							output << "\t\tWave Contents Length / End:\t0x" << lava::numToHexStringWithPadding(currWaveEntry->getAudioLengthInBytes(), 0x04) << " / 0x" << lava::numToHexStringWithPadding(waveDataBaseAddress + currWaveEntry->dataLocation + currWaveEntry->getAudioLengthInBytes(), 0x08) << "\n";
							emplaceResult.first->second.push_back(u);
						}
						else
						{
							output << "\t\tSkipping Wave Summary, see Data Entry 0x" << lava::numToHexStringWithPadding(emplaceResult.first->second.front(), 0x04) << "\n";
							emplaceResult.first->second.push_back(u);
						}
					}
				}
				result = output.good();
			}

			return result;
		}
		bool rwsd::summarize(std::string filepath)
		{
			bool result = 0;

			std::ofstream output(filepath, std::ios_base::out);
			if (output.is_open())
			{
				result = summarize(output);
			}

			return result;
		}

		bool rwsd::overwriteWaveRawData(unsigned long waveSectionIndex, const std::vector<unsigned char>& rawDataIn)
		{
			bool result = 0;

			if (waveSectionIndex < waveSection.entries.size())
			{
				waveInfo* targetWaveInfo = &waveSection.entries[waveSectionIndex];
				targetWaveInfo->packetContents.body = rawDataIn;
				targetWaveInfo->packetContents.padding = std::vector<unsigned char> (0x10 - (targetWaveInfo->packetContents.body.size() % 0x10), 0x00);
				targetWaveInfo->nibbles = targetWaveInfo->packetContents.body.size() * 2;
				result = updateWaveEntryDataLocations();
			}

			return result;
		}
		bool rwsd::overwriteWaveRawDataWithDSP(unsigned long waveSectionIndex, const dsp& dspIn)
		{
			bool result = 0;

			if (waveSectionIndex < waveSection.entries.size())
			{
				waveInfo* targetWaveInfo = &waveSection.entries[waveSectionIndex];
				targetWaveInfo->encoding = 2;
				targetWaveInfo->channels = 1;
				targetWaveInfo->looped = dspIn.loops;
				targetWaveInfo->loopStartSample = dspIn.loopStart;
				targetWaveInfo->nibbles = dspIn.nibbleCount;
				targetWaveInfo->sampleRate24 = dspIn.sampleRate >> 16;
				targetWaveInfo->sampleRate = 0x0000FFFF & dspIn.sampleRate;
				targetWaveInfo->channelInfoTable.resize(1);
				targetWaveInfo->channelInfoEntries.resize(1);
				targetWaveInfo->adpcmInfoEntries.resize(1);
				targetWaveInfo->adpcmInfoEntries.back() = dspIn.soundInfo;
				result = overwriteWaveRawData(waveSectionIndex, dspIn.body);
			}

			return result;
		}
		bool rwsd::overwriteWaveRawDataWithDSP(unsigned long waveSectionIndex, std::string dspPathIn)
		{
			bool result = 0;

			if (std::filesystem::is_regular_file(dspPathIn))
			{
				dsp tempDSP;
				if (tempDSP.populate(dspPathIn, 0x00))
				{
					result = overwriteWaveRawDataWithDSP(waveSectionIndex, tempDSP);
				}
			}

			return result;
		}
		bool rwsd::overwriteWaveRawDataWithWAV(unsigned long waveSectionIndex, std::string wavPathIn)
		{
			bool result = 0;

			if (std::filesystem::is_regular_file(wavPathIn))
			{
				system(lava::brawl::generateVGAudioWavToDSPCommand(wavPathIn, VGAudioTempConvFilename).c_str());
				if (std::filesystem::is_regular_file(VGAudioTempConvFilename))
				{
					result = overwriteWaveRawDataWithDSP(waveSectionIndex, VGAudioTempConvFilename);
					std::filesystem::remove(VGAudioTempConvFilename);
				}
			}

			return result;
		}

		bool rwsd::grantDataEntryUniqueWave(unsigned long dataSectionIndex, const waveInfo& sourceWave, bool pushFront)
		{
			bool result = 1;

			if (!pushFront)
			{
				waveSection.waveEntryPushFront(sourceWave);
			}
			else
			{
				waveSection.waveEntryPushBack(sourceWave);
			}
			if (updateWaveEntryDataLocations())
			{
				dataSection.entries[dataSectionIndex].ntWaveIndex = waveSection.entries.size() - 1;
				header.headerLength += 0x04 + sourceWave.size();
				header.waveLength += 0x04 + sourceWave.size();
			}

			return result;
		}

		bool rwsd::populateWavePacket(const lava::byteArray& bodyIn, unsigned long waveIndex, unsigned long rawDataAddressIn, unsigned long specificDataEndAddressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				waveInfo* currWave = &waveSection.entries[waveIndex];
				unsigned long length = currWave->getAudioLengthInBytes();
				unsigned long paddingLength = 0x00;
				unsigned long currWaveDataEndpoint = rawDataAddressIn + currWave->dataLocation + length;
				if (currWaveDataEndpoint <= specificDataEndAddressIn)
				{
					paddingLength = specificDataEndAddressIn - currWaveDataEndpoint;
				}
				else
				{
					unsigned long overflowAmount = currWaveDataEndpoint - specificDataEndAddressIn;
					length -= overflowAmount;
					currWave->nibbles -= overflowAmount * 2;
				}
				result &= currWave->packetContents.populate(bodyIn, rawDataAddressIn + currWave->dataLocation, length, paddingLength);
			}

			return result;
		}
		bool rwsd::populateWavePackets(const lava::byteArray& bodyIn, unsigned long waveDataAddressIn, unsigned long waveDataLengthIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				for (std::size_t i = 0; i < (waveSection.entries.size() - 1); i++)
				{
					waveInfo* currWave = &waveSection.entries[i];
					waveInfo* nextWave = &waveSection.entries[i + 1];
					result &= populateWavePacket(bodyIn, i, waveDataAddressIn, waveDataAddressIn + nextWave->dataLocation);
				}
				waveInfo* finalWave = &waveSection.entries.back();
				result &= populateWavePacket(bodyIn, waveSection.entries.size() - 1, waveDataAddressIn, waveDataAddressIn + waveDataLengthIn);
			}

			return result;
		}
		bool rwsd::populate(const byteArray& fileBodyIn, unsigned long fileBodyAddressIn, const byteArray& rawDataIn, unsigned long rawDataAddressIn, unsigned long rawDataLengthIn)
		{
			bool result = 0;

			if (fileBodyIn.getLong(fileBodyAddressIn) == brsarHexTags::bht_RWSD)
			{
				if (header.populate(fileBodyIn, fileBodyAddressIn))
				{
					result = dataSection.populate(fileBodyIn, fileBodyAddressIn + header.dataOffset);
					result &= waveSection.populate(fileBodyIn, fileBodyAddressIn + header.waveOffset);
					if (result)
					{
						result &= populateWavePackets(rawDataIn, rawDataAddressIn, rawDataLengthIn);
					}
				}
			}

			return result;
		}
		bool rwsd::populate(const brsarFileFileContents& fileContentsIn)
		{
			bool result = 0;

			if (bytesToFundamental<unsigned long>(fileContentsIn.header.data()) == brsarHexTags::bht_RWSD)
			{
				byteArray headerArr(fileContentsIn.header.data(), fileContentsIn.header.size());
				byteArray dataArr(fileContentsIn.data.data(), fileContentsIn.data.size());
				result = populate(headerArr, 0x00, dataArr, 0x00, dataArr.size());
			}

			return result;
		}

		dsp rwsd::exportWaveRawDataToDSP(unsigned long waveSectionIndex)
		{
			dsp result;

			if (waveSectionIndex < waveSection.entries.size())
			{
				const waveInfo* targetWaveInfo = &waveSection.entries[waveSectionIndex];
				result.nibbleCount = targetWaveInfo->nibbles;
				result.sampleCount = nibblesToSamples(result.nibbleCount);
				result.nibbleCount = samplesToNibbles(result.sampleCount);
				result.sampleRate = unsigned long(targetWaveInfo->sampleRate24) << 16;
				result.sampleRate |= targetWaveInfo->sampleRate;
				result.loops = targetWaveInfo->looped;
				result.loopStart = targetWaveInfo->loopStartSample;
				if (result.loops)
				{
					result.loopEnd = result.sampleCount - 1;
				}
				else
				{
					result.loopEnd = 0x00;
				}
				result.soundInfo = targetWaveInfo->adpcmInfoEntries.back();
				result.body = targetWaveInfo->packetContents.body;
				//result.body.insert(result.body.end(), targetWaveInfo->packetContents.padding.begin(), targetWaveInfo->packetContents.padding.end());
				unsigned long desiredLength = nibblesToBytes(result.nibbleCount);
				if (result.body.size() < desiredLength)
				{
					result.body.resize(desiredLength);
				}
			}

			return result;
		}
		bool rwsd::exportWaveRawDataToWAV(unsigned long waveSectionIndex, std::string wavOutputPath)
		{
			bool result = 0;

			dsp conversionDSP = exportWaveRawDataToDSP(waveSectionIndex);
			if (!conversionDSP.body.empty())
			{
				std::ofstream convDSPOut(VGAudioTempConvFilename, std::ios_base::out | std::ios_base::binary);
				if (convDSPOut.is_open())
				{
					if (conversionDSP.exportContents(convDSPOut))
					{
						convDSPOut.close();
						system(lava::brawl::generateVGAudioDSPToWavCommand(VGAudioTempConvFilename, wavOutputPath).c_str());
						result = std::filesystem::is_regular_file(wavOutputPath);
					}
					std::filesystem::remove(VGAudioTempConvFilename);
				}
			}

			return result;
		}
		bool rwsd::exportFileSection(std::ostream& destinationStream)
		{
			bool result = 0;

			if (destinationStream.good())
			{
				result = 1;
				result &= header.exportContents(destinationStream);
				result &= dataSection.exportContents(destinationStream);
				result &= waveSection.exportContents(destinationStream);
			}

			return result;
		}
		bool rwsd::exportRawDataSection(std::ostream& destinationStream)
		{
			bool result = 0;

			if (destinationStream.good())
			{
				for (unsigned long i = 0; i < waveSection.entries.size(); i++)
				{
					waveInfo* currWave = &waveSection.entries[i];
					destinationStream.write((const char*)currWave->packetContents.body.data(), currWave->packetContents.body.size());
					destinationStream.write((const char*)currWave->packetContents.padding.data(), currWave->packetContents.padding.size());
				}
				result = destinationStream.good();
			}

			return result;
		}
		std::vector<unsigned char> rwsd::fileSectionToVec()
		{
			std::vector<unsigned char> result;

			std::stringstream tempStream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
			if (exportFileSection(tempStream))
			{
				result = streamContentsToVec(tempStream);
			}

			return result;
		}
		std::vector<unsigned char> rwsd::rawDataSectionToVec()
		{
			std::vector<unsigned char> result;

			std::stringstream tempStream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
			if (exportRawDataSection(tempStream))
			{
				result = streamContentsToVec(tempStream);
			}

			return result;
		}

		/* RWSD */

		/* BRSAR File Section */



		/* BRSAR */

		unsigned long brsar::size()
		{
			return headerLength + getSYMBSectionSize() + getINFOSectionSize() + getFILESectionSize();
		}
		bool brsar::init(std::string filePathIn)
		{
			bool result = 0;
			std::ifstream fileIn;
			fileIn.open(filePathIn, std::ios_base::in | std::ios_base::binary);
			if (fileIn.is_open())
			{
				std::cout << "Parsing \"" << filePathIn << "\"...\n";
				byteArray contents(fileIn);
				fileIn.close();
				if (contents.populated() && contents.getLong(0x00) == brsarHexTags::bht_RSAR)
				{
					std::size_t cursor = 0x04;
					result = 1;
					byteOrderMarker = contents.getShort(cursor, &cursor);
					version = contents.getShort(cursor, &cursor);
					contents.getLong(cursor, &cursor); // Move Past Length Entry
					headerLength = contents.getShort(cursor, &cursor);
					sectionCount = contents.getShort(cursor, &cursor);

					result &= symbSection.populate(*this, contents, contents.getLong(0x10));
					result &= infoSection.populate(*this, contents, contents.getLong(0x18));
					result &= fileSection.populate(*this, contents, contents.getLong(0x20), infoSection);

					updateInfoSectionFileOffsets();
				}
			}
			return result;
		}
		bool brsar::exportContents(std::ostream& destinationStream)
		{
			bool result = 1;

			writeRawDataToStream(destinationStream, brsarHexTags::bht_RSAR);
			writeRawDataToStream(destinationStream, byteOrderMarker);
			writeRawDataToStream(destinationStream, version);
			writeRawDataToStream(destinationStream, size());
			writeRawDataToStream(destinationStream, headerLength);
			writeRawDataToStream(destinationStream, sectionCount);
			writeRawDataToStream(destinationStream, getSYMBSectionAddress());
			writeRawDataToStream(destinationStream, symbSection.paddedSize());
			writeRawDataToStream(destinationStream, getINFOSectionAddress());
			writeRawDataToStream(destinationStream, infoSection.paddedSize());
			writeRawDataToStream(destinationStream, getFILESectionAddress());
			writeRawDataToStream(destinationStream, fileSection.size());
			if (headerLength > destinationStream.tellp())
			{
				std::vector<char> padding(headerLength - destinationStream.tellp(), 0x00);
				destinationStream.write(padding.data(), padding.size());
			}
			result &= symbSection.exportContents(destinationStream);
			result &= infoSection.exportContents(destinationStream);
			result &= fileSection.exportContents(destinationStream);

			return result;
		}
		bool brsar::exportContents(std::string outputFilename)
		{
			bool result = 0;

			std::ofstream output(outputFilename, std::ios_base::out | std::ios_base::binary);
			if (output.is_open())
			{
				result = exportContents(output);
			}

			return result;
		}

		void brsar::signalSYMBSectionSizeChange()
		{
			symbSectionCachedSize = ULONG_MAX;
		}
		void brsar::signalINFOSectionSizeChange()
		{
			infoSectionCachedSize = ULONG_MAX;
		}
		void brsar::signalFILESectionSizeChange()
		{
			fileSectionCachedSize = ULONG_MAX;
		}

		unsigned long brsar::getSYMBSectionSize()
		{
			if (symbSectionCachedSize == ULONG_MAX)
			{
				symbSectionCachedSize = symbSection.paddedSize();
			}
			return symbSectionCachedSize;
		}
		unsigned long brsar::getINFOSectionSize()
		{
			if (infoSectionCachedSize == ULONG_MAX)
			{
				infoSectionCachedSize = infoSection.paddedSize();
			}
			return infoSectionCachedSize;
		}
		unsigned long brsar::getFILESectionSize()
		{
			if (fileSectionCachedSize == ULONG_MAX)
			{
				fileSectionCachedSize = fileSection.size();
			}
			return fileSectionCachedSize;
		}

		unsigned long brsar::getSYMBSectionAddress()
		{
			return headerLength;
		}
		unsigned long brsar::getINFOSectionAddress()
		{
			return headerLength + getSYMBSectionSize();
		}
		unsigned long brsar::getFILESectionAddress()
		{
			return headerLength + getSYMBSectionSize() + getINFOSectionSize();
		}

		std::string brsar::getSymbString(unsigned long indexIn)
		{
			return symbSection.getString(indexIn);
		}
		unsigned long brsar::getGroupOffset(unsigned long groupIDIn)
		{
			std::size_t result = SIZE_MAX;

			std::size_t i = 0;
			bool done = 0;

			const brsarInfoGroupHeader* currentGroup = nullptr;
			while (!done && i < infoSection.groupHeaders.size())
			{
				currentGroup = infoSection.groupHeaders[i].get();
				if (groupIDIn == currentGroup->groupID)
				{
					result = currentGroup->getAddress();
					done = 1;
				}
				else
				{
					i++;
				}
			}

			return result;
		}

		bool brsar::updateInfoSectionFileOffsets()
		{
			bool result = 1;
			// Update each groupHeader and its entries with current file location info
			for (unsigned long i = 0; i < infoSection.groupHeaders.size(); i++)
			{
				// Grab a pointer to a given header
				brsarInfoGroupHeader* currGroupHeader = infoSection.groupHeaders[i].get();
				// Reset its lengths to 0, because we'll be using this and building it as we go.
				currGroupHeader->headerLength = 0x00;
				currGroupHeader->dataLength = 0x00;
				// If there are entries to iterate through...
				if (!currGroupHeader->entries.empty())
				{
					// Iterate through each of this headers entries, and update its info.
					for (unsigned long u = 0; u < currGroupHeader->entries.size(); u++)
					{
						// Grab a pointer to the entry
						brsarInfoGroupEntry* currGroupEntry = &currGroupHeader->entries[u];
						// Grab a pointer to the relevant fileContents entry
						brsarFileFileContents* currFileContents = fileSection.getFileContentsPointer(currGroupEntry->fileID, currGroupHeader->groupID);
						// If we grabbed the pointer successfully (this shouldn't ever fail, though)
						if (currFileContents != nullptr)
						{
							// If this is the first entry...
							if (u == 0)
							{
								// ... update the header and data address values to point to the right spots.
								currGroupHeader->headerAddress = currFileContents->getHeaderAddress();
								currGroupHeader->dataAddress = currFileContents->getDataAddress();
							}
							// Additionally, we'll always update the offset values if we're on the first entry.
							if (u == 0)
							{
								// Update the offset values.
								// These are tied to the current groupHeader's length values, which we update every run of this loop.
								// As a result, each offset points to right after the previous item.
								currGroupEntry->headerOffset = currGroupHeader->headerLength;
								currGroupEntry->dataOffset = currGroupHeader->dataLength;
							}
							// We won't, however, always update them otherwise.
							else
							{
								// In some instances, an entry after the first will have offsets of 0x00.
								// We need to maintain this in the modified file, so we only update the offsets otherwise.
								if (currGroupEntry->headerOffset != 0x00000000)
								{
									currGroupEntry->headerOffset = currGroupHeader->headerLength;
								}
								if (currGroupEntry->dataOffset != 0x00000000)
								{
									currGroupEntry->dataOffset = currGroupHeader->dataLength;
								}
							}
							// Update the length values. Self explanatory.
							currGroupEntry->headerLength = currFileContents->header.size();
							currGroupEntry->dataLength = currFileContents->data.size();
							// Update the groupHeader's length values. See above explanation.
							currGroupHeader->headerLength += currGroupEntry->headerLength;
							currGroupHeader->dataLength += currGroupEntry->dataLength;

						}
						else
						{
							result = 0;
						}
					}
				}
				else
				{
					// Otherwise, just point the header and data address fields to just after the previous data section.
					// I don't expect that this actually has any practical effect, since I don't expect the addresses of an empty group to ever be called.
					// However, vBrawl handles it this way, so that's how we'll do it as well.
					if (i > 0)
					{
						brsarInfoGroupHeader* prevGroupHeader = infoSection.groupHeaders[i - 1].get();
						currGroupHeader->headerAddress = prevGroupHeader->dataAddress + prevGroupHeader->dataLength;
						currGroupHeader->dataAddress = currGroupHeader->headerAddress;
					}
				}
			}
			return result;
		}
		bool brsar::overwriteFile(const std::vector<unsigned char>& headerIn, const std::vector<unsigned char>& dataIn, unsigned long fileIDIn)
		{
			bool result = 0;

			std::vector<brsarFileFileContents*> fileOccurences = fileSection.getFileContentsPointerVector(fileIDIn);
			if (!fileOccurences.empty())
			{
				brsarInfoFileHeader* fileHeaderPtr = infoSection.getFileHeaderPointer(fileIDIn);
				if (fileHeaderPtr != nullptr)
				{
					result = 1;
					// Handle File Section Adjustments
					for (unsigned long i = 0; i < fileOccurences.size(); i++)
					{
						// Replace the header and data
						brsarFileFileContents* currOccurence = fileOccurences[i];
						signed long headerLengthChange = signed long(headerIn.size()) - signed long(currOccurence->header.size());
						signed long dataLengthChange = signed long(dataIn.size()) - signed long(currOccurence->data.size());
						currOccurence->header = headerIn;
						currOccurence->data = dataIn;
						// Propogate the changes in size across the fileSection
						// Note that this needs to be done in two steps, because the dataAddress will likely change after the first step.
						result &= fileSection.propogateFileLengthChange(headerLengthChange, currOccurence->getHeaderAddress());
						result &= fileSection.propogateFileLengthChange(dataLengthChange, currOccurence->getDataAddress());
						// Note that the above propogation functions will update the FILE section's length.
					}
					// Update this file's fileHeader (it's the only one that needs its length updated)
					fileHeaderPtr->headerLength = headerIn.size();
					fileHeaderPtr->dataLength = dataIn.size();
					// Update the rest of the infoSection to correct the changes to file locations
					result &= updateInfoSectionFileOffsets();
				}
			}

			return result;
		}
		bool brsar::cloneFileTest(unsigned long fileIDToClone, unsigned long groupToLink)
		{
			bool result = 0;

			brsarInfoFileHeader* targetFileHeader = infoSection.getFileHeaderPointer(fileIDToClone);
			if (targetFileHeader != nullptr)
			{
				if (!fileSection.getFileContentsPointerVector(fileIDToClone).empty())
				{
					fileSection.fileContents.push_back(brsarFileFileContents());
					brsarFileFileContents* sourceFileContents = fileSection.getFileContentsPointerVector(fileIDToClone).front();
					brsarFileFileContents* newFileContents = &fileSection.fileContents.back();
					newFileContents->parent = &fileSection;
					newFileContents->originalHeaderAddress = bac_NOT_IN_FILE;
					newFileContents->originalDataAddress = bac_NOT_IN_FILE;
					newFileContents->header = sourceFileContents->header;
					newFileContents->data = sourceFileContents->data;
					newFileContents->fileID = infoSection.fileHeaders.size();
					newFileContents->groupID = groupToLink;
					newFileContents->parentRelativeHeaderOffset = getFILESectionSize();
					newFileContents->parentRelativeDataOffset = getFILESectionSize() + newFileContents->header.size();
					fileSection.fileIDToIndex[newFileContents->fileID] = std::vector<std::size_t>{ fileSection.fileContents.size() - 1 };

					infoSection.fileHeaders.push_back(std::make_unique<brsarInfoFileHeader>());
					brsarInfoFileHeader* newFileHeader = infoSection.fileHeaders.back().get();
					newFileHeader->parent = &infoSection;
					newFileHeader->headerLength = newFileContents->header.size();
					newFileHeader->dataLength = newFileContents->data.size();
					newFileHeader->entries.push_back(brsarInfoFileEntry());
					brsarInfoFileEntry* newFileEntry = &newFileHeader->entries.back();
					newFileEntry->parent = newFileHeader;
					newFileEntry->index = 0x00;
					newFileEntry->groupID = groupToLink;

					signalINFOSectionSizeChange();
					signalFILESectionSizeChange();

					infoSection.updateChildStructOffsetValues(brsarInfoSection::infoSectionLandmark::iSL_FileHeaders);
					updateInfoSectionFileOffsets();
				}
			}

			return result;
		}

		bool brsar::summarizeSymbStringData(std::ostream& output)
		{
			return symbSection.dumpStrings(output);
		}
		bool brsar::outputConsecutiveSoundEntryStringsWithSameFileID(unsigned long startingIndex, std::ostream& output)
		{
			bool result = 0;

			if (output.good() && symbSection.address != ULONG_MAX)
			{
				unsigned long fileID = infoSection.soundEntries[startingIndex]->fileID;
				std::size_t i = startingIndex;
				while (i < infoSection.soundEntries.size() && infoSection.soundEntries[i]->fileID == fileID)
				{
					output << "\t[String 0x" << numToHexStringWithPadding(i, 0x04) << "] " << getSymbString(i) << "\n";
					i++;
				}
			}

			return result;
		}
		bool brsar::doFileDump(std::string dumpRootFolder, bool joinHeaderAndData)
		{
			bool result = 0;

			std::filesystem::create_directories(dumpRootFolder);
			brsarInfoGroupHeader* currHeader = nullptr;
			brsarInfoGroupEntry* currEntry = nullptr;
			std::ofstream metadataOutput(dumpRootFolder + "summary.txt");
			metadataOutput << "lavaBRSARLib " << version << "\n\n";
			metadataOutput << "BRSAR File Dump Summary:\n";
			MD5 md5Object;
			for (std::size_t i = 0; i < infoSection.groupHeaders.size(); i++)
			{
				currHeader = infoSection.groupHeaders[i].get();
				std::string groupName = symbSection.getString(currHeader->groupID);
				if (groupName.size() == 0x00)
				{
					groupName = "[NAMELESS]";
				}
				std::string groupFolder = dumpRootFolder + groupName + "/";
				std::filesystem::create_directory(groupFolder);
				metadataOutput << "File(s) in \"" << groupName << "\" (Raw ID: " 
					<< numToDecStringWithPadding(currHeader->groupID, 0x03) << " / 0x" << numToHexStringWithPadding(currHeader->groupID, 0x03) << 
					", Info Index: " << numToDecStringWithPadding(i, 0x03) << " / 0x" << numToHexStringWithPadding(i, 0x03) << ")...\n";
				for (std::size_t u = 0; u < currHeader->entries.size(); u++)
				{
					currEntry = &currHeader->entries[u];
					std::vector<std::size_t>* idToIndexVecPtr = &fileSection.fileIDToIndex[currEntry->fileID];
					std::string fileBaseName = numToDecStringWithPadding(currEntry->fileID, 0x03) + "_(0x" + numToHexStringWithPadding(currEntry->fileID, 0x03) + ")";
					bool entryExported = 0;
					std::size_t y = 0;
					while(!entryExported)
					{
						brsarFileFileContents* fileContentsPtr = &fileSection.fileContents[(*idToIndexVecPtr)[y]];
						if (fileContentsPtr->groupID == currHeader->groupID)
						{
							metadataOutput << "\tFile " << numToDecStringWithPadding(currEntry->fileID, 0x03) << " (0x" << numToHexStringWithPadding(currEntry->fileID, 0x03) << ") @ 0x" << numToHexStringWithPadding(fileContentsPtr->getHeaderAddress(), 0x08) << "\n";
							metadataOutput << "\t\tFile Type: ";
							if (fileContentsPtr->header.size() >= 0x04)
							{
								metadataOutput << fileContentsPtr->header[0] << fileContentsPtr->header[1] << fileContentsPtr->header[2] << fileContentsPtr->header[3] << "\n";
							}
							else
							{
								metadataOutput << "----\n";
							}
							metadataOutput << "\t\tHeader Size: " << fileContentsPtr->header.size() << " byte(s) (" << bytesToFileSizeString(fileContentsPtr->header.size()) << ")\n";
							metadataOutput << "\t\tData Size: " << fileContentsPtr->data.size() << " byte(s) (" << bytesToFileSizeString(fileContentsPtr->data.size()) << ")\n";
							metadataOutput << "\t\tTotal Size: " << fileContentsPtr->header.size() + fileContentsPtr->data.size() << " byte(s) (" << bytesToFileSizeString(fileContentsPtr->header.size() + fileContentsPtr->data.size()) << ")\n";
							metadataOutput << "\t\tNumber Times File Occurs in BRSAR: " << idToIndexVecPtr->size() << "\n";
							metadataOutput << "\t\tNumber of Current Occurence: " << numberToOrdinal(y + 1) << " (Suffixed with \"_" << std::string(1, 'A' + y) << "\")\n";
							metadataOutput << "\t\tHeader MD5 Hash: " << md5Object((char*)fileContentsPtr->header.data(), fileContentsPtr->header.size()) << "\n";
							metadataOutput << "\t\tData MD5 Hash: " << md5Object((char*)fileContentsPtr->data.data(), fileContentsPtr->data.size()) << "\n";

							std::string headerFilename = "";
							std::string dataFilename = "";
							headerFilename += "_" + std::string(1, 'A' + y);
							dataFilename += "_" + std::string(1, 'A' + y);
							if (joinHeaderAndData)
							{
								headerFilename += "_joined.dat";
								fileContentsPtr->dumpToFile(headerFilename);
							}
							else
							{
								headerFilename += "_header.dat";
								dataFilename += "_data.dat";
								fileContentsPtr->dumpHeaderToFile(headerFilename);
								fileContentsPtr->dumpDataToFile(dataFilename);
							}
							entryExported = 1;
						}
						y++;
					}
				}
			}

			return result = 1;
		}

		/* BRSAR */
	}
}