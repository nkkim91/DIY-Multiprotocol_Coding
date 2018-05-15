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

int main(int argc, char *argv[])
{
	int fd, i, j;
	FILE *fpo;

	if( argc < 2 ) {
		printf("Not enough argument \n");
		exit(1);
	}

#if 0
	fd = open(argv[1], O_RDWR | O_CREATE);

	if( fd < 0 ) {
		printf("Error - File open (%s)\n", argv[1]);
		exit(1);
	}
#else
	fpo = fopen(argv[1], "w+");
	if( fpo == NULL ) {
		printf("Error - File open (%s)\n", argv[1]);
		exit(1);
	}
#endif

	printf("sizeof(unsigned int)    : %lu\n", sizeof(unsigned int));
	printf("sizeof(struct tx_data)  : %lu\n", sizeof(struct tx_data));
	printf("sizeof(union tx_buffer) : %lu\n", sizeof(union tx_buffer));

	for(j = 0; j < 4096; j++) {
		clear_tx_buffer();
		fill_tx_data();

		fprintf(fpo, "rx_ok_buff :");
		for( i = 0; i < MAX_TX_DATA; i++ ) {
			fprintf(fpo, " 0x%02x", stTXBuffer.bytes[i]);
		}
		fprintf(fpo, "\n");
	}

	fclose(fpo);
}
