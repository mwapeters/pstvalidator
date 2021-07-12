#pragma once
#include <map>
#include "parsers.h"

class FileIntegrity
{
public:
	FileIntegrity(ULONGLONG headerOffset, ULONGLONG headerBID, ULONGLONG filesize, std::map<ULONGLONG, Recurringdata> recurringdata);
	bool VerifyRecurringStructures(ULONGLONG* corruptFrom);
	bool VerifyReferences(std::map<BID, IB> blockrefs, std::map<BID, IB> blocksfound, ULONGLONG* corruptFrom);
private:
	bool VerifyStructure(PageType pagetype, ULONGLONG firstentry, ULONGLONG interval, ULONGLONG* invalidFrom);
	ULONGLONG FirstStructureLoc(PageType pagetype);
	ULONGLONG headerOffset;
	ULONGLONG headerBID;
	ULONGLONG filesize;
	std::map<ULONGLONG, Recurringdata> recurringdata;
	ULONGLONG validFrom;
	ULONGLONG validUntil;
	std::map<ULONGLONG, BYTE*> filestatus;
};
