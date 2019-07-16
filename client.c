#include<unistd.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netinet/in.h>
#include<arpa/inet.h>

int main(int argc,char *argv[])
{
	if(argc < 4)
	{
		printf("Please enter <filename> <IP address> <Port Number>");
		return 0;
	}

	char image[100];
	strcpy(image, argv[1]);
	int result;

	if(!(strstr(argv[1], ".png") || strstr(argv[1], ".jpeg") || strstr(argv[1], ".jpg") || strstr(argv[1], ".bmp")))
	{
		printf("Wrong file format\n");
		return 0;
	}

	int fd;
	char buf[512];
	FILE *f = NULL;
	char path[100] = "./Client/";
	char *dest_ip = argv[2];
	int dest_port = atoi(argv[3]);
	

	fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd<0)
	{
		perror("socket");
		return 0;
	}

	struct sockaddr_in serv_addr;
	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(dest_port);
	serv_addr.sin_addr.s_addr=inet_addr(dest_ip);

	
	if(connect(fd,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
	{
		perror("connect");
		return 0;
	}

	if(write(fd, image,(strlen(image) + 1)) < 0)
	{
		printf("Error occurred while writing");
		close(fd);
	}

	/* Sending image file to server */
	f = fopen(image, "rb");
	if(f == NULL)
	{
		printf("Could not open file\n");
		fclose(f);
	}

	while(!feof(f))
	{
		fread(buf,1,512,f);
		send(fd,buf,512,0);
		bzero(buf,sizeof(buf));
	}
	fclose(f);

	send(fd, "end", 512, 0);
	printf("File send successfully\n");

	strcat(path, image);

	/* Receiving greyscale image from server */
	f = fopen(path, "a+");
    if(f == NULL)
    {
        printf("Could not open file\n");
        fclose(f);
    }

    while(read(fd, buf, 512) > 0)
    {
    	if(strcmp(buf, "end") == 0)
                break;
    	fwrite(buf, 1, 512, f);
       	bzero(buf, sizeof(buf));
    }
    printf("File received successfully\n");
    fclose(f);

	close(fd);
	return 0;
}