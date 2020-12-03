/* Author: Mike Lubinets (aka mersinvald)
 * Date: 29.12.15
 *
 * See LICENSE */

#include <iostream>
#include "rs.hpp"
#include "string.h"
using namespace std;

#define ECC_LENGTH 16

typedef struct msg_buffers{
    char** buffers;
    int length;
    int chunksize;
} msg_buffers;

msg_buffers split_messages(char* message, int message_length){
    int chunksize = (255-ECC_LENGTH);
    int chunks = message_length / chunksize;
    int lastchunk = message_length % chunksize;
    if(lastchunk != 0){
        printf("Buffers must be divisible by chunksize %d. Message is length: %d. Aborting\n", chunksize, message_length);
        exit(1);
    }
        ++chunks;
    
    // make a list of buffer pointers to contain the split messages
    char** buffers = (char**) malloc(sizeof(char*) * chunks);

    for(int i = 0; i < chunks; ++i){
        buffers[i] = message + (chunksize * i);
    }

    msg_buffers retval;
    retval.buffers = buffers;
    retval.length = chunks;
    retval.chunksize = chunksize;

    return retval;
}

int main() {

    char messagelong[] = "We the People of the United States, in Order to form a more perfect Union, establish Justice, insure domestic Tranquility, provide for the common defense, promote the general Welfare, and secure the Blessings of Liberty to ourselves and our Posterity, do ordain and establish this Constitution for the United States of America. All legislative Powers herein granted shall be vested in a Congress of the United States, which shall consist of a Senate and House of Representative";

    int msglength = sizeof(messagelong);

    msg_buffers buffers = split_messages(messagelong, msglength);

    const int chunklen = 255 - ECC_LENGTH;

    char repaired[msglength];
    char encoded[msglength + (ECC_LENGTH * buffers.length)];


    RS::ReedSolomon<chunklen, ECC_LENGTH> rs;

    for(int i = 0; i < buffers.length; ++i){
        char* encode_dest = encoded + ((buffers.chunksize + ECC_LENGTH) * i);
        rs.Encode(buffers.buffers[i], encode_dest);

        // Corrupting maximum amount per chunk
        for(uint i = 0; i < 0+(ECC_LENGTH / 2); i++) {
            encode_dest[i] = 'E';
        }
    }

    for(int i = 0; i < buffers.length; ++i){
        char* encode_src = encoded + ((buffers.chunksize + ECC_LENGTH) * i);
        rs.Decode(encode_src, repaired + (i * buffers.chunksize));
    }

    std::cout << "Original:  " << messagelong  << std::endl;
    std::cout << "Corrupted: " << encoded  << std::endl;
    std::cout << "Repaired:  " << repaired << std::endl;

    std::cout << ((memcmp(messagelong, repaired, msglength) == 0) ? "SUCCESS" : "FAILURE") << std::endl;
    return 0;
}

