#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>


int nr_masini = 0; // nr. masini
int siguranta = 1; // 1 - tunel sigur, 0, nu e sigur
int gaz_detectat = 0;  // 0 - nu este detectat gaz, 1 - detectat
int blocare_intrare = 0; // 0 - permite intrarea, 1 - nu permite intrarea
int blocare_iesire = 0; // 0 - permite iesirea, 1 - nu permite intrarea

// Declarare semafoare, mutexuri si variabile conditionale
sem_t semafor_intrare, semafor_iesire, semafor_monitorizare;
pthread_mutex_t mutex_nr_m, mutex_siguranta, mutex_blocare_intrare, mutex_blocare_iesire;
pthread_cond_t condvar_intrare, condvar_iesire;

//Thread-ul asociat task-ului intrare
void *intrare(void *arg) {
    pthread_mutex_lock(&mutex_blocare_intrare);
    // Daca intrarea este blocata, thread-ul asteapta pana cand se deblocheaza
    while (blocare_intrare) {
        // Elibereaza thread-ul cand este notificat de ondvar_intrare
        pthread_cond_wait(&condvar_intrare, &mutex_blocare_intrare);
    }
    pthread_mutex_unlock(&mutex_blocare_intrare);
    
    pthread_mutex_lock(&mutex_blocare_iesire);
        while (blocare_iesire)
            //Daca iesirea este blocata, thread-ul asteapta pana cand se deblocheaza
            pthread_cond_wait(&condvar_iesire, &mutex_blocare_iesire);
        pthread_mutex_unlock(&mutex_blocare_iesire);

    // Se asteapta permiterea intrarii in tunel ( nr_masini )
    sem_wait(&semafor_intrare);
    pthread_mutex_lock(&mutex_nr_m);
    pthread_mutex_lock(&mutex_siguranta);
    
    if (nr_masini < 5 && siguranta) {
        nr_masini++;
        printf("A intrat o masina: %d masini in tunel\n", nr_masini);
        // Notifica 'monitorizarea' ca a intrat o masina in tunel
        sem_post(&semafor_monitorizare);
    }
    pthread_mutex_unlock(&mutex_siguranta);
    pthread_mutex_unlock(&mutex_nr_m);
    // Notifica 'iesirea' ca a intrat o masina in tunel
    sem_post(&semafor_iesire);
    return NULL;
}

//Thread-ul asociat task-ului iesire
void *iesire(void *arg) {
    while (1) {
        pthread_mutex_lock(&mutex_blocare_iesire);
        while (blocare_iesire)
            //Daca iesirea este blocata, thread-ul asteapta pana cand se deblocheaza
            pthread_cond_wait(&condvar_iesire, &mutex_blocare_iesire);
        pthread_mutex_unlock(&mutex_blocare_iesire);
        // Se asteapta permiterea iesirii din tunel
        sem_wait(&semafor_iesire);
        pthread_mutex_lock(&mutex_nr_m);
        if (nr_masini > 0) {
            nr_masini--;
            printf("A iesit o masina: %d masini in tunel\n", nr_masini);
            //  Notifica 'monitorizarea' ca a iesit o masina din tunel
            sem_post(&semafor_monitorizare);
            if (nr_masini == 0) 
            {  // Daca toate masinile au iesit din tunel
               // transmite o notificare  thread-urilor care asteapta condvar_iesire
                pthread_cond_broadcast(&condvar_iesire); 
            }
        }
        pthread_mutex_unlock(&mutex_nr_m);
        // Notifica 'iesirea' ca a iesit o masina
        // si ca e loc in tunel ca sa intre alta masina
        sem_post(&semafor_intrare);
        sleep(3);
    }
    return NULL;
}

//Thread-ul asociat task-ului monitorizare
void *monitorizare(void *arg) {
    while (1) {
        // Asteapta sa intre/ iasa o masina 
        sem_wait(&semafor_monitorizare);
        pthread_mutex_lock(&mutex_nr_m);
        if (nr_masini >= 5) {
            printf("Limita de 5 masini atinsa! Intrare blocata.\n");
            // Blocheaza intrarea masinilor
            sem_wait(&semafor_intrare);
        } else {
            // Permite intrarea masinilor
            sem_post(&semafor_intrare); 
        }
        pthread_mutex_unlock(&mutex_nr_m);
    }
    return NULL;
}

//Thread-ul asociat task-ului fum
void *fum(void *arg) {
    while (1) {
        //Generare eveniment 
        int detectare_fum = rand() % 10;
        pthread_mutex_lock(&mutex_siguranta);
        if (detectare_fum < 1 && siguranta) {
            siguranta = 0;
            printf("Fum detectat! Intrare blocata.\n");
            //Blocheaza intrarea masinilor
            sem_wait(&semafor_intrare);
            // Deblocare iesire daca este blocata
            pthread_mutex_lock(&mutex_blocare_iesire);
            if (blocare_iesire) {
                blocare_iesire = 0;
                pthread_cond_broadcast(&condvar_iesire);
                printf("Iesirea deblocata.\n");
            }
            pthread_mutex_unlock(&mutex_blocare_iesire);
        } else if (!gaz_detectat && !siguranta) {
            siguranta = 1;
            printf("Nu se mai detecteaza fum. Intrare deblocata.\n");
            // Permite intrarea masinilor
            sem_post(&semafor_intrare);
        }
        pthread_mutex_unlock(&mutex_siguranta);
        sleep(1);
    }
}

//Thread-ul asociat task-ului gaz
void *gaz(void *arg) {
    while (1) {
        //Generare eveniment 
        int detectare_gaz = rand() % 10;
        pthread_mutex_lock(&mutex_siguranta);
        if (detectare_gaz < 1 && !gaz_detectat) {
            gaz_detectat = 1;
            siguranta = 0;
            printf("Gaz detectat! Intrare blocata.\n");
            //Blocheaza intrarea masinilor
            sem_wait(&semafor_intrare);
            
            // Deblocare iesire daca este blocata
            pthread_mutex_lock(&mutex_blocare_iesire);
            if (blocare_iesire) {
                blocare_iesire = 0;
                pthread_cond_broadcast(&condvar_iesire);
                printf("Iesirea deblocata.\n");
            }
            pthread_mutex_unlock(&mutex_blocare_iesire);
        } else if (gaz_detectat) {
            gaz_detectat = 0;
            if (siguranta == 0) {
                siguranta = 1;
                printf("Nu se mai detecteaza gaz. Intrare deblocata\n");
                // Permite intrarea masinilor
                sem_post(&semafor_intrare);
            }
        }
        pthread_mutex_unlock(&mutex_siguranta);
        sleep(1);
    }
}

//Thread-ul asociat task-ului buton_panica
void *buton_de_panica(void *arg) {
    clock_t start_time = clock();
    // Intervalul de timp la care se apasa butonul de panica
    int interval = 60000;

    while (1) {
        clock_t current_time = clock();
        // Verifica daca a expirat timpul si daca sunt masini in tunel
        if (((double)(current_time - start_time) / CLOCKS_PER_SEC) * 1000 >= interval && nr_masini >0) {
            // Blocheaza intrarea 
            pthread_mutex_lock(&mutex_blocare_intrare);
            blocare_intrare = 1;
            printf("Buton de panică a fost apasat. Intrare blocata.\n");
            pthread_mutex_unlock(&mutex_blocare_intrare);

            // Deblocare iesire daca este blocata
            pthread_mutex_lock(&mutex_blocare_iesire);
            if (blocare_iesire) {
                blocare_iesire = 0;
                pthread_cond_broadcast(&condvar_iesire);
                printf("Iesirea deblocata.\n");
            }
            pthread_mutex_unlock(&mutex_blocare_iesire);

            pthread_mutex_lock(&mutex_nr_m);
            while (nr_masini > 0) {
                // Deblocare iesire
                pthread_cond_wait(&condvar_iesire, &mutex_nr_m);
            }
            printf("Toate masinile au iesit din tunel.\n");
            pthread_mutex_unlock(&mutex_nr_m);

            // Resetare timer
            start_time = clock();
            // Deblocare intrare
            pthread_mutex_lock(&mutex_blocare_intrare);
            blocare_intrare = 0;
            pthread_mutex_unlock(&mutex_blocare_intrare);
        }
    }
}

int main() {
    srand(time(NULL));

    // Declarare si initializare thread-uri
    pthread_t senzor_iesire, thread_monitorizare, senzor_intrare, thread_fum, thread_gaz, thread_buton;
    pthread_mutex_init(&mutex_nr_m, NULL);
    pthread_mutex_init(&mutex_siguranta, NULL);
    pthread_mutex_init(&mutex_blocare_intrare, NULL);
    pthread_mutex_init(&mutex_blocare_iesire, NULL);
    
    // Initializare variabile conditionale
    pthread_cond_init(&condvar_intrare, NULL);
    pthread_cond_init(&condvar_iesire, NULL);

    // Initializare semafoare
    if( sem_init(&semafor_intrare, 0, 5) == -1 ){
        printf("Eroare la initializare: ");
    }

    if( sem_init(&semafor_iesire, 0, 0) == -1 ){
        printf("Eroare la initializare: ");
    }

    if( sem_init(&semafor_monitorizare, 0, 0) == -1 ){
        printf("Eroare la initializare: ");
    }

    pthread_create(&thread_buton, NULL, buton_de_panica, NULL);
    pthread_create(&thread_fum, NULL, fum, NULL);
    pthread_create(&thread_gaz, NULL, gaz, NULL);
    pthread_create(&senzor_iesire, NULL, iesire, NULL);
    pthread_create(&thread_monitorizare, NULL, monitorizare, NULL);

    printf("Apasa 1 pentru a intra o masina, 2 pentru a bloca/debloca intrarea, 3 pentru a bloca/debloca iesirea.\n");
    int option;
    while (1) {
        scanf("%d", &option);
        if (option == 1) {
            pthread_create(&senzor_intrare, NULL, intrare, NULL);
            pthread_detach(senzor_intrare);
        }else if (option == 2) {
            pthread_mutex_lock(&mutex_blocare_intrare);
            blocare_intrare = !blocare_intrare;
            if (blocare_intrare) {
                printf("Intrarea a fost blocata.\n");
                
            } else {
                printf("Intrarea a fost deblocata.\n");
                pthread_cond_broadcast(&condvar_intrare);
            }
            pthread_mutex_unlock(&mutex_blocare_intrare);
        } else if (option == 3) {
            pthread_mutex_lock(&mutex_blocare_iesire);
            blocare_iesire = !blocare_iesire;
            if (blocare_iesire) {
                printf("Iesirea a fost blocata.\n");
            } else {
                printf("Iesirea a fost deblocata.\n");
                pthread_cond_broadcast(&condvar_iesire);
            }
            pthread_mutex_unlock(&mutex_blocare_iesire);
        }
    }
    
    pthread_join(senzor_iesire, NULL);
    pthread_join(thread_monitorizare, NULL);
    pthread_join(thread_fum, NULL);
    pthread_join(thread_gaz, NULL);

    sem_destroy(&semafor_intrare);
    sem_destroy(&semafor_iesire);
    sem_destroy(&semafor_monitorizare);

    pthread_mutex_destroy(&mutex_nr_m);
    pthread_mutex_destroy(&mutex_siguranta);
    pthread_mutex_destroy(&mutex_blocare_intrare);
    pthread_mutex_destroy(&mutex_blocare_iesire);
}

