#include "stdafx.h"
#include "blockparser.h"
#include "ndbport.h"
#include <iostream>

BlockParser::BlockParser(std::ofstream* clusterReport, std::ofstream* fileReport, std::ofstream* datablocksReport, int clusterSize)
{
	cluster = clusterReport;
	file = fileReport;
	datablocks = datablocksReport;
	for (int p = 0; p < sizeof(blockbuffer); p++)
	{
		blockbuffer[p] = 0; // Initialize all elements to zero.
	}
	clustersize = clusterSize;
}

bool BlockParser::ValidateBlocks(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG headeroffset, ULONGLONG headerBID, std::map<BID, BlockSignature> blockrefs, ULONGLONG* invalidBlockLoc)
{
	*invalidBlockLoc = -1;
	bool result = TRUE;
	BLOCKTRAILER bt;
	WORD cb;
	WORD padding;
	int startofData;
	int endofData;
	std::map<BID, BlockSignature>::iterator it;
	DWORD dwCRC;
	BYTE blockdata[8192];  // reconstruct the blockdata, maximum block size is 8192 bytes

	//  blocktrailer always at 64 bytes intervals
	for (ULONGLONG i = 0; i < buffersize && result; i ++)
	{
		memcpy((BYTE*)&bt, (pbufRead + i + 48), sizeof(bt)); // only interested in last 16 bytes of a page (pagetrailer/blocktrailer location)
		it = blockrefs.find(bt.bid); // check whether the current data is a known signature of a datablock trailer.
		if (it != blockrefs.end())
		{
			if (bt.bid == it->second.bid && bt.cb == it->second.cb && bt.wSig == it->second.wSig)
			{
				// valid blocktrailer recognized, reconstruct block data
				cb = it->second.cb;
				// validate data block (this might require data before the current cluster)
				// padding might be present between the data and block trailer
				padding = 64 - ((cb + 16) % 64);
				if (padding == 64)
				{
					padding = 0;
				}
				startofData = i - cb - padding + 48;  // calculate the start of the data block, trailer located at 48 byte offset, last 16 bytes.
				endofData = startofData + cb;  // calculate the end of the data block
				//  invalid blocks at invalid locations are allowed, only interested in invalid blocks at valid locations
				if (startofData + (iteration * clustersize) == it->second.ib)
				{
					// blockbuffer contains the previously read data, since block data might be larger than the currently read data
					if (startofData < 0)
					{
						int prevlength = 0 - startofData;
						memcpy(blockdata, blockbuffer + (sizeof(blockbuffer) - prevlength), prevlength);
						if (endofData > 0)
						{
							memcpy(blockdata + prevlength, pbufRead, endofData);
						}
					}
					else
					{
						memcpy(blockdata, pbufRead + startofData, cb);
					}
					dwCRC = ComputeCRC(blockdata, cb);
					if (dwCRC == bt.dwCRC) // compare computed CRC of reconstructed block data with the CRC value present in the block trailer
					{
						*(this->file) << std::dec << ((iteration * clustersize) + i) << " found valid datablock BID: " << std::hex << bt.bid << " cb: " << std::dec << bt.cb << std::endl;
						blocksfound.insert(std::pair<BID, IB>(bt.bid, GetCurrentIB(headerBID, headeroffset, (startofData + (iteration * clustersize)))));
						result = TRUE;
					}
					else
					{
						*(this->file) << std::dec << ((iteration * clustersize) + i) << " found invalid datablock BID: " << std::hex << bt.bid << " cb: " << std::dec << bt.cb << std::endl;
						result = FALSE;
						*invalidBlockLoc = startofData + (iteration * clustersize);
					}
				}
			}
		}
	}
	if (blockbufferpos + buffersize <= sizeof(blockbuffer))
	{
		memcpy((BYTE*)&blockbuffer + blockbufferpos, pbufRead, buffersize);
	}
	else
	{
		// LIFO buffer, append the latest read data in the buffer and delete the oldest data in the buffer
		BYTE tempblockbuffer[8192];
		memcpy((BYTE*)&tempblockbuffer, blockbuffer, sizeof(blockbuffer));
		memcpy((BYTE*)&tempblockbuffer, blockbuffer + buffersize, (sizeof(blockbuffer) - buffersize));
		memcpy((BYTE*)&blockbuffer, tempblockbuffer, sizeof(blockbuffer));
		// append latest data
		memcpy((BYTE*)&blockbuffer + (sizeof(blockbuffer) - buffersize), pbufRead, buffersize);
	}
	// determine the current position in the blockbuffer
	if (blockbufferpos < sizeof(blockbuffer))
	{
		blockbufferpos += buffersize;
	}
	if (blockbufferpos > sizeof(blockbuffer))
	{
		blockbufferpos = sizeof(blockbuffer);
	}
	return result;
}