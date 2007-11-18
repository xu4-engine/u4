/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_HEADER_SIZE 0x1c
#define MZ_SIG 0x5a4d

#define LAST_BLOCK 2
#define BLOCKS 4
#define HEADER_PARAS 8
#define MIN_ADD_PARAS 0xa
#define INITIAL_SS 0xe
#define INITIAL_SP 0x10
#define INITIAL_IP 0x14
#define INITIAL_CS 0x16

#define FAR_RET_SIZE 0x23

#define NEW_SEG1 5
#define NEW_SEG2 0xd
#define NEW_SP 0xa
#define NEW_IP 0x11

long getWord(unsigned char *ofs);
void putWord(unsigned char *ofs, long word);

/**
 * Removes the outer packing layer from AVATAR.EXE, so that it can be decompressed with a Microsoft Exepack unpacker
 */
int main (int argc, char *argv[]) {
  FILE *file;
  unsigned char *buffer;
  long oldMemSize;
  long headerSize;
  long oldEntrySeg, oldEntryOfs;
  long newEntrySeg, newEntryOfs;
  long newStackSeg, newStackOfs;
  long infileSize, outfileSize;

  /* Are the command-line arguments valid? */
  if (argc != 3) {
    printf("Usage:\nu4unpackexe infile outfile\n");
    return(EXIT_FAILURE);
  }
  if (strcmp(argv[1],argv[2]) == 0) {
    printf("File names must not be identical!\n");
    return(EXIT_FAILURE);
  }

  /* Does the infile exist? */
  file = fopen(argv[1],"rb");
  if (!file) {
    printf("Couldn't open %s\n for reading!", argv[1]);
    return(EXIT_FAILURE);
  }
  /* Get the filesize of the infile */
  fseek(file, 0, SEEK_END);   /* move file pointer to file end */
  infileSize = ftell(file);
  fseek(file, 0, SEEK_SET);   /* move file pointer to file start */

  /* Read the infile into memory */
  buffer = (unsigned char *) malloc(sizeof(unsigned char) * infileSize);
  if (!buffer) {
    printf("Couldn't allocate buffer for %s\n", argv[1]);
    return(EXIT_FAILURE);
  }
  fread(buffer, 1, infileSize, file);
  fclose(file);

  /* Is the infile a valid exe file? */
  /* todo: check more than just the MZ signature */
  if (infileSize < BASE_HEADER_SIZE) {
    printf("Not enough space in %s for %d-byte base header!\n", argv[1], BASE_HEADER_SIZE);
    return(EXIT_FAILURE);
  }
  if (getWord(buffer) != MZ_SIG) {
    printf("%s doesn't have an 'MZ' signature!\n", argv[1]);
    return(EXIT_FAILURE);
  }

  /* Get the old entry point */
  headerSize = getWord(buffer + HEADER_PARAS) * 0x10;
  oldEntryOfs = getWord(buffer + INITIAL_IP);
  oldEntrySeg = getWord(buffer + INITIAL_CS);

  /* Has the infile been packed with the unknown packer? */
  /* todo: check if the unknown unpacker is actually there, not just if there's enough space for it */
  if (headerSize + oldEntrySeg*0x10 + oldEntryOfs + FAR_RET_SIZE > infileSize) {
    printf("Not enough space in %s for %d-byte code section!\n", argv[1], FAR_RET_SIZE);
    return(EXIT_FAILURE);
  }

  /* Get the new stack location and the new entry point */
  newStackSeg = getWord(buffer + headerSize + oldEntrySeg*0x10 + oldEntryOfs + NEW_SEG1) - 0x10;
  newStackOfs = getWord(buffer + headerSize + oldEntrySeg*0x10 + oldEntryOfs + NEW_SP);

  newEntrySeg = newStackSeg - getWord(buffer + headerSize + oldEntrySeg*0x10 + oldEntryOfs + NEW_SEG2);
  newEntryOfs = getWord(buffer + headerSize + oldEntrySeg*0x10 + oldEntryOfs + NEW_IP);

  /* Modify the header: number of blocks, new SS:SP, new initial CS:IP */
  outfileSize = headerSize + oldEntrySeg*0x10 + oldEntryOfs;
  putWord(buffer + LAST_BLOCK, outfileSize % 0x200);
  putWord(buffer + BLOCKS, (outfileSize + 0x1ff) / 0x200);

  oldMemSize = infileSize + getWord(buffer + MIN_ADD_PARAS) * 0x10;
  putWord(buffer + MIN_ADD_PARAS, (oldMemSize - outfileSize + 0xf) / 0x10);

  putWord(buffer + INITIAL_SS, newStackSeg);
  putWord(buffer + INITIAL_SP, newStackOfs);

  putWord(buffer + INITIAL_CS, newEntrySeg);
  putWord(buffer + INITIAL_IP, newEntryOfs);

  /* Write the outfile */
  file = fopen(argv[2],"wb");
  if (!file) {
    printf("Couldn't open %s for writing!\n", argv[2]);
    return(EXIT_FAILURE);
  }
  fwrite(buffer, 1, outfileSize, file);
  fclose(file);
  return(EXIT_SUCCESS);
}

long getWord(unsigned char *ofs) {
  long word;

  word = *ofs;
  word += (*(ofs+1))*0x100;
  return(word);
}

void putWord(unsigned char *ofs, long word) {
  *ofs = word & 0xff;
  *(ofs+1) = word >> 8;
}
