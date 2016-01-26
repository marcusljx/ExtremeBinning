//
// Created by marcusljx on 26/01/16.
//
#include <cstdlib>
#include <iostream>
#include <dirent.h>
#include <sys/stat.h>

using namespace std;

string backupDirPath;
string restoreDirPath;

void m_err(string error_message) {
	perror(error_message.c_str());
	exit(EXIT_FAILURE);
}
//===========================================

void restoreFileFromRecipe(string recipeFilePath) {
	//todo: read 1st line : bin_repChunkID

	//todo: read 2nd+ lines that begin with '\t' : filepath(s) of files with this file contents (all same)

	//todo: read all remaining lines (chunk order), read chunk file and patch.
}

void restoreDir() {
	DIR* dirD = opendir(backupDirPath.c_str());
	if(!dirD) {
		m_err("Error Reading Target Directory (" + backupDirPath + ")");
	}

	struct dirent* item;	// read first file
	struct stat st;
	item = readdir(dirD);
	while(item) {	// read through directory items
		string name = string(item->d_name);

		if(name.substr(0,2) == "r_") {	// only operate on recipe files
			string filePath = backupDirPath + name;
			cout << filePath << endl;
			restoreFileFromRecipe(filePath);
		}

		item = readdir(dirD);
	}

}

int main(int argc, char* argv[]) {
	if(argc != 3) {
		m_err("Error: Wrong Number of Arguments\nUsage: XB_Backup <TargetDir> <DestinationDir>");
	}

	backupDirPath = string(argv[1]);
	restoreDirPath = string(argv[2]);

	restoreDir();

}