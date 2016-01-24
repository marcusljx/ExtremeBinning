//
// Created by marcusljx on 23/01/16.
//

#ifndef EXTREMEBINNING_PRIMARYINDEX_H
#define EXTREMEBINNING_PRIMARYINDEX_H


#include <cstdlib>
#include <string>
#include <vector>
#include "XB_includes.h"

using namespace std;

struct PrimaryIndexEntry {
	string RepresentativeChunkID;
	string WholeFileHash;
	string BinPath;
};

class PrimaryIndex {
private:
	vector<PrimaryIndexEntry*> index;

public:
	PrimaryIndex();
	~PrimaryIndex();

	void addEntry(string repChunkId, string hash, string BinPath);	// adds an entry based on representative chunkID
	PrimaryIndexEntry* findEntry(string repChunkId);					// finds an entry based on representative chunkID, returns whole entry


};


#endif //EXTREMEBINNING_PRIMARYINDEX_H
