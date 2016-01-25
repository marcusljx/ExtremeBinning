#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <openssl/md5.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <string>
#include <map>
#include <bits/stl_set.h>
#include <linux/limits.h>
#include "XB_includes.h"
#include "PrimaryIndex.h"

using namespace std;

#define WINDOW_SIZE 16
#define FINGERPRINT_DIVISOR 5
#define SLIDINGWINDOW_DIVISOR 3
#define SLIDINGWINDOW_REMAINDER 2

PrimaryIndex* primaryIndex;

//extern function declared in XB_includes.h
bool compareHexStrings(string A, string B) {
	if(A.size() == B.size()) { // sizes same, compare characters
		for(int i=0; i<A.size(); i++) {
			if(A[i] < B[i]) {
				return true;
			} else if (A[i] > B[i]) {
				return false;
			} // else check next character
		}
	}
	// if sizes doesn't match, order by size (clearly 0x100 > 0xff)
	return (A.size() < B.size());
}

//======================================
void m_err(string error_message) {
	perror(error_message.c_str());
	exit(EXIT_FAILURE);
}

unsigned int fingerprint(unsigned char *arr_ptr) {	// simple fingerprint
	unsigned long tally = 0;
	int size = sizeof(arr_ptr)/ sizeof(unsigned char);
	for(int i=1; i<size; i++) {
		tally = (tally*10) + arr_ptr[i];	// Horner's Method?
	}

	return (unsigned int) (tally % FINGERPRINT_DIVISOR);	// result will fit definitely within (unsigned int)
}

void chunkFile(char* filePath, Bin* binptr) {
	// read file into mapped memory
	int fd = open(filePath, O_RDONLY);
	if(fd==-1) m_err("Error reading file.");

	// assign mmap
	off_t fileLength = lseek(fd, 0, SEEK_END);
	unsigned char* contents = (unsigned char*) mmap(NULL, fileLength, PROT_READ, MAP_SHARED, fd, 0);
	if(contents==NULL) m_err("Error setting memory pointer.");
	close(fd);

	// containers for hashing
	unsigned char slidingWindow[WINDOW_SIZE];

	unsigned char* chunk_begin = contents;
	unsigned char* chunk_end;

//	bin_entry* newEntry;

	for(off_t i=0; i<(fileLength-WINDOW_SIZE); i++) {
		memcpy(slidingWindow, (contents+i), WINDOW_SIZE);

		if((fingerprint(slidingWindow) % SLIDINGWINDOW_DIVISOR == SLIDINGWINDOW_REMAINDER) || (i + WINDOW_SIZE == fileLength - 1 ) ){
			chunk_end = contents + i + WINDOW_SIZE;

			// FIND CHUNK ID
			size_t chunkSize = chunk_end - chunk_begin;
			unsigned char CHUNK[chunkSize];
			unsigned char DIGEST[MD5_DIGEST_LENGTH];
			unsigned char ChunkID[MD5_DIGEST_LENGTH*2];
			memcpy(CHUNK, chunk_begin, chunkSize);
			MD5((unsigned char *) &CHUNK, MD5_DIGEST_LENGTH, (unsigned char *) &DIGEST);
			for(int j=0; j<MD5_DIGEST_LENGTH; j++) {
				sprintf((char *) &ChunkID[j * 2], "%02x", (unsigned int)DIGEST[j]);
			}
			cout << "[" << chunkSize << "] \t" << CHUNK << " --> " << ChunkID << " \t";

			// create a new entry in the bin
			bin_entry newEntry;
			string tempID(reinterpret_cast<const char*> (ChunkID), strlen((const char *) ChunkID));
			string tempChunk(reinterpret_cast<const char*> (CHUNK), strlen((const char *) CHUNK));

			newEntry.chunkID = tempID;
			newEntry.chunkSize = chunkSize;
			newEntry.chunkContents = tempChunk;
			binptr->insert(newEntry);
			cout << "binptr size = " << binptr->size() << endl;

			// set chunk begin to next chunk
			chunk_begin = chunk_end + 1;
			i += WINDOW_SIZE;	// shift window to next chunk
			cout << endl;
		}
	}

	munmap(contents, fileLength);
}

void writeBinToDisk(char *destinationPath, Bin *binptr) {	// writes a bin to disk
	//todo: write chunks to disk (files in destination)

	//todo: write bin to disk (single file with details)
}

void backupFile(char *filepath, char *destinationDirPath) {	// process for backing up a file
	Bin* binptr = new Bin;
	chunkFile(filepath, binptr);

	// Representative Chunk ID is smallest value
	string repChunkID = (binptr->begin())->chunkID;
	cout << "Total number of Chunks = " << binptr->size() << endl;
	cout << "Representative Chunk ID: \t" << repChunkID << endl;

	//todo: check if repChunkID found in primaryIndex and branch as necessary

}

void backupDir(char* targetDirPath, char* destinationDirPath) {
	//todo: loop through files in dir and perform backup on each

}



int main(int argc, char* argv[]) {
	// Initialise Primary Index in heap
	primaryIndex = new PrimaryIndex;

	cout << "0:" << argv[0] << endl;
	cout << "1:" << argv[1] << endl;
	cout << "2:" << argv[2] << endl;
	cout << "3:" << argv[3] << endl;

	if(strcmp(argv[1], "-b")==0) {	// backup directory
		// set fullpath of target dir
		char targetDir[PATH_MAX];
		getcwd(targetDir, PATH_MAX);
		strcat(targetDir, "/");
		strcat(targetDir, argv[2]);

		// set fullpath of destination dir
		char destDir[PATH_MAX];
		getcwd(destDir, PATH_MAX);
		strcat(destDir, "/");
		strcat(destDir, argv[3]);

		backupDir(targetDir, destDir);
	}

	return 0;
}