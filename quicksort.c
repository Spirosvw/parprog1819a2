
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>


//defines
#define CUTOFF 5//Below this threshold the program uses the function inssort to sort the partition of the array
#define N 50000// the size of the message array
#define ARRAY_SIZE 100000// the size of the array to be sorted
#define THREADS 4//the number of the threads to be used
#define WORK 0// This is a message type. This implements that the thread must either split the array partition into two new partitions or it must use inssort to sort the partition
#define FINISH 1// This is a message type. This implements that this partition of the array is sorted 
#define SHUTDOWN 2// This is a message type. This implements that the array is sorted successfully and the program can close


pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;// Initialising the mutex 
pthread_cond_t msg_in = PTHREAD_COND_INITIALIZER;// it is manipulated by send function and used by receive function
pthread_cond_t msg_out = PTHREAD_COND_INITIALIZER;// it is manipulated by receive function and used by send function

struct message // The message struct
{
    int type;//type of message
    int begin;//points to the beginning of the partition of the array
    int end;//points to the end of the partition of the array
};

struct message mQueue[N];//creates the message array
int qin=0,qout=0;//both point at the beginning of the message array. qin points where to store message and qout from where to export message
int m_count=0;//message counter


/*
This function is used by the threads to store messages on the message array
It uses three input arguements. The type of the message to store, the begin of the partition of the array 
and the end of the partition of the array
*/
void send(int type, int begin,int end){
    pthread_mutex_lock(&mutex);//locks the mutex so that the other threads won't use it 

    while (m_count>=N){
            pthread_cond_wait(&msg_out,&mutex);//while it waits for the signal, it unlocks the mutex 
    }
    mQueue[qin].type = type;//store type
    mQueue[qin].begin = begin;//store beginning
    mQueue[qin].end = end;//store end
    qin++;//points qin to the next array cell
    if (qin >= N){//when qin reaches the end of the message array it returns to the beginning
        qin = 0;
    }
    m_count++;// increase message counter by one
    pthread_cond_signal(&msg_in);//signal msg_in
    pthread_mutex_unlock(&mutex);//unlock mutex
}


/*
This function is used by the threads to export messages from the message array
It uses three input arguements. The type of the message that was stored, the begin of the partition of the array 
and the end of the partition of the array
*/
void receive(int *type, int *begin,int *end){
    pthread_mutex_lock(&mutex);//lock mutex

    while (m_count<=0){
        pthread_cond_wait(&msg_in,&mutex);//while it waits for the signal, it unlocks the mutex 
    }
    *type = mQueue[qout].type;// pointer set to the message array cell that type is stored
    *begin  = mQueue[qout].begin;// pointer set to the message array cell that beginning is stored
    *end = mQueue[qout].end;// pointer set to the message array cell that end is stored
    qout++;//points qout to the next array cell
    if (qout >= N){//when qout reaches the end of the message array it returns to the beginning
        qout = 0;
    }
    m_count--;// decrease message counter by one
    pthread_cond_signal(&msg_out);//signal msg_out
    pthread_mutex_unlock(&mutex);//unlock mutex

}

/*
This function is used when the size of a partition is smaller than the threshold CUTOFF
It uses three input arguements. The pointer to the array to be sorted, the begin of the partition of the array 
and the end of the partition of the array.
It sorts that partition using insertion sorting
*/

void inssort(double *a,int b,int e){//
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

/*
This function splits the partition of the array into two new partitions or it uses inssort to sort it
It uses three input arguements. The pointer to the array to be sorted, the begin of the partition of the array 
and the end of the partition of the array
*/
void quicksort(double *a,int b,int e){
    
    if ((e-b)<=CUTOFF){
        
        inssort(a,b,e);
        send(FINISH,b,e);// afte the insertion sort is completed the thread sends a message that the sorting in this partition is finished
        return;
        
    }

    //The partitioning is executed Below
    int first = b;
    int last = e-1;
    int middle = (e+b)/2;
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
    double pivot =  a[middle];// the pivot used by quicksort

    for (i=b+1,j=e-2;;i++,j--){
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
    // i is the splitting point of the partition

    //the thread sends to the message array two new messages
    //the partition is split to two smaller partitions 
    send(WORK,b,i);
    send(WORK,i,e);
    
        
    

}





//this struct consists of an array of double and its length. These parameters are part of every thread
struct thread_params
{
    double *a;
    int n;
};


/*
This is the main function run by every thread
It runs repeatedly until the threads are told to Shutdown by the main function
*/
void *thread_func(void *params){
    
    struct thread_params *tparams;//pointer of the parameters of the thread
    tparams= (struct thread_params *)params;//the parameters of the thread
    double *pa;//the pointer of the array to be sorted
    int n;//its length
    pa=tparams->a;
    n=tparams->n;
    
    int t=0,b=0,e=0;
    while (1){
        receive(&t,&b,&e);// exports a message from the message array
        
        if (t == WORK){// if the type was WORK then the thread uses the quicksort function with the pointer of the array pa, the beginning of the partition b and the end e.
            printf("Working...\n");
            
            quicksort(pa,b,e);
        
            
        }
        else if(t == SHUTDOWN){// if the type was SHUTDOWN it breaks out of the loop and the function comes to an end
            printf("Shutdown\n");
            send(SHUTDOWN,0,0);
            break;
        }
        else if(t == FINISH){// if the type was FINISH the thread sends it back to the message array because it is not useful here
            printf("Finish\n");
            send(FINISH,b,e);
        }
        printf("type  %d,begin %d,end %d\n",t,b,e);
        
    }
    pthread_exit(NULL);
}



int main() {
    double *a; // array to be sorted
    pthread_t my_thread[THREADS];// array of threads
    struct thread_params tparams[THREADS];// array of thread parameters
    printf("Allocating a\n");

    //Allocating memory
    a = (double *) malloc(sizeof(double) * ARRAY_SIZE);
    if (!a) {
        exit(-1);
    }
    srand(time(NULL));
    //Initialising Array
    for(int i=0;i<ARRAY_SIZE;i++){
        a[i] = (double)rand()/RAND_MAX;
    }

    int completed = 0;// when completed is equal to the size of the array to be sorted, main will send the SHUTDOWN message because the sorting is done successfully
    //creating threads       
    printf("Creating my threads\n");
    for(int i=0;i<THREADS;i++){

        pthread_create(&my_thread[i],NULL,&thread_func,&tparams[i]);
        tparams[i].a = a;
        tparams[i].n = ARRAY_SIZE;
    }

    //sends the first message to the message array to start the sorting
    send(WORK,0,ARRAY_SIZE);
    //type is the message type,begin is the begin of the partition and end is the end of the partition
    int type,begin,end;
    
    while (completed < ARRAY_SIZE){
        receive(&type,&begin,&end);// exports a message from the message array
        if (type == FINISH){// if the type is FINISH, the size of the sorted partition is added to the completed variable

            completed += end - begin;
            printf("%d\n",completed);
        }
        else {//otherwise it sends it back to the message array because it is not useful
            send(type,begin,end);
        }
    }

    int ok = 0 ;// if the sorting is done correctly this variable will stay at 0
    for (int i = 0 ;i<ARRAY_SIZE-1;i++ ){// go through the array to chech that it is sorted
        if (a[i]>a[i+1]){
            ok = 1;
        }
    }
    printf("%d\n",ok); // see f the sorting was ok 
    send(SHUTDOWN,0,0); //  sends the SHUTDOWN message so that the thread_func comes to an end
    
     printf("Destoying my threads\n");// destroys all the threads since the sorting is done
     for(int i=0;i<THREADS;i++){
         pthread_join(my_thread[i],NULL);
     }
     pthread_mutex_destroy(&mutex);//destroys mutex
     pthread_cond_destroy(&msg_in);//destroys msg_in
     pthread_cond_destroy(&msg_out);//destroys msg_out



    return 0;
}
