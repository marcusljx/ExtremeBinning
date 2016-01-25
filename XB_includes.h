//
// Created by marcusljx on 23/01/16.
//

#ifndef EXTREMEBINNING_XB_INCLUDES_H
#define EXTREMEBINNING_XB_INCLUDES_H

#include <cstdlib>
#include <string>
#include <set>

using namespace std;

//=========================================== BIN STUFF
struct bin_entry {
	string chunkID;
	size_t chunkSize;
	string chunkContents = "";
};

extern bool compareHexStrings(string A, string B);


struct binEntryCompare {
	bool operator() (const bin_entry A, const bin_entry B) const {	// assuming all chunkID's size is same.
		return compareHexStrings(A.chunkID, B.chunkID);
	}
};

typedef set<bin_entry , binEntryCompare> Bin;

//============================================ FILE READING STUFF
struct mappedFile {
	char* contents_ptr;
	size_t contents_size;
};



#endif //EXTREMEBINNING_XB_INCLUDES_H
