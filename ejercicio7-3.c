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
    double *array_received = NULL;
    double final_value = 0;
    double initial_time, final_time;

    /* Init the MPI interfeace */
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

        /* Init the array */
        for(int i = 0; i < array_size; i++){
            array[i] = (double) rand() / (double) RAND_MAX;
        } 
    } 

    /* This code is executed by all nodes (master included) */

    /* Allocate memory for the receiving array */
    array_received = malloc(partition_size * sizeof(double));

    /* Memory error control */
    if (!array_received){
        fprintf(stderr, "\nERROR: Cannot allocate array \n");
        fflush(stderr);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 1;
    }

    /* Start measuring time */
    if (num_local == 0){
        initial_time = MPI_Wtime();
    }

    /* Scatter the data, it sends to all the nodes a partition of the data, it is stored in array received and have partition_size length */
    MPI_Scatter(array, partition_size, MPI_DOUBLE, array_received, partition_size, MPI_DOUBLE, 0, MPI_COMM_WORLD); 
    

    /* Compute the data */
    computed_value = compute(array_received, partition_size);

    /* Frees the memory */  
    free(array_received);

    /* Gathers the data and sums it up together */
    MPI_Reduce(&computed_value, &final_value, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    
    if (num_local == 0){

        /* This code is exclusive to the master */

        /* Stop measuring time */
        final_time = MPI_Wtime();

        /* Print the time elapsed */
        printf("%lf\n", (final_time - initial_time) * 1000);
        
        /* Free the array exclusive to the master */
        free(array);
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