#include "lavaBRSARLib.h"

namespace lava
{
	namespace brawl
	{
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

		/* Misc. */



		/* Brawl Reference */

		brawlReference::brawlReference(unsigned long long valueIn)
		{
			addressType = valueIn >> 0x20;
			address = valueIn & 0x00000000FFFFFFFF;
		}
		bool brawlReference::isOffset()
		{
			return addressType & 0x01000000;
		}
		unsigned long brawlReference::getAddress(unsigned long relativeToIn)
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
		std::string brawlReference::getAddressString(unsigned long relativeToIn)
		{
			return numToHexStringWithPadding(getAddress(relativeToIn), 8);
		}
		unsigned long long brawlReference::getHex()
		{
			unsigned long long result = address | (unsigned long long(addressType) << 0x20);
			return result;
		}

		bool brawlReferenceVector::populate(lava::byteArray& bodyIn, std::size_t addressIn)
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
		std::vector<unsigned long> brawlReferenceVector::getHex()
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
		bool brawlReferenceVector::exportContents(std::ostream& destinationStream)
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

		/* Brawl Reference */



		/* BRSAR Symb Section */

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

		bool brsarSymbSection::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				std::size_t cursor1 = address + 0x04;

				length = bodyIn.getLong(cursor1, &cursor1);
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
						std::size_t numGotten = SIZE_MAX;
						stringBlock = bodyIn.getBytes(stringBlockEndAddr - stringBlockStartAddr, stringBlockStartAddr, numGotten);
					}
				}
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
				lava::writeRawDataToStream(destinationStream, length);
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
				if (length > amountWritten)
				{
					std::vector<char> finalPadding(length - amountWritten, 0x00);
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

		bool brsarInfo3DSoundInfo::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				std::size_t cursor = address;
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

		bool brsarInfoSequenceSoundInfo::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				std::size_t cursor = address;
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

		bool brsarInfoStreamSoundInfo::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				std::size_t cursor = address;
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

		bool brsarInfoWaveSoundInfo::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				std::size_t cursor = address;
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

		bool brsarInfoSoundEntry::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				stringID = bodyIn.getLong(address);
				fileID = bodyIn.getLong(address + 0x04);
				playerID = bodyIn.getLong(address + 0x08);
				param3DRefOffset = brawlReference(bodyIn.getLLong(address + 0x0C));
				volume = bodyIn.getChar(address + 0x14);
				playerPriority = bodyIn.getChar(address + 0x15);
				soundType = bodyIn.getChar(address + 0x16);
				remoteFilter = bodyIn.getChar(address + 0x17);
				soundInfoRef = brawlReference(bodyIn.getLLong(address + 0x18));
				userParam1 = bodyIn.getLong(address + 0x20);
				userParam2 = bodyIn.getLong(address + 0x24);
				panMode = bodyIn.getChar(address + 0x28);
				panCurve = bodyIn.getChar(address + 0x29);
				actorPlayerID = bodyIn.getChar(address + 0x2A);
				reserved = bodyIn.getChar(address + 0x2B);
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

		bool brsarInfoBankEntry::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				std::size_t cursor = address;
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

		bool brsarInfoPlayerEntry::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				std::size_t cursor = address;
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

		bool brsarInfoFileEntry::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				groupID = bodyIn.getLong(address);
				index = bodyIn.getLong(address + 0x04);
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
		bool brsarInfoFileHeader::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				headerLength = bodyIn.getLong(address);
				dataLength = bodyIn.getLong(address + 0x04);
				entryNumber = bodyIn.getLong(address + 0x08);
				stringOffset = brawlReference(bodyIn.getLLong(address + 0x0C));
				listOffset = brawlReference(bodyIn.getLLong(address + 0x14));
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
				result &= entryReferenceList.exportContents(destinationStream);
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					result &= entries[i].exportContents(destinationStream);
				}
				result &= destinationStream.good();
			}
			return result;
		}

		bool brsarInfoGroupEntry::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				fileID = bodyIn.getLong(address);
				headerOffset = bodyIn.getLong(address + 0x04);
				headerLength = bodyIn.getLong(address + 0x08);
				dataOffset = bodyIn.getLong(address + 0x0C);
				dataLength = bodyIn.getLong(address + 0x10);
				reserved = bodyIn.getLong(address + 0x14);

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
		bool brsarInfoGroupHeader::usesFileID(unsigned long fileIDIn) const
		{
			bool result = 0;

			for (unsigned long i = 0; !result && i < entries.size(); i++)
			{
				result = fileIDIn == entries[i].fileID;
			}

			return result;
		}
		bool brsarInfoGroupHeader::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				groupID = bodyIn.getLong(address);
				entryNum = bodyIn.getLong(address + 0x04);
				extFilePathRef = brawlReference(bodyIn.getLLong(address + 0x08));
				headerAddress = bodyIn.getLong(address + 0x10);
				headerLength = bodyIn.getLong(address + 0x14);
				dataAddress = bodyIn.getLong(address + 0x18);
				dataLength = bodyIn.getLong(address + 0x1C);
				listOffset = brawlReference(bodyIn.getLLong(address + 0x20));

				result = 1;
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
				result &= entryReferenceList.exportContents(destinationStream);
				for (std::size_t i = 0; i < entries.size(); i++)
				{
					result &= entries[i].exportContents(destinationStream);
				}
				result &= destinationStream.good();
			}
			return result;
		}

		brsarInfoGroupHeader* brsarInfoSection::getGroupWithID(unsigned long groupIDIn)
		{
			brsarInfoGroupHeader* result = nullptr;

			unsigned long i = 0;

			while (result == nullptr && i < groupHeaders.size())
			{
				if (groupIDIn == groupHeaders[i].groupID)
				{
					result = &groupHeaders[i];
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
				result = &groupHeaders[infoIndexIn];
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
						result.push_back( &fileHeaders[currentGroupEntry->fileID]);
					}
				}
			}

			//brsarInfoFileHeader* currHeader = nullptr;
			//for (std::size_t i = 0; i < fileHeaders.size(); i++)
			//{
			//	currHeader = &fileHeaders[i];
			//	for (std::size_t u = 0; u < currHeader->entries.size(); u++)
			//	{
			//		if (currHeader->entries[u].groupID == groupIDIn)
			//		{
			//			result.push_back(currHeader);
			//		}
			//	}
			//}

			return result;
		}
		brsarInfoFileHeader* brsarInfoSection::getFileHeaderPointer(unsigned long fileID)
		{
			brsarInfoFileHeader* result = nullptr;

			if (fileID < fileHeaders.size())
			{
				result = &fileHeaders[fileID];
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
					brsarInfoFileHeader* currHeaderPtr = &fileHeaders[i];
					output << "File Info #" << i << " (@ 0x" << numToHexStringWithPadding(currHeaderPtr->address, 0x08) << "):\n";
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
						output << "\tEntry #" << u << " (@ 0x" << numToHexStringWithPadding(currEntryPtr->address, 0x08) << "):\n";
						output << "\t\tEntry Index: " << currEntryPtr->index << "\n";
						output << "\t\tGroup ID: " << currEntryPtr->groupID << "\n";
					}
				}
				result = output.good();
			}
			return result;
		}
		bool brsarInfoSection::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				result = 1;
				address = addressIn;

				length = bodyIn.getLong(address + 0x04);
				soundsSectionReference = brawlReference(bodyIn.getLLong(address + 0x08));
				banksSectionReference = brawlReference(bodyIn.getLLong(address + 0x10));
				playerSectionReference = brawlReference(bodyIn.getLLong(address + 0x18));
				filesSectionReference = brawlReference(bodyIn.getLLong(address + 0x20));
				groupsSectionReference = brawlReference(bodyIn.getLLong(address + 0x28));
				footerReference = brawlReference(bodyIn.getLLong(address + 0x30));

				result &= soundsSection.populate(bodyIn, soundsSectionReference.getAddress(address + 0x08));
				result &= banksSection.populate(bodyIn, banksSectionReference.getAddress(address + 0x08));
				result &= playerSection.populate(bodyIn, playerSectionReference.getAddress(address + 0x08));
				result &= filesSection.populate(bodyIn, filesSectionReference.getAddress(address + 0x08));
				result &= groupsSection.populate(bodyIn, groupsSectionReference.getAddress(address + 0x08));

				brsarInfoSoundEntry* currSoundEntry = nullptr;
				soundEntries.resize(soundsSection.refs.size());
				for (std::size_t i = 0; i < soundsSection.refs.size(); i++)
				{
					currSoundEntry = &soundEntries[i];
					result &= currSoundEntry->populate(bodyIn, soundsSection.refs[i].getAddress(address + 0x08));
					currSoundEntry->sound3DInfo.populate(bodyIn, currSoundEntry->param3DRefOffset.getAddress(address + 0x08));
					switch (currSoundEntry->soundType)
					{
						case sit_SEQUENCE:
						{
							currSoundEntry->seqSoundInfo.populate(bodyIn, currSoundEntry->soundInfoRef.getAddress(address + 0x08));
							break;
						}
						case sit_STREAM:
						{
							currSoundEntry->streamSoundInfo.populate(bodyIn, currSoundEntry->soundInfoRef.getAddress(address + 0x08));
							break;
						}
						case sit_WAVE:
						{
							currSoundEntry->waveSoundInfo.populate(bodyIn, currSoundEntry->soundInfoRef.getAddress(address + 0x08));
							break;
						}
						default:
						{
							break;
						}
					}
				}

				bankEntries.resize(banksSection.refs.size());
				for (std::size_t i = 0; i < banksSection.refs.size(); i++)
				{
					result &= bankEntries[i].populate(bodyIn, banksSection.refs[i].getAddress(address + 0x08));
				}

				playerEntries.resize(playerSection.refs.size());
				for (std::size_t i = 0; i < playerSection.refs.size(); i++)
				{
					result &= playerEntries[i].populate(bodyIn, playerSection.refs[i].getAddress(address + 0x08));
				}

				fileHeaders.resize(filesSection.refs.size());
				brsarInfoFileHeader* currFileHeader = nullptr;
				for (std::size_t i = 0; i < filesSection.refs.size(); i++)
				{
					currFileHeader = &fileHeaders[i];
					currFileHeader->populate(bodyIn, filesSection.refs[i].getAddress(address + 0x08));
					if (currFileHeader->stringOffset.getAddress() != 0x00)
					{
						std::size_t numGotten = SIZE_MAX;
						std::string measurementString = bodyIn.data() + (currFileHeader->stringOffset.getAddress(address + 0x08));
						std::size_t sizePrescription = measurementString.size() + (0x04 - (measurementString.size() % 0x04));
						currFileHeader->stringContent = bodyIn.getBytes(sizePrescription, currFileHeader->stringOffset.getAddress(address + 0x08), numGotten);
						result &= numGotten == sizePrescription;
					}
					if (currFileHeader->listOffset.getAddress() != 0x00)
					{
						result &= currFileHeader->entryReferenceList.populate(bodyIn, currFileHeader->listOffset.getAddress(address + 0x08));
						for (std::size_t u = 0; u < currFileHeader->entryReferenceList.refs.size(); u++)
						{
							currFileHeader->entries.push_back(brsarInfoFileEntry());
							result &= currFileHeader->entries.back().populate(bodyIn, currFileHeader->entryReferenceList.refs[u].getAddress(address + 0x08));
						}
					}
				}

				groupHeaders.resize(groupsSection.refs.size());
				brsarInfoGroupHeader* currGroupHeader = nullptr;
				for (std::size_t i = 0; i < groupsSection.refs.size(); i++)
				{
					currGroupHeader = &groupHeaders[i];
					currGroupHeader->populate(bodyIn, groupsSection.refs[i].getAddress(address + 0x08));
					result &= currGroupHeader->entryReferenceList.populate(bodyIn, currGroupHeader->listOffset.getAddress(address + 0x08));

					for (std::size_t u = 0; u < currGroupHeader->entryReferenceList.refs.size(); u++)
					{
						currGroupHeader->entries.push_back(brsarInfoGroupEntry());
						result &= currGroupHeader->entries.back().populate(bodyIn, currGroupHeader->entryReferenceList.refs[u].getAddress(address + 0x08));
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
				lava::writeRawDataToStream(destinationStream, length);
				lava::writeRawDataToStream(destinationStream, soundsSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, banksSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, playerSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, filesSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, groupsSectionReference.getHex());
				lava::writeRawDataToStream(destinationStream, footerReference.getHex());
				soundsSection.exportContents(destinationStream);
				for (std::size_t i = 0; i < soundEntries.size(); i++)
				{
					soundEntries[i].exportContents(destinationStream);
				}
				banksSection.exportContents(destinationStream);
				for (std::size_t i = 0; i < bankEntries.size(); i++)
				{
					bankEntries[i].exportContents(destinationStream);
				}
				playerSection.exportContents(destinationStream);
				for (std::size_t i = 0; i < playerEntries.size(); i++)
				{
					playerEntries[i].exportContents(destinationStream);
				}
				filesSection.exportContents(destinationStream);
				for (std::size_t i = 0; i < fileHeaders.size(); i++)
				{
					fileHeaders[i].exportContents(destinationStream);
				}
				groupsSection.exportContents(destinationStream);
				for (std::size_t i = 0; i < groupHeaders.size(); i++)
				{
					groupHeaders[i].exportContents(destinationStream);
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
				if (length > amountWritten)
				{
					std::vector<char> finalPadding(length - amountWritten, 0x00);
					destinationStream.write(finalPadding.data(), finalPadding.size());
				}

				result = destinationStream.good();
			}
			return result;
		}

		/* BRSAR Info Section */



		/* BRSAR File Section */

		/* RWSD */
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

		bool dataInfo::populate(lava::byteArray& bodyIn, std::size_t addressIn)
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
		bool dataInfo::exportContents(std::ostream& destinationStream)
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
		bool rwsdDataSection::populate(lava::byteArray& bodyIn, std::size_t addressIn)
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

		bool wavePacket::populate(lava::byteArray& bodyIn, unsigned long addressIn, unsigned long lengthIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;
				length = lengthIn;

				std::size_t numGotten = SIZE_MAX;
				body = bodyIn.getBytes(lengthIn, addressIn, numGotten);
				result = numGotten == lengthIn;
				populated = result;
				if (!result)
				{
					length = ULONG_MAX;
					address = ULONG_MAX;
					body.clear();
				}
			}

			return result;
		}

		unsigned long waveInfo::getLengthInBytes() const
		{
			unsigned long result = ULONG_MAX;

			if (address != ULONG_MAX)
			{
				auto divResult = ldiv(nibbles, 2);
				result = divResult.quot + divResult.rem;
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
			//dataLocationType = sourceInfo.dataLocationType;
			pad = sourceInfo.pad;
			loopStartSample = sourceInfo.loopStartSample;
			nibbles = sourceInfo.nibbles;
			channelInfoTableOffset = sourceInfo.channelInfoTableOffset;
			//dataLocation = sourceInfo.dataLocation;
			reserved = sourceInfo.reserved;

			channelInfoTableLength = sourceInfo.channelInfoTableLength;
			channelInfoTable = sourceInfo.channelInfoTable;
		}
		bool waveInfo::populate(lava::byteArray& bodyIn, unsigned long addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				encoding = bodyIn.getChar(address);
				looped = bodyIn.getChar(address + 0x01);
				channels = bodyIn.getChar(address + 0x02);
				sampleRate24 = bodyIn.getChar(address + 0x03);
				sampleRate = bodyIn.getShort(address + 0x04);
				dataLocationType = bodyIn.getChar(address + 0x06);
				pad = bodyIn.getChar(address + 0x07);
				loopStartSample = bodyIn.getLong(address + 0x08);
				nibbles = bodyIn.getLong(address + 0x0C);
				channelInfoTableOffset = bodyIn.getLong(address + 0x10);
				dataLocation = bodyIn.getLong(address + 0x14);
				reserved = bodyIn.getLong(address + 0x18);

				channelInfoTableLength = channels * (0x20 + (encoding == 2 ? 0x30 : 0x00));
				for (unsigned long cursor = 0x0; cursor < channelInfoTableLength; cursor += 0x04)
				{
					channelInfoTable.push_back(bodyIn.getLong(address + channelInfoTableOffset + cursor));
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

				result = destinationStream.good();
			}
			return result;
		}

		void rwsdWaveSection::pushEntry(const waveInfo& entryIn)
		{
			if (!entryOffsets.empty())
			{
				for (unsigned long i = 0; i < entryOffsets.size(); i++)
				{
					entryOffsets[i] += 0x04;
				}
				entryOffsets.push_back(entryOffsets.back() + 0x1C + entryIn.channelInfoTableLength);
			}
			else
			{
				entryOffsets.push_back(0x10);
			}

			entries.push_back(entryIn);
			entries.back().address = brsarAddressConsts::bac_NOT_IN_FILE;
			if (entries.size() > 1)
			{
				waveInfo* formerFinalEntry = &entries[entries.size() - 2];
				unsigned long newDataLocation = formerFinalEntry->dataLocation + formerFinalEntry->getLengthInBytes();
				if (formerFinalEntry->packetContents.populated)
				{
					newDataLocation += formerFinalEntry->packetContents.paddingLength;
				}
				entries.back().dataLocation = newDataLocation;
			}
			else
			{
				entries.back().dataLocation = 0x00;
			}

			length += 0x04 + 0x1C + entryIn.channelInfoTableLength;
		}

		bool rwsdWaveSection::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				length = bodyIn.getLong(addressIn + 0x04);
				unsigned long entryCount = bodyIn.getLong(addressIn + 0x08);

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

		bool rwsdHeader::populate(lava::byteArray& bodyIn, std::size_t addressIn)
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

		bool rwsd::hasExclusiveWave(unsigned long dataSectionIndex)
		{
			bool result = 1;
			if (dataSectionIndex < dataSection.entries.size())
			{
				dataInfo* dataInfoEntry = &dataSection.entries[dataSectionIndex];
				unsigned long dataWaveIndex = dataInfoEntry->ntWaveIndex;

				std::size_t i = 0;
				while (result && i < dataSection.entries.size())
				{
					if (i != dataSectionIndex)
					{
						result = dataWaveIndex != dataSection.entries[i].ntWaveIndex;
					}
					i++;
				}
			}
			return result;
		}
		bool rwsd::isFirstToUseWave(unsigned long dataSectionIndex)
		{
			bool result = 1;
			if (dataSectionIndex < dataSection.entries.size())
			{
				dataInfo* dataInfoEntry = &dataSection.entries[dataSectionIndex];
				unsigned long dataWaveIndex = dataInfoEntry->ntWaveIndex;

				std::size_t i = 0;
				while (result && i < dataSectionIndex)
				{
					result = dataWaveIndex != dataSection.entries[i].ntWaveIndex;
					i++;
				}
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
		bool rwsd::populateWavePacket(lava::byteArray& bodyIn, unsigned long parentGroupWaveDataAddress, unsigned long collectionDataOffset, unsigned long dataSectionIndex)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				waveInfo* associatedWaveInfo = getWaveInfoAssociatedWithDataInfo(dataSectionIndex);
				if (associatedWaveInfo != nullptr && !associatedWaveInfo->packetContents.populated)
				{
					dataInfo* associatedDataInfo = &dataSection.entries[dataSectionIndex];
					unsigned long waveDataStartLocation = parentGroupWaveDataAddress + collectionDataOffset + associatedWaveInfo->dataLocation;
					unsigned long waveDataLength = associatedWaveInfo->getLengthInBytes();
					if (waveDataStartLocation + waveDataLength < bodyIn.size())
					{
						result = associatedWaveInfo->packetContents.populate(bodyIn, waveDataStartLocation, waveDataLength);
						if (associatedDataInfo->ntWaveIndex < (waveSection.entries.size() - 1))
						{
							unsigned long nextEntryDataLocation = waveSection.entries[associatedDataInfo->ntWaveIndex + 1].dataLocation;
							unsigned long currentEntryWaveInfoEnd = associatedWaveInfo->dataLocation + associatedWaveInfo->getLengthInBytes();
							if (nextEntryDataLocation >= currentEntryWaveInfoEnd)
							{
								associatedWaveInfo->packetContents.paddingLength = nextEntryDataLocation - currentEntryWaveInfoEnd;
							}
							else
							{
								unsigned long overlapSize = currentEntryWaveInfoEnd - nextEntryDataLocation;
								associatedWaveInfo->nibbles -= overlapSize * 2;
								associatedWaveInfo->packetContents.length -= overlapSize;
								associatedWaveInfo->packetContents.body.resize(associatedWaveInfo->packetContents.body.size() - overlapSize);
								associatedWaveInfo->packetContents.paddingLength = 0x00;
								std::cout << "[WARNING] Wave Packet Truncation performed! Lost 0x" << numToHexStringWithPadding(overlapSize, 0x02) << " byte(s) as a result.\n";
							}
						}
						else
						{
							associatedWaveInfo->packetContents.paddingLength = (0x10 - (associatedWaveInfo->packetContents.length % 0x10));
						}
					}
				}
			}

			return result;
		}


		signed long rwsd::overwriteSound(unsigned long dataSectionIndex, const dataInfo& dataInfoIn, const waveInfo& waveInfoIn, bool allowSharedWaveSplit)
		{
			signed long changeInWaveDataSize = LONG_MAX;

			if (waveInfoIn.packetContents.populated)
			{
				dataInfo* dataEntryPtr = &dataSection.entries[dataSectionIndex];
				dataEntryPtr->copyOverDataInfoProperties(dataInfoIn);

				if (isFirstToUseWave(dataSectionIndex))
				{
					waveInfo* associatedWaveInfo = getWaveInfoAssociatedWithDataInfo(dataSectionIndex);
					if (associatedWaveInfo != nullptr && associatedWaveInfo->packetContents.populated)
					{
						signed long differenceInWaveLength = signed long(waveInfoIn.getLengthInBytes()) - signed long(associatedWaveInfo->getLengthInBytes());
						signed long differenceInPaddingLength = signed long(waveInfoIn.packetContents.paddingLength) - signed long(associatedWaveInfo->packetContents.paddingLength);

						changeInWaveDataSize = differenceInWaveLength + differenceInPaddingLength;
						associatedWaveInfo->copyOverWaveInfoProperties(waveInfoIn);
						associatedWaveInfo->packetContents = waveInfoIn.packetContents;
						associatedWaveInfo->packetContents.address = brsarAddressConsts::bac_NOT_IN_FILE;

						for (std::size_t i = dataEntryPtr->ntWaveIndex + 1; i < waveSection.entries.size(); i++)
						{
							waveSection.entries[i].address = brsarAddressConsts::bac_NOT_IN_FILE;
							waveSection.entries[i].dataLocation += changeInWaveDataSize;
						}
					}
				}
				else
				{
					if (allowSharedWaveSplit)
					{
						std::cout << "Inserting new Wave!\n";
						waveSection.pushEntry(waveInfoIn);
						dataEntryPtr->ntWaveIndex = waveSection.entries.size() - 1;
						header.waveLength += 0x04 + 0x1C + waveInfoIn.channelInfoTableLength;
						header.headerLength += 0x04 + 0x1C + waveInfoIn.channelInfoTableLength;
						changeInWaveDataSize = (signed long)waveSection.entries.back().getLengthInBytes();
						changeInWaveDataSize += (signed long)waveSection.entries.back().packetContents.paddingLength;
					}
					else
					{
						changeInWaveDataSize = brsarErrorReturnCodes::berc_OVERWRITE_SOUND_SHARED_WAVE;
					}
				}
			}
			return changeInWaveDataSize;
		}
		signed long rwsd::shareWaveTargetBetweenDataEntries(unsigned long recipientDataSectionIndex, unsigned long donorDataSectionIndex, const dataInfo* dataInfoIn, bool voidOutExistingSound)
		{
			signed long changeInWaveDataSize = LONG_MAX;

			if (recipientDataSectionIndex < dataSection.entries.size())
			{
				if (donorDataSectionIndex < dataSection.entries.size())
				{
					dataInfo* dataEntryPtr = &dataSection.entries[recipientDataSectionIndex];
					if (dataInfoIn != nullptr)
					{
						dataEntryPtr->copyOverDataInfoProperties(*dataInfoIn);
					}
					if (voidOutExistingSound)
					{
						if (isFirstToUseWave(recipientDataSectionIndex))
						{
							waveInfo* associatedWaveInfo = getWaveInfoAssociatedWithDataInfo(recipientDataSectionIndex);
							if (associatedWaveInfo != nullptr && associatedWaveInfo->packetContents.populated)
							{
								// Create an empty wavePacket.
								wavePacket tempEmptyPacket;
								tempEmptyPacket.body = std::vector<unsigned char>(_EMPTY_SOUND_SOUND_LENGTH, 0x00);
								tempEmptyPacket.length = _EMPTY_SOUND_SOUND_LENGTH;
								tempEmptyPacket.paddingLength = _EMPTY_SOUND_TOTAL_LENGTH - _EMPTY_SOUND_SOUND_LENGTH;
								tempEmptyPacket.populated = 1;
								tempEmptyPacket.address = brsarAddressConsts::bac_NOT_IN_FILE;

								// Record the difference in size between the new empty packet and the old one.
								signed long differenceInWaveLength = signed long(tempEmptyPacket.length) - signed long(associatedWaveInfo->getLengthInBytes());
								signed long differenceInPaddingLength = signed long(tempEmptyPacket.paddingLength) - signed long(associatedWaveInfo->packetContents.paddingLength);
								changeInWaveDataSize = differenceInWaveLength + differenceInPaddingLength;

								// Overwrite old wavePacket with new one.
								associatedWaveInfo->address = brsarAddressConsts::bac_NOT_IN_FILE;
								associatedWaveInfo->nibbles = _EMPTY_SOUND_SOUND_LENGTH * 2;
								associatedWaveInfo->packetContents = tempEmptyPacket;

								// Propogate change in size through all other waves.
								for (std::size_t i = dataEntryPtr->ntWaveIndex + 1; i < waveSection.entries.size(); i++)
								{
									waveSection.entries[i].address = brsarAddressConsts::bac_NOT_IN_FILE;
									waveSection.entries[i].dataLocation += changeInWaveDataSize;
								}
							}
						}
						else
						{
							changeInWaveDataSize = brsarErrorReturnCodes::berc_OVERWRITE_SOUND_SHARED_WAVE;
						}
					}
					else
					{
						changeInWaveDataSize = 0;
					}
					dataInfo* donorDataEntryPtr = &dataSection.entries[donorDataSectionIndex];
					dataEntryPtr->ntWaveIndex = donorDataEntryPtr->ntWaveIndex;
				}
			}

			return changeInWaveDataSize;
		}

		bool rwsd::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				if (bodyIn.getLong(addressIn) == brsarHexTags::bht_RWSD)
				{
					address = addressIn;

					if (header.populate(bodyIn, address))
					{
						result = dataSection.populate(bodyIn, address + header.dataOffset);
						result &= waveSection.populate(bodyIn, address + header.waveOffset);
					}
				}
			}

			return result;
		}
		bool rwsd::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;

			if (destinationStream.good())
			{
				destinationStream << std::hex;
				result &= header.exportContents(destinationStream);
				result &= dataSection.exportContents(destinationStream);
				result &= waveSection.exportContents(destinationStream);
			}

			return result;
		}
		/* RWSD */

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

			length += changeAmount;
			for (unsigned long i = 0; i < fileContents.size(); i++)
			{
				brsarFileFileContents* currFile = &fileContents[i];
				adjustOffset(0x00, currFile->headerAddress, changeAmount, pastThisAddress);
				adjustOffset(0x00, currFile->dataAddress, changeAmount, pastThisAddress);
			}

			return result;
		}

		bool brsarFileSection::populate(lava::byteArray& bodyIn, std::size_t addressIn, brsarInfoSection& infoSectionIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				length = bodyIn.getLong(address + 0x04);
				brsarInfoGroupHeader* currHeader = nullptr;
				brsarInfoGroupEntry* currEntry = nullptr;
				for (std::size_t i = 0; i < infoSectionIn.groupHeaders.size(); i++)
				{
					currHeader = &infoSectionIn.groupHeaders[i];
					for (std::size_t u = 0; u < currHeader->entries.size(); u++)
					{
						currEntry = &currHeader->entries[u];
						std::size_t numGotten = ULONG_MAX;
						fileContents.push_back(brsarFileFileContents());
						fileContents.back().fileID = currEntry->fileID;
						fileContents.back().groupInfoIndex = i;
						fileContents.back().groupID = currHeader->groupID;
						fileContents.back().headerAddress = currHeader->headerAddress + currEntry->headerOffset;
						fileContents.back().header = bodyIn.getBytes(currEntry->headerLength, currHeader->headerAddress + currEntry->headerOffset, numGotten);
						fileContents.back().dataAddress = currHeader->dataAddress + currEntry->dataOffset;
						fileContents.back().data = bodyIn.getBytes(currEntry->dataLength, currHeader->dataAddress + currEntry->dataOffset, numGotten);
						fileIDToIndex[currEntry->fileID].push_back(fileContents.size() - 1);
					}
				}

				result = 1;
			}

			return result;
		}
		bool brsarFileSection::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;

			if (destinationStream.good())
			{
				byteArray tempArray(length, 0x00);
				tempArray.setLong(brsarHexTags::bht_FILE, 0x00);
				tempArray.setLong(length, 0x04);
				for (unsigned long i = 0; i < fileContents.size(); i++)
				{
					brsarFileFileContents* currFilePtr = &fileContents[i];
					if (currFilePtr->headerAddress > address)
					{
						tempArray.setBytes(currFilePtr->header, currFilePtr->headerAddress - address);
					}
					else
					{
						std::cerr << "Invalid header address!\n";
					}
					if (currFilePtr->dataAddress > address)
					{
						tempArray.setBytes(currFilePtr->data, currFilePtr->dataAddress - address);
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

		/* BRSAR File Section */



		/* BRSAR */
		bool brsar::init(std::string filePathIn)
		{
			bool result = 0;
			std::ifstream fileIn;
			fileIn.open(filePathIn, std::ios_base::in | std::ios_base::binary);
			if (fileIn.is_open())
			{
				result = 1;
				std::cout << "Parsing \"" << filePathIn << "\"...\n";
				byteArray contents(fileIn);
				fileIn.close();
				std::size_t cursor = 0x04;
				byteOrderMarker = contents.getShort(cursor, &cursor);
				version = contents.getShort(cursor, &cursor);
				length = contents.getLong(cursor, &cursor);
				headerLength = contents.getShort(cursor, &cursor);
				sectionCount = contents.getShort(cursor, &cursor);

				result &= symbSection.populate(contents, contents.getLong(0x10));
				result &= infoSection.populate(contents, contents.getLong(0x18));
				result &= fileSection.populate(contents, contents.getLong(0x20), infoSection);
			}
			return result;
		}
		bool brsar::exportContents(std::ostream& destinationStream)
		{
			bool result = 1;

			writeRawDataToStream(destinationStream, brsarHexTags::bht_RSAR);
			writeRawDataToStream(destinationStream, byteOrderMarker);
			writeRawDataToStream(destinationStream, version);
			writeRawDataToStream(destinationStream, length);
			writeRawDataToStream(destinationStream, headerLength);
			writeRawDataToStream(destinationStream, sectionCount);
			writeRawDataToStream(destinationStream, symbSection.address);
			writeRawDataToStream(destinationStream, symbSection.length);
			writeRawDataToStream(destinationStream, infoSection.address);
			writeRawDataToStream(destinationStream, infoSection.length);
			writeRawDataToStream(destinationStream, fileSection.address);
			writeRawDataToStream(destinationStream, fileSection.length);
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
				currentGroup = &infoSection.groupHeaders[i];
				if (groupIDIn == currentGroup->groupID)
				{
					result = currentGroup->address;
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
				brsarInfoGroupHeader* currGroupHeader = &infoSection.groupHeaders[i];
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
								currGroupHeader->headerAddress = currFileContents->headerAddress;
								currGroupHeader->dataAddress = currFileContents->dataAddress;
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
						brsarInfoGroupHeader* prevGroupHeader = &infoSection.groupHeaders[i - 1];
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
						result &= fileSection.propogateFileLengthChange(headerLengthChange, currOccurence->headerAddress);
						result &= fileSection.propogateFileLengthChange(dataLengthChange, currOccurence->dataAddress);
						// Update BRSAR's length value.
						// Note that the above propogation functions will update the FILE section's length.
						length += headerLengthChange + dataLengthChange;
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

		bool brsar::summarizeSymbStringData(std::ostream& output)
		{
			return symbSection.dumpStrings(output);
		}
		bool brsar::outputConsecutiveSoundEntryStringsWithSameFileID(unsigned long startingIndex, std::ostream& output)
		{
			bool result = 0;

			if (output.good() && symbSection.address != ULONG_MAX)
			{
				unsigned long fileID = infoSection.soundEntries[startingIndex].fileID;
				std::size_t i = startingIndex;
				while (i < infoSection.soundEntries.size() && infoSection.soundEntries[i].fileID == fileID)
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
				currHeader = &infoSection.groupHeaders[i];
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
							metadataOutput << "\tFile " << numToDecStringWithPadding(currEntry->fileID, 0x03) << " (0x" << numToHexStringWithPadding(currEntry->fileID, 0x03) << ") @ 0x" << numToHexStringWithPadding(fileContentsPtr->headerAddress, 0x08) << "\n";
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
								std::ofstream headerOutput(groupFolder + fileBaseName + headerFilename, std::ios_base::out | std::ios_base::binary);
								if (headerOutput.is_open())
								{
									headerOutput.write((char*)fileContentsPtr->header.data(), fileContentsPtr->header.size());
									headerOutput.write((char*)fileContentsPtr->data.data(), fileContentsPtr->data.size());
									headerOutput.close();
								}
							}
							else
							{
								headerFilename += "_header.dat";
								dataFilename += "_data.dat";
								std::ofstream headerOutput(groupFolder + fileBaseName + headerFilename, std::ios_base::out | std::ios_base::binary);
								if (headerOutput.is_open())
								{
									headerOutput.write((char*)fileContentsPtr->header.data(), fileContentsPtr->header.size());
									headerOutput.close();
								}
								std::ofstream dataOutput(groupFolder + fileBaseName + dataFilename, std::ios_base::out | std::ios_base::binary);
								if (dataOutput.is_open())
								{
									dataOutput.write((char*)fileContentsPtr->data.data(), fileContentsPtr->data.size());
									dataOutput.close();
								}
							}
							entryExported = 1;
						}
						y++;
					}
				}
			}

			return result = 1;
		}
		bool brsar::exportSawnd(std::size_t groupID, std::string targetFilePath)
		{
			bool result = 0;
			std::cout << "Creating \"" << targetFilePath << "\" from Group #" << numToDecStringWithPadding(groupID, 0x03) << " / 0x"  << numToHexStringWithPadding(groupID, 0x03) << "...\n";

			std::ofstream sawndOutput;
			sawndOutput.open(targetFilePath, std::ios_base::out | std::ios_base::binary);
			if (sawndOutput.is_open())
			{
				const brsarInfoGroupHeader* targetGroup = infoSection.getGroupWithID(groupID);
				if (targetGroup != nullptr)
				{
					sawndOutput.put(2);
					lava::writeRawDataToStream(sawndOutput, groupID);
					lava::writeRawDataToStream(sawndOutput, targetGroup->dataLength);

					for (std::size_t i = 0; i < targetGroup->entries.size(); i++)
					{
						const brsarInfoGroupEntry* currEntry = &targetGroup->entries[i];
						//std::cout << "Collection #" << i << " (@ 0x" << numToHexStringWithPadding(currEntry->address, 8) << ")\n";
						//std::cout << "\tFile ID: 0x" << numToHexStringWithPadding(currEntry->fileID, 4) << "\n";
						//std::cout << "\tHeader Offset: 0x" << numToHexStringWithPadding(currEntry->headerOffset, 4) << "\n";
						//std::cout << "\tHeader Length: 0x" << numToHexStringWithPadding(currEntry->headerLength, 4) << "\n";
						//std::cout << "\tData Offset: 0x" << numToHexStringWithPadding(currEntry->dataOffset, 4) << "\n";
						//std::cout << "\tData Length: 0x" << numToHexStringWithPadding(currEntry->dataLength, 4) << "\n";
						lava::writeRawDataToStream(sawndOutput, currEntry->fileID);
						lava::writeRawDataToStream(sawndOutput, currEntry->dataOffset);
						lava::writeRawDataToStream(sawndOutput, currEntry->dataLength);
					}
					byteArray fileContentsExportArr(targetGroup->dataLength + targetGroup->headerLength);
					for (std::size_t i = 0; i < targetGroup->entries.size(); i++)
					{
						const brsarInfoGroupEntry* currEntry = &targetGroup->entries[i];
						const brsarFileFileContents* currFileContentsPtr = fileSection.getFileContentsPointer(currEntry->fileID, targetGroup->groupID);
						fileContentsExportArr.setBytes(currFileContentsPtr->header, currEntry->headerOffset);
						fileContentsExportArr.setBytes(currFileContentsPtr->data, targetGroup->headerLength + currEntry->dataOffset);
					}
					fileContentsExportArr.dumpToStream(sawndOutput);
				}
				else
				{
					std::cerr << "[ERROR] Provided group ID couldn't be located. Aborting export.\n";
					remove(targetFilePath.c_str());
				}
			}
			else
			{
				std::cerr << "[ERROR] Couldn't write to target file location (\"" << targetFilePath << "\").\n";
			}
			return result;
		}
		bool brsar::importSawnd(std::string sourceFilePath)
		{
			bool result = 0;

			std::ifstream sawndIn(sourceFilePath, std::ios_base::in | std::ios_base::binary);
			if (sawndIn.is_open())
			{
				byteArray sawndBody(sawndIn);
				sawndIn.close();
				if (sawndBody.populated())
				{
					sawnd sawndContent;
					sawndContent.populate(sawndBody, 0x00);
					const brsarInfoGroupHeader* targetGroupHeader = infoSection.getGroupWithID(sawndContent.groupID);
					std::cout << "Importing .sawnd (for Group #" << lava::numToDecStringWithPadding(sawndContent.groupID, 0x03) << " / 0x" << lava::numToHexStringWithPadding(sawndContent.groupID, 0x03) << ")...\n";
					if (targetGroupHeader != nullptr)
					{
						if (targetGroupHeader->entries.size() == sawndContent.fileEntries.size())
						{
							if (!sawndContent.fileEntries.empty())
							{
								for (unsigned long i = 0; i < sawndContent.fileEntries.size(); i++)
								{
									sawndFileEntry* currEntry = &sawndContent.fileEntries[i];
									if (targetGroupHeader->usesFileID(currEntry->fileID))
									{
										std::cout << "\tOverwriting File (ID: " << lava::numToDecStringWithPadding(currEntry->fileID, 0x03) << " / 0x" << lava::numToHexStringWithPadding(currEntry->fileID, 0x03) << ")...";
										if (overwriteFile(currEntry->headerContent, currEntry->dataContent, currEntry->fileID))
										{
											std::cout << " Success!\n";
										}
										else
										{
											std::cerr << " Failure! Something has gone wrong!\n";
										}
									}
									else
									{
										std::cout << "\t[ERROR] Unable to import File (ID: " << lava::numToDecStringWithPadding(currEntry->fileID, 0x03) << " / 0x" << lava::numToHexStringWithPadding(currEntry->fileID, 0x03) << ") from \"" << sourceFilePath << "\" into Group (" << lava::numToDecStringWithPadding(targetGroupHeader->groupID, 0x03) << " / 0x" << lava::numToHexStringWithPadding(targetGroupHeader->groupID, 0x03) << ")! The specified file isn't used in the target group!\n";
									}
								}
							}
							else
							{
								std::cout << "\t[WARNING] Successfully loaded provided .sawnd, but no file entries could be found!\n";
							}
						}
						else
						{
							std::cout << "\t[ERROR] Unable to import content of \"" << sourceFilePath << "\"! The targeted Group (" << lava::numToDecStringWithPadding(sawndContent.groupID, 0x03) << " / 0x" << lava::numToHexStringWithPadding(sawndContent.groupID, 0x03) << ") contains a different number of files (" << targetGroupHeader->entries.size() << ") than the provided .sawnd (" << sawndContent.fileEntries.size() << ")!\n";
						}
					}
					else
					{
						std::cout << "\t[ERROR] Unable to import content of \"" << sourceFilePath << "\"! The targeted Group (" << lava::numToDecStringWithPadding(sawndContent.groupID, 0x03) << " / 0x" << lava::numToHexStringWithPadding(sawndContent.groupID, 0x03) << ") couldn't be found in this .brsar!\n";
					}
				}
				else
				{
					std::cout << "[ERROR] Was able to open \"" << sourceFilePath << "\", but was unable to build a byteArray!\n";
				}
			}
			else
			{
				std::cout << "[ERROR] Unable to open \"" << sourceFilePath << "\" for import!\n";
			}

			return result;
		}
		/* BRSAR */



		/* SAWND */
		bool sawnd::populate(const lava::byteArray& bodyIn, unsigned long addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				sawndVersion = bodyIn.getChar(address);
				groupID = bodyIn.getLong(address + 0x01);
				waveDataLength = bodyIn.getLong(address + 0x05);

				bool firstEntryReached = 0;
				std::size_t cursor = address + 0x09;
				std::vector<unsigned long> currFileTriple(3, ULONG_MAX);
				unsigned long currFileTripleSlot = 0;
				while (!firstEntryReached)
				{
					unsigned long harvestedLong = bodyIn.getLong(cursor);
					if (!(validateHexTag(harvestedLong) == brsarHexTagType::bhtt_FILE_SECTION))
					{
						currFileTriple[currFileTripleSlot] = harvestedLong;
						currFileTripleSlot++;
						if (currFileTripleSlot >= 3)
						{
							fileEntries.push_back(sawndFileEntry());
							fileEntries.back().fileID = currFileTriple[0];
							fileEntries.back().dataOffset = currFileTriple[1];
							fileEntries.back().dataLength = currFileTriple[2];
							currFileTripleSlot = 0;
							currFileTriple = {ULONG_MAX, ULONG_MAX, ULONG_MAX};
						}
						cursor += 0x4;
					}
					else
					{
						firstEntryReached = 1;
					}
				}
				headerSectionOffset = cursor;
				for (unsigned long currFileIndex = 0; cursor < bodyIn.size() && currFileIndex < fileEntries.size(); currFileIndex++)
				{
					if (validateHexTag(bodyIn.getLong(cursor)) == brsarHexTagType::bhtt_FILE_SECTION)
					{
						sawndFileEntry* currEntry = &fileEntries[currFileIndex];
						currEntry->headerOffset = cursor - headerSectionOffset;
						currEntry->headerLength = bodyIn.getLong(cursor + 0x08);
						std::size_t numGotten = SIZE_MAX;
						currEntry->headerContent = bodyIn.getBytes(currEntry->headerLength, headerSectionOffset + currEntry->headerOffset, numGotten);
						cursor += currEntry->headerLength;
					}
					else
					{
						cursor = SIZE_MAX;
					}
				}
				dataSectionOffset = cursor;
				for (unsigned long currFileIndex = 0; currFileIndex < fileEntries.size(); currFileIndex++)
				{
					sawndFileEntry* currEntry = &fileEntries[currFileIndex];
					std::size_t numGotten = SIZE_MAX;
					currEntry->dataContent = bodyIn.getBytes(currEntry->dataLength, dataSectionOffset + currEntry->dataOffset, numGotten);
				}
				result = 1;
			}
			return result;
		}
		/* SAWND */
	}
}