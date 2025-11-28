#define _XOPEN_SOURCE 500 //usleep için gerekliymiş.
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/time.h>
#include <errno.h>
#include <stdbool.h>

// ödevdeki sem/mutexler
sem_t sem_board;   
sem_t sem_full;    
sem_t sem_unboard; 
sem_t sem_empty;   
pthread_mutex_t mutex;
//anlık araç sayısı, capacity, fery yoldayken araba in bin engellemek için flag.
int cars_on_board = 0; 
const int FERRIES_CAPACITY = 5; 

pthread_mutex_t car_production_flag_mutex;
bool is_car_prod_active = true; 


// Zaman hesabi
struct timeval start_time;
double get_current_time() {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    double elapsed = (current_time.tv_sec-start_time.tv_sec)+(current_time.tv_usec - start_time.tv_usec)/1000000.0;
    return elapsed;
}

void* car_thread(void* arg){
    int car_id = *((int*)arg);
    free(arg); 
    // binme iznini bekle
    sem_wait(&sem_board); 
    //izin geldi ozaman bin.
    pthread_mutex_lock(&mutex);
    //mutexle koruyarak araba sayısını artır.
    cars_on_board++;
    printf("[Clock: %.4f] Car %d entered the ferry\n", get_current_time(), car_id);
    
    // son araba ise doldu ferry threade doldu sinyali gönder
    if (cars_on_board == FERRIES_CAPACITY) {
        sem_post(&sem_full); 
    }
    pthread_mutex_unlock(&mutex);//artık +- serbest
    //Artık binildi. inmek için izin bekle.
    sem_wait(&sem_unboard); 
    //mutexle koruyarak araba sayısını azalt.
    pthread_mutex_lock(&mutex);
    cars_on_board--;
    printf("[Clock: %.4f] Car %d left the ferry\n", get_current_time(), car_id);

    // Son inensen boş sinyali postla
    if (cars_on_board == 0) {
        sem_post(&sem_empty); 
    }
    pthread_mutex_unlock(&mutex);//artık +- serbest
    return NULL;
}

void* ferry_thread(void* arg) {
    (void)arg;
    while (1) { //main 60 saniyede terminate edilene kadar çalış
        // kapasite kadar biniş izni
        for (int i = 0; i < FERRIES_CAPACITY; i++) {
            sem_post(&sem_board);
        }

        // son arabayı bekle, gelince kalk ve araba +-'yi flagla durdur. 3 saniye seyahati bekle vardı, logunu bas.
        sem_wait(&sem_full);
        printf("[Clock: %.4f] Ferry leaves the dock\n", get_current_time());
        
        pthread_mutex_lock(&car_production_flag_mutex);
        is_car_prod_active = false;
        pthread_mutex_unlock(&car_production_flag_mutex);
        
        sleep(3);
        printf("[Clock: %.4f] Ferry arrives to new dock\n", get_current_time());

        //ferrydeki arabalara iniş izni ver. Boşalmalarını bekle. Araç +- flagini geri aç.
        for (int i = 0; i<FERRIES_CAPACITY;i++) {
            sem_post(&sem_unboard);
        }
        sem_wait(&sem_empty);
        
        pthread_mutex_lock(&car_production_flag_mutex);
        is_car_prod_active = true; 
        pthread_mutex_unlock(&car_production_flag_mutex);
        
        //Her şeyi main thread terminate edilene kadar tekrarla.
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    gettimeofday(&start_time, NULL);
    // sem ve mutexleri initiate et hata çıkarsa fail çık.
    if (sem_init(&sem_board, 0, 0) != 0) { perror("sem_init board error"); exit(1); }
    if (sem_init(&sem_full, 0, 0) != 0) { perror("sem_init full error"); exit(1); }
    if (sem_init(&sem_unboard, 0, 0) != 0) { perror("sem_init unboard error"); exit(1); }
    if (sem_init(&sem_empty, 0, 0) != 0) { perror("sem_init empty error"); exit(1); }
    if (pthread_mutex_init(&mutex, NULL) != 0) { perror("mutex_init error"); exit(1); }
    if (pthread_mutex_init(&car_production_flag_mutex, NULL) != 0) { 
        perror("flag mutex init error"); exit(1); 
    }
    // Ferry thread inf loopunu başlat(mainden çıkana kadar çalışacak.)
    pthread_t ferry;
    int err = pthread_create(&ferry, NULL, ferry_thread, NULL);
    if (err != 0) {
        perror("Creating ferry thread error");
        exit(EXIT_FAILURE);
    }
    if (err == 0) {
    	// Ferry threadi ayırıyorum çünkü sonsuz döngü içerdiği için wait diyemeyiz. Bu yüzden detach edip main sonunda cancel ile thread kontrolü sağladım.
    	if (pthread_detach(ferry) != 0) {
        	perror("pthread_detach ferry error");
    	}
    }
    // car geliş simülasyonu 
    int car_id_counter = 0;
    while (get_current_time() < 60.0){
    	bool prod_active_stat;
        pthread_mutex_lock(&car_production_flag_mutex);
        prod_active_stat = is_car_prod_active;//Thread safe flage dönüştürdm.
        pthread_mutex_unlock(&car_production_flag_mutex);
        if (prod_active_stat) {
            int sleep_usec = rand()%1001*1000; // 0-1 sn arasi beklemek için
            usleep(sleep_usec);
            if (get_current_time() >= 60.0){ // tam 59.9 gibi bişeyde 1 saniye bekleme denk gelirse araba 60+ bir saniyede üretilebilir bunu engellemek için buraya ek bir kontrol ekledim.
    		break;
	    }
            pthread_mutex_lock(&car_production_flag_mutex);
            prod_active_stat = is_car_prod_active;
            pthread_mutex_unlock(&car_production_flag_mutex);
            if (prod_active_stat) { // hareket halinde değilsek car oluştur
                pthread_t car;
                int* id = malloc(sizeof(int));
                if (id == NULL) { perror("malloc error"); exit(1); }
                *id = car_id_counter++;
                err = pthread_create(&car, NULL, car_thread, id);
                if (err != 0) {
                    perror("Creating car thread error");
                    free(id);
                } else {
                    pthread_detach(car); // Main thread car'ı beklemesin diye ayırdım.
                }
            }
        } else {
            usleep(10000); // CPU relaxing
        }
    }

    // Vapur thread'ini sonsuz döngüden çıkarmak zor olduğu için main bitince process bitecek
    pthread_cancel(ferry);
    sem_destroy(&sem_board);
    sem_destroy(&sem_full);
    sem_destroy(&sem_unboard);
    sem_destroy(&sem_empty);
    
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&car_production_flag_mutex);

    return 0;
}
