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
	void* BinPointer;
};

class PrimaryIndex {
private:
	vector<PrimaryIndexEntry*> index;

public:
	PrimaryIndex();
	~PrimaryIndex();

	void addEntry(Bin* binptr);		// adds an entry based on representative chunkID
	PrimaryIndexEntry* findEntry(Bin* binptr);	// finds an entry based on representative chunkID, returns whole entry


};


#endif //EXTREMEBINNING_PRIMARYINDEX_H