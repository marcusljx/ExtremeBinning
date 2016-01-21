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
#define FINGERPRINT_DIVISOR 2
#define	FINGERPRINT_REMAINDER 1

void m_err(string error_message) {
	perror(error_message.c_str());
	exit(EXIT_FAILURE);
}

unsigned int xor_fingerprint(unsigned char* arr_ptr) {	// simple hash using xor
	unsigned char tally = *arr_ptr;
	int size = sizeof(arr_ptr)/ sizeof(unsigned char);
	for(int i=1; i<size; i++) {
		tally = tally ^ arr_ptr[i];
	}
	return (unsigned int)tally;
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

	string chunk;

	for(off_t i=0; i<(fileLength-WINDOW_SIZE); i++) {
		memcpy(slidingWindow, (contents+i), WINDOW_SIZE);
		
		cout << xor_fingerprint(slidingWindow);
		if( (xor_fingerprint(slidingWindow) % FINGERPRINT_DIVISOR == FINGERPRINT_REMAINDER) || (i+WINDOW_SIZE == fileLength-1 ) ){
			cout << "(Y) \t";
			chunk_end = contents + i + WINDOW_SIZE;
			// do things with chunk begin and end
				//todo
			cout << "chunkSize = " << chunk_end - chunk_begin ;
			cout << " \t[" << (void*)chunk_begin << " --> " << (void*)chunk_end << "]";
//			memcpy(chunk_begin, chunk_end-chunk_begin);
			

			// set chunk begin to next chunk
			chunk_begin = chunk_end + 1;
			i += WINDOW_SIZE;	// shift window to next chunk

		} 


		cout << endl;

		// MD5(slidingWindow, WINDOW_SIZE, digest);
		// cout << slidingWindow << endl;
		// cout << digest << endl;
		// cout << xor_fingerprint(slidingWindow) << endl;
	}

	munmap(contents, fileLength);
	return 0;
}