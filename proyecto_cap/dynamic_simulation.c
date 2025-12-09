/**
 * @file simulacion.c
 * @brief MPI program that simulates scheduling algorithms of parallelized tasks
 * 
 * To compile it:
 * - If you want debug prints that shows how the computation is being carried:
 *      mpicc -DDEBUG -o dynamic_simulation.out dynamic_simulation.c
 * 
 * To execute it:
 *  ./dinamic_simulation.out <n_tasks> <type_tasks> <scheduler> <first_parameter_scheduler> <second_parameter_scheduler>
 *  
 *  Arguments:
 *      - ntasks: integer that specifies the number of tasks to simulate
 *      - type_tasks: string that specifies the duration of each task
 *          - "equal": each task takes around 5ms
 *          - "equal_big": each tasks takes around 50ms
 *          - "random": each task takes around 1ms and 10ms
 *          - "random_big": each task takes around 10ms and 100ms
 *          - "increasing": the tasks durations increases from 1ms to 10ms
 *          - "increasing_big": the tasks durations increases from 10ms to 100ms
 *          - "decreasing": the tasks durations decreases from 10ms to 1ms
 *          - "decreasing_big": the tasks durations decreases from 100ms to 10ms
 *      - scheduler: string that specifies the type of dynamic scheduler
 *          - "chunk": The scheduler gives to each worker a fixed number of tasks
 *          The size of the chunk is specified in the <first_parameter_scheduler>, the <second_parameter_scheduler> must be blank.
 *          - "guided": The scheduler gives to each worker a proportional number of the remaining tasks.
 *          Both the <first_parameter_scheduler> and the <second_parameter_scheduler> must be blank.
 *          - "trapezoidal": The scheduler gives to each worker a decreasing number of tasks.
 *          The initial size of the chunk is specified in the <first_parameter_scheduler>.
 *          The diference between chunk sizes is specified in the <second_parameter_scheduler>.
 * 
 */

#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define EQUAL_TIME_SMALL 5
#define INITIAL_TIME_SMALL 1
#define FINAL_TIME_SMALL 10
#define EQUAL_TIME_BIG 10 * EQUAL_TIME_SMALL
#define INITIAL_TIME_BIG 10 * INITIAL_TIME_SMALL
#define FINAL_TIME_BIG 10 * FINAL_TIME_SMALL

typedef enum {
    SEND_N_TASKS,
    SEND_TIMES,
    TASK_COMPLETED
}Tag;

typedef enum {
    CHUNK,
    GUIDED,
    TRAPEZOIDAL,
}SchType;



typedef int (*sch_fnc)(int, int, int, int, int);

/**
 * @brief Scheduler function that implements the dynamic chunk shceduler. This shceduler distributes the iterations in chunks of size Z.
 * @param num_procs not used
 * @param num_tasks number of tasks to shcedule
 * @param remaining_tasks number of tasks that remains to be assigned
 * @param Z chunk size
 * @param k not used
 * 
 * @returns the number of tasks to be assigned to the next process
 */
int dynamic_chunk_scheduler(int num_procs, int num_tasks, int remaining_tasks, int Z, int k){
    if(remaining_tasks >= Z){
        return Z;
    }else{
        return remaining_tasks;
    }
}

/**
 * @brief Scheduler function that implements the dynamic guided shceduler. This shceduler distributes iterations smaller as we aproach the end. 
 
 The size of the iterations is proportional to the remaining tasks 
 * @param num_procs number of process to schedule
 * @param num_tasks not used
 * @param remaining_tasks number of tasks that remains to be assign
 * @param Z not used
 * @param k not used
 * 
 * @return the number of tasks to be assigned to the next process
 */
int dynamic_guided_scheduler(int num_procs, int num_tasks, int remaining_tasks, int Z, int k){
    return (int) ceil(((double) remaining_tasks)/((double) num_procs));
}

/**
 * @brief Scheduler function that implements the dynamic guided shceduler. This shceduler distributes iterations smaller as we aproach the end. 
 
 The size of the iterations is proportional to the remaining tasks 
 * @param num_procs number of process to schedule
 * @param num_tasks number of tasks to shcedule
 * @param remaining_tasks number of tasks that remains to be assign
 * @param Z initial size of the iterations
 * @param k size to be reduced
 * 
 * @return the number of tasks to be assigned to the next process
 */
int dynamic_trapezoidal_scheduler(int num_procs, int num_tasks, int remaining_tasks, int Z, int k){
    static int last_size = 0;

    // If this iteration is the first one, set the last_size to Z
    if(last_size == 0){
        last_size = Z;
    // If we can keep reducing the iterations size, keep doing it, if not, maintain the size to the last one
    }else if(last_size - k > 0){
        last_size = last_size - k;
    }
    
   
    return last_size;
}

void populate_equal_time_array(int *time_array, int num_tasks, int time) {
    
    /* Error control */
    if (!time_array)
        return;

    for (int i = 0; i < num_tasks; i++) {
        time_array[i] = time;
    }
}

void populate_increasing_time_array(int *time_array, int num_tasks, int initial_time, int final_time) {
    
    /* Error control */
    if (!time_array)
        return;

    for (int i = 0; i < num_tasks; i++) {
        time_array[i] = (int) floor((double) initial_time + i * (double) (final_time - initial_time) / (double) num_tasks);
    }
}


void populate_random_time_array(int *time_array, int num_tasks, int min_time, int max_time) {
    
    /* Error control */
    if (!time_array)
        return;

    srand(time(NULL));

    for (int i = 0; i < num_tasks; i++) {
        time_array[i] = min_time + (int) round(((double)rand() / RAND_MAX) * (max_time - min_time));
    }
}


int master(int num_procs, int argc, char *argv[]) {
   
   /* Total number of tasks to simulate */
    int num_tasks;
    int task_not_assigned;
    int n_tasks_assigned;
    int task_completed = 0;

    /* Completed tasks by workers */
    int *tasks_completed_workers;

    /* Array containing the time it takes to compute each task (in ms)*/
    int *time_array;

    /* Scheduling function */
    SchType scheduling_type;
    sch_fnc scheduling_function;

    /* Scheduling parameters */
    int first_scheduling_parameter = 0;
    int second_scheduling_parameter = 0;

    /* Received value */
    int value_received;
    MPI_Status status;
    int worker_id;

    /* Measure time */
    double initial_time, final_time;

    /* Get the arguments */
    if (argc < 4){
        fprintf(stderr, "\nERROR: Not enough arguments \n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 0;

    } else if (argc > 6) {
        fprintf(stderr, "\nERROR: Too many arguments \n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 0;
    }

    /* Get the number of tasks */
    num_tasks = atoi(argv[1]);
    task_not_assigned = num_tasks;

    #ifdef DEBUG
    fprintf(stderr, "[Master] Number of tasks: %d. \n", num_tasks);
    fflush(stderr);
    #endif

    /* Error control */
    if (num_tasks <= 0) {
        fprintf(stderr, "\nERROR: The number of tasks must be positive: %d \n", num_tasks);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 0;
    }

    /* Initialize the time array */
    time_array = malloc(sizeof(int) * num_tasks);

    /* Error control */
    if (!time_array){
        fprintf(stderr, "\nERROR: Couldnt allocate memory for time array \n");
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 0;
    }

    /* Get the type of tasks and populate the array */
    if (strcmp(argv[2], "random") == 0){
        populate_random_time_array(time_array, num_tasks, INITIAL_TIME_SMALL, FINAL_TIME_SMALL);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: random. \n");
        fflush(stderr);
        #endif

    } else if (strcmp(argv[2], "equal") == 0) {
        populate_equal_time_array(time_array, num_tasks, EQUAL_TIME_SMALL);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: equal. \n");
        fflush(stderr);
        #endif

    } else if (strcmp(argv[2], "increasing") == 0) {
        populate_increasing_time_array(time_array, num_tasks, INITIAL_TIME_SMALL, FINAL_TIME_SMALL);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: increasing. \n");
        fflush(stderr);
        #endif
        
    } else if (strcmp(argv[2], "decreasing") == 0) {
        populate_increasing_time_array(time_array, num_tasks, FINAL_TIME_SMALL, INITIAL_TIME_SMALL);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: increasing. \n");
        fflush(stderr);
        #endif

    if (strcmp(argv[2], "random_big") == 0){
        populate_random_time_array(time_array, num_tasks, INITIAL_TIME_BIG, FINAL_TIME_BIG);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: random. \n");
        fflush(stderr);
        #endif

    } else if (strcmp(argv[2], "equal_big") == 0) {
        populate_equal_time_array(time_array, num_tasks, EQUAL_TIME_BIG);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: equal. \n");
        fflush(stderr);
        #endif

    } else if (strcmp(argv[2], "increasing_big") == 0) {
        populate_increasing_time_array(time_array, num_tasks, INITIAL_TIME_BIG, FINAL_TIME_BIG);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: increasing. \n");
        fflush(stderr);
        #endif
        
    } else if (strcmp(argv[2], "decreasing_big") == 0) {
        populate_increasing_time_array(time_array, num_tasks, FINAL_TIME_BIG, INITIAL_TIME_BIG);

        #ifdef DEBUG
        fprintf(stderr, "[Master] Time distribution of tasks: increasing. \n");
        fflush(stderr);
        #endif
        
        
    } else {
        fprintf(stderr, "\nERROR: Not recognised the type of task \n");
        free(time_array);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 0;
    }
    

    /* Get the scheduler */
    if (strcmp(argv[3], "chunk") == 0){
        scheduling_type = CHUNK;
        scheduling_function = dynamic_chunk_scheduler;

        #ifdef DEBUG
        fprintf(stderr, "[Master] Scheduler selected: chunk. \n");
        fflush(stderr);
        #endif

    } else if (strcmp(argv[3], "guided") == 0) {
        scheduling_type = GUIDED;
        scheduling_function = dynamic_guided_scheduler;

        #ifdef DEBUG
        fprintf(stderr, "[Master] Scheduler selected: guided. \n");
        fflush(stderr);
        #endif

    } else if (strcmp(argv[3], "trapezoidal") == 0) {
        scheduling_type = TRAPEZOIDAL;
        scheduling_function = dynamic_trapezoidal_scheduler;

        #ifdef DEBUG
        fprintf(stderr, "[Master] Scheduler selected: trapezoidal. \n");
        fflush(stderr);
        #endif
        
    } else {
        fprintf(stderr, "\nERROR: Not recognised the type of task \n");
        free(time_array);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 0;
    }

    /* Get the parameter of the chunk scheduler */
    if (scheduling_type == CHUNK) {
        
        /* Error control */
        if (argc != 5) {
            fprintf(stderr, "\nERROR: The number of arguments must be four \n");
            free(time_array);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 0;
        }

        /* Get the first parameter */
        first_scheduling_parameter  = atoi(argv[4]);

        /* Error control */
        if (first_scheduling_parameter <= 0) {
            fprintf(stderr, "\nERROR: The first scheduling parameter must be positive \n");
            free(time_array);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 0;
        }

        #ifdef DEBUG
        fprintf(stderr, "[Master] Chunk size: %d. \n", first_scheduling_parameter);
        fflush(stderr);
        #endif
        
    }

    /* Get the parameters of the trapezoidal scheduler */
    if (scheduling_type == TRAPEZOIDAL) {
        
        /* Error control */
        if (argc != 6) {
            fprintf(stderr, "\nERROR: The number of arguments must be five \n");
            free(time_array);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 0;
        }

        /* Get the first parameter */
        first_scheduling_parameter  = atoi(argv[4]);

        /* Error control */
        if (first_scheduling_parameter <= 0) {
            fprintf(stderr, "\nERROR: The first scheduling parameter must be positive \n");
            free(time_array);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 0;
        }

        /* Get the first parameter */
        second_scheduling_parameter  = atoi(argv[5]);

        /* Error control */
        if (second_scheduling_parameter <= 0) {
            fprintf(stderr, "\nERROR: The second scheduling parameter must be positive \n");
            free(time_array);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 0;
        }

        #ifdef DEBUG
        fprintf(stderr, "[Master] Initial size: %d, k: %d. \n", first_scheduling_parameter, second_scheduling_parameter);
        fflush(stderr);
        #endif
        
    }

    /* Init the array containing the tasks completed by each worker */
    tasks_completed_workers = malloc(sizeof(int) * num_procs - 1);

    /* Error control */
    if (!tasks_completed_workers) {
        fprintf(stderr, "\nERROR: Couldnt allocate memory for tasks_completed_workers \n");
        free(time_array);
        MPI_Abort(MPI_COMM_WORLD, 1);
        return 0;
    }

    for (int i = 0; i < (num_procs - 1); i++) {
        tasks_completed_workers[i] = 0;
    }

    /* Start measuring time */
    initial_time = MPI_Wtime();

    /* First we assign tasks to all the workers */

    for (int worker = 1; worker < num_procs; worker++){

        if (task_not_assigned > 0) {

            /* Ask the scheduler for a number of tasks */
            n_tasks_assigned = scheduling_function(num_procs - 1, num_tasks, task_not_assigned, first_scheduling_parameter, second_scheduling_parameter);
            
            /* Error control */
            if (n_tasks_assigned <= 0 && task_not_assigned > 0) {
                fprintf(stderr, "\nERROR: The scheduler must assign a positive number of tasks, tasks_assigned: %d, tasks_remaining: %d \n", n_tasks_assigned, task_not_assigned);
                free(time_array);
                free(tasks_completed_workers);
                MPI_Abort(MPI_COMM_WORLD, 1);
                return 0;
            }

            /* Check for overflow */
            if (n_tasks_assigned > task_not_assigned) {
                n_tasks_assigned = task_not_assigned;
            }

            /* Send to the worker the number of tasks to perform */
            MPI_Send(&n_tasks_assigned, 1, MPI_INT, worker, SEND_N_TASKS, MPI_COMM_WORLD);
            
            /* Send the worker the time of each task */
            MPI_Send(time_array + (num_tasks - task_not_assigned), n_tasks_assigned, MPI_INT, worker, SEND_TIMES, MPI_COMM_WORLD);

            task_not_assigned -= n_tasks_assigned;

            #ifdef DEBUG
            fprintf(stderr, "[Master] %d tasks assigned to worker %d, tasks remaining: %d. \n", n_tasks_assigned, worker, task_not_assigned);
            fflush(stderr);
            #endif

        } else {

            /* There wasnt enough task for all the workers */

            /* Send kill message */
            n_tasks_assigned = -1;
            MPI_Send(&n_tasks_assigned, 1, MPI_INT, worker, SEND_N_TASKS, MPI_COMM_WORLD);

            #ifdef DEBUG
            fprintf(stderr, "[Master] Send kill message to worker %d.\n", worker);
            fflush(stderr);
            #endif
        }

    }

    /* Then the master listens for workers that have finished and assigns them another task (if there are tasks left) */
    while (task_completed < num_tasks) {
        
        /* Receives a message from a finished worker with the number of tasks completed */
        MPI_Recv(&value_received, 1, MPI_INT, MPI_ANY_SOURCE, TASK_COMPLETED, MPI_COMM_WORLD, &status);

        /* Error control */
        if (value_received <= 0){
            fprintf(stderr, "\nERROR: The worker must return a positive number of tasks \n");
            free(time_array);
            free(tasks_completed_workers);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 0;
        }

        /* Get the worker id in order to send him more tasks */
        worker_id = status.MPI_SOURCE;
        task_completed += value_received;

        /* Stats */
        tasks_completed_workers[worker_id - 1] += value_received;

        #ifdef DEBUG
        fprintf(stderr, "[Master] Received that worker %d completed %d tasks, task completed: %d/%d.\n", worker_id, value_received, task_completed, num_tasks);
        fflush(stderr);
        #endif

        /* If there is tasks left assign the worker new tasks, if not kill it :(*/
        if (task_not_assigned > 0) {

            /* Send new tasks */

            /* Ask the scheduler for a number of tasks */
            n_tasks_assigned = scheduling_function(num_procs - 1, num_tasks, task_not_assigned, first_scheduling_parameter, second_scheduling_parameter);
            
            /* Error control */
            if (n_tasks_assigned <= 0 && task_not_assigned > 0) {
                fprintf(stderr, "[Master] %d tasks assigned to worker %d, tasks remaining: %d. \n", n_tasks_assigned, worker_id, task_not_assigned);
                free(time_array);
                free(tasks_completed_workers);
                MPI_Abort(MPI_COMM_WORLD, 1);
                return 0;
            }

            /* Check for overflow */
            if (n_tasks_assigned > task_not_assigned) {
                n_tasks_assigned = task_not_assigned;
            }
            /* Send to the worker the number of tasks to perform */
            MPI_Send(&n_tasks_assigned, 1, MPI_INT, worker_id, SEND_N_TASKS, MPI_COMM_WORLD);
            
            /* Send the worker the time of each task */
            MPI_Send(time_array + (num_tasks - task_not_assigned), n_tasks_assigned, MPI_INT, worker_id, SEND_TIMES, MPI_COMM_WORLD);

            task_not_assigned -= n_tasks_assigned;

            #ifdef DEBUG
            fprintf(stderr, "[Master] %d tasks assigned to worker %d, tasks remaining: %d. \n", n_tasks_assigned, worker_id, task_not_assigned);
            fflush(stderr);
            #endif

        } else {
            
            /* Send kill message */
            n_tasks_assigned = -1;
            MPI_Send(&n_tasks_assigned, 1, MPI_INT, worker_id, SEND_N_TASKS, MPI_COMM_WORLD);

            #ifdef DEBUG
            fprintf(stderr, "[Master] Send kill message to worker %d.\n", worker_id);
            fflush(stderr);
            #endif

        }
    }

    #ifdef DEBUG
    fprintf(stderr, "[Master] All tasks completed.\n", worker_id);
    fflush(stderr);
    #endif

    /* Stop measuring time */
    final_time = MPI_Wtime();

    /* Print the time elapsed */
    printf("[Master] Time: %lf ms\n", (final_time - initial_time) * 1000);

    /* Print the tasks completed by each worker */
    for(int i = 0; i < (num_procs - 1); i++) {
        printf("[Master] Worker %d completed a total of %d tasks.\n", i + 1, tasks_completed_workers[i]);
    }

    /* Finalizes the MPI interface */
    MPI_Finalize();

    /* Free the resources */
    free(time_array);
    free(tasks_completed_workers);

    return EXIT_SUCCESS;

}

int follower(int num_local) {
    int n_tasks_received;
    int *tasks_received;

    /* Measure time */
    double initial_time, final_time;

    /* Infinite loop, it stops when we get a kill message */
    while(1){

        /* Receives the first message from the master, containing the number of tasks to perform */
        MPI_Recv(&n_tasks_received, 1, MPI_INT, 0, SEND_N_TASKS, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        /* Check if it is a kill message */
        if (n_tasks_received == -1){

            #ifdef DEBUG
            fprintf(stderr, "[Node %d] Killed by message :o \n", num_local);
            fflush(stderr);
            #endif
            break;
        }

        #ifdef DEBUG
        fprintf(stderr, "[Node %d] %d tasks to perform. \n", num_local, n_tasks_received);
        fflush(stderr);
        #endif

        /* Allocate memory for the array containing the time of each task */
        tasks_received = malloc(sizeof(int) * n_tasks_received);

        /* Error control */
        if (!tasks_received){
            fprintf(stderr, "\nERROR: Cannot allocate array \n");
            fflush(stderr);
            MPI_Abort(MPI_COMM_WORLD, 1);
            return 1;
        }

        /* Receives the array containing the time of each task */
        MPI_Recv(tasks_received, n_tasks_received, MPI_INT, 0, SEND_TIMES, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        /* Simulate the tasks */
        for (int i = 0; i < n_tasks_received; i++) {
            #ifdef DEBUG
            fprintf(stderr, "[Node %d] Simulating task %d of %d ms. \n", num_local, i, tasks_received[i]);
            fflush(stderr);
            #endif
            usleep(tasks_received[i] * 1000);
        }

        #ifdef DEBUG
        fprintf(stderr, "[Node %d] All %d tasks completed. \n", num_local, n_tasks_received);
        fflush(stderr);
        #endif

        /* Send back a confirmation of the completed tasks */
        MPI_Send(&n_tasks_received, 1, MPI_INT, 0, TASK_COMPLETED, MPI_COMM_WORLD);

        /* Free the resources */
        free(tasks_received);
    }

    /* Finalizes the MPI interface */
    MPI_Finalize();

    return EXIT_SUCCESS;
    
}


int main(int argc, char *argv[]) {
    int num_procs, num_local;
    char mach_name[MPI_MAX_PROCESSOR_NAME];
    int mach_len;

    


    /* Init the MPI interface*/
    MPI_Init (&argc,&argv);
    MPI_Comm_size (MPI_COMM_WORLD, &num_procs);
    MPI_Comm_rank (MPI_COMM_WORLD, &num_local);
    MPI_Get_processor_name(mach_name,&mach_len);

    

    if (num_local == 0){
        return master(num_procs, argc, argv);
        
    } else {
        return follower(num_local);  
    }  
}