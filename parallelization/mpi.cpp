/* Author: Allison Chilton
 * See LICENSE */

#include <iostream>
#include "rs.hpp"
#include "string.h"
#include "timer.h"
#include "mpi.h"

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

int main(int argc, char** argv) {

    char* filename;
    if(argc == 2){
        filename = argv[1];
    }else{
        printf("Usage: <program name> <.txt filename>\n");
        exit(1);
    }

    int id;	    	     /* process id */
    int num_procs;		     /* number of processes */
    MPI_Status status;	     /* return status for receive */
    double *message;
    MPI_Request req;


    MPI_Init( &argc, &argv );
    MPI_Comm_rank( MPI_COMM_WORLD, &id );
    MPI_Comm_size( MPI_COMM_WORLD, &num_procs );

    FILE* fp = fopen(filename,"r");
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

    int mpi_iter_width = chunks / num_procs;
    if( chunks % num_procs != 0){
        mpi_iter_width++;
    }


    /* Timer setup */
    initialize_timer();
    start_timer();
    double timings[10];
    int timeIdx = 0;


    /* initialization */

    int encoded_size = sizeof(char) * (msglength + (ECC_LENGTH * chunks));
    const int chunklen = 255 - ECC_LENGTH;
    chunks = msglength / chunklen;
    char* repaired = (char*) malloc(sizeof(char) * msglength);
    char* encoded = (char*) malloc(encoded_size);
    RS::ReedSolomon<chunklen, ECC_LENGTH> rs;
    char* rbuf;
    if ( id == 0){
        rbuf = (char*) malloc(encoded_size);
    }

    timings[timeIdx++] = get_time();

    /* split buffers */
    msg_buffers buffers = split_messages(messagelong, msglength);
    timings[timeIdx++] = get_time();

    int start = id * mpi_iter_width;
    int stop = min(buffers.length, mpi_iter_width * (id + 1));

    /* encode */
    for(int i = start ; i < stop; ++i){
        char* encode_dest = encoded + ((buffers.chunksize + ECC_LENGTH) * i);
        rs.Encode(buffers.buffers[i], encode_dest);
    }

    int eoffset = (buffers.chunksize + ECC_LENGTH) * start;
    int soffset = (buffers.chunksize + ECC_LENGTH) * stop;
    char* sendbuf = encoded + (eoffset);
    int root = 0;
    int send_size = soffset - eoffset;

    // this is technically unnecessary before moving on to the decoding but to get an apples to apples comparison I feel its only fair to account for sync time 
    // MPI_Gather(sendbuf, send_size, MPI_CHAR, rbuf, send_size, MPI_CHAR, root, MPI_COMM_WORLD);

    timings[timeIdx++] = get_time();
    // if(id == 0)
    //     memcpy(encoded, rbuf, encoded_size);
    // MPI_Bcast(encoded, encoded_size, MPI_CHAR, root, MPI_COMM_WORLD);

    /* corrupt */
    for(int i = start; i < stop; ++i){
        char* encode_dest = encoded + ((buffers.chunksize + ECC_LENGTH) * i);

        // Corrupting maximum amount per chunk
        for(uint i = 0; i < 0+(ECC_LENGTH / 2); i++) {
            encode_dest[i] = 'E';
        }
    }

    timings[timeIdx++] = get_time();

    /* decode */
    for(int i = start; i < stop; ++i){
        char* encode_src = encoded + ((buffers.chunksize + ECC_LENGTH) * i);
        rs.Decode(encode_src, repaired + (i * buffers.chunksize));
    }


    eoffset = start * buffers.chunksize;
    soffset = stop * buffers.chunksize;
    sendbuf = repaired + (eoffset);
    send_size = soffset - eoffset;

    MPI_Gather(sendbuf, send_size, MPI_CHAR, rbuf, send_size, MPI_CHAR, root, MPI_COMM_WORLD);

    timings[timeIdx++] = get_time();
    if(id == 0)
        memcpy(repaired, rbuf, msglength);

    if(VERBOSE){
        std::cout << "Original:  " << messagelong  << std::endl;
        std::cout << "Corrupted: " << encoded  << std::endl;
        std::cout << "Repaired:  " << repaired << std::endl;
    }

    if( id == 0){
        std::cout << ((memcmp(messagelong, repaired, msglength) == 0) ? "SUCCESS" : "FAILURE") << std::endl;
        #ifdef PRINT_TIMING
        for(int i = 0; i < timeIdx; ++i){
            printf("Checkpoint %d: %.6f\n",  i, timings[i]);
        }
    }
    #endif

    free(encoded); free(messagelong); free(repaired);
   MPI_Finalize();

    return 0;
}

