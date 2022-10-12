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
				{
					result = brsarHexTagType::bhtt_FILE_SECTION;
					break;
				}
				case brsarHexTags::bht_RWSD_DATA:
				case brsarHexTags::bht_RWSD_WAVE:
				case brsarHexTags::bht_RWSD_RWAR:
				{
					result = brsarHexTagType::bhtt_RWSD_SUBSECTION;
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
		int padLengthTo(unsigned long lengthIn, unsigned long padTo, bool allowZeroPaddingLength)
		{
			unsigned long padLength = padTo - (lengthIn % padTo);
			if (!allowZeroPaddingLength && padLength == padTo)
			{
				padLength = 0x00;
			}
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
				}
				if (encoding == 2)
				{
					for (unsigned long i = 0; i < channelInfoEntries.size(); i++)
					{
						adpcmInfoEntries.push_back(adpcmInfo());
						adpcmInfoEntries.back().populate(bodyIn, address + channelInfoEntries[i].adpcmInfoOffset);
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
				}
				if (encoding == 2)
				{
					for (unsigned long i = 0x0; i < channelInfoEntries.size(); i++)
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

				lava::writeRawDataToStream(destinationStream, unsigned long(stringEntryOffsets.size()));
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

		// File File Contents Struct Functions - Used in BRSAR INFO Header
		unsigned long brsarFileFileContents::size() const
		{
			return header.size() + data.size();
		}
		unsigned long brsarFileFileContents::getFileType() const
		{
			unsigned long result = ULONG_MAX;

			if (header.size() >= 0x04)
			{
				result = lava::bytesToFundamental<unsigned long>(header.data());
			}

			return result;
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

			result += sizeof(unsigned long); // File Header Length Field Size
			result += sizeof(unsigned long); // File Data Length Field Size
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
				// Store these for use in externalized files, whose header/data lengths we can't determine based on the BRSAR itself
				originalFileHeaderLength = bodyIn.getLong(cursor, &cursor);
				originalFileDataLength = bodyIn.getLong(cursor, &cursor);
				entryNumber = bodyIn.getLong(cursor, &cursor);
				stringOffset = brawlReference(bodyIn.getLLong(cursor, &cursor));
				listOffset = brawlReference(bodyIn.getLLong(cursor, &cursor));

				if (stringOffset.getAddress() != 0x00)
				{
					std::string measurementString = bodyIn.data() + (stringOffset.getAddress(parent->getAddress() + 0x08));
					// Note: the final bool here determines whether allowing 0 padding length is allowed. 9stars allows this, other BRSARs do not?)
					std::size_t sizePrescription = padLengthTo(measurementString.size(), 0x04, 1);
					// The following code is for logging whether or not we get the prescribed lengths right when reading in a brsar.
					// Leaving this in cuz I'll need to come back to it later to ensure compatability with different BRSARs.
					/*std::cout << "\"" << measurementString << "\" @ 0x" << 
						numToHexStringWithPadding(stringOffset.getAddress(parent->getAddress() + 0x08), 0x08) << ":\n" << 
						"\tReal Size = 0x" << numToHexStringWithPadding(measurementString.size(), 0x02) << "\n" <<
						"\tPrescribed Size = 0x" << numToHexStringWithPadding(sizePrescription, 0x02) << "\n" <<
						"\tActual Padded Size = 0x" << numToHexStringWithPadding(listOffset.address - stringOffset.address, 0x02) << "\n" <<
						"\tRegion End = 0x" << numToHexStringWithPadding(stringOffset.getAddress(parent->getAddress() + 0x08) + sizePrescription, 0x08) <<
						"\n";*/
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
				// If this is a normal, internal file...
				if (stringContent.empty())
				{
					// Use the currently stored fileContents to determine our length values.
					lava::writeRawDataToStream(destinationStream, unsigned long(fileContents.header.size()));
					lava::writeRawDataToStream(destinationStream, unsigned long(fileContents.data.size()));
				}
				// But if this is instead an external file...
				else
				{
					// Fall back to the length values we loaded from the BRSAR originally.
					lava::writeRawDataToStream(destinationStream, originalFileHeaderLength);
					lava::writeRawDataToStream(destinationStream, originalFileDataLength);
				}
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
			relativeOffset += sizeof(unsigned long); // File Header Length Field Size
			relativeOffset += sizeof(unsigned long); // File Data Length Field Size
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
					for (std::size_t u = 0; u < currGroupHeader->entries.size(); u++)
					{
						brsarInfoGroupEntry* currGroupEntry = &currGroupHeader->entries[u];
						fileIDsToGroupInfoIndecesThatUseThem[currGroupEntry->fileID].push_back(currGroupHeader->groupID);
					}
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
			parent->signalINFOSectionSizeChange();
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

		unsigned long brsarInfoSection::virutalFileSectionSize() const
		{
			unsigned long result = 0x00;
			result += 0x20; // Size of the FILE Section Header
			for (int i = 0; i < groupHeaders.size(); i++)
			{
				brsarInfoGroupHeader* currGroupHeader = groupHeaders[i].get();
				unsigned long groupHeaderStartRelativeOffset = 0x00;
				for (int u = 0; u < currGroupHeader->entries.size(); u++)
				{
					brsarInfoGroupEntry* currGroupEntry = &currGroupHeader->entries[u];
					result += currGroupEntry->headerLength;
					result += currGroupEntry->dataLength;
				}
			}
			return result;
		}
		bool brsarInfoSection::updateGroupEntryAddressValues()
		{
			bool result = 1;

			try
			{
				unsigned long fileSectionAddress = parent->getVirtualFILESectionAddress();
				unsigned long fileSectionRelativeOffset = 0x20;
				for (int i = 0; i < groupHeaders.size(); i++)
				{
					brsarInfoGroupHeader* currGroupHeader = groupHeaders[i].get();
					currGroupHeader->headerAddress = fileSectionAddress + fileSectionRelativeOffset;
					unsigned long groupHeaderStartRelativeOffset = 0x00;
					for (int u = 0; u < currGroupHeader->entries.size(); u++)
					{
						brsarInfoGroupEntry* currGroupEntry = &currGroupHeader->entries[u];
						brsarInfoFileHeader* currFileHeader = getFileHeaderPointer(currGroupEntry->fileID);
						if (u == 0 || currGroupEntry->headerOffset != 0x00000000)
						{
							currGroupEntry->headerOffset = groupHeaderStartRelativeOffset;
						}
						else
						{
							currGroupEntry->headerOffset = 0x00000000;
						}
						currGroupEntry->headerLength = currFileHeader->fileContents.header.size();
						groupHeaderStartRelativeOffset += currGroupEntry->headerLength;
					}
					currGroupHeader->headerLength = groupHeaderStartRelativeOffset;
					fileSectionRelativeOffset += groupHeaderStartRelativeOffset;
					currGroupHeader->dataAddress = fileSectionAddress + fileSectionRelativeOffset;
					unsigned long groupDataStartRelativeOffset = 0x00;
					for (int u = 0; u < currGroupHeader->entries.size(); u++)
					{
						brsarInfoGroupEntry* currGroupEntry = &currGroupHeader->entries[u];
						brsarInfoFileHeader* currFileHeader = getFileHeaderPointer(currGroupEntry->fileID);
						if (u == 0 || currGroupEntry->dataOffset != 0x00000000)
						{
							currGroupEntry->dataOffset = groupDataStartRelativeOffset;
						}
						else
						{
							currGroupEntry->dataOffset = 0x00000000;
						}
						currGroupEntry->dataLength = currFileHeader->fileContents.data.size();
						groupDataStartRelativeOffset += currGroupEntry->dataLength;
					}
					currGroupHeader->dataLength = groupDataStartRelativeOffset;
					fileSectionRelativeOffset += groupDataStartRelativeOffset;
				}
			}
			catch (std::exception oopsie)
			{
				std::cerr << "Something went wrong updating Group Entry Address Values!\n\tException: " << oopsie.what();
				result = 0;
			}

			return result;
		}

		unsigned long brsarInfoSection::addNewFileEntry()
		{
			unsigned long newID = ULONG_MAX;

			fileHeaders.push_back(std::make_unique<brsarInfoFileHeader>());
			brsarInfoFileHeader* newFileHeader = fileHeaders.back().get();
			if (newFileHeader != nullptr)
			{
				newFileHeader->parent = this;
				newFileHeader->originalFileHeaderLength = 0x00;
				newFileHeader->originalFileDataLength = 0x00;
				newID = fileHeaders.size() - 1;

				parent->signalINFOSectionSizeChange();
				parent->signalVirtualFILESectionSizeChange();
				updateChildStructOffsetValues(brsarInfoSection::infoSectionLandmark::iSL_FileHeaders);
				updateGroupEntryAddressValues();
			}
			
			return newID;
		}
		bool brsarInfoSection::linkFileEntryToGroup(unsigned long fileID, unsigned long groupInfoIndexToLinkTo)
		{
			bool result = 0;

			brsarInfoFileHeader* targetFileHeader = getFileHeaderPointer(fileID);
			if (targetFileHeader != nullptr)
			{
				brsarInfoGroupHeader* targetGroupHeader = getGroupWithInfoIndex(groupInfoIndexToLinkTo);
				if (targetGroupHeader != nullptr)
				{
					targetFileHeader->entries.push_back(brsarInfoFileEntry());
					brsarInfoFileEntry* newFileEntry = &targetFileHeader->entries.back();
					newFileEntry->parent = targetFileHeader;
					newFileEntry->originalAddress = brsarAddressConsts::bac_NOT_IN_FILE;
					newFileEntry->groupID = groupInfoIndexToLinkTo;
					newFileEntry->index = targetGroupHeader->entries.size();

					targetGroupHeader->entries.push_back(brsarInfoGroupEntry());
					brsarInfoGroupEntry* newGroupEntry = &targetGroupHeader->entries.back();
					newGroupEntry->parent = targetGroupHeader;
					newGroupEntry->originalAddress = brsarAddressConsts::bac_NOT_IN_FILE;;
					newGroupEntry->fileID = fileID;

					fileIDsToGroupInfoIndecesThatUseThem[fileID].push_back(targetGroupHeader->groupID);

					parent->signalINFOSectionSizeChange();
					parent->signalVirtualFILESectionSizeChange();
					updateChildStructOffsetValues(brsarInfoSection::infoSectionLandmark::iSL_FileHeaders);
					updateGroupEntryAddressValues();
					result = 1;
				}
			}

			return result;
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
					output << "\tFile Header Length: " << currHeaderPtr->fileContents.header.size() << " byte(s)\n";
					output << "\tFile Data Length: " << currHeaderPtr->fileContents.data.size() << " byte(s)\n";
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

		/* RWSD */

		void rwsdWaveSection::waveEntryPushBack(const waveInfo& sourceWave)
		{
			entries.push_back(sourceWave);
			entries.front().address = brsarAddressConsts::bac_NOT_IN_FILE;
		}
		void rwsdWaveSection::waveEntryPushFront(const waveInfo& sourceWave)
		{
			entries.insert(entries.begin(), sourceWave);
			entries.front().address = brsarAddressConsts::bac_NOT_IN_FILE;
		}
		unsigned long rwsdWaveSection::size() const
		{
			unsigned long result = 0x00;
			result += sizeof(unsigned long); // Size of TAG Field
			result += sizeof(unsigned long); // Size of Length Field
			result += sizeof(unsigned long); // Size of Entry Count
			result += sizeof(unsigned long) * entries.size(); // Length of Entry Offset List
			for (unsigned long i = 0x0; i < entries.size(); i++)
			{
				result += entries[i].size();
			}
			return result;
		}
		std::vector<unsigned long> rwsdWaveSection::calculateOffsetVector() const
		{
			std::vector<unsigned long> result{};

			unsigned long calculatedEntryOffset =
				sizeof(unsigned long) // Length of WAVE Tag
				+ sizeof(unsigned long) // Length of WAVE Section length field
				+ sizeof(unsigned long) // Length of entry count field
				+ entries.size() * sizeof(unsigned long) // Length of entry offset vector
				;

			for (unsigned long i = 0x0; i < entries.size(); i++)
			{
				result.push_back(calculatedEntryOffset);
				calculatedEntryOffset += entries[i].size();
			}

			return result;
		}
		unsigned long rwsdWaveSection::paddedSize(unsigned long padTo) const
		{
			return padLengthTo(size(), padTo);
		}
		bool rwsdWaveSection::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated() && bodyIn.getLong(addressIn) == brsarHexTags::bht_RWSD_WAVE)
			{
				address = addressIn;

				originalLength = bodyIn.getLong(addressIn + 0x04); // We no longer need this, we calculate lengths ourselves
				unsigned long entryCount = bodyIn.getLong(addressIn + 0x08);

				std::vector<unsigned long> entryOffsets{};
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
				unsigned long expectedLength = paddedSize();

				lava::writeRawDataToStream(destinationStream, brsarHexTags::bht_RWSD_WAVE);
				lava::writeRawDataToStream(destinationStream, expectedLength);
				lava::writeRawDataToStream(destinationStream, unsigned long(entries.size()));

				std::vector<unsigned long> calculatedEntryOffsetVec = calculateOffsetVector();
				for (unsigned long i = 0x0; i < entries.size(); i++)
				{
					lava::writeRawDataToStream(destinationStream, calculatedEntryOffsetVec[i]);
				}
				for (unsigned long i = 0x0; i < entries.size(); i++)
				{
					entries[i].exportContents(destinationStream);
				}

				unsigned long finalStreamPos = destinationStream.tellp();
				unsigned long lengthOfExport = finalStreamPos - initialStreamPos;
				if (lengthOfExport != expectedLength)
				{
					if (lengthOfExport < expectedLength)
					{
						std::vector<char> padding(expectedLength - lengthOfExport, 0x00);
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
		unsigned long rwsdDataSection::size() const
		{
			unsigned long result = 0x00;

			result += sizeof(unsigned long); // Size of DATA Tag
			result += sizeof(unsigned long); // Size of Length Field
			result += calcRefVecSize(entries.size()); // Length of the Entries Ref Vec
			for (int i = 0; i < entries.size(); i++) 
			{
				result += entries[i].size(); // Size of Each Entry
			}

			return result;
		}
		unsigned long rwsdDataSection::paddedSize(unsigned long padTo) const
		{
			return padLengthTo(size(), padTo);
		}
		bool rwsdDataSection::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated() && bodyIn.getLong(addressIn) == brsarHexTags::bht_RWSD_DATA)
			{
				address = addressIn;
				originalLength = bodyIn.getLong(addressIn + 0x04); // We no longer need this either, this will be calculated as needed
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
				unsigned long expectedLength = paddedSize();

				lava::writeRawDataToStream(destinationStream, brsarHexTags::bht_RWSD_DATA);
				lava::writeRawDataToStream(destinationStream, expectedLength);
				entryReferences.exportContents(destinationStream);
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					entries[i].exportContents(destinationStream);
				}
				unsigned long finalStreamPos = destinationStream.tellp();
				unsigned long lengthOfExport = finalStreamPos - initialStreamPos;
				if (lengthOfExport != expectedLength)
				{
					if (lengthOfExport < expectedLength)
					{
						std::vector<char> padding(expectedLength - lengthOfExport, 0x00);
						destinationStream.write(padding.data(), padding.size());
					}
				}
				result = destinationStream.good();
			}
			return result;
		}

		/*bool rwsdHeader::populate(const lava::byteArray& bodyIn, std::size_t addressIn)
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
		constexpr unsigned long rwsdHeader::size()
		{
			unsigned long result = 0x0;

			result += sizeof(unsigned long); // Length of RWSD Tag
			result += sizeof(endianType);
			result += sizeof(version);
			result += sizeof(headerLength);
			result += sizeof(entriesOffset);
			result += sizeof(entriesCount);
			result += sizeof(dataOffset);
			result += sizeof(dataLength);
			result += sizeof(waveOffset);
			result += sizeof(waveLength);

			return result;
		}*/

		unsigned long rwsd::size()
		{
			unsigned long result = 0x00;

			result =
				0x20 // Header Length
				+ getDATASectionSize() // DATA Section Size
				+ getWAVESectionSize(); // WAVE Section Size

			return result;
		}
		void rwsd::signalDATASectionSizeChange()
		{
			dataSectionCachedSize = ULONG_MAX;
		}
		void rwsd::signalWAVESectionSizeChange()
		{
			waveSectionCachedSize = ULONG_MAX;
		}
		unsigned long rwsd::getDATASectionSize()
		{
			if (dataSectionCachedSize == ULONG_MAX)
			{
				dataSectionCachedSize = dataSection.paddedSize();
			}
			return dataSectionCachedSize;
		}
		unsigned long rwsd::getWAVESectionSize()
		{
			if (waveSectionCachedSize == ULONG_MAX)
			{
				waveSectionCachedSize = waveSection.paddedSize();
			}
			return waveSectionCachedSize;
		}
		unsigned long rwsd::getDATASectionOffset()
		{
			return 0x20;
		}
		unsigned long rwsd::getWAVESectionOffset()
		{
			return getDATASectionOffset() + getDATASectionSize();
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
				address = fileBodyAddressIn;

				endianType = fileBodyIn.getShort(0x04);
				versionNumber = fileBodyIn.getShort(0x06);
				unsigned long dataSectionOffset = fileBodyIn.getLong(0x10);
				unsigned long waveSectionOffset = fileBodyIn.getLong(0x18);

				result = dataSection.populate(fileBodyIn, fileBodyAddressIn + dataSectionOffset);
				result &= waveSection.populate(fileBodyIn, fileBodyAddressIn + waveSectionOffset);
				if (result && rawDataAddressIn != ULONG_MAX && (rawDataAddressIn + rawDataLengthIn) <= rawDataIn.size())
				{
					result &= populateWavePackets(rawDataIn, rawDataAddressIn, rawDataLengthIn);
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

		bool rwsd::exportFileSection(std::ostream& destinationStream)
		{
			bool result = 0;

			if (destinationStream.good())
			{
				result = 1;

				// Write Header Data
				unsigned long cachedDataSectionSize = dataSection.paddedSize();
				unsigned long cachedWaveSectionSize = waveSection.paddedSize();
				writeRawDataToStream(destinationStream, brsarHexTags::bht_RWSD); // Write RWSD Tag
				writeRawDataToStream(destinationStream, endianType); // Write Big-Endian BOM
				writeRawDataToStream(destinationStream, versionNumber); // Write Version
				writeRawDataToStream(destinationStream, size()); // RWSD Length
				writeRawDataToStream(destinationStream, unsigned short(getDATASectionOffset())); // Offset to first subsection
				writeRawDataToStream(destinationStream, unsigned short(0x02)); // Write number of subsections
				writeRawDataToStream(destinationStream, getDATASectionOffset()); // DATA Subsection Offset
				writeRawDataToStream(destinationStream, getDATASectionSize()); // DATA Subsection Length
				writeRawDataToStream(destinationStream, getWAVESectionOffset()); // WAVE Subsection Offset
				writeRawDataToStream(destinationStream, getWAVESectionSize()); // WAVE Subsection Length

				// Write Subsections
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

			if (output.good() && address != UINT_MAX)
			{
				output << "RWSD Content Summary - lavaBRSARLib " << version << "\n";
				output << "\nNote: In the following summarization, all addresses are relative to the start of the described RWSD file.\n";
				output << "Additionally, any absolute Wave Content Address values assume the wave content block directly follows the WAVE section.\n";
				output << "While this is always true of individual RWSD files produced with this library, it *is not guaranteed* in any other case!\n";
				output << "\nStructure Overview:\n";
				output << "\tTotal Length/End:\t0x" << lava::numToHexStringWithPadding(size(), 0x08) << " / 0x" << lava::numToHexStringWithPadding(size(), 0x08) << "\n";
				output << "\tData Section Offset:\t0x" << lava::numToHexStringWithPadding(getDATASectionOffset(), 0x08) << "\n";
				output << "\tData Section Length/End:\t0x" << lava::numToHexStringWithPadding(getDATASectionSize(), 0x08) << " / 0x" << lava::numToHexStringWithPadding(getDATASectionOffset() + getDATASectionSize(), 0x08) << "\n";
				output << "\tData Entry Count:\t\t0x" << lava::numToHexStringWithPadding(dataSection.entries.size(), 0x04) << "\n";
				output << "\tWave Section Offset:\t0x" << lava::numToHexStringWithPadding(getWAVESectionOffset(), 0x08) << "\n";
				output << "\tWave Section Length/End:\t0x" << lava::numToHexStringWithPadding(getWAVESectionSize(), 0x08) << " / 0x" << lava::numToHexStringWithPadding(getWAVESectionOffset() + getWAVESectionSize(), 0x08) << "\n";
				output << "\tWave Entry Count:\t\t0x" << lava::numToHexStringWithPadding(waveSection.entries.size(), 0x04) << "\n";
				output << "\nEntry Summary:\n";

				std::unordered_map<unsigned long, std::vector<unsigned long>> waveIndecesToReferrerDataIndeces{};

				std::vector<dataInfo>* dataVecPtr = &dataSection.entries;
				std::vector<waveInfo>* waveVecPtr = &waveSection.entries;
				brawlReferenceVector* dataRefVecPtr = &dataSection.entryReferences;
				std::vector<unsigned long> waveOffVec = waveSection.calculateOffsetVector();

				unsigned long waveDataBaseAddress = getWAVESectionOffset() + getWAVESectionSize();

				for (unsigned long u = 0; u < dataVecPtr->size(); u++)
				{
					dataInfo* currDataEntry = &(*dataVecPtr)[u];
					output << "\tData Entry 0x" << lava::numToHexStringWithPadding(u, 0x04) << "\n";
					output << "\t\tOffset / Address:\t0x" << lava::numToHexStringWithPadding(dataRefVecPtr->refs[u].address, 0x08) << " / 0x" << lava::numToHexStringWithPadding(getDATASectionOffset() + dataRefVecPtr->refs[u].address, 0x08) << "\n";
					output << "\t\tAssociated Wave ID:\t0x" << lava::numToHexStringWithPadding(currDataEntry->ntWaveIndex, 0x04) << "\n";
					if (currDataEntry->ntWaveIndex != ULONG_MAX)
					{
						auto emplaceResult = waveIndecesToReferrerDataIndeces.emplace(std::make_pair(currDataEntry->ntWaveIndex, std::vector<unsigned long>()));
						if (emplaceResult.second)
						{
							waveInfo* currWaveEntry = &(*waveVecPtr)[currDataEntry->ntWaveIndex];
							output << "\t\tWave Entry Offset / Address:\t0x" << lava::numToHexStringWithPadding(waveOffVec[currDataEntry->ntWaveIndex], 0x08) << " / 0x" << lava::numToHexStringWithPadding(getWAVESectionOffset() + waveOffVec[currDataEntry->ntWaveIndex], 0x08) << "\n";
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

			if (pushFront)
			{
				// Add entry to front of WAVE entry vec.
				waveSection.waveEntryPushFront(sourceWave);
				// Since we've added the entry to the front, the indeces for every other sound have been shifted up by 1
				// So we need to iterate through all the DATA entries and shift their indeces.
				for (unsigned long i = 0; i < dataSection.entries.size(); i++)
				{
					dataSection.entries[i].ntWaveIndex++;
				}
				// Recalculate WAVE entry data locations.
				if (updateWaveEntryDataLocations())
				{
					// Point the targeted DATA entry to the WAVE entry we just inserted.
					dataSection.entries[dataSectionIndex].ntWaveIndex = 0;
				}
			}
			else
			{
				// Add entry to back of WAVE entry vec.
				waveSection.waveEntryPushBack(sourceWave);
				// Recalculate WAVE entry data locations.
				if (updateWaveEntryDataLocations())
				{
					dataSection.entries[dataSectionIndex].ntWaveIndex = waveSection.entries.size() - 1;
				}
			}

			signalWAVESectionSizeChange();

			return result;
		}

		/* RWSD */

		/* BRSAR File Section */



		/* BRSAR */

		unsigned long brsar::size()
		{
			return headerLength + getSYMBSectionSize() + getINFOSectionSize() + getVirtualFILESectionSize();
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
					// Populate INFO Section's brsarInfoFileHeader structs with their respective raw File Contents
					if (contents.populated() && contents.getLong(getVirtualFILESectionAddress()) == brsarHexTags::bht_FILE)
					{
						brsarInfoGroupHeader* currHeader = nullptr;
						brsarInfoGroupEntry* currEntry = nullptr;
						for (std::size_t i = 0; i < infoSection.groupHeaders.size(); i++)
						{
							currHeader = infoSection.groupHeaders[i].get();
							for (std::size_t u = 0; u < currHeader->entries.size(); u++)
							{
								currEntry = &currHeader->entries[u];
								brsarInfoFileHeader* targetFileHeader = infoSection.getFileHeaderPointer(currEntry->fileID);
								if (targetFileHeader != nullptr)
								{
									targetFileHeader->fileContents.header = contents.getBytes(currEntry->headerLength, currHeader->headerAddress + currEntry->headerOffset);
									targetFileHeader->fileContents.data = contents.getBytes(currEntry->dataLength, currHeader->dataAddress + currEntry->dataOffset);
								}
							}
						}
					}
					// Recalculate the referenced addresses and offsets used in the INFO Section's brsarInfoGroupHeader/Entry structs
					signalVirtualFILESectionSizeChange();
					infoSection.updateGroupEntryAddressValues();
				}
			}
			return result;
		}
		bool brsar::exportVirtualFileSection(std::ostream& destinationStream)
		{
			unsigned long fileLengthTotal = 0x20;
			unsigned long preFileDumpStreamPos = destinationStream.tellp();
			destinationStream.write(std::vector<char>(0x20, 0x00).data(), 0x20);
			for (int i = 0; i < infoSection.groupHeaders.size(); i++)
			{
				brsarInfoGroupHeader* currGroupHeader = infoSection.groupHeaders[i].get();
				for (int u = 0; u < currGroupHeader->entries.size(); u++)
				{
					brsarInfoGroupEntry* currGroupEntry = &currGroupHeader->entries[u];
					brsarInfoFileHeader* currFileHeader = infoSection.getFileHeaderPointer(currGroupEntry->fileID);
					if (currFileHeader != nullptr)
					{
						fileLengthTotal += currFileHeader->fileContents.header.size();
						destinationStream.write((char*)currFileHeader->fileContents.header.data(), currFileHeader->fileContents.header.size());
					}
				}
				for (int u = 0; u < currGroupHeader->entries.size(); u++)
				{
					brsarInfoGroupEntry* currGroupEntry = &currGroupHeader->entries[u];
					brsarInfoFileHeader* currFileHeader = infoSection.getFileHeaderPointer(currGroupEntry->fileID);
					if (currFileHeader != nullptr)
					{
						fileLengthTotal += currFileHeader->fileContents.data.size();
						destinationStream.write((char*)currFileHeader->fileContents.data.data(), currFileHeader->fileContents.data.size());
					}
				}
			}
			destinationStream.seekp(preFileDumpStreamPos);
			writeRawDataToStream(destinationStream, unsigned long(brsarHexTags::bht_FILE));
			writeRawDataToStream(destinationStream, fileLengthTotal);
			destinationStream.write(std::vector<char>(0x18, 0x00).data(), 0x18);
			return 1;
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
			writeRawDataToStream(destinationStream, getVirtualFILESectionAddress());
			writeRawDataToStream(destinationStream, getVirtualFILESectionSize());
			if (headerLength > destinationStream.tellp())
			{
				std::vector<char> padding(headerLength - destinationStream.tellp(), 0x00);
				destinationStream.write(padding.data(), padding.size());
			}
			result &= symbSection.exportContents(destinationStream);
			result &= infoSection.exportContents(destinationStream);
			result &= exportVirtualFileSection(destinationStream);
			
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
		void brsar::signalVirtualFILESectionSizeChange()
		{
			virtualFileSectionCachedSize = ULONG_MAX;
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
		unsigned long brsar::getVirtualFILESectionSize()
		{
			if (virtualFileSectionCachedSize == ULONG_MAX)
			{
				virtualFileSectionCachedSize = infoSection.virutalFileSectionSize();
			}
			return virtualFileSectionCachedSize;
		}

		unsigned long brsar::getSYMBSectionAddress()
		{
			return headerLength;
		}
		unsigned long brsar::getINFOSectionAddress()
		{
			return headerLength + getSYMBSectionSize();
		}
		unsigned long brsar::getVirtualFILESectionAddress()
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

		bool brsar::overwriteFile(const std::vector<unsigned char>& headerIn, const std::vector<unsigned char>& dataIn, unsigned long fileIDIn)
		{
			bool result = 0;

			brsarInfoFileHeader* fileHeaderPtr = infoSection.getFileHeaderPointer(fileIDIn);
			if (fileHeaderPtr != nullptr)
			{
				result = 1;
				// Replace File Contents
				fileHeaderPtr->fileContents.header = headerIn;
				fileHeaderPtr->fileContents.data = dataIn;
				// Update the rest of the infoSection to correct the changes to file locations
				result &= infoSection.updateGroupEntryAddressValues();
				signalVirtualFILESectionSizeChange();
			}

			return result;
		}
		bool brsar::cloneFile(unsigned long fileIDToClone, unsigned long groupToLink)
		{
			bool result = 0;

			brsarInfoFileHeader* targetFileHeader = infoSection.getFileHeaderPointer(fileIDToClone);
			if (targetFileHeader != nullptr)
			{
				unsigned long newFileID = infoSection.addNewFileEntry();
				if (newFileID != ULONG_MAX)
				{
					result = infoSection.linkFileEntryToGroup(newFileID, groupToLink);
					if (result)
					{
						result &= overwriteFile(targetFileHeader->fileContents.header, targetFileHeader->fileContents.data, newFileID);
						signalINFOSectionSizeChange();
						signalVirtualFILESectionSizeChange();
						infoSection.updateChildStructOffsetValues(brsarInfoSection::infoSectionLandmark::iSL_FileHeaders);
						infoSection.updateGroupEntryAddressValues();
					}
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
		bool brsar::doFileDump(std::string dumpRootFolder, bool joinHeaderAndData, bool doSummaryOnly)
		{
			bool result = 0;

			std::filesystem::create_directories(dumpRootFolder);
			brsarInfoGroupHeader* currHeader = nullptr;
			brsarInfoGroupEntry* currEntry = nullptr;
			std::ofstream metadataOutput(dumpRootFolder + "summary.txt");
			metadataOutput << "lavaBRSARLib " << lava::brawl::version << "\n\n";
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
				if (!doSummaryOnly)
				{
					std::filesystem::create_directory(groupFolder);
				}
				metadataOutput << "File(s) in \"" << groupName << "\" (Raw ID: " 
					<< numToDecStringWithPadding(currHeader->groupID, 0x03) << " / 0x" << numToHexStringWithPadding(currHeader->groupID, 0x03) << 
					", Info Index: " << numToDecStringWithPadding(i, 0x03) << " / 0x" << numToHexStringWithPadding(i, 0x03) << ")...\n";
				for (std::size_t u = 0; u < currHeader->entries.size(); u++)
				{
					currEntry = &currHeader->entries[u];
					brsarInfoFileHeader* relevantFileHeader = infoSection.getFileHeaderPointer(currEntry->fileID);
					if (relevantFileHeader != nullptr)
					{
						std::string fileBaseName = numToDecStringWithPadding(currEntry->fileID, 0x03) + "_(0x" + numToHexStringWithPadding(currEntry->fileID, 0x03) + ")";
						bool entryExported = 0;

						if (infoSection.fileIDsToGroupInfoIndecesThatUseThem.find(currEntry->fileID) != infoSection.fileIDsToGroupInfoIndecesThatUseThem.end())
						{
							std::vector<std::size_t> groupsThisFileOccursIn = infoSection.fileIDsToGroupInfoIndecesThatUseThem[currEntry->fileID];
							for (std::size_t t = 0; !entryExported && t < groupsThisFileOccursIn.size(); t++)
							{
								if (groupsThisFileOccursIn[t] == currHeader->groupID)
								{
									brsarFileFileContents* fileContentsPtr = &relevantFileHeader->fileContents;
									metadataOutput << "\tFile " << numToDecStringWithPadding(currEntry->fileID, 0x03) << " (0x" << numToHexStringWithPadding(currEntry->fileID, 0x03) << ") @ 0x" << numToHexStringWithPadding(currHeader->headerAddress + currEntry->headerOffset, 0x08) << "\n";
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
									metadataOutput << "\t\tHeader Offset / Absolute Address: 0x"
										<< numToHexStringWithPadding(currEntry->headerOffset, 0x02) << " / 0x" 
										<< numToHexStringWithPadding(currHeader->headerAddress + currEntry->headerOffset, 0x08) << "\n";
									metadataOutput << "\t\tData Offset / Absolute Address: 0x"
										<< numToHexStringWithPadding(currEntry->dataOffset, 0x02) << " / 0x"
										<< numToHexStringWithPadding(currHeader->dataAddress + currEntry->dataOffset, 0x08) << "\n";
									metadataOutput << "\t\tNumber Times File Occurs in BRSAR: " << groupsThisFileOccursIn.size() << "\n";
									metadataOutput << "\t\tNumber of Current Occurrence: " << numberToOrdinal(t + 1) << " (Suffixed with \"_" << std::string(1, 'A' + t) << "\")\n";
									metadataOutput << "\t\tHeader MD5 Hash: " << md5Object((char*)fileContentsPtr->header.data(), fileContentsPtr->header.size()) << "\n";
									metadataOutput << "\t\tData MD5 Hash: " << md5Object((char*)fileContentsPtr->data.data(), fileContentsPtr->data.size()) << "\n";

									std::string headerFilename = fileBaseName + "_" + std::string(1, 'A' + t);
									std::string dataFilename = fileBaseName + "_" + std::string(1, 'A' + t);
									if (!doSummaryOnly)
									{
										if (joinHeaderAndData)
										{
											unsigned long fileType = fileContentsPtr->getFileType();
											switch (fileType)
											{
												case brsarHexTags::bht_RWSD:
												{
													headerFilename += ".brwsd";
													break;
												}
												case brsarHexTags::bht_RBNK:
												{
													headerFilename += ".brbnk";
													break;
												}
												case brsarHexTags::bht_RSEQ:
												{
													headerFilename += ".brseq";
													break;
												}
												default:
												{
													headerFilename += ".dat";
													break;
												}
											}
											fileContentsPtr->dumpToFile(groupFolder + headerFilename);
										}
										else
										{
											headerFilename += "_header.dat";
											dataFilename += "_data.dat";
											fileContentsPtr->dumpHeaderToFile(groupFolder + headerFilename);
											fileContentsPtr->dumpDataToFile(groupFolder + dataFilename);
										}
									}
									entryExported = 1;
								}
							}
						}
					}
				}
			}

			return result = 1;
		}

		/* BRSAR */
	}
}