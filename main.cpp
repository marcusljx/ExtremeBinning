#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <openssl/md5.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

#define MD5_DIGEST_SIZE 16

void m_err(string error_message) {
	perror(error_message.c_str());
	exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
	// read file into mapped memory
	int fd = open(argv[1], O_RDONLY);
	if(fd==-1) m_err("Error reading file.");


	off_t fileLength = lseek(fd, 0, SEEK_END);
	unsigned char* contents = (unsigned char*) mmap(NULL, fileLength, PROT_READ, MAP_SHARED, fd, 0);
	close(fd);

	// containers for hashing
	unsigned char digest[MD5_DIGEST_SIZE];
	unsigned char* slidingWindow;

	for(off_t i=0; i<(fileLength-MD5_DIGEST_SIZE); i++) {
		slidingWindow = contents + i;

	// 	// MD5(slidingWindow, MD5_DIGEST_SIZE, digest);
		cout << slidingWindow << endl;
	// 	// cout << digest << endl;		
	}


	munmap(contents, fileLength);
	return 0;
}