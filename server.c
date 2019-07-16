#include<unistd.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<string.h>
#include<netinet/in.h>
#include<semaphore.h> 

#define BACKLOG 5

int main(int argc, char* argv[])
{
	if(argc<2)
	{
		printf("Please specify <port number>\n");
		return 0;
	}
	
	int port = atoi(argv[1]);
	int serv_fd,cli_fd,ret;
	int stat;
	FILE *f = NULL;
	char buf[512];
	char filename[100];
    char path[100] = "./Server/";


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

    struct sockaddr_in cli_addr;
    socklen_t len=sizeof(cli_addr);
	while(1)
	{
       	cli_fd = accept(serv_fd, (struct sockaddr*)&cli_addr, &len);
		if(cli_fd < 0)
       	{
        	perror("accept");
            return 0;
       	}
		
		read(cli_fd, filename, (100 * sizeof(char)));
        printf("Filename is %s\n",filename);

        strcat(path, filename);
		
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
        printf("File received sucesfully\n");
        fclose(f);


        ret = fork();
        if(ret == 0)
        {
        	//execl("image.out","./image.out",filename,NULL);
            execl("display","./display",filename,NULL);
        }
        else
        {
            wait(&stat);
            if (WIFEXITED(stat)) 
                printf("Exit status: %d\n", WEXITSTATUS(stat)); 
            else if (WIFSIGNALED(stat)) 
                psignal(WTERMSIG(stat), "Exit signal"); 

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
            printf("File send sucesfully\n");

            close(cli_fd);
        //    close(serv_fd);
        }
	}
	return 0;
}
