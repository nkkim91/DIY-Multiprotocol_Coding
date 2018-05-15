#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

union tx_data {
	unsigned int unSeqNumber;
    unsigned char ucByte[4];
};

union tx_data stTXData;

unsigned int unSeqNum;

int ReadData(int fd, unsigned char *buffer, int size)
{
	int nRet, nError = 0, data_read = 0;
	int size_to_read;

	size_to_read = size;

	while( 1 ) {
		nRet = read(fd, buffer+data_read, size_to_read);
		printf("nRet : %d\n", nRet);

		if( nRet < 0 ) {
			printf("Error - read \n");
			nError = nRet;	
			break;
		}
		if( nRet == 0 ) {
			nError = 0;
			break;
		}

		if( nRet == size_to_read ) {
			nError = nRet;
			break;
		} else {
			data_read += nRet;
			size_to_read -= nRet;
		}
	}

	return nError;
}

int main(int argc, char *argv[])
{
	int fd, i, j, nRet;

	if( argc < 2 ) {
		printf("Not enough argument \n");
		exit(1);
	}

	fd = open(argv[1], O_RDONLY | O_SYNC);

	if( fd < 0 ) {
		printf("Error - File open (%s)\n", argv[1]);
		exit(1);
	}

	printf("sizeof(unsigned int)    : %lu\n", sizeof(unsigned int));
	printf("sizeof(union tx_data)  : %lu\n", sizeof(union tx_data));

	for(j = 0, unSeqNum = 0; 1 ; j++) {

		nRet = ReadData(fd, &stTXData.ucByte[0], 4);
		printf("nRet : %d\n", nRet);

		if( nRet == 0 ) {
			printf("End of file \n");
			break;
		}

		if( stTXData.unSeqNumber != unSeqNum ) {
			printf("Error - Sequencial number(%d) mismatch !!\n", unSeqNum);
			exit(1);
		}
		unSeqNum++;
	}

	close(fd);
}
