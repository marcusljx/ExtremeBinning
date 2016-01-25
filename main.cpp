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
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include "XB_includes.h"
#include "PrimaryIndex.h"

using namespace std;

#define WINDOW_SIZE 32
#define FINGERPRINT_DIVISOR 31
#define SLIDINGWINDOW_DIVISOR 29
#define SLIDINGWINDOW_REMAINDER 17

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

mappedFile* mapFileIntoMem_read(char* filePath) {
	int fd = open(filePath, O_RDONLY);
	if(fd==-1) m_err("Error reading file.");

	// assign mmap
	mappedFile* mf = new mappedFile;
	mf->contents_size = lseek(fd, 0, SEEK_END);
	mf->contents_ptr = (unsigned char*) mmap(NULL, mf->contents_size, PROT_READ, MAP_SHARED, fd, 0);
	if(mf->contents_ptr == NULL) m_err("Error setting memory pointer.");
	close(fd);

	return mf;
}

unsigned int fingerprint(unsigned char *arr_ptr) {	// simple fingerprint
	unsigned long tally = 0;
	int size = sizeof(arr_ptr)/ sizeof(unsigned char);
	for(int i=1; i<size; i++) {
		tally = (tally*10) + arr_ptr[i];	// Horner's Method?
	}

	return (unsigned int) (tally % FINGERPRINT_DIVISOR);	// result will fit definitely within (unsigned int)
}

string md5_hash(unsigned char* input, size_t size) {	// 32-bit md5 hash
	unsigned char CHUNK[size];
	unsigned char DIGEST[MD5_DIGEST_LENGTH];
	unsigned char hash[MD5_DIGEST_LENGTH * 2];
	memcpy(CHUNK, input, size);
	MD5((unsigned char *) &CHUNK, MD5_DIGEST_LENGTH, (unsigned char *) &DIGEST);
	for(int j=0; j<MD5_DIGEST_LENGTH; j++) {
		sprintf((char *) &hash[j * 2], "%02x", (unsigned int)DIGEST[j]);
	}

	string result(reinterpret_cast<const char*> (hash), strlen((const char *) hash));
	return result;
}

void chunkFile(char* filePath, Bin* binptr) {
	mappedFile* mf = mapFileIntoMem_read(filePath);
	unsigned char* contents = mf->contents_ptr;
	size_t fileLength = mf->contents_size;

	// containers for hashing
	unsigned char slidingWindow[WINDOW_SIZE];

	unsigned char* chunk_begin = contents;
	unsigned char* chunk_end;

	for(off_t i=0; i<(fileLength-WINDOW_SIZE); i++) {
		memcpy(slidingWindow, (contents+i), WINDOW_SIZE);

		if((fingerprint(slidingWindow) % SLIDINGWINDOW_DIVISOR == SLIDINGWINDOW_REMAINDER) || (i + WINDOW_SIZE == fileLength - 1 ) ){
			chunk_end = contents + i + WINDOW_SIZE;

			// Isolate Chunk
			size_t chunkSize = chunk_end - chunk_begin;
			unsigned char CHUNK[chunkSize];
			memcpy(CHUNK, chunk_begin, chunkSize);

			// create a new entry in the bin
			bin_entry newEntry;

			// calculate chunkID and fill entry
			newEntry.chunkID = md5_hash(chunk_begin, chunkSize);
			newEntry.chunkSize = chunkSize;
			newEntry.chunkContents = string(reinterpret_cast<const char*> (CHUNK), strlen((const char *) CHUNK));;
			binptr->insert(newEntry);

			cout << "[" << chunkSize << "] \t" << newEntry.chunkContents << " --> " << newEntry.chunkID << " \t";
			cout << "binptr size = " << binptr->size() << endl;

			// set chunk begin to next chunk
			chunk_begin = chunk_end + 1;
			i += WINDOW_SIZE;	// shift window to next chunk
			cout << endl;
		}
	}

	munmap(contents, fileLength);
}

string writeBinToDisk(char *destinationPath, Bin *binptr, string wholeFileHash) {	// writes a bin to disk, returns path to bin location
	string repChunkID = binptr->begin()->chunkID;	// using representative chunk id as directory name
	char destinationDirPath[PATH_MAX];
	strcpy(destinationDirPath, destinationPath);
	strcat(destinationDirPath, repChunkID.c_str());

	// Create directory for chunks
	if(mkdir(destinationDirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 ) {
		if(errno == EEXIST) {
			errno = 0;	// ignore error if bin directory already exists.
		} else {
			m_err("Error Creating Chunks Folder " + repChunkID);	// otherwise exit.
		}
	}
	// Write chunks to directory
	set<bin_entry , binEntryCompare>::iterator iter = binptr->begin();
	char destinationFilePath[PATH_MAX];
	while(iter != binptr->end()) {
		// create file in directory
		strcpy(destinationFilePath, destinationDirPath);
		strcat(destinationFilePath, "/");
		strcat(destinationFilePath, (*iter).chunkID.c_str());

		// write chunk content to chunk file (OVERWRITES FILE WITH SAME DATA(?) IF FILE EXISTS)
		ofstream fs;
		fs.open(destinationFilePath);
		fs << (*iter).chunkContents;
		fs.close();

		iter++;
	}

	//todo: write bin to disk (single file with details)	// eg. <repChunkId>_B.txt


	//todo: write file recipe to disk
}

void backupFile(char *filepath, char *destinationDirPath) {	// process for backing up a file
	Bin* binptr = new Bin;
	chunkFile(filepath, binptr);

	// Representative Chunk ID is smallest value
	string repChunkID = (binptr->begin())->chunkID;
	cout << "Total number of Chunks = " << binptr->size() << endl;
	cout << "Representative Chunk ID: \t" << repChunkID << endl;

	// Calculate whole file hash
	mappedFile* mfile = mapFileIntoMem_read(filepath);
	string wholeFileHash = md5_hash(mfile->contents_ptr, mfile->contents_size);
	cout << "Whole File Hash: " << wholeFileHash << endl;

	//todo: check if repChunkID found in primaryIndex and branch as necessary
	PrimaryIndexEntry* EntryFound = primaryIndex->findEntry(repChunkID);
	if( EntryFound == nullptr) {	// entry does not exists
		// Write Chunks to disk
		string binPath = writeBinToDisk(destinationDirPath, binptr, wholeFileHash);

		// Add bin as entry to primary index
		primaryIndex->addEntry(repChunkID, wholeFileHash, binPath);
	}

}

void backupDir(char* targetDirPath, char* destinationDirPath) {
	DIR* dirD = opendir(targetDirPath);
	if(!dirD) {
		m_err("Error Reading Target Directory");
	}

	struct dirent* dir = readdir(dirD);	// read first file
	while(dir) {	// read through directory items
		char *name = dir->d_name;

		if ((strcmp(name, ".") != 0) && (strcmp(name, "..") != 0)) {    // don't operate on parent directories
			if (!opendir(dir->d_name)) {    // item is not another directory
				char filePath[PATH_MAX];
				strcpy(filePath, targetDirPath);
				strcat(filePath, name);

				// perform backup on file
				backupFile(filePath, destinationDirPath);
			} // Q: possible recursive backup if it's a directory? And how to handle symbolic links?
		}
		dir = readdir(dirD);	// read next file
	}
}



int main(int argc, char* argv[]) {
	// Initialise Primary Index in heap
	primaryIndex = new PrimaryIndex;

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