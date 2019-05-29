
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>


//defines
#define CUTOFF 5
#define QUEUE_SIZE 10
#define N 30
#define THREADS 4
#define WORK 0
#define FINISH 1
#define SHUTDOWN 2


pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;// thn 8etei o producer kai thn epe3ergazetai o consumer
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;//thn 8etei o consumer kai thn epe3ergazetai o producer

struct message // mhnuma 
{
    int type;//tupos mhnumatos
    int begin;//arxh tou tmhmatos tou pinaka
    int end;//telos tou tmhmatos tou pinaka
};

struct message mQueue[QUEUE_SIZE];//pinakas me mege8os QUEUE_SIZE
int qin=0,qout=0;//deixnoun sthn arxh ths ouras mhnumatwn. qin pou kanoume eisagwgh qout pou kanoume e3agwgh
int m_count=0;//message counter

void send(int type, int begin,int end){
    pthread_mutex_lock(&mutex);

    while (m_count>=QUEUE_SIZE){
            pthread_cond_wait(&msg_out,&mutex);//edw oso perimenei to signal kanei to mutex unlock apo mono tou !!!
    }
    mQueue[qin].type = type;
    mQueue[qin].begin = begin;
    mQueue[qin].end = end;
    qin++;
    if (qin >= QUEUE_SIZE){
        qin = 0;
    }
    m_count++;
    pthread_cond_signal(&msg_in);
    pthread_mutex_unlock(&mutex);
}

void receive(int *type, int *begin,int *end){
    pthread_mutex_lock(&mutex);

    while (m_count<=0){
        pthread_cond_wait(&msg_in,&mutex);//edw oso perimenei to signal kanei to mutex unlock apo mono tou !!!
    }
    *type = mQueue[qout].type;
    *begin  = mQueue[qout].begin;
    *end = mQueue[qout].end;
    qout++;
    if (qout >= QUEUE_SIZE){
        qout = 0;
    }
    m_count--;
    pthread_cond_signal(&msg_out);
    pthread_mutex_unlock(&mutex);

}

void inssort(double *a,int b,int e){
    int i,j;
    double temp;
    for (i=b+1;i<e;i++){
        j=i;
        while ((j>b) && (a[j-1]>a[j])){
            temp=a[j-1];
           a[j-1]=a[j];
            a[j]=temp;
            j--;
        }
    }
}


void quicksort(double *a,int b,int e){
    
    if ((e-b)<=CUTOFF){
        send(FINISH,0,0);
        inssort(a,b,e);
        return;
        
    }
    int first = b;
    int last = e;
    int middle = ((e-b)/2)+b;
    int n = e-b;
    printf("%d\n",n);
    int i,j;
    double temp = 0;
    
    if (a[first] >a [middle]){
        temp = a[first];
        a[first] = a[middle];
        a[middle] = temp;
    }
    if (a[middle] >a [last]){
        temp=a[middle];
        a[middle] = a[last];
        a[last] = temp; 
    }
    if (a[first] >a [middle]){
        temp = a[first];
        a[first] = a[middle];
        a[middle] = temp;
    }
    double pivot =  a[middle];

    for (i=1,j=n-2;;i++,j--){
        while (a[i]<pivot){
            i++;
        }
        while (a[j]>pivot){
            j--;
        }
        if (i>=j){
            break;
        }
        temp = a[i];
        a[i]=a[j];
        a[j]=temp;

    } 

    send(WORK,b,i);
    send(WORK,i+1,e);
    
        
    

}






struct thread_params
{
    double *a;
    int n;
};


void *thread_func(void *params){
    
    struct thread_params *tparams;
    tparams= (struct thread_params *)params;
    double *pa;
    int n;
    pa=tparams->a;
    n=tparams->n;
    
    int t=0,b=0,e=0;
    while (1){
        receive(&t,&b,&e);
        
        if (t == WORK){
            printf("Working...\n");
            
            quicksort(pa,b,e);
        
            
        }
        else if(t == SHUTDOWN){
            printf("Shutdown\n");
            send(SHUTDOWN,0,0);
            break;
        }
        printf("type  %d,begin %d,end %d\n",t,b,e);
        
    }
    pthread_exit(NULL);
}



int main() {
    double *a;
    pthread_t my_thread[THREADS];
    struct thread_params tparams[THREADS];
    printf("Allocating a\n");

    //Allocating memory
    a = (double *) malloc(sizeof(double) * N);
    if (!a) {
        exit(-1);
    }
    srand(time(NULL));
    //Initialising Array
    for(int i=0;i<N;i++){
        a[i] = (double)rand()/RAND_MAX;
    }
    int completed = 0;

       
    printf("Creating my threads\n");
    for(int i=0;i<THREADS;i++){

        pthread_create(&my_thread[i],NULL,&thread_func,&tparams[i]);
        tparams[i].a = a;
        tparams[i].n = N;
    }


    send(WORK,0,N/2);
    send(WORK,(N/2)+1,N);
    int type,begin,end;
    
    while (completed < (QUEUE_SIZE/2)){
        receive(&type,&begin,&end);
        if (type == FINISH){
            completed++;
        }
        else {
            send(type,begin,end);
        }
    }
    for (int i = 0 ;i<N;i++ ){
        printf("%f\n",a[i]);
    }
    
    send(SHUTDOWN,0,0);
    
     printf("Destoying my threads\n");
     for(int i=0;i<THREADS;i++){
         pthread_join(my_thread[i],NULL);
     }
     pthread_mutex_destroy(&mutex);
     pthread_cond_destroy(&msg_in);
     pthread_cond_destroy(&msg_out);



    return 0;
}
