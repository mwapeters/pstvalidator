#pragma once
#include <fstream>

class Reporter
{
public:
	Reporter();
	std::ofstream* ClusterReport() { return &cluster; }
	std::ofstream* FileReport() { return &file; }
	std::ofstream* DataBlocksReport() { return &datablocks; }
private:
	std::ofstream cluster;
	std::ofstream file;
	std::ofstream datablocks;
};