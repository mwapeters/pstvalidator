#include "stdafx.h"
#include "pageparser.h"
#include "ndbport.h"
#include <iostream>

PageParser::PageParser(std::ofstream* clusterReport, std::ofstream* fileReport, std::ofstream* datablocksReport, int clusterSize)
{
	cluster = clusterReport;
	file = fileReport;
	datablocks = datablocksReport;
	lastDetectedAmapLoc = 0;
	clustersize = clusterSize;
	for (int p = 0; p < sizeof(lastAmap); p++)
	{
		lastAmap[p] = 0; // Initialize all elements to zero.
	}
}

PageType getPageType(PTYPE type)
{
	PageType result = PageType::Unrecognized;
	if (type == ptypeAMap)
	{
		result = PageType::Amap;
	}
	if (type == ptypePMap)
	{
		return PageType::Pmap;
	}
	if (type == ptypeFMap)
	{
		return PageType::Fmap;
	}
	if (type == ptypeFPMap)
	{
		return PageType::Fpmap;
	}
	return result;
}

bool PageParser::ValidatePages(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG headeroffset, ULONGLONG headerBID, ULONGLONG* invalidPageLoc)
{
	BYTE page[cbPage];
	BTPAGE btPage;
	PAGETRAILER pt;
	DWORD dwCRC;
	WORD wSig;
	bool invalidPageDetected = FALSE;
	*invalidPageLoc = 0;

	for (ULONGLONG i = 0; i < (buffersize - sizeof(page)); i ++)  // pages occur at 64 byte interval
	{
		ULONGLONG currentLocation = (iteration * clustersize) + i;
		memcpy(page, (pbufRead + i), sizeof(page)); // copy page
		dwCRC = ComputeCRC(page, cbPageData);
		memcpy((BYTE*)&pt, (page + (cbPage - sizeof(PAGETRAILER))), sizeof(pt));  // copy page trailer
		if ((pt.ptype == ptypeBBT || pt.ptype == ptypeNBT || pt.ptype == ptypeFMap || pt.ptype == ptypePMap || pt.ptype == ptypeAMap || pt.ptype == ptypeFPMap) && (pt.ptype == pt.ptypeRepeat))
		{
			if (dwCRC == pt.dwCRC)
			{
				*(this->file) << std::hex << (currentLocation) << "Valid page found with BID: 0x" << std::hex << pt.bid << std::endl;
				Recurringdata newEntry;
				newEntry.bid = pt.bid;
				newEntry.type = getPageType(pt.ptype);
				if (newEntry.type != PageType::Unrecognized)
				{
					recurringdata.insert(std::pair<ULONGLONG, Recurringdata>(currentLocation, newEntry));
					if (newEntry.type == PageType::Amap)
					{
						// store the last detected AMap, used for finding references from allocated pages
						lastDetectedAmapLoc = currentLocation;
						memcpy(lastAmap, page, sizeof(lastAmap));
						allocations.insert(std::pair<ULONGLONG, BYTE*>(currentLocation, page));
					}
				}
				if (pt.ptype == ptypeBBT || pt.ptype == ptypeNBT)
				{
					memcpy((BYTE*)&btPage, page, sizeof(btPage));
					if (pt.ptype == ptypeBBT && btPage.cLevel == 0)
					{
						//btpage contains a list of bbtentry in rgentries
						BBTENTRY* pbbtEntry = (BBTENTRY*)(btPage.rgbte);
						*(this->file) << "found btpage, cbent: " << std::dec << (ULONGLONG)btPage.cbEnt << " entries: " << std::dec << (ULONGLONG)btPage.cEnt << " max entries: " << std::dec << (ULONGLONG)btPage.cEntMax << std::endl;
						for (int entry = 0; entry < btPage.cEnt; entry++, pbbtEntry++)
						{
							*(this->file) << "(data) ref BID 0x" << std::hex << pbbtEntry->bref.bid << " IB: " << std::hex << pbbtEntry->bref.ib << " size (bytes): " << std::dec << pbbtEntry->cb << " cref: " << std::dec << pbbtEntry->cRef << std::endl;
							wSig = ComputeSig(pbbtEntry->bref.ib, pbbtEntry->bref.bid);
							*(this->datablocks) << std::dec << pbbtEntry->bref.bid << " " << std::dec << pbbtEntry->cb << " " << std::dec << wSig << std::endl;
							//  add blockref, checks whether it is already present.
							BlockSignature newEntry;
							newEntry.bid = pbbtEntry->bref.bid;
							newEntry.cb = pbbtEntry->cb;
							newEntry.wSig = wSig;
							newEntry.ib = pbbtEntry->bref.ib;
							if (IsAllocatedPage(currentLocation))
							{
								blockrefs.insert(std::pair<BID, BlockSignature>(pbbtEntry->bref.bid, newEntry));
								blocklocs.insert(std::pair<BID, IB>(pbbtEntry->bref.bid, pbbtEntry->bref.ib));
							}
						}
					}
				}
			}
			else
			{
				if (ValidPageTrailer(pt, GetCurrentIB(headerBID, headeroffset, currentLocation)))
				{
					//  invalid page at allocated location is an indication of corruption
					if (IsAllocatedPage(currentLocation))
					{
						invalidPageDetected = TRUE;
						*invalidPageLoc = currentLocation;
					}
				}
			}
		}
	}
	return !invalidPageDetected;
}

// AMap pages can be used as header for validation, recognizable and contains allocation information of the next file section.
bool PageParser::FindHeader(BYTE* pbufRead, int buffersize, int iteration, ULONGLONG* offset, ULONGLONG* bid)
{
	BYTE page[cbPage];
	PAGETRAILER pt;
	DWORD dwCRC;
	bool result = FALSE;
	for (ULONGLONG i = 0; i < (buffersize - sizeof(page)); i += 64)  // pages occur at 64 byte interval
	{
		ULONGLONG currentLocation = (iteration * clustersize) + i;
		memcpy(page, (pbufRead + i), sizeof(page)); // copy page
		dwCRC = ComputeCRC(page, cbPageData);
		memcpy((BYTE*)&pt, (page + (cbPage - sizeof(PAGETRAILER))), sizeof(pt));  // copy page trailer
		if (pt.ptype == ptypeAMap && pt.ptype == pt.ptypeRepeat)
		{
			if (dwCRC == pt.dwCRC)
			{
				*offset = (iteration * clustersize) + i;
				*bid = pt.bid;
				result = TRUE;
				*(this->file) << std::hex << (currentLocation) << "Valid AMap page found with BID: 0x" << std::hex << pt.bid << std::endl;
				Recurringdata newEntry;
				newEntry.bid = pt.bid;
				newEntry.type = getPageType(pt.ptype);
				if (newEntry.type != PageType::Unrecognized)
				{
					recurringdata.insert(std::pair<ULONGLONG, Recurringdata>(currentLocation, newEntry));
					if (newEntry.type == PageType::Amap)
					{
						// store the last detected AMap, used for finding references from allocated pages
						lastDetectedAmapLoc = currentLocation;
						memcpy(lastAmap, page, sizeof(lastAmap));
						allocations.insert(std::pair<ULONGLONG, BYTE*>(currentLocation, page));
					}
				}
			}
		}
	}
	
	return result;
}

bool PageParser::IsAllocatedPage(ULONGLONG pageloc)
{
	bool result = FALSE;
	ULONGLONG maxAmapLoc = lastDetectedAmapLoc + (8 * 64 * 496);
	int aMapEntry = 0;
	if (pageloc >= lastDetectedAmapLoc && pageloc <= maxAmapLoc)
	{
		aMapEntry = (pageloc - lastDetectedAmapLoc) / 512;  // first entry in the AMap is the AMap itself, 1 bit maps to 64 bytes
		if (lastAmap[aMapEntry] == 0xFF)  // 1 page is 512 bytes, the value of 8 bits needs to be true (8*64=512) 
		{
			result = TRUE;
		}
	}
	return result;
}

bool PageParser::ValidPageTrailer(PAGETRAILER pt, ULONGLONG ib)
{
	bool result = FALSE;
	if ((pt.ptype == ptypeBBT || pt.ptype == ptypeNBT || pt.ptype == ptypeFMap || pt.ptype == ptypePMap || pt.ptype == ptypeAMap || pt.ptype == ptypeFPMap) && (pt.ptype == pt.ptypeRepeat))
	{
		if ((pt.ptype == ptypeFMap || pt.ptype == ptypePMap || pt.ptype == ptypeAMap || pt.ptype == ptypeFPMap) && (pt.wSig == 0))
		{
			result = TRUE;
			if (ib != -1 && pt.bid != ib)
			{
				result = FALSE;
			}
		}
		if ((pt.ptype == ptypeBBT || pt.ptype == ptypeNBT) && (pt.wSig == ComputeSig(ib, pt.bid)) && (ib != -1))
		{
			result = TRUE;
		}
	}
	return result;
}
