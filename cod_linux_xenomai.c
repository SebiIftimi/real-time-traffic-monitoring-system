#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>


int num_cars = 0; // number of cars
int safety = 1; // 1 - tunnel is safe, 0 - tunnel is not safe
int gas_detected = 0;  // 0 - gas not detected, 1 - gas detected
int block_entry = 0; // 0 - entry allowed, 1 - entry not allowed
int block_exit = 0; // 0 - exit allowed, 1 - exit not allowed

// Declare semaphores, mutexes, and condition variables
sem_t sem_entry, sem_exit, sem_monitoring;
pthread_mutex_t mutex_num_cars, mutex_safety, mutex_block_entry, mutex_block_exit;
pthread_cond_t condvar_entry, condvar_exit;

// Thread associated with the entry task
void *entry(void *arg) {
    pthread_mutex_lock(&mutex_block_entry);
    // If entry is blocked, the thread waits until it is unblocked
    while (block_entry) {
        // Release the thread when notified by condvar_entry
        pthread_cond_wait(&condvar_entry, &mutex_block_entry);
    }
    pthread_mutex_unlock(&mutex_block_entry);
    
    pthread_mutex_lock(&mutex_block_exit);
    while (block_exit)
        // If exit is blocked, the thread waits until it is unblocked
        pthread_cond_wait(&condvar_exit, &mutex_block_exit);
    pthread_mutex_unlock(&mutex_block_exit);

    // Waits for permission to enter the tunnel (num_cars)
    sem_wait(&sem_entry);
    pthread_mutex_lock(&mutex_num_cars);
    pthread_mutex_lock(&mutex_safety);
    
    if (num_cars < 5 && safety) {
        num_cars++;
        printf("A car has entered: %d cars in the tunnel\n", num_cars);
        // Notify 'monitoring' that a car has entered the tunnel
        sem_post(&sem_monitoring);
    }
    pthread_mutex_unlock(&mutex_safety);
    pthread_mutex_unlock(&mutex_num_cars);
    // Notify 'exit' that a car has entered the tunnel
    sem_post(&sem_exit);
    return NULL;
}

// Thread associated with the exit task
void *exit(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex_block_exit);
        while (block_exit)
            // If exit is blocked, the thread waits until it is unblocked
            pthread_cond_wait(&condvar_exit, &mutex_block_exit);
        pthread_mutex_unlock(&mutex_block_exit);
        // Waits for permission to exit the tunnel
        sem_wait(&sem_exit);
        pthread_mutex_lock(&mutex_num_cars);
        if (num_cars > 0) {
            num_cars--;
            printf("A car has exited: %d cars in the tunnel\n", num_cars);
            // Notify 'monitoring' that a car has exited the tunnel
            sem_post(&sem_monitoring);
            if (num_cars == 0) 
            {  // If all cars have exited the tunnel
               // broadcast a notification to threads waiting on condvar_exit
                pthread_cond_broadcast(&condvar_exit); 
            }
        }
        pthread_mutex_unlock(&mutex_num_cars);
        // Notify 'exit' that a car has exited
        // and there is space in the tunnel for another car to enter
        sem_post(&sem_entry);
        sleep(3);
    }
    return NULL;
}

// Thread associated with the monitoring task
void *monitoring(void *arg) {
    while (1) {
        // Wait for a car to enter or exit
        sem_wait(&sem_monitoring);
        pthread_mutex_lock(&mutex_num_cars);
        if (num_cars >= 5) {
            printf("Limit of 5 cars reached! Entry blocked.\n");
            // Block the entry of cars
            sem_wait(&sem_entry);
        } else {
            // Allow the entry of cars
            sem_post(&sem_entry); 
        }
        pthread_mutex_unlock(&mutex_num_cars);
    }
    return NULL;
}

// Thread associated with the smoke task
void *smoke(void *arg) {
    while (1) {
        // Generate event
        int smoke_detected = rand() % 10;
        pthread_mutex_lock(&mutex_safety);
        if (smoke_detected < 1 && safety) {
            safety = 0;
            printf("Smoke detected! Entry blocked.\n");
            // Block the entry of cars
            sem_wait(&sem_entry);
            // Unblock exit if it is blocked
            pthread_mutex_lock(&mutex_block_exit);
            if (block_exit) {
                block_exit = 0;
                pthread_cond_broadcast(&condvar_exit);
                printf("Exit unblocked.\n");
            }
            pthread_mutex_unlock(&mutex_block_exit);
        } else if (!gas_detected && !safety) {
            safety = 1;
            printf("No more smoke detected. Entry unblocked.\n");
            // Allow the entry of cars
            sem_post(&sem_entry);
        }
        pthread_mutex_unlock(&mutex_safety);
        sleep(1);
    }
}

// Thread associated with the gas task
void *gas(void *arg) {
    while (1) {
        // Generate event
        int gas_detection = rand() % 10;
        pthread_mutex_lock(&mutex_safety);
        if (gas_detection < 1 && !gas_detected) {
            gas_detected = 1;
            safety = 0;
            printf("Gas detected! Entry blocked.\n");
            // Block the entry of cars
            sem_wait(&sem_entry);

            // Unblock exit if it is blocked
            pthread_mutex_lock(&mutex_block_exit);
            if (block_exit) {
                block_exit = 0;
                pthread_cond_broadcast(&condvar_exit);
                printf("Exit unblocked.\n");
            }
            pthread_mutex_unlock(&mutex_block_exit);
        } else if (gas_detected) {
            gas_detected = 0;
            if (safety == 0) {
                safety = 1;
                printf("No more gas detected. Entry unblocked.\n");
                // Allow the entry of cars
                sem_post(&sem_entry);
            }
        }
        pthread_mutex_unlock(&mutex_safety);
        sleep(1);
    }
}

// Thread associated with the panic button task
void *panic_button(void *arg) {
    clock_t start_time = clock();
    // The time interval at which the panic button is pressed
    int interval = 60000;

    while (1) {
        clock_t current_time = clock();
        // Check if the time has expired and if there are cars in the tunnel
        if (((double)(current_time - start_time) / CLOCKS_PER_SEC) * 1000 >= interval && num_cars > 0) {
            // Block entry
            pthread_mutex_lock(&mutex_block_entry);
            block_entry = 1;
            printf("Panic button pressed. Entry blocked.\n");
            pthread_mutex_unlock(&mutex_block_entry);

            // Unblock exit if it is blocked
            pthread_mutex_lock(&mutex_block_exit);
            if (block_exit) {
                block_exit = 0;
                pthread_cond_broadcast(&condvar_exit);
                printf("Exit unblocked.\n");
            }
            pthread_mutex_unlock(&mutex_block_exit);

            pthread_mutex_lock(&mutex_num_cars);
            while (num_cars > 0) {
                // Unblock exit
                pthread_cond_wait(&condvar_exit, &mutex_num_cars);
            }
            printf("All cars have exited the tunnel.\n");
            pthread_mutex_unlock(&mutex_num_cars);

            // Reset timer
            start_time = clock();
            // Unblock entry
            pthread_mutex_lock(&mutex_block_entry);
            block_entry = 0;
            pthread_mutex_unlock(&mutex_block_entry);
        }
    }
}

int main() {
    srand(time(NULL)); // Initialize random seed

    // Declare and initialize threads
    pthread_t exit_sensor, monitoring_thread, entry_sensor, smoke_thread, gas_thread, panic_button_thread;
    pthread_mutex_init(&mutex_num_cars, NULL);
    pthread_mutex_init(&mutex_safety, NULL);
    pthread_mutex_init(&mutex_block_entry, NULL);
    pthread_mutex_init(&mutex_block_exit, NULL);
    
    // Initialize condition variables
    pthread_cond_init(&condvar_entry, NULL);
    pthread_cond_init(&condvar_exit, NULL);

    // Initialize semaphores
    if(sem_init(&sem_entry, 0, 5) == -1){
        printf("Error in initialization: ");
    }

    if(sem_init(&sem_exit, 0, 0) == -1){
        printf("Error in initialization: ");
    }

    if(sem_init(&sem_monitoring, 0, 0) == -1){
        printf("Error in initialization: ");
    }

    // Create threads for each task
    pthread_create(&panic_button_thread, NULL, panic_button, NULL);
    pthread_create(&smoke_thread, NULL, smoke, NULL);
    pthread_create(&gas_thread, NULL, gas, NULL);
    pthread_create(&exit_sensor, NULL, exit, NULL);
    pthread_create(&monitoring_thread, NULL, monitoring, NULL);

    printf("Press 1 to let a car enter, 2 to block/unblock entry, 3 to block/unblock exit.\n");
    int option;
    while (1) {
        scanf("%d", &option);
        if (option == 1) {
            // Create and detach entry sensor thread each time option 1 is chosen
            pthread_create(&entry_sensor, NULL, entry, NULL);
            pthread_detach(entry_sensor);
        } else if (option == 2) {
            pthread_mutex_lock(&mutex_block_entry);
            block_entry = !block_entry;
            if (block_entry) {
                printf("Entry has been blocked.\n");
            } else {
                printf("Entry has been unblocked.\n");
                pthread_cond_broadcast(&condvar_entry);
            }
            pthread_mutex_unlock(&mutex_block_entry);
        } else if (option == 3) {
            pthread_mutex_lock(&mutex_block_exit);
            block_exit = !block_exit;
            if (block_exit) {
                printf("Exit has been blocked.\n");
            } else {
                printf("Exit has been unblocked.\n");
                pthread_cond_broadcast(&condvar_exit);
            }
            pthread_mutex_unlock(&mutex_block_exit);
        }
    }
    
    // Join threads to ensure all are completed before terminating
    pthread_join(exit_sensor, NULL);
    pthread_join(monitoring_thread, NULL);
    pthread_join(smoke_thread, NULL);
    pthread_join(gas_thread, NULL);

    // Clean up semaphores
    sem_destroy(&sem_entry);
    sem_destroy(&sem_exit);
    sem_destroy(&sem_monitoring);

    // Destroy mutexes
    pthread_mutex_destroy(&mutex_num_cars);
    pthread_mutex_destroy(&mutex_safety);
    pthread_mutex_destroy(&mutex_block_entry);
    pthread_mutex_destroy(&mutex_block_exit);
}
