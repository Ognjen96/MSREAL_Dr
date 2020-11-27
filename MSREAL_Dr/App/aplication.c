#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <string.h>
#include <stdint.h>

#define MMAP
#define MAX_BRAM_SIZE 2048*4
#define MAX_ED_SIZE 16384
#define MAX_KERNEL_SIZE 4096
#define ED_SEND 16
#define BLOCK_SIZE 2048*4

unsigned long long txt_length=0;
unsigned long long txt_length_res=0;
char txt[2048];
char mem_txt[2048];

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int checkPrime(int n) {
	int i;
	int m = n/2;

	for(i=2; i <=m; i++) {
		if (n % i ==0) {
			return 0;//not prime
		}
	}
	return 1;//
}

char citaj(void){

	size_t size = 0;
	char *buffer = NULL;	
	int i = 0;	
	int j = 0;

	FILE *fp = fopen("tekst", "r");
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	printf("velicina %d\n", size);
	rewind(fp);
	buffer = malloc((size+1) * sizeof(*buffer));
	fread(buffer, size, 1, fp);
	buffer[size] = '\0';
	printf("%s\n", buffer);	
	if (strlen(buffer)%2 == 1){
		txt_length = (strlen(buffer) / 2)+1;
		txt_length = 2*txt_length-1;
	}
	else
	{
		txt_length = strlen(buffer) /2;
		txt_length_res = 2*txt_length;
	}
		printf("Message to encrypt: %s\n", buffer);
	for (i = 0; i < (2*txt_length);i++){
		txt[i] = buffer[i];
		printf("txt[%d]: %d, ",i,txt[i]);
	}
	
	printf("\nEncryption test !!!\n");
	
	for(j = 0; j < txt_length; j++)
	{
		mem_txt[j] = (buffer[2*j] << 16) | buffer[2*j+1];
		printf("%d: %d, ", j, mem_txt[j]);
	}
	
}
	



int main(int argc, char *argv[])
{



	int text_array[2048];
	int p=0;
	int q=0;
	char *buffer = NULL;
	size_t size = 0;
	citaj();

/*	while(1) {
	
	printf("Unesite dva prosta broja key:");
	scanf("%d %d", &p, &q);

	if(!(checkPrime(p) && checkPrime(q)))
		printf("Nijedan od dva broja nije prost...\n");
        else if (!checkPrime(p))
		printf("Prvi broj koji ste uneli nije prost.");
	else if (!checkPrime(q))
		printf("Drugi broj koji ste uneli nije prost.");
	else
		break;
			
	}*/
	return 0;
}
