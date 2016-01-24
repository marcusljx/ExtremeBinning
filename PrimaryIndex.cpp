//
// Created by marcusljx on 23/01/16.
//

#include <string.h>
#include "PrimaryIndex.h"

PrimaryIndex::PrimaryIndex() {

}

PrimaryIndex::~PrimaryIndex() {}

void PrimaryIndex::addEntry(string repChunkId, string hash, string BinPath) {
	PrimaryIndexEntry* newEntry = new PrimaryIndexEntry;
	newEntry->RepresentativeChunkID = repChunkId;
	newEntry->WholeFileHash = hash;
	newEntry->BinPath = BinPath;
	index.push_back(newEntry);
}

PrimaryIndexEntry* PrimaryIndex::findEntry(string repChunkId) {
	for(int i=0; i<index.size(); i++) {
		if(index[i]->RepresentativeChunkID == repChunkId) {
			return index[i];	// if found, return whole entry
		}
	}
	// if not found, return nullptr
	return nullptr;
}