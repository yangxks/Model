#include <stdio.h>
#include <pthread.h>
#define N 10
int n = 100;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void *thread(void *arg)
{
    int who = *((int *)arg);
    pthread_mutex_lock(&mutex);
        /*
    asm("pushl");
    asm("movl n, %eax");
    asm("dec %eax");
    asm("pushl");
    sleep(1);
    asm("popl");
    asm("movl %eax, n");
    asm("popl");
        */
        /*who++;
    sleep(1);
    n--;*/
    printf("%d: n = %d\n", who, n);
    pthread_mutex_unlock(&mutex);
    
    return NULL;
}

void *thread11(void *arg)
{
    char str[] = "curl -d \"type=ads&json={\\\"ip\\\": \\\"8.8.8.8\\\",\\\"size\\\": \\\"180*180\\\",\\\"ua\\\": \\\"Mozilla/5.0 (iPhone; CPU iPhone OS 9_3_4 like Mac OS X) AppleWebKit/601.1.46 (KHTML, like Gecko) Version/9.0 Mobile/13G35 Safari/601.1\\\",\\\"type\\\": \\\"banner\\\",\\\"count\\\": 1}\" http://180.76.141.191:12345/advertising?";
    system(str);
    
    return NULL;
}

int main()
{
    int i, a[n];
    pthread_t tid[N];

    for(i=0; i < N; i++)
        a[i] = i;
    
    for(i=0; i < N; i++)
        pthread_create(&tid[i], NULL, thread11, (void*)&a[i]);

    for(i=0; i < N; i++)
        pthread_join(tid[i], NULL);
    
    return 0;
}
