//
// Created by marcusljx on 26/01/16.
//
#include <cstdlib>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <vector>
#include "XB_includes.h"

using namespace std;

string targetDirPath;
string restoreDirPath;

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

string getRelativeParentPath(string path) {
	string result = path.substr(0, path.find_last_of('/'));
	if(result == path) {
		return "";
	}

	return result;
}
//===========================================

void restoreRecipeFile(string recipeFilePath) {		// Restores a file and its duplicates to the relative path indicated in the recipe file
	string line;
	string binPath = "";
	string chunkPath;
	string fileContent = "";
	vector<string> files;

	fstream fs(recipeFilePath);
	//todo: read 1st line : bin_repChunkID
	int lineCount = 0;
	while(fs >> line) {
		if(lineCount == 0) {
			// first line refers to bin
			binPath = targetDirPath + line;
//			cout << "BINPATH: \t" << binPath << endl;

		} else {
			// 2nd+ line with hash prefix refers to relative filepath for restore
			if(line.find('#') != string::npos ) {
				files.push_back(line.substr(1));	// remove prefix
//				cout << "FILEPATH: \t" << line.substr(1) << endl;

			} else {
				// Remaining+ lines are all chunk IDs in order
				if(binPath == "") m_err("Error Reading Bin Path from Recipe File.");
				chunkPath = binPath + "/" + line;

				// Append chunk to fileContent (Q: not sure if there are more efficient methods for doing this?)
				mappedFile* mf = mapFileIntoMem_read(chunkPath);
				fileContent += string(mf->contents_ptr);
				munmap(mf->contents_ptr, mf->contents_size);

				//todo: read all remaining lines (chunk order), read chunk file and patch.
			}
		}
		lineCount++;
	}
	fs.close();	// close recipe file

	// Write fileContents to all files (including duplicates)
	for(int i=0; i<files.size(); i++) {
		string parentPath = restoreDirPath + getRelativeParentPath(files[i]);
		string restorePath = restoreDirPath + files[i];

		// Create directories if needed
		if(parentPath.size() != 0) {
			if(mkdir(parentPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 ) {
				if(errno == EEXIST) {
					errno = 0;	// ignore error if directory already exists.
				} else {
					m_err("Error Creating Folder " + parentPath);	// otherwise exit.
				}
			}
		}

		// Write content to file
		cout << "Restoring " + restorePath << endl;
		fs.open(restorePath, fstream::out);	// open file for writing
		fs << fileContent;
		fs.close();
	}

	// Error reporting
	if(lineCount < 3) m_err("Error Reading Recipe File: Recipe requires at least 3 lines of the correct format.");
}

void restoreDir() {
	DIR* dirD = opendir(targetDirPath.c_str());
	if(!dirD) {
		m_err("Error Reading Target Directory (" + targetDirPath + ")");
	}

	struct dirent* item;	// read first file
	item = readdir(dirD);
	while(item) {	// read through directory items
		string name = string(item->d_name);

		if(name.substr(0,2) == "r_") {	// only operate on recipe files
			string filePath = targetDirPath + name;
			restoreRecipeFile(filePath);
			cout << "================================" << endl;
		}

		item = readdir(dirD);
	}
}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		m_err("Error: Wrong Number of Arguments\nUsage: XB_Backup <TargetDir> <DestinationDir>");
	}

	char temp[PATH_MAX];
	getcwd(temp, PATH_MAX);
	string cwd(temp);

	// Check if input is absolute path or relative
	string targetPath(argv[1]);
	if( (targetPath[0] == '~') || (targetPath[0] == '/') ) {	// absolute path given
		targetDirPath = targetPath;
	} else {
		targetDirPath = cwd + "/" + targetPath;	// otherwise use relative path
	}

	// Check if input is absolute path or relative
	string destPath(argv[2]);
	if( (destPath[0] == '~') || (destPath[0] == '/') ) {	// absolute path given
		restoreDirPath = destPath;
	} else {
		restoreDirPath = cwd + "/" + destPath;	// otherwise use relative path
	}

	// Create Restore Directory if necessary
	if(mkdir(restoreDirPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0 ) {
		if(errno == EEXIST) {
			errno = 0;	// ignore error if bin directory already exists.
		} else {
			m_err("Error Creating Backup Destination Folder " + restoreDirPath);	// otherwise exit.
		}
	}

	restoreDir();

	return 0;
}