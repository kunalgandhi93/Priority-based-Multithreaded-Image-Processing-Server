#include<unistd.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
#include<netinet/in.h>
#include<semaphore.h>
#include<pthread.h>
#include<sys/sysinfo.h>


#define BACKLOG 5
#define BUFFERSIZE ((get_nprocs_conf() == 1)? (1) : (get_nprocs_conf() - 1))

int serv_fd, in = 0, out = 0;
int *buff = NULL;
char path[100] = "./Server/";
char filename[100];
sem_t mutex, full, empty; 

void *producer(void *arg)
{
    int cli_fd;

    struct sockaddr_in cli_addr;
    socklen_t len=sizeof(cli_addr);

    while(1)
    {
        sem_wait(&empty);

        cli_fd = accept(serv_fd, (struct sockaddr*)&cli_addr, &len);
        if(cli_fd < 0)
        {
            perror("accept");
            return 0;
        }
        
        bzero(filename, sizeof(filename));
        read(cli_fd, filename, (100 * sizeof(char)));

        strcat(path, filename);
        
        sem_wait(&mutex);
        buff[in] = cli_fd;
        in = (in + 1) % BUFFERSIZE;
        sem_post(&mutex);

        sem_post(&full);
    }
}

void *consumer(void *arg)
{
    FILE *f = NULL;
    int stat, ret, cli_fd;
    char buf[512];

    while(1)
    {
        sem_wait(&full);

        printf("Thread Number %d\n", *(int *)arg);

        sem_wait(&mutex);
        cli_fd = buff[out];
        out = (out + 1) % BUFFERSIZE;
        sem_post(&mutex);

        /* Receive image file from client */
        f = fopen(path, "a+");
        if(f == NULL)
        {
            printf("Could not open file\n");
            fclose(f);
        }

        while(read(cli_fd, buf, 512) > 0)
        {
            if(strcmp(buf, "end") == 0)
                break;
            fwrite(buf, 1, 512, f);
            bzero(buf, sizeof(buf));
        }
        printf("File received successfully\n");
        fclose(f);

        /* Converting image into greyscale image */
        ret = fork();
        if(ret == 0)
        {
            execl("image.out","./image.out",filename,NULL);
        }
        else
        {
            wait(&stat);
            if (WIFEXITED(stat)) 
                printf("Exit status: %d\n", WEXITSTATUS(stat)); 
            else if (WIFSIGNALED(stat)) 
                psignal(WTERMSIG(stat), "Exit signal");

            /* Sending greyscale image to client */
            f = fopen(path, "rb");
            if(f == NULL)
            {
                printf("Could not open file\n");
                fclose(f);
            }

            while(!feof(f))
            {
                fread(buf,1,512,f);
                send(cli_fd,buf,512,0);
                bzero(buf,sizeof(buf));
            }
            send(cli_fd, "end", 512, 0);
            fclose(f);
            printf("File send successfully\n");

            close(cli_fd);

            sem_post(&empty);
        }
    }
}

int main(int argc, char* argv[])
{
	if(argc<2)
	{
		printf("Please specify <port number>\n");
		return 0;
	}
	
	int port = atoi(argv[1]);
    int max, min;
    struct sched_param sched_val1, sched_val2;
    pthread_attr_t t_attr;
    pthread_t t1, t2[BUFFERSIZE];

    buff = calloc(BUFFERSIZE, sizeof(int));

    serv_fd=socket(AF_INET,SOCK_STREAM,0);
    if(serv_fd<0)
   	{
        perror("socket");
        return 0;
    }

	struct sockaddr_in serv_addr;
    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family=AF_INET;
    serv_addr.sin_port=htons(port);
    serv_addr.sin_addr.s_addr=INADDR_ANY;

    if( bind(serv_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
   	{
        perror("bind");
        return 0;
    }

    if(listen(serv_fd, BACKLOG) < 0)
    {
        perror("listen");
        return 0;
    }

    sem_init(&mutex, 0, 1);
    sem_init(&full, 0, 0); 
    sem_init(&empty, 0, 7); 
    pthread_attr_init(&t_attr);
    pthread_attr_setschedpolicy(&t_attr, SCHED_OTHER);

    max = sched_get_priority_max(SCHED_OTHER);
    min = sched_get_priority_min(SCHED_OTHER);

    sched_val1.sched_priority = max;
    sched_val2.sched_priority = min;

    pthread_attr_setschedparam(&t_attr, &sched_val1);
    pthread_create(&t1, &t_attr, producer, NULL);
    pthread_attr_setschedparam(&t_attr, &sched_val2);

    for(int i = 0; i < BUFFERSIZE; i++)
    {
        int arg = i;
        pthread_create(&t2[i], &t_attr, consumer, &arg);
    }

    while(1);

	return 0;
}
