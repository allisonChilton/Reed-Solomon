/* Author: Mike Lubinets (aka mersinvald)
 * Date: 29.12.15
 *
 * See LICENSE */

#include <iostream>
#include "rs.hpp"
#include "string.h"
#include "timer.h"

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

#ifndef VERBOSE
#define VERBOSE 0
#endif
#define PRINT_TIMING

double get_time(){
  stop_timer();
  double retval = elapsed_time();
  reset_timer();
  start_timer();
  return retval;
}

int main() {

    FILE* fp = fopen("examplefile.txt","r");
    fseek(fp, 0, SEEK_END);
    int fsize = ftell(fp);
    int chunks = fsize / (255-ECC_LENGTH);
    int msglength = chunks * (255-ECC_LENGTH); // truncate to nearest block size for simplicity
    fseek(fp, 0, SEEK_SET);
    char *messagelong = (char*) malloc(msglength);
    if(messagelong == 0){
        printf("Malloc failed. Aborting\n");
        exit(1);
    }
    fread(messagelong, 1, msglength, fp);
    fclose(fp);

    /* Timer setup */
    initialize_timer();
    start_timer();
    double timings[10];
    int timeIdx = 0;


    /* initialization */
    const int chunklen = 255 - ECC_LENGTH;
    chunks = msglength / chunklen;
    char* repaired = (char*) malloc(sizeof(char) * msglength);
    char* encoded = (char*) malloc(sizeof(char) * (msglength + (ECC_LENGTH * chunks)));
    RS::ReedSolomon<chunklen, ECC_LENGTH> rs;

    timings[timeIdx++] = get_time();

    /* split buffers */
    msg_buffers buffers = split_messages(messagelong, msglength);
    timings[timeIdx++] = get_time();

    /* encode */
    for(int i = 0; i < buffers.length; ++i){
        char* encode_dest = encoded + ((buffers.chunksize + ECC_LENGTH) * i);
        rs.Encode(buffers.buffers[i], encode_dest);
    }

    timings[timeIdx++] = get_time();

    /* corrupt */
    for(int i = 0; i < buffers.length; ++i){
        char* encode_dest = encoded + ((buffers.chunksize + ECC_LENGTH) * i);

        // Corrupting maximum amount per chunk
        for(uint i = 0; i < 0+(ECC_LENGTH / 2); i++) {
            encode_dest[i] = 'E';
        }
    }

    timings[timeIdx++] = get_time();

    /* decode */
    for(int i = 0; i < buffers.length; ++i){
        char* encode_src = encoded + ((buffers.chunksize + ECC_LENGTH) * i);
        rs.Decode(encode_src, repaired + (i * buffers.chunksize));
    }

    timings[timeIdx++] = get_time();

    if(VERBOSE){
        std::cout << "Original:  " << messagelong  << std::endl;
        std::cout << "Corrupted: " << encoded  << std::endl;
        std::cout << "Repaired:  " << repaired << std::endl;
    }

    std::cout << ((memcmp(messagelong, repaired, msglength) == 0) ? "SUCCESS" : "FAILURE") << std::endl;
    #ifdef PRINT_TIMING
    for(int i = 0; i < timeIdx; ++i){
        printf("Checkpoint %d: %.6f\n",  i, timings[i]);
    }
    #endif

    free(encoded); free(messagelong); free(repaired);
    return 0;
}

