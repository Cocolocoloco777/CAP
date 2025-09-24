#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

typedef enum {
    SEND,
    RECEIVE,
}Tag;

double compute(double* array, int size);

int main(int argc, char *argv[])
{
    int num_procs, num_local;
    char mach_name[MPI_MAX_PROCESSOR_NAME];
    int mach_len;

    int array_size;
    double* array = NULL;
    int partition_size;
    double computed_value = 0;
    double value_received;
    double *array_received = NULL;

    /* Init the MPI interface*/
    MPI_Init (&argc,&argv);
    MPI_Comm_size (MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank (MPI_COMM_WORLD, &num_local);
    MPI_Get_processor_name(mach_name,&mach_len);

    /* Get the size of the array from the arguments*/
    if (argc < 2){

        /* Defalut size if not given */
        array_size = 1000000;

    } else if (argc == 2){

        /* Get the size from the arguments */
        array_size = atoi(argv[1]);

        /* Error control (if the input is not a number atoi return 0, and also controls that is positive)*/
        if (array_size <= 0){
            fprintf(stderr, "\nERROR: Must input a valid positive integer\n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

    } else {

        /* Error control (too many arguments given)*/
        fprintf(stderr, "\nERROR: Too many arguments \n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
        
    }

    /* Determine the partition size */
    partition_size = array_size / num_procs;
  
    if(num_local == 0){
        /* This code is exclusive to the master */

        /* Allocate memory for the input array */
        array = malloc(array_size * sizeof(double));

        /* Memory error control */
        if (!array){
            fprintf(stderr, "\nERROR: Cannot allocate array \n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        /* Init the array with random numbers between 0 and 1 */
        for(int i = 0; i < array_size; i++){
            array[i] = (double) rand() / (double) RAND_MAX;
        }

        fprintf(stderr, "[Node 0] Array of size %d created\n",array_size);
        fflush(stderr);

        /* Sends a partition of the array to all the nodes except itself */
        for (int i = 0; i < num_procs - 1; i++){
            fprintf(stderr, "[Node 0] Sent array portion [%d-%d] to process %d\n",partition_size * i, partition_size * (i + 1), i);
            fflush(stderr);
            MPI_Send(array + partition_size * i, partition_size, MPI_DOUBLE, i + 1, SEND, MPI_COMM_WORLD);
        }

        /* Computes its partition (The master always get the last partition that may vary in size 
        if the number of processes doesnt divide the size of the array) */
        computed_value = compute(array + partition_size * (num_procs - 1), array_size - (partition_size * (num_procs - 1)));
        fprintf(stderr, "[Node 0] Partition computed %lf\n", computed_value);
        fflush(stderr);
        
        free(array);

        /* Listens to any source and retrieves the messages */
        for (int i = 0; i < num_procs - 1; i++){

            /* Receives a message */
            MPI_Status status;
            MPI_Recv(&value_received, 1, MPI_DOUBLE, MPI_ANY_SOURCE, RECEIVE, MPI_COMM_WORLD, &status);

            /* Sums the values */
            computed_value += value_received;

            fprintf(stderr, "[Node 0] Value %lf received from %d\n", value_received, status.MPI_SOURCE);
            fflush(stderr);
        }


    } else {
        /* This code is exclusive to the followers */

        /* Allocates the memory */
        array_received = malloc(partition_size * sizeof(double));

        /* Memory error control */
        if(!array_received){
            fprintf(stderr, "\nERROR: Cannot allocate array \n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        /* Receives a portion of the array */
        MPI_Recv(array_received, partition_size, MPI_DOUBLE, 0, SEND, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        /* Computes the partition */
        computed_value = compute(array_received, partition_size);
        fprintf(stderr, "[Node %d] Partition computed (%lf)\n", num_local, computed_value);
        fflush(stderr);
        free(array_received);

        /* Sends back the result */
        MPI_Send(&computed_value, 1, MPI_DOUBLE, 0, RECEIVE, MPI_COMM_WORLD);
    }

    /* Print the final value */
    if (num_local == 0){
        fprintf(stderr, "[Node 0] Computation completed %lf\n", computed_value);
        fflush(stderr);
    }
    
    /* Finalizes the MPI interface */
    MPI_Finalize();
    return 0;
}

/**
 * @brief Given an array of doubles, it squares each value and sums it up
 *
 * @param array The array of doubles to perform the computation
 * @param size The size of the array
 * @return  A double, the result of the computation
 */
double compute(double* array, int size) {
    double value = 0;

    /* Error control */
    if (!array){
        fprintf(stderr, "ERROR: Null array in compute\n");
        fflush(stderr);
    }

    /* Error control */
    if (size <= 0){
        fprintf(stderr, "ERROR: The size of the array must be positive \n");
        fflush(stderr);
    }
    
    /* Iterates through the values, it squares each values and sums it all together */
    for(int i = 0; i < size; i++){
        value += array[i] * array[i];
    }

    return value;

}