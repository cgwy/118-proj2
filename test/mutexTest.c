#include<stdio.h>
#include<string.h>
#include<pthread.h>
#include<stdlib.h>
#include<unistd.h>

pthread_t tid[5];
int count;
pthread_mutex_t lock;

void* doSomeThing(void *arg)
{

	puts("\n========");
	for(int i=0;i<10;i++)
		printf("thread:%d  count: %d\n",*(int*)arg, ++count);
	sleep(1);

	return NULL;
}

int main(void)
{
    int i = 0;
    int err;
    
	if(pthread_mutex_init(&lock, NULL) != 0)
	{
			puts("failed to init mutex");
			return 1;
	}
    while(i < 5)
    {
		int ii = i;
		pthread_mutex_lock(&lock);
        err = pthread_create(&(tid[i]), NULL, &doSomeThing, (void*)&ii);
        if (err != 0)
            printf("\ncan't create thread :[%s]", strerror(err));
        i++;
		pthread_mutex_unlock(&lock);
    }
    
	int j=0;
	while(j<5)
	{
		pthread_join(tid[j++], NULL);
	} 
    
    return 0;
}
