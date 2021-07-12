#pragma once
#include <fstream>
#include <map>
#include "parsers.h"

class BlockParser
{
public:
	BlockParser(std::ofstream* clusterReport, std::ofstream* fileReport, std::ofstream* datablocksReport, int clusterSize);
	bool ValidateBlocks(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG headeroffset, ULONGLONG headerBID, std::map<IB, BlockSignature> blockrefs, ULONGLONG* invalidBlockLoc);
	std::map<BID, IB> GetBlocksFound() { return blocksfound; }
	void PrintRefs(std::map<IB, BlockSignature> blockrefs);
private:

	int clustersize;
	BYTE blockbuffer[8192];  // 8192 is the maximum block size
	int blockbufferpos = 0;
	std::map<BID, IB> blocksfound; // contains the BID of the block and the offset relative in PST file (IB)
	std::ofstream* cluster = NULL;
	std::ofstream* file = NULL;
	std::ofstream* datablocks = NULL;
};