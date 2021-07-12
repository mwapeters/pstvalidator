#include "fileintegrity.h"
#include <iostream>

FileIntegrity::FileIntegrity(ULONGLONG headerOffset, ULONGLONG headerBID, ULONGLONG filesize, std::map<ULONGLONG, Recurringdata> recurringdata)
{
	this->headerOffset = headerOffset;
	this->headerBID = headerBID;
	this->filesize = filesize;
	this->recurringdata = recurringdata;
}

bool FileIntegrity::VerifyStructure(PageType pagetype, ULONGLONG firstentry, ULONGLONG interval, ULONGLONG* invalidFrom)
{
	*invalidFrom = -1;
	bool result = TRUE;
	// calculate the first occurance of a recurring structure, this depends on which part (fragment) of a PST file is analyzed
	ULONGLONG calculatedfirstentry = -1;
	if (headerBID == 0)
	{
		calculatedfirstentry = firstentry;
	}
	else
	{
		for (ULONGLONG loc = firstentry; loc < filesize && loc <= headerBID && calculatedfirstentry == -1; loc += interval)
		{
			calculatedfirstentry = loc;
		}
	}
	std::map<ULONGLONG, Recurringdata>::iterator it;
	for (ULONGLONG loc = headerOffset + calculatedfirstentry - headerBID; loc < filesize; loc += interval)
	{
		it = recurringdata.find(loc);
		if (it != recurringdata.end())
		{
			if (it->second.type != pagetype)
			{
				result = FALSE;
				*invalidFrom = loc;
			}
			else
			{
				if (it->second.bid != GetCurrentIB(headerBID, headerOffset, loc)) // normalized IB
				{
					result = FALSE;  // file format: each BID of the AMAP, PMAP, FMAP and FPMAP contains the absolute file offset
					*invalidFrom = loc;
				}
			}
		}
		else
		{
			*invalidFrom = loc;
			result = FALSE;
		}
	}
	return result;
}

ULONGLONG FileIntegrity::FirstStructureLoc(PageType pagetype)
{
	ULONGLONG first = -1;
	std::map<ULONGLONG, Recurringdata>::iterator it;
	for (it = recurringdata.begin(); it != recurringdata.end(); ++it)
	{
		if (it->second.type == pagetype && first == -1)
		{
			first = it->first;
			if (headerOffset == -1)
			{
				headerOffset = it->second.bid - it->first;
			}
		}
	}
	return first;
}

bool FileIntegrity::VerifyRecurringStructures(ULONGLONG* corruptFrom)
{
	bool result = TRUE;
	*corruptFrom = -1;
	ULONGLONG invalidFrom;
	result = this->VerifyStructure(PageType::Amap, 0x4400, 253952, &invalidFrom);
	if (!result && invalidFrom < *corruptFrom)
	{
		*corruptFrom = invalidFrom;
	}
	result = this->VerifyStructure(PageType::Pmap, 0x4600, 2031616, &invalidFrom);
	if (!result && invalidFrom < *corruptFrom)
	{
		*corruptFrom = invalidFrom;
	}
	result = this->VerifyStructure(PageType::Fmap, 32523264, 125960192, &invalidFrom);
	if (!result && invalidFrom < *corruptFrom)
	{
		*corruptFrom = invalidFrom;
	}
	result = this->VerifyStructure(PageType::Fpmap, 2080392192, 8061452288, &invalidFrom);
	if (!result && invalidFrom < *corruptFrom)
	{
		*corruptFrom = invalidFrom;
	}
	return (*corruptFrom == -1);
}

bool FileIntegrity::VerifyReferences(std::map<BID, IB> blockrefs, std::map<BID, IB> blocksfound, ULONGLONG* corruptFrom)
{
	bool result = TRUE;
	*corruptFrom = -1;
	std::map<BID, IB>::iterator blockrefsit;
	std::map<BID, IB>::iterator blocksfoundit;
	for (blockrefsit = blockrefs.begin(); blockrefsit != blockrefs.end() && *corruptFrom == -1; ++blockrefsit)
	{
		blocksfoundit = blocksfound.find(blockrefsit->first);
		if (blocksfoundit != blocksfound.end())
		{
			if (blockrefsit->second != blocksfoundit->second)
			{
				// found block at unexpected location			
				std::cout << "unexpected datablock, BID: " << blockrefsit->first << " IB: " << blockrefsit->second << std::endl;
				*corruptFrom = FromIBToFileLocation(headerBID, headerOffset, blocksfoundit->second);
				result = FALSE;
			}
			else
			{
				result = TRUE;
			}
		}
		else
		{
			std::cout << "missing datablock, BID: " << blockrefsit->first << " IB: " << blockrefsit->second << std::endl;	
			*corruptFrom = FromIBToFileLocation(headerBID, headerOffset, blockrefsit->second);
			result = FALSE;
		}
	}
	return result;
}
