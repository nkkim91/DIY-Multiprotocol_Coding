IDIR =./
CC=gcc
CFLAGS=-I$(IDIR)

ODIR=./
LDIR=./

#LIBS=-lm
LIBS=

EXEC = DIY-Multiprotocol_Decode_Log DIY-Multiprotocol_TX ValidateOutputFile DIY-Multiprotocol_Encode_Log GenerateTestInput ValidateOutputFile2 edecode_test

all: DIY-Multiprotocol_Decode_Log DIY-Multiprotocol_TX ValidateOutputFile DIY-Multiprotocol_Encode_Log GenerateTestInput ValidateOutputFile2 edecode_test

DIY-Multiprotocol_Decode_Log: DIY-Multiprotocol_Decode_Log.o 
DIY-Multiprotocol_TX: DIY-Multiprotocol_TX.o joystick.o axbtnmap.o
ValidateOutputFile: ValidateOutputFile.o
DIY-Multiprotocol_Encode_Log: DIY-Multiprotocol_Encode_Log.o
GenerateTestInput: GenerateTestInput.o
ValidateOutputFile2: ValidateOutputFile2.o
edecode_test: edecode_test.o

_OBJ = DIY-Multiprotocol_Decode_Log.o DIY-Multiprotocol_TX.o ValidateOutputFile.o DIY-Multiprotocol_Encode_Log.o GenerateTestInput.o ValidateOutputFile2.o edecode_test.o joystick.o axbtnmap.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

distclean: clean
clean:
	$(RM) -f $(ODIR)/*.o $(EXEC) *.swp core *.orig *.rej map *~

.PHONY: all clean distclean
