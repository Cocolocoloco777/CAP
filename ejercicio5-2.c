/**
 * @file ejercicio5-2.c
 * @brief MPI program that computes the cuadratic sum of a randomly generated 
 * array using the MPI_Scatterv and MPI_Gather functions
 * 
 * To compile it:
 * - If you want debug prints that shows how the computation is being carried:
 *      mpicc -DDEBUG -o ejercicio5-2 ejercicio5-2.c
 * - If you want to just see the time it takes to compute:
 *      mpicc -DTIME -o ejercicio5-2 ejercicio5-2.c
 * 
 * To execute it:
 * - If you want to compute an array of custom size (N long):
 *      ./ejercicio5-2 N
 * - If you want the default array size of 1000000:
 *      ./ejercicio5-2
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

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
    double *values_received = NULL;
    double *array_received = NULL;
    double final_value = 0;
    int *sendcounts = NULL;
    int *displacement = NULL;

    #ifdef TIME
    double initial_time, final_time;
    #endif

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
 
    if(num_local == 0) {
        /* This code is exclusive to the master */

        /* Allocate memory for the initial array */
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

        #ifdef DEBUG
        fprintf(stderr, "[Node 0] Array of size %d created, total sum: %lf\n",array_size, compute(array, array_size));
        fflush(stderr);
        #endif

        /* Allocate memory for the computed values array that the master receives at the end */
        values_received = malloc(num_procs * sizeof(double));

        /* Memory error control */
        if (!values_received){
            free(array);
            fprintf(stderr, "\nERROR: Cannot allocate array \n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        /* Allocate the array of the number of elements to send to each one */
        sendcounts = malloc(num_procs * sizeof(int));

        /* Memory error control */
        if (!sendcounts){
            free(array);
            free(values_received);
            fprintf(stderr, "\nERROR: Cannot allocate array \n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        /* Initialize sendcount array */
        for(int i = 0; i < num_procs; i++){
            if (i < array_size % num_procs){
                sendcounts[i] = partition_size + 1; 
            } else {
                sendcounts[i] = partition_size;
            }
        }

        #ifdef DEBUG
        fprintf(stderr, "[Node 0] Sendcounts created: ");
        for(int i = 0; i < num_procs; i++){
            fprintf(stderr, "[%d] %d ", i, sendcounts[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        #endif

        /* Allocate the array of the displacement */
        displacement = malloc(num_procs * sizeof(int));

        /* Memory error control */
        if (!displacement){
            free(array);
            free(values_received);
            free(sendcounts);
            fprintf(stderr, "\nERROR: Cannot allocate array \n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        /* Initialize the displacement array */
        displacement[0] = 0;

        for(int i = 1; i < num_procs; i++){
            displacement[i] = displacement[i - 1] + sendcounts[i - 1]; 
        }

        #ifdef DEBUG
        fprintf(stderr, "[Node 0] Displacement created: ");
        for(int i = 0; i < num_procs; i++){
            fprintf(stderr, "[%d] %d ", i, displacement[i]);
        }
        fprintf(stderr, "\n");
        fflush(stderr);
        #endif
    } 

    /* This code is executed by all nodes (master included) */

    /* Allocate memory for the receiving array */
    array_received = malloc((partition_size + ((num_local < array_size % num_procs) ? 1 : 0)) * sizeof(double));

    #ifdef DEBUG
    fprintf(stderr, "[Node %d] Receiving array created of size %d\n", num_local, (partition_size + ((num_local < array_size % num_procs) ? 1 : 0)));
    fflush(stderr);
    #endif

    /* Memory error control */
    if (!array_received){
        if (array){
            free(array);
        }
        if (values_received){
            free(values_received);
        }
        if (sendcounts){
            free(sendcounts);
        }
        if (displacement){
            free(displacement);
        }
        fprintf(stderr, "\nERROR: Cannot allocate array \n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    #ifdef TIME
    /* Start measuring time */
    initial_time = MPI_Wtime();
    #endif

    /* Scatter the data, it sends to all the nodes a partition of the data, it is stored in array received and have partition_size length */
    MPI_Scatterv(array, sendcounts, displacement, MPI_DOUBLE, array_received, (partition_size + ((num_local < array_size % num_procs) ? 1 : 0)), MPI_DOUBLE, 0, MPI_COMM_WORLD); 
    
    /* Compute the data */
    computed_value = compute(array_received, (partition_size + ((num_local < array_size % num_procs) ? 1 : 0)));

    #ifdef DEBUG
    fprintf(stderr, "[Node %d] Partition computed %lf\n", num_local, computed_value);
    fflush(stderr);
    #endif

    /* Frees the memory */ 
    free(array_received);

    /* Gathers the data, it stores the completed computation of all the nodes in values_received */
    MPI_Gather(&computed_value, 1, MPI_DOUBLE, values_received, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    
    if (num_local == 0){
        /* This code is exclusive to the master */

        /* Sum the data from the values received that MPI_Gather has gathered */
        for (int i = 0; i < num_procs; i++){
            final_value += values_received[i];
        }

        #ifdef TIME
        /* Stop measuring time */
        final_time = MPI_Wtime();

        /* Print the time elapsed */
        printf("[Node 0] Time: %lf, computed value: %lf\n", (final_time - initial_time) * 1000, computed_value);
        #endif

        #ifdef DEBUG
        fprintf(stderr, "[Node 0] Computation completed %lf\n", final_value);
        fflush(stderr);
        #endif

        /* Free the arrays exclusive to the master */
        free(array);
        free(sendcounts);
        free(displacement);
        free(values_received);
    }
    
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