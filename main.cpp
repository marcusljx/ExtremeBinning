#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <openssl/md5.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>

using namespace std;

#define WINDOW_SIZE 16
#define FINGERPRINT_DIVISOR 5
#define SLIDINGWINDOW_DIVISOR 3
#define SLIDINGWINDOW_REMAINDER 2

void m_err(string error_message) {
	perror(error_message.c_str());
	exit(EXIT_FAILURE);
}

unsigned int fingerprint(unsigned char *arr_ptr) {	// simple hash using xor
	unsigned long tally = 0;
	int size = sizeof(arr_ptr)/ sizeof(unsigned char);
	for(int i=1; i<size; i++) {
		tally = (tally*10) + arr_ptr[i];	// Horner's Method?
	}

	return (unsigned int) (tally % FINGERPRINT_DIVISOR);	// result will fit definitely within (unsigned int)
}

int main(int argc, char* argv[]) {
	// read file into mapped memory
	int fd = open(argv[1], O_RDONLY);
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

	for(off_t i=0; i<(fileLength-WINDOW_SIZE); i++) {
		memcpy(slidingWindow, (contents+i), WINDOW_SIZE);
		
//		cout << fingerprint(slidingWindow);
		if((fingerprint(slidingWindow) % SLIDINGWINDOW_DIVISOR == SLIDINGWINDOW_REMAINDER) || (i + WINDOW_SIZE == fileLength - 1 ) ){
//			cout << "(Y) \t";
			chunk_end = contents + i + WINDOW_SIZE;

			// FIND CHUNK ID
			size_t chunkSize = chunk_end - chunk_begin;
//			cout << "chunkSize = " << chunk_end - chunk_begin ;
//			cout << " \t[" << (void*)chunk_begin << " --> " << (void*)chunk_end << "]";
			unsigned char CHUNK[chunkSize];
			unsigned char DIGEST[MD5_DIGEST_LENGTH];
			unsigned char hash[MD5_DIGEST_LENGTH];
			memcpy(CHUNK, chunk_begin, chunkSize);
			MD5((unsigned char *) &CHUNK, MD5_DIGEST_LENGTH, (unsigned char *) &DIGEST);
			for(int j=0; j<MD5_DIGEST_LENGTH; j++) {
				sprintf((char *) &hash[j * 2], "%02x", (unsigned int)DIGEST[j]);
			}

			cout << "[" << chunkSize << "] \t" << CHUNK << " --> " << hash;

			// set chunk begin to next chunk
			chunk_begin = chunk_end + 1;
			i += WINDOW_SIZE;	// shift window to next chunk
			cout << endl;
		} 



		// MD5(slidingWindow, WINDOW_SIZE, digest);
		// cout << slidingWindow << endl;
		// cout << digest << endl;
		// cout << fingerprint(slidingWindow) << endl;
	}

	munmap(contents, fileLength);
	return 0;
}