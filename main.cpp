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
#include <limits>
#include "XB_includes.h"
#include "PrimaryIndex.h"

using namespace std;

#define WINDOW_SIZE 32
#define FINGERPRINT_DIVISOR 31
#define SLIDINGWINDOW_DIVISOR 29
#define SLIDINGWINDOW_REMAINDER 17

PrimaryIndex* primaryIndex;
size_t initialTargetDirPathLength;

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

void chunkFile(char* filePath, Bin* binptr, vector<string>* recipe) {
	mappedFile* mf = mapFileIntoMem_read(filePath);
	unsigned char* contents = mf->contents_ptr;
	size_t fileLength = mf->contents_size;

	// containers for hashing
	unsigned char slidingWindow[WINDOW_SIZE];

	unsigned char* chunk_begin = contents;
	unsigned char* chunk_end;

	int r=0;	// iterator for recipe

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
			recipe->push_back(newEntry.chunkID);

//			cout << "[" << chunkSize << "] \t" << newEntry.chunkContents << " --> " << newEntry.chunkID << " \t";
//			cout << "binptr size = " << binptr->size() << endl;

			// set chunk begin to next chunk
			chunk_begin = chunk_end + 1;
			i += WINDOW_SIZE;	// shift window to next chunk
		}
	}

	munmap(contents, fileLength);
}

string writeBinToDisk(char *destinationPath, Bin *binptr, vector<string>* recipe_ptr, string wholeFileHash) {	// writes a bin to disk, returns path to bin location
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
	// Prepare to write chunks to directory
	set<bin_entry , binEntryCompare>::iterator iter = binptr->begin();
	char destinationFilePath[PATH_MAX];
	ofstream fs_chunks;

	// Prepare to write chunk info to bin
	char binFilePath[PATH_MAX];
	strcpy(binFilePath, destinationDirPath);
	strcat(binFilePath, "_bin.txt");
	ofstream fs_forBinAndRecipeFiles;
	fs_forBinAndRecipeFiles.open(binFilePath);

	while(iter != binptr->end()) {
		// create file in directory
		strcpy(destinationFilePath, destinationDirPath);
		strcat(destinationFilePath, "/");
		strcat(destinationFilePath, (*iter).chunkID.c_str());

		// write chunk content to chunk file (OVERWRITES FILE WITH SAME DATA(?) IF FILE EXISTS)
		fs_chunks.open(destinationFilePath);
		fs_chunks << (*iter).chunkContents;
		fs_chunks.close();

		// Write chunk info to binFile (chunk is already unique)
		string line = (*iter).chunkID + " \t" + to_string((*iter).chunkSize) + "\n";
		fs_forBinAndRecipeFiles << line;

		iter++;
	}
	fs_forBinAndRecipeFiles.close();

	// File recipe includes binID, original path, and chunk order. File name is r_<wholeFileHash>.recipe
	char recipeFile[PATH_MAX];
	strcpy(recipeFile, destinationPath);
	strcat(recipeFile, "r_");	// r for recipe
	strcat(recipeFile, wholeFileHash.c_str());
	strcat(recipeFile, ".recipe");

	// Write order of ChunkIDs to recipe file
	fs_forBinAndRecipeFiles.open(recipeFile);
	fs_forBinAndRecipeFiles << repChunkID + "\n";	// add the binID (representative chunkID)
	for(int i=0; i<recipe_ptr->size(); i++) {	// first entry is original filepath, the rest are order of the chunks
		fs_forBinAndRecipeFiles << (*recipe_ptr)[i] + "\n";
	}
	fs_forBinAndRecipeFiles.close();

	return string(binFilePath);
}

void addTargetPathToRecipeFile(char* recipeFilePath, string originalFilePath) {	// inserts a duplicate file's filepath in a recipe
	string lineToAdd = originalFilePath + "\n";

	// Split old recipe at first linebreak
	char* memblock = (char *) mapFileIntoMem_read(recipeFilePath)->contents_ptr;
	string contents(memblock);
	size_t breakpos = contents.find_first_of('\n');
	string before = contents.substr(0,breakpos);
	string after = contents.substr(breakpos+1);

	// Write new data to file
	fstream fs(recipeFilePath);
	fs << before + "\n" + lineToAdd + after;
	fs.close();
}

void backupFile(char *filepath, char *destinationDirPath) {	// process for backing up a file
	// Chunk file into bin object
	Bin* binptr = new Bin;
	vector<string>*recipe_ptr = new vector<string>;

	// Relative Path from initial target directory
	string relativePath = string(filepath).substr(initialTargetDirPathLength);	// cut away the number of chars of the initial target dir
	recipe_ptr->push_back(relativePath);	// first entry in recipe is the original filepath.

	// Chunk the file
	chunkFile(filepath, binptr, recipe_ptr);

	// Representative Chunk ID is smallest value
	string repChunkID = (binptr->begin())->chunkID;

	// Calculate whole file hash
	mappedFile* mfile = mapFileIntoMem_read(filepath);
	string wholeFileHash = md5_hash(mfile->contents_ptr, mfile->contents_size);

	PrimaryIndexEntry*found_piEntry = primaryIndex->findEntry(repChunkID);
	if(found_piEntry == nullptr) {	// entry does not exist
		// Write Chunks, Bin, and Recipe to disk
		string binFilePath = writeBinToDisk(destinationDirPath, binptr, recipe_ptr, wholeFileHash);
		// Add bin (entry) to primary index
		primaryIndex->addEntry(repChunkID, wholeFileHash, binFilePath);

	} else {	// entry exists
		// Compare whole file hash
		if(found_piEntry->WholeFileHash != wholeFileHash) {	// if whole file hash doesn't match
			//todo: read binFile and add binFile's chunkIDs into current bin (same as adding current bin's entries into binFile, because all duplicates will be removed)

		} else {
			// Reconstruct Recipe File Path
			char recipeFilePath[PATH_MAX];
			strcpy(recipeFilePath, destinationDirPath);
			strcat(recipeFilePath, "r_");
			strcat(recipeFilePath, wholeFileHash.c_str());
			strcat(recipeFilePath, ".recipe");

			addTargetPathToRecipeFile(recipeFilePath, (*recipe_ptr)[0]);	// first entry in recipe is target file path
		}

	}

}

void backupDir(char* targetDirPath, char* destinationDirPath) {
	DIR* dirD = opendir(targetDirPath);
	if(!dirD) {
		m_err("Error Reading Target Directory");
	}

	struct dirent* item;	// read first file
	struct stat st;
	item = readdir(dirD);
	while(item) {	// read through directory items
		char *name = item->d_name;
		char filePath[PATH_MAX];
		strcpy(filePath, targetDirPath);
		strcat(filePath, name);

		if ((strcmp(name, ".") != 0) && (strcmp(name, "..") != 0)) {    // don't operate on parent directories
			if(lstat(filePath, &st) != 0) m_err("Error calling lstat() on file.");	// check for directory or file

			// item is not another directory
			if(S_ISREG(st.st_mode)) {

				// perform backup on file
				backupFile(filePath, destinationDirPath);
			} else {
				cout << "Item is Directory" << endl;
			}
		}
		item = readdir(dirD);
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
		initialTargetDirPathLength = strlen(targetDir);	// set length of path (used for finding relative path for recipe)

		// set fullpath of destination dir
		char destDir[PATH_MAX];
		getcwd(destDir, PATH_MAX);
		strcat(destDir, "/");
		strcat(destDir, argv[3]);

		backupDir(targetDir, destDir);
	}

	return 0;
}