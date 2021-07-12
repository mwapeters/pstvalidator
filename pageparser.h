#pragma once
#include <fstream>
#include <map>
#include "ndbport.h"
#include "parsers.h"

class PageParser
{
public:
	PageParser(std::ofstream* clusterReport, std::ofstream* fileReport, std::ofstream* datablocksReport, int clusterSize);
	bool ValidatePages(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG headeroffset, ULONGLONG headerBID, ULONGLONG* invalidPageLoc); // returns true if page found in provided data.
	bool FindHeader(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG* offset, ULONGLONG* bid);
	std::map<BID, BlockSignature> GetBlockRefs() { return blockrefs; }
	std::map<BID, IB> GetBlockLocs() { return blocklocs; }
	std::map<ULONGLONG, Recurringdata> GetRecurringData() { return recurringdata; }
private:
	bool IsAllocatedPage(ULONGLONG pageloc);
	bool ValidPageTrailer(PAGETRAILER pt, ULONGLONG headeroffset);
	ULONGLONG lastDetectedAmapLoc;
	BYTE lastAmap[cbPage];  // first 496 bytes indicate the page allocation status
	std::map<ULONGLONG, BYTE*> allocations;
	std::map<BID, BlockSignature> blockrefs;   // contains the found references, used for finding block trailers
	std::map<BID, IB> blocklocs; // contains the found references, used for matching references with found blocks
	std::map<ULONGLONG, Recurringdata> recurringdata;
	std::ofstream* cluster = NULL;
	std::ofstream* file = NULL;
	std::ofstream* datablocks = NULL;
	int clustersize;
};
