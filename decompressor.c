/* 
 * File:   decompress.c
 * Author: ceyhun
 *
 * Created on May 21, 2009, 3:17 AM
 */


#include <stdio.h>
#include <stdlib.h>

/*
 * Buffer used to read a byte from file.
 */
unsigned char buffer;

/*
 * Current bit number in the buffer.
 */
int currentBit = 8;

/*
 * Number of characters will be on the decompressed file.
 */
int charCount = 0;

/*
 * Codes of characters.
 */
unsigned char* codes[255];

/*
 * Maximum length for a code.
 */
int maxCodeLength = 0;

/*
 * Minimum length for a code.
 */
int minCodeLength = 6;

/*
 * Alphabet characters. Only files that are made up from these characters can be
 * compressed and decompressed correctly!
 */
char characters[26] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
    'x', 'y', 'z'};

/*
 * Returns next bit read from file.
 */
int nextBit(FILE* inFd);

/*
 * Returns a consecutive bits from file.
 */
unsigned char* readNBits(FILE* inFd, int n);

/*
 * Convers given binary bits into decimal number.
 */
int readDecimal(FILE* inFd, int n);

/*
 * Convers given binary bits into decimal number.
 */
int toDecimal(unsigned char* bits, int length);

/*
 * Compares two bitstring, checks wheter they are equal or not.
 */
int equals(unsigned char* first, unsigned char* second, int length);

/*
 * Returns readable form of bits.
 */
char* toString(unsigned char* bits, int length);

/*
 * Reads the dictionary in the header that contains the codes of characters.
 */
void readDictionary(FILE* inFd);

/*
 * Reads the length of the original file.
 */
void readFileLength(FILE* inFd);

/*
 * Reads next code from the file.
 */
char readACode(FILE* inFd);

/*
 * Decompresses given file.
 */
void createDecompressedFile(char* compressedFileName, char* decompressedFileName);

/*
 * Finds the character that the given code stands for.
 */
char searchForCode(unsigned char* code, int length);

/*
 * Prints the content of decompressed file to the standard ouptput.
 */
void printOutputAsCharacters(char* decompressedFileName);

int nextBit(FILE* inFd) {
    int nextB;
    if(currentBit == 8) { // if buffer all read,
        fread(&buffer, 1, 1, inFd); // read one more
        currentBit = 0; // reset buffer position
    }
    nextB = (buffer >> (7-currentBit)) & 1; // get current bit
    ++currentBit; // increment position
    return nextB;
}

unsigned char* readNBits(FILE* inFd, int n) {
    int i;
    unsigned char* bits;

    bits = malloc(n);
    for(i = 0; i < n; ++i) {
        bits[i] = nextBit(inFd); // set ith bit starting from left
    }
    return bits;
}

int toDecimal(unsigned char* bits, int length) {
    int i;
    int val = 0;

    for(i = length-1; i >= 0; --i) {
        val |= bits[i] << (length - i - 1);
    }
    return val;
}
 
int readDecimal(FILE* inFd, int n) {
    int value;
    unsigned char* bits;

    bits = readNBits(inFd, n);
    value = toDecimal(bits, n);
    free(bits);
    return value;
}

void readDictionary(FILE* inFd) {
    int i;
    unsigned char* code;
    int codeLength;
    
    for(i = 0; i < 26; ++i) {
        codeLength = readDecimal(inFd, 5); //read 5 bit code length
        if(codeLength != 0) { // if it is coded,
            code = readNBits(inFd, codeLength); // read code
            codes[characters[i]] = code; // record code
            if(codeLength > maxCodeLength) { // if the current code length is bigger
                maxCodeLength = codeLength; // it is the max code length
            }
            if(codeLength < minCodeLength) {
                minCodeLength = codeLength;
            }
        }
    }
}

void readFileLength(FILE* inFd) {
    int read;
    while((read = readDecimal(inFd, 2)) != 3) {
        charCount = charCount*3 + read; // convert base-3 number to decimal
    }
}

char readACode(FILE* inFd) {
    unsigned char* code;
    int current;
    char c = '*';
    
    code = malloc(32);
    for(current = 0; current < maxCodeLength; ++current) {
        code[current] = nextBit(inFd);
        // <editor-fold defaultstate="collapsed" desc="old-ill-implementation">
        /*
        readBit = nextBit(inFd);
        if (readBit == 0) { // if stop bit seen
            code << 1; // shift to left
            break; // and leave
        } else if (current == maxCodeLength - 1) {
            code = code + 1; // it is 1 and the last bit, it is the odd code.
        } else {
            code = (code + 1) << 1; // otherwise calculate routine
        code = (code << 1) + readBit;
        }*/// </editor-fold>
        if(current >= minCodeLength && (c = searchForCode(code, current+1)) != '*') {
            free(code);
            return c;
        }
    }
    free(code);
    return c;
}

void createDecompressedFile(char* compressedFileName, char* decompressedFileName) {
    char c;
    int i = 0;
    FILE* compFile;
    FILE* decompFile;

    compFile = fopen(compressedFileName, "rb");
    decompFile = fopen(decompressedFileName, "w");

    readDictionary(compFile);
    readFileLength(compFile);

    for(i = 0; i < charCount; ++i) {
        c = readACode(compFile);
        //c = searchForCode(code);
        fprintf(decompFile, "%c", c); // output decoded char to decompressed file.
    }
    fclose(compFile);
    fclose(decompFile);
}

int equals(unsigned char* first, unsigned char* second, int length) {
    int i;
    for(i = 0; i < length; ++i) {
        if(first[i] != second[i]) {
            return 0;
        }
    }
    return 1;
}

char* toString(unsigned char* bits, int length) {
    int i;
    char* string;

    string = malloc(length + 1);

    for(i = 0; i < length; ++i) {
        if(bits[i]) {
            string[i] = '1';
        }
        else {
            string[i] = '0';
        }
    }
    string[i] = '\0';
    return string;
}

char searchForCode(unsigned char* code, int length) {
    int i;
    for(i = 0; i < 255; ++i) {
        if(equals(codes[i], code, length)) {
            return (char) i;
        }
    }
    return '*'; // indicates an unkown code to take attention of coder.
}

void printOutputAsCharacters(char* decompressedFileName) {
    char c;
    FILE* input = fopen(decompressedFileName, "r");
    fscanf(input, "%c", &c);
    while(!feof(input)) {
        printf("%c", c);
        fscanf(input, "%c", &c);
    }
    fclose(input);
}

int main(int argc, char** argv) {
    if(argc < 3) {
        argv[1] = "file2.txt";
        argv[2] = "file3.txt";
    }
    createDecompressedFile(argv[1], argv[2]);
    printOutputAsCharacters(argv[2]);
    return (EXIT_SUCCESS);
}

