#include "stdafx.h"
#include "parsers.h"
#include "ndbport.h"
#include <iostream>
#include <fstream>

//  returns the absolute file offset within the PST file.
ULONGLONG GetCurrentIB(ULONGLONG headerBID, ULONGLONG headeroffset, ULONGLONG currentLoc)
{
	return (currentLoc - headeroffset) + headerBID;
}

// returns the file offset within the data under analyses with respect to a specific absolute file offset of the PST file (IB)
ULONGLONG FromIBToFileLocation(ULONGLONG headerBID, ULONGLONG headeroffset, ULONGLONG IB)
{
	return headeroffset + (IB - headerBID);
}
