#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <openssl/md5.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
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
size_t initialTargetDirPathLength;

const string pathIdentifierPrefix = "#";

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

mappedFile* mapFileIntoMem_read(string filePath) {
	int fd = open(filePath.c_str(), O_RDONLY);
	if(fd==-1) m_err("Error reading file.");

	// assign mmap
	mappedFile* mf = new mappedFile;
	mf->contents_size = lseek(fd, 0, SEEK_END);
	mf->contents_ptr = (char*) mmap(NULL, mf->contents_size, PROT_READ, MAP_SHARED, fd, 0);
	if(mf->contents_ptr == NULL) m_err("Error setting memory pointer.");
	close(fd);

	return mf;
}

int fingerprint(string input) {	// simple fingerprint
	long tally = 0;
	long size = input.size();
	for(int i=1; i<size; i++) {
		tally = (tally*10) + input[i];	// Horner's Method?
	}

	return (int) (tally % FINGERPRINT_DIVISOR);	// result will fit definitely within (unsigned int)
}

string md5_hash(char *input) {	// 32-bit md5 hash
	unsigned char digest[16];
	MD5_CTX ctx;
	MD5_Init(&ctx);
	MD5_Update(&ctx, input, strlen((const char *) input));
	MD5_Final(digest, &ctx);

	char hash[33];

	for(int j=0; j<16; j++) {
		sprintf(&hash[j * 2], "%02x", (unsigned int)digest[j]);
	}
	string result(reinterpret_cast<const char*> (hash), strlen((const char *) hash));
	return result;
}

void chunkFile(string filePath, Bin* binptr, vector<string>* recipe) {
	mappedFile* mf = mapFileIntoMem_read(filePath);
	char* contents = mf->contents_ptr;
	size_t fileLength = mf->contents_size;


	char* chunk_begin = contents;
	char* chunk_end;

	for(off_t i=0; i<(fileLength-WINDOW_SIZE); i++) {
		// containers for hashing
		string slidingWindow((contents+i), WINDOW_SIZE);

		if((fingerprint(slidingWindow) % SLIDINGWINDOW_DIVISOR == SLIDINGWINDOW_REMAINDER) || (i + WINDOW_SIZE == fileLength - 1 ) ){
			chunk_end = (contents+i) + WINDOW_SIZE;

			// Isolate Chunk
			size_t chunkSize = chunk_end - chunk_begin + 1;
			string CHUNK(chunk_begin, chunkSize);

			// create a new entry in the bin
			bin_entry newEntry;

			// calculate chunkID and fill entry
			newEntry.chunkID = md5_hash(chunk_begin);
			newEntry.chunkSize = chunkSize;
			newEntry.chunkContents = CHUNK;
			binptr->insert(newEntry);
			recipe->push_back(newEntry.chunkID);

			// set chunk begin to next chunk
			chunk_begin = chunk_end + 1;
			i += WINDOW_SIZE;	// shift window to next chunk
		}
	}

	munmap(contents, fileLength);
}

string writeBinAndRecipeToDisk(string destinationPath, Bin *binptr, vector<string> *recipe_ptr, string wholeFileHash) {	// writes a bin to disk, returns path to bin location
	string repChunkID = binptr->begin()->chunkID;	// using representative chunk id as directory name
	string destinationDirPath = destinationPath + repChunkID;

	// Create directory for chunks
	if(mkdir(destinationDirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 ) {
		if(errno == EEXIST) {
			errno = 0;	// ignore error if directory already exists.
		} else {
			m_err("Error Creating Chunks Folder " + destinationDirPath);	// otherwise exit.
		}
	}
	// Prepare to write chunks to directory
	set<bin_entry , binEntryCompare>::iterator iter = binptr->begin();
	string destinationFilePath;
	ofstream fs_chunks;

	// Prepare to write chunk info to bin
	string binFilePath = destinationDirPath + "_bin.txt";

	ofstream fs_forBinAndRecipeFiles;
	fs_forBinAndRecipeFiles.open(binFilePath);

	while(iter != binptr->end()) {
		// create file in directory
		destinationFilePath = destinationDirPath + "/" + (*iter).chunkID;

		// write chunk content to chunk file if chunk has content (no content means it is a duplicate chunk. Chunk already exists)
		fs_chunks.open(destinationFilePath);
		if((*iter).chunkContents.size() != 0) {
			fs_chunks << (*iter).chunkContents;
		}
		fs_chunks.close();

		// Write chunk info to binFile (chunk is already unique)
		string line = (*iter).chunkID + " \t" + to_string((*iter).chunkSize) + "\n";
		fs_forBinAndRecipeFiles << line;

		iter++;
	}
	fs_forBinAndRecipeFiles.close();

	// File recipe includes binID, original path, and chunk order. File name is r_<wholeFileHash>.recipe
	string recipeFile = destinationPath + "r_" + wholeFileHash + ".recipe";

	// Write order of ChunkIDs to recipe file
	fs_forBinAndRecipeFiles.open(recipeFile);
	fs_forBinAndRecipeFiles << repChunkID + "\n" + pathIdentifierPrefix;	// add the binID (representative chunkID) and prefix for path on next line
	for(int i=0; i<recipe_ptr->size(); i++) {	// first entry is original filepath, the rest are order of the chunks
		fs_forBinAndRecipeFiles << (*recipe_ptr)[i] + "\n";
	}
	fs_forBinAndRecipeFiles.close();

	return string(binFilePath);
}

void readBinFile(string binFilePath, Bin* binptr) {
	ifstream fs(binFilePath.c_str());

	string cID;
	while(fs >> cID) {
		bin_entry be;
		be.chunkID = cID;
		fs >> be.chunkSize;
		binptr->insert(be);
	}
}

void addTargetPathToRecipeFile(string recipeFilePath, string originalFilePath) {	// inserts a duplicate file's filepath in a recipe
	string lineToAdd = originalFilePath + "\n";

	// Split old recipe at first linebreak
	char* memblock = mapFileIntoMem_read(recipeFilePath)->contents_ptr;
	string contents(memblock);
	size_t breakpos = contents.find_first_of('\n');
	string before = contents.substr(0,breakpos);
	string after = contents.substr(breakpos+1);

	// Write new data to file
	fstream fs(recipeFilePath);
	fs << before + "\n" + pathIdentifierPrefix + lineToAdd + after;
	fs.close();
}

void backupFile(string filepath, string destinationDirPath) {	// process for backing up a file
	cout << "Backing up [" << filepath << "] \t";

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
	string wholeFileHash = md5_hash(mfile->contents_ptr);
	munmap(mfile->contents_ptr, mfile->contents_size);
	cout << "## \t" << wholeFileHash << endl;

	PrimaryIndexEntry*found_piEntry = primaryIndex->findEntry(repChunkID);
	if(found_piEntry == nullptr) {	// entry does not exist
		// Write Chunks, Bin, and Recipe to disk
		string binFilePath = writeBinAndRecipeToDisk(destinationDirPath, binptr, recipe_ptr, wholeFileHash);

		// Add bin (entry) to primary index
		primaryIndex->addEntry(repChunkID, wholeFileHash, binFilePath);

	} else {	// entry exists
		// Compare whole file hash
		if(found_piEntry->WholeFileHash != wholeFileHash) {	// if whole file hash doesn't match
			// Load bin from disk and add unique enties to current bin
			readBinFile(found_piEntry->BinPath, binptr);

			// Write new current bin to disk
			writeBinAndRecipeToDisk(destinationDirPath, binptr, recipe_ptr, wholeFileHash);

		} else {
			// Reconstruct Recipe File Path
			string recipeFilePath = destinationDirPath + "r_" + wholeFileHash + ".recipe";

			addTargetPathToRecipeFile(recipeFilePath, (*recipe_ptr)[0]);	// first entry in recipe is target file path
		}
	}
}

void backupDir(string targetDirPath, string destinationDirPath) {
	DIR* dirD = opendir(targetDirPath.c_str());
	if(!dirD) {
		m_err("Error Reading Target Directory (" + targetDirPath + ")");
	}

	struct dirent* item;	// read first file
	struct stat st;
	item = readdir(dirD);
	while(item) {	// read through directory items
		string name = string(item->d_name);
		string filePath = targetDirPath + name;

		if ((name != ".") && (name != "..")) {    // don't operate on parent directories
			if(lstat(filePath.c_str(), &st) != 0) m_err("Error calling lstat() on file.");	// check for directory or file

			// item is not another directory
			if(S_ISREG(st.st_mode)) {
				// perform backup on file
				backupFile(filePath, destinationDirPath);
			} else {
				// Recursive backup on Directory
				filePath += "/";
				backupDir(filePath, destinationDirPath);
			}
		}
		item = readdir(dirD);
	}
}



int main(int argc, char* argv[]) {
	// Initialise Primary Index in heap
	primaryIndex = new PrimaryIndex;

	if(argc != 3) {
		m_err("Error: Wrong Number of Arguments\nUsage: XB_Backup <TargetDir> <DestinationDir>");
	}

	// set fullpath of target dir
	char cwd[PATH_MAX];
	getcwd(cwd, PATH_MAX);

	string targetDir(cwd);
	targetDir += "/" + string(argv[1]);
	initialTargetDirPathLength = targetDir.size();	// set length of path (used for finding relative path for recipe)

	// set fullpath of destination dir
	string destDir(cwd);
	destDir += "/" + string(argv[2]);

	if(mkdir(destDir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 ) {
		if(errno == EEXIST) {
			errno = 0;	// ignore error if directory already exists.
		} else {
			m_err("Error Creating Backup Destination Folder " + destDir);	// otherwise exit.
		}
	}

	backupDir(targetDir, destDir);


	return 0;
}