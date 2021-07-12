#pragma once
#include <afx.h>
#include "ndbport.h"

struct BlockSignature
{
	BID bid;
	WORD cb;
	WORD wSig;
	IB ib;
};

enum class PageType
{
	Amap,
	Pmap,
	Fmap,
	Fpmap,
	Unrecognized
};

struct Recurringdata
{
	BID bid;
	PageType type;
};

ULONGLONG GetCurrentIB(ULONGLONG headerBID, ULONGLONG headeroffset, ULONGLONG currentLoc);
ULONGLONG FromIBToFileLocation(ULONGLONG headerBID, ULONGLONG headeroffset, ULONGLONG IB);