#include "logging.h"

Reporter::Reporter()
{
	cluster = std::ofstream("cluster.txt");
	file = std::ofstream("file.txt");
	datablocks = std::ofstream("datablocks.txt");
}