//
// Created by marcusljx on 23/01/16.
//

#include <string.h>
#include "PrimaryIndex.h"

PrimaryIndex::PrimaryIndex() {

}

PrimaryIndex::~PrimaryIndex() {}

void PrimaryIndex::addEntry(Bin* binptr, string hash, string BinPath) {
	PrimaryIndexEntry newEntry;
	newEntry.RepresentativeChunkID = binptr->begin()->chunkID;
	newEntry.WholeFileHash = hash;
	newEntry.BinPath = BinPath;
}

PrimaryIndexEntry PrimaryIndex::findEntry(Bin *binptr) {
	string repChunkId = binptr->begin()->chunkID;
	for(int i=0; i<index.size(); i++) {
		if(index[i].RepresentativeChunkID == repChunkId) {
			return index[i];	// if found, return whole entry
		}
	}
	// if not found, return nullptr
	return nullptr;
}