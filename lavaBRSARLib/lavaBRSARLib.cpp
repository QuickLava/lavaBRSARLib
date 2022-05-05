#include "lavaBRSARLib.h"

namespace lava
{
	namespace brawl
	{
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
			return lava::numToHexStringWithPadding(getAddress(relativeToIn), 8);
		}
		unsigned long long brawlReference::getHex()
		{
			unsigned long long result = address | (unsigned long long(addressType) << 0x20);
			return result;
		}

		bool brawlReferenceVector::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated() && (addressIn < bodyIn.body.size()))
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
		bool brsarSymbPTrieNode::exportContents(std::ostream& destinationStream)
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
		unsigned long brsarSymbPTrieNode::getBit()
		{
			return posAndBit & 0b00000111;
		}
		unsigned long brsarSymbPTrieNode::getPos()
		{
			return posAndBit >> 3;
		}
		bool brsarSymbPTrieNode::compareCharAndBit(char charIn)
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
		bool brsarSymbPTrie::exportContents(std::ostream& destinationStream)
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
		brsarSymbPTrieNode brsarSymbPTrie::findString(std::string stringIn)
		{
			if (rootID < numEntries)
			{
				brsarSymbPTrieNode* currentNode = &entries[rootID];

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

				/*for (unsigned long i = 0x04; i < stringListOffset; i += 0x04)
				{
					trieOffsets.push_back(bodyIn.getLong(address + 0x8 + i));
					tries.push_back(brsarSymbPTrie());
					result &= tries.back().populate(bodyIn, address + 0x08 + trieOffsets.back());
				}*/

				unsigned long cursor = address + 0x08 + stringListOffset;
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
		bool brsarSymbSection::exportContents(std::ostream& destinationStream)
		{
			bool result = 0;
			if (destinationStream.good())
			{
				result = 1;

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
				if (length > finalStreamPos)
				{
					std::vector<char> finalPadding(length - finalStreamPos, 0x00);
					destinationStream.write(finalPadding.data(), finalPadding.size());
				}

				result &= destinationStream.good();
			}
			return result;
		}
		std::string brsarSymbSection::getString(std::size_t idIn)
		{
			std::string result = "";
			if (idIn < stringEntryOffsets.size())
			{
				unsigned long stringAddr = (stringEntryOffsets[idIn] - stringEntryOffsets.front());
				result = (char*)(stringBlock.data() + stringAddr);
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
		bool brsarInfoGroupHeader::populate(lava::byteArray& bodyIn, std::size_t addressIn)
		{
			bool result = 0;

			if (bodyIn.populated())
			{
				address = addressIn;

				stringID = bodyIn.getLong(address);
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
				lava::writeRawDataToStream(destinationStream, stringID);
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

		std::vector<brsarInfoFileHeader*> brsarInfoSection::findFilesWithGroupID(unsigned long groupIDIn)
		{
			std::vector<brsarInfoFileHeader*> result{};

			brsarInfoFileHeader* currHeader = nullptr;
			for (std::size_t i = 0; i < fileHeaders.size(); i++)
			{
				currHeader = &fileHeaders[i];
				for (std::size_t u = 0; u < currHeader->entries.size(); u++)
				{
					if (currHeader->entries[u].groupID == groupIDIn)
					{
						result.push_back(currHeader);
					}
				}
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
						std::string measurementString = bodyIn.body.data() + (currFileHeader->stringOffset.getAddress(address + 0x08));
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
				if (length > finalStreamPos)
				{
					std::vector<char> finalPadding(length - finalStreamPos, 0x00);
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
					if (waveDataStartLocation + waveDataLength < bodyIn.body.size())
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
								std::cout << "[WARNING] Wave Packet Truncation performed! Lost 0x" << lava::numToHexStringWithPadding(overlapSize, 0x02) << " byte(s) as a result.\n";
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
						neoFileContents.push_back(brsarFileFileContents());
						neoFileContents.back().fileID = currEntry->fileID;
						neoFileContents.back().groupID = currHeader->stringID;
						neoFileContents.back().headerAddress = currHeader->headerAddress + currEntry->headerOffset;
						neoFileContents.back().header = bodyIn.getBytes(currEntry->headerLength, currHeader->headerAddress + currEntry->headerOffset, numGotten);
						neoFileContents.back().dataAddress = currHeader->dataAddress + currEntry->dataOffset;
						neoFileContents.back().data = bodyIn.getBytes(currEntry->dataLength, currHeader->dataAddress + currEntry->dataOffset, numGotten);
						fileIDToIndex[currEntry->fileID].push_back(neoFileContents.size() - 1);
					}
				}

				result = 1;
			}

			return result;
		}

		/* BRSAR File Section */



		/* BRSAR */

		bool brsarFile::init(std::string filePathIn)
		{
			bool result = 0;
			std::ifstream fileIn;
			fileIn.open(filePathIn, std::ios_base::in | std::ios_base::binary);
			if (fileIn.is_open())
			{
				result = 1;
				std::cout << "Parsing \"" << filePathIn << "\"...\n";
				contents.populate(fileIn);
				fileIn.close();
				result &= symbSection.populate(contents, contents.getLong(0x10));
				result &= infoSection.populate(contents, contents.getLong(0x18));
				result &= fileSection.populate(contents, contents.getLong(0x20), infoSection);
			}
			return result;
		}

		std::string brsarFile::getSymbString(unsigned long indexIn)
		{
			return symbSection.getString(indexIn);
		}
		bool brsarFile::summarizeSymbStringData(std::ostream& output)
		{
			bool result = 0;

			if (output.good() && symbSection.address != ULONG_MAX)
			{
				unsigned long stringOffset = ULONG_MAX;
				unsigned long stringAddress = ULONG_MAX;
				unsigned long stringOffsetAddress = symbSection.address + 0x08 + symbSection.stringListOffset + 0x04;
				for (std::size_t i = 0; i < symbSection.stringEntryOffsets.size(); i++)
				{
					stringOffset = symbSection.stringEntryOffsets[i];
					stringAddress = symbSection.address + stringOffset + 0x08;
					char* ptr = contents.body.data() + stringAddress;
					output << "[0x" << lava::numToHexStringWithPadding(i, 4) << "] 0x" << lava::numToHexStringWithPadding(stringOffsetAddress, 4) << "->0x" << lava::numToHexStringWithPadding(stringAddress, 4) << ": " << ptr << "\n";
					stringOffsetAddress += 0x04;
				}

			}

			return result;
		}
		bool brsarFile::outputConsecutiveSoundEntryStringsWithSameFileID(unsigned long startingIndex, std::ostream& output)
		{
			bool result = 0;

			if (output.good() && symbSection.address != ULONG_MAX)
			{
				unsigned long fileID = infoSection.soundEntries[startingIndex].fileID;
				std::size_t i = startingIndex;
				while (i < infoSection.soundEntries.size() && infoSection.soundEntries[i].fileID == fileID)
				{
					output << "\t[String 0x" << lava::numToHexStringWithPadding(i, 0x04) << "] " << getSymbString(i) << "\n";
					i++;
				}
			}

			return result;
		}

		unsigned long brsarFile::getGroupOffset(unsigned long groupIDIn)
		{
			std::size_t result = SIZE_MAX;

			std::size_t i = 0;
			brsarInfoGroupHeader currentGroup;
			bool done = 0;

			while (!done && i < infoSection.groupsSection.refs.size())
			{
				currentGroup.populate(contents, infoSection.groupsSection.refs[i].getAddress(infoSection.address + 0x08));
				if (groupIDIn == currentGroup.stringID)
				{
					result = currentGroup.address;
					done = 1;
				}
				else
				{
					i++;
				}
			}

			return result;
		}
		bool brsarFile::doFileDump(std::string dumpRootFolder, bool joinHeaderAndData)
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
				std::string groupName = symbSection.getString(currHeader->stringID);
				if (groupName.size() == 0x00)
				{
					groupName = "[NAMELESS]";
				}
				std::string groupFolder = dumpRootFolder + groupName + "/";
				std::filesystem::create_directory(groupFolder);
				metadataOutput << "File(s) in \"" << groupName << "\" (ID: " 
					<< lava::numToDecStringWithPadding(currHeader->stringID, 0x03) << " / 0x" << lava::numToHexStringWithPadding(currHeader->stringID, 0x03) << ")...\n";
				for (std::size_t u = 0; u < currHeader->entries.size(); u++)
				{
					currEntry = &currHeader->entries[u];
					std::vector<std::size_t>* idToIndexVecPtr = &fileSection.fileIDToIndex[currEntry->fileID];
					std::string fileBaseName = lava::numToDecStringWithPadding(currEntry->fileID, 0x03) + "_(0x" + lava::numToHexStringWithPadding(currEntry->fileID, 0x03) + ")";
					bool entryExported = 0;
					std::size_t y = 0;
					while(!entryExported)
					{
						brsarFileFileContents* fileContentsPtr = &fileSection.neoFileContents[(*idToIndexVecPtr)[y]];
						if (fileContentsPtr->groupID == currHeader->stringID)
						{
							metadataOutput << "\tFile " << lava::numToDecStringWithPadding(currEntry->fileID, 0x03) << " (0x" << lava::numToHexStringWithPadding(currEntry->fileID, 0x03) << ") @ 0x" << lava::numToHexStringWithPadding(fileContentsPtr->headerAddress, 0x08) << "\n";
							metadataOutput << "\t\tFile Type: ";
							if (fileContentsPtr->header.size() >= 0x04)
							{
								metadataOutput << fileContentsPtr->header[0] << fileContentsPtr->header[1] << fileContentsPtr->header[2] << fileContentsPtr->header[3] << "\n";
							}
							else
							{
								metadataOutput << "----\n";
							}
							metadataOutput << "\t\tHeader Size: " << bytesToFileSizeString(fileContentsPtr->header.size()) << "\n";
							metadataOutput << "\t\tData Size: " << bytesToFileSizeString(fileContentsPtr->data.size()) << "\n";
							metadataOutput << "\t\tTotal Size: " << bytesToFileSizeString(fileContentsPtr->header.size() + fileContentsPtr->data.size()) << "\n";
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

			/*for (auto i : fileSection.fileIDToIndex)
			{
				std::string newBaseFolder = dumpRootFolder + "File_" + lava::numToDecStringWithPadding(i.first, 0x03) + "_(0x" + lava::numToHexStringWithPadding(i.first, 0x03) + ")/";
				std::vector<std::size_t>* idToIndexVecPtr = &i.second;
				std::filesystem::create_directory(newBaseFolder);
				for (std::size_t y = 0; y < idToIndexVecPtr->size(); y++)
				{
					brsarFileFileContents* fileContentsPtr = &fileSection.neoFileContents[(*idToIndexVecPtr)[y]];
					std::string headerFilename = "";
					std::string dataFilename = "";
					headerFilename += symbSection.getString(fileContentsPtr->groupID) + "_";
					dataFilename += symbSection.getString(fileContentsPtr->groupID) + "_";
					if (joinHeaderAndData)
					{
						headerFilename += "file.dat";
						std::ofstream headerOutput(newBaseFolder + headerFilename, std::ios_base::out | std::ios_base::binary);
						if (headerOutput.is_open())
						{
							headerOutput.write((char*)fileContentsPtr->header.data(), fileContentsPtr->header.size());
							headerOutput.write((char*)fileContentsPtr->data.data(), fileContentsPtr->data.size());
							headerOutput.close();
						}
					}
					else
					{
						headerFilename += "header.dat";
						dataFilename += "data.dat";
						std::ofstream headerOutput(newBaseFolder + headerFilename, std::ios_base::out | std::ios_base::binary);
						if (headerOutput.is_open())
						{
							headerOutput.write((char*)fileContentsPtr->header.data(), fileContentsPtr->header.size());
							headerOutput.close();
						}
						std::ofstream dataOutput(newBaseFolder + dataFilename, std::ios_base::out | std::ios_base::binary);
						if (dataOutput.is_open())
						{
							dataOutput.write((char*)fileContentsPtr->data.data(), fileContentsPtr->data.size());
							dataOutput.close();
						}
					}
					if (!outputPotentialDupes)
					{
						break;
					}
				}
			}*/

			return result = 1;
		}
		bool brsarFile::exportSawnd(std::size_t groupID, std::string targetFilePath)
		{
			bool result = 0;
			std::cout << "Creating \"" << targetFilePath << "\" from Group #" << groupID << "...\n";

			std::ofstream sawndOutput;
			sawndOutput.open(targetFilePath, std::ios_base::out | std::ios_base::binary);
			if (sawndOutput.is_open())
			{
				std::size_t groupOffset = getGroupOffset(groupID);
				std::cout << "\nGroup found @ 0x" << lava::numToHexStringWithPadding(groupOffset, 8) << "!\n";

				if (groupOffset != SIZE_MAX)
				{
					brsarInfoGroupHeader targetGroup;
					targetGroup.populate(contents, groupOffset);

					std::cout << "Address: 0x" << lava::numToHexStringWithPadding(targetGroup.address, 8) << "\n";
					std::cout << "Header: Offset = 0x" << lava::numToHexStringWithPadding(targetGroup.headerAddress, 8) <<
						", Length = 0x" << lava::numToHexStringWithPadding(targetGroup.headerLength, 8) << " bytes\n";
					std::cout << "Wave Data: Offset = 0x" << lava::numToHexStringWithPadding(targetGroup.dataAddress, 8) <<
						", Length = 0x" << lava::numToHexStringWithPadding(targetGroup.dataLength, 8) << " bytes\n";

					sawndOutput.put(2);
					lava::writeRawDataToStream(sawndOutput, groupID);
					lava::writeRawDataToStream(sawndOutput, targetGroup.dataLength);

					std::size_t collectionRefListAddress = targetGroup.listOffset.getAddress(infoSection.address + 0x08);
					brawlReferenceVector collectionReferences;
					collectionReferences.populate(contents, collectionRefListAddress);
					std::cout << "Collection List Address(0x" << lava::numToHexStringWithPadding(collectionRefListAddress, 8) << ")\n";
					std::cout << "Collection Count: " << collectionReferences.refs.size() << "\n";
					std::vector<brsarInfoGroupEntry> groupInfoEntries;
					brsarInfoGroupEntry* currEntry = nullptr;
					unsigned long currentCollectionAddress = ULONG_MAX;

					for (std::size_t i = 0; i < collectionReferences.refs.size(); i++)
					{
						currentCollectionAddress = collectionReferences.refs[i].getAddress(infoSection.address + 0x08);
						std::cout << "Collection #" << i << ": Info Section Offset = 0x" << lava::numToHexStringWithPadding(currentCollectionAddress, 8) << "\n";
						groupInfoEntries.push_back(brsarInfoGroupEntry());
						currEntry = &groupInfoEntries.back();
						currEntry->populate(contents, currentCollectionAddress);

						std::cout << "\tFile ID: 0x" << lava::numToHexStringWithPadding(currEntry->fileID, 4) << "\n";
						std::cout << "\tHeader Offset: 0x" << lava::numToHexStringWithPadding(currEntry->headerOffset, 4) << "\n";
						std::cout << "\tHeader Length: 0x" << lava::numToHexStringWithPadding(currEntry->headerLength, 4) << "\n";
						std::cout << "\tData Offset: 0x" << lava::numToHexStringWithPadding(currEntry->dataOffset, 4) << "\n";
						std::cout << "\tData Length: 0x" << lava::numToHexStringWithPadding(currEntry->dataLength, 4) << "\n";

						lava::writeRawDataToStream(sawndOutput, currEntry->fileID);
						lava::writeRawDataToStream(sawndOutput, currEntry->dataOffset);
						lava::writeRawDataToStream(sawndOutput, currEntry->dataLength);
					}

					std::cout << "Total Size:" << targetGroup.headerLength + targetGroup.dataLength << "\n";
					sawndOutput.write(contents.body.data() + targetGroup.headerAddress, targetGroup.headerLength + targetGroup.dataLength);
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

		/* BRSAR */

	}
}