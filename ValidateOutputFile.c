#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_TX_DATA		(26)
#define NUM_OF_TX_DATA	(MAX_TX_DATA/sizeof(unsigned int)+1)

struct tx_data {
	unsigned int data[NUM_OF_TX_DATA];
};

union tx_buffer {
	struct tx_data sTXData;
	unsigned char bytes[sizeof(struct tx_data)];
};


union tx_buffer stTXBuffer;
unsigned int unSeqNum;

void clear_tx_buffer(void)
{
	int i, j;
	for( i = 0; i < NUM_OF_TX_DATA; i++)
	{
		stTXBuffer.sTXData.data[i] = 0; 
	}
}

void fill_tx_data(void)
{
	int i, j;
	for( i = 0; i < NUM_OF_TX_DATA; i++)
	{
		stTXBuffer.sTXData.data[i] = unSeqNum++;
	}
}

int ReadData(int fd, char *buffer, int size)
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
	unsigned int unTmp[MAX_TX_DATA];

	if( argc < 2 ) {
		printf("Not enough argument \n");
		exit(1);
	}

#if 1
	fd = open(argv[1], O_RDONLY | O_SYNC);

	if( fd < 0 ) {
		printf("Error - File open (%s)\n", argv[1]);
		exit(1);
	}
#else
	fpi = fopen(argv[1], "r");
	if( fpi == NULL ) {
		printf("Error - File open (%s)\n", argv[1]);
		exit(1);
	}
#endif

	printf("sizeof(unsigned int)    : %lu\n", sizeof(unsigned int));
	printf("sizeof(struct tx_data)  : %lu\n", sizeof(struct tx_data));
	printf("sizeof(union tx_buffer) : %lu\n", sizeof(union tx_buffer));

	for(j = 0, unSeqNum = 0; 1 ; j++) {
		clear_tx_buffer();

		nRet = ReadData(fd, &stTXBuffer.bytes[0], MAX_TX_DATA);
		printf("nRet : %d\n", nRet);

		if( nRet == 0 ) {
			printf("End of file \n");
			break;
		}

		printf("stTxBuffer [%d][%d] : 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", j, unSeqNum,
						stTXBuffer.bytes[0],
						stTXBuffer.bytes[1],
						stTXBuffer.bytes[2],
						stTXBuffer.bytes[3],
						stTXBuffer.bytes[4],
						stTXBuffer.bytes[5],
						stTXBuffer.bytes[6],
						stTXBuffer.bytes[7],
						stTXBuffer.bytes[8],
						stTXBuffer.bytes[9],
						stTXBuffer.bytes[10],
						stTXBuffer.bytes[11],
						stTXBuffer.bytes[12],
						stTXBuffer.bytes[13],
						stTXBuffer.bytes[14],
						stTXBuffer.bytes[15],
						stTXBuffer.bytes[16],
						stTXBuffer.bytes[17],
						stTXBuffer.bytes[18],
						stTXBuffer.bytes[19],
						stTXBuffer.bytes[20],
						stTXBuffer.bytes[21],
						stTXBuffer.bytes[22],
						stTXBuffer.bytes[23],
						stTXBuffer.bytes[24],
						stTXBuffer.bytes[25]);

		if( stTXBuffer.sTXData.data[0] != unSeqNum ||
			stTXBuffer.sTXData.data[1] != unSeqNum+1 ||
			stTXBuffer.sTXData.data[2] != unSeqNum+2 ||
			stTXBuffer.sTXData.data[3] != unSeqNum+3 ||
			stTXBuffer.sTXData.data[4] != unSeqNum+4 ||
			stTXBuffer.sTXData.data[5] != unSeqNum+5 ) {
			printf("Error - Sequencial number(%d) mismatch !!\n", unSeqNum);
			exit(1);
		}
		unSeqNum += NUM_OF_TX_DATA;
	}

	close(fd);
}
