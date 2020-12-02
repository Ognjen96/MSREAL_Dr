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
#include <math.h>
#include <time.h>
#include <unistd.h>

#define MMAP
#define MAX_BRAM_SIZE 2048*4
#define MAX_ED_SIZE 16384
#define MAX_KERNEL_SIZE 2048
#define ED_SEND 36
#define BLOCK_SIZE 2048

int prost = 0, p = 0, q = 0;

static unsigned int txt_length=0;
static unsigned int txt_length_res=0;
static char txt[2048];

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

int checkPrime(int n) {
	int i=0;
	int j=0;

	for(i=1; i <=n; i++) {
		if ((n % i) ==0) {
			j++;
		}
	}
	if(j==2){
		prost=1;
	}else{
		prost = 0;
	}
	
}

void delay(int time)
{
	long pause;
	clock_t time1, time2;
	pause = time;
	(CLOCKS_PER_SEC/1000);

	time2 = time1 = clock();
	while((time1-time2)<pause)
	{
		time1 = clock();
	}
}

void prosti(){
		
	LOOP:
	printf("Unesite prost broj p: ");
	scanf("%d", &p);
	checkPrime(p);
	if(prost==0){
		printf("Broj nije prost.\n");
		prost=1;
		goto LOOP;
	}
	LOOP1:
	printf("Unesite prost broj q: ");
	scanf("%d", &q);
	checkPrime(q);
	if(prost==0){
		printf("Broj nije prost.\n");
		prost = 1;
		goto LOOP1;
	}
}



int main(int argc, char *argv[])
{

	FILE *fs;
	int *encrypted_txt_array, *decrypted_txt_array;
	int txt_array[2048];
	int fk, fc, fy, fb;
	int *k, *c, *y, *g, *kk;

	prosti();


//*****************************************gcd************************************************************//

int gcd(int n1, int n2){
	while(n1!=n2)
	{
		if(n1>n2)
			n1-=n2;
		else
			n2-=n1;
	}
}
//*********************************************************************************************************//

//*****************************ovde izracuvavam potreban e_key, public i private*********************************************//

int i1, Phi,x, Pk, e, r, d, h;

	Pk=p*q;
	Phi=(p-1)*(q-1);

	x=2;
	e=1;

	while(x>1){
		e=e+1;
		x=gcd(Phi, e);
	}
	
	i1=1;
	r=1;
	while(r>0){
		h=(Phi*i1)+1;
		r=h%e;
		i1=i1+1;
	}
	
	d=h/e;
	
	printf("\nVrednost N: %d \n", Pk);
	printf("Vrednost public key-a e: %d \n", e);
	printf("Vrednost Phi: %d \n", Phi);
	printf("Vrednost private key-a d: %d \n", d);
	



//*****************************************reading from .txt and storing data in array**********************//

	size_t size = 0;
	char *buffer = NULL;
	int *buffer1 = NULL;	
	int i = 0;	
	int j = 0;
	int buffer_size = 0;
	
	FILE *fp = fopen("tekst", "r");
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	printf("velicina %d\n", size);
	rewind(fp);
	buffer = malloc((size+1) * sizeof(*buffer));
	buffer1 = (int *) malloc(2048 * sizeof(int));
	fread(buffer, size, 1, fp);
	buffer[size] = '\0';
	printf("%s\n", buffer);	
	if (strlen(buffer)%2 == 1){
		txt_length = (strlen(buffer) / 2);
		txt_length_res = 2*txt_length-1;
	}
	else
	{
		txt_length = strlen(buffer) / 2;
		txt_length_res = 2*txt_length;
	}
		printf("Duzina teksta: %d\n", txt_length);
		printf("Message to encrypt: %s\n", buffer);
	for (i = 0; i < (2*txt_length);i++){
		txt[i] = buffer[i];
//		printf("txt[%d]: %d, ",i,txt[i]);
	}
	
	printf("\nEncryption test !!!\n");
	
	for(j = 0; j < txt_length; j++)
	{
		buffer1[j] = (buffer[2*j] << 16) | buffer[2*j+1];
//		printf("%d: %d", j, buffer1[j]);
//		printf("\n");
	}
	
//**************************************************************************************************************//
char reset[] = "reset";
fs = fopen("/dev/Enc_dec", "w");
if (fs == NULL)
{
	printf("Cannot open /dev/Enc_dec for write\n");
	return -1;
}
//fprintf(fs, "%s = %d", start_enc, 1);
//fflush(fs);
fprintf(fs, "%s = %d",reset, 1);
fflush(fs);
fprintf(fs, "%s = %d",reset, 0);
fflush(fs);
//fprintf(fs, "%s = %d", start_enc, 0);
fclose(fs);

//*************************************sending data to bram0***************************************************//

fk = open("/dev/bram0", O_RDWR|O_NDELAY);
	if (fk<0)
	{
		printf("Cannot open /dev/bram0 for write\n");
		return -1;
	}
k=(int*)mmap(0, MAX_KERNEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fk, 0);
		if(k == NULL) {
			printf("\ncouldn't mmap\n");
			return 0;
		}
	memcpy(k, buffer1, BLOCK_SIZE);
	munmap(k, BLOCK_SIZE);
	printf("bram0 done\n");
	close(fk);
	if(fk < 0)
	{
		printf("cannot close /dev/bram0 for write\n");
		return -1;
	}

//************************************************************************************************************************//
//*************************************sending keys and length to IP**************************************//

int ED_reg[4]={e,d,Pk,txt_length};
		
	fc = open("/dev/Enc_dec", O_RDWR|O_NDELAY);
	if (c<0)
	{
		printf("Cannot open /dev/Enc_dec for write\n");
		return -1;
	}
	c=(int*)mmap(0, MAX_ED_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fc, 0);
	if (c == NULL) {
		printf("\ncouldn't mmap\n");
		return 0;
	}
	memcpy(c, ED_reg, ED_SEND);
	munmap(c, ED_SEND);
	printf("IP done\n");
	close(fc);
	if(fc<0)
	{
		printf("Cannot close /dev/Enc_dec for write\n");
		return -1;
	}

//************************************************************************************************************//
//***************************************pokusaj preko fprintf***********************************************//
char start[] = "start";
char start_enc[] = "start_enc";
fs = fopen("/dev/Enc_dec", "w");
if (fs == NULL)
{
	printf("Cannot open /dev/Enc_dec for write\n");
	return -1;
}
fprintf(fs, "%s = %d", start_enc, 1);
fflush(fs);
fprintf(fs, "%s = %d", start, 1);
fflush(fs);
fprintf(fs, "%s = %d", start, 0);
fflush(fs);
fprintf(fs, "%s = %d", start_enc, 0);
fclose(fs);
//************************************************************************************************************//

//*************************************writing from bram1 to enc_txt**********************************************//
fb = open("/dev/bram1", O_RDWR|O_NDELAY);
	if(fb < 0)
	{
		printf("Cannot open /dev/bram1 for write\n");
		return -1;
	}
	encrypted_txt_array = (int*) malloc(MAX_BRAM_SIZE);
	g=(int*)mmap(0, MAX_BRAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	if(g == NULL) {
		printf("\ncouldnt mmap\n");
		return 0;
	}
	
	memcpy(encrypted_txt_array, g, MAX_BRAM_SIZE);
	munmap(g, MAX_BRAM_SIZE);
	printf("bram1 done\n");
	close(fb);
	if(fb < 0)
	{
		printf("cannot close /dev/bram1 for write\n");
		return -1;
	}
//************************************************************************************************************//
//*************************************writing encrypted data to file****************************************//
FILE *fm;
	fm = fopen("encrypted.txt", "w");
	if(fm==NULL)
	{
		printf("cannot open encrypted.txt\n");
		return -1;
	}

	printf("Otvoren fajl.\n");
	for(i = 0;i<=(txt_length*2)-1; i++)
	{
		fprintf(fm, "%d ", encrypted_txt_array[i]);
		fflush(fm);
	
	}
	fprintf(fm, "\n");
	printf("encrypted data writen\n");
	
	if(fclose(fm) == EOF)
	{
		printf("cannot close encrypted.txt\n");
		return -1;
	}
//******************************************************************************************************************//
//**********************************************writing from encr_txt in buffer3************************************//
	
int *buffer2 = NULL;
int *buffer3 = NULL;
buffer2 = (int *) malloc(2048 * sizeof(int));
buffer3 = (int *) malloc(2048 * sizeof(int));
memcpy(buffer2, encrypted_txt_array, MAX_BRAM_SIZE);
for(i = 0; i<txt_length;i++){
	buffer3[i] = (buffer2[2*i] << 16) | buffer2[2*i+1];

}
//***********************************************************************************************************************//
//*************************************sending data from buff to bram0***************************************************//

fk = open("/dev/bram0", O_RDWR|O_NDELAY);
	if (fk<0)
	{
		printf("Cannot open /dev/bram0 for write\n");
		return -1;
	}
	kk=(int*)mmap(0, MAX_KERNEL_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fk, 0);
		if(kk == NULL) {
			printf("\ncouldn't mmap\n");
			return 0;
		}
	memcpy(kk, buffer3, BLOCK_SIZE);
	munmap(kk, BLOCK_SIZE);
	printf("bram0 done\n");
	close(fk);
	if(fk < 0)
	{
		printf("cannot close /dev/bram0 for write\n");
		return -1;
	}

//************************************************reset bit**************************************************************//

fs = fopen("/dev/Enc_dec", "w");
if (fs == NULL)
{
	printf("Cannot open /dev/Enc_dec for write\n");
	return -1;
}
//fprintf(fs, "%s = %d", start_dec, 1);
//fflush(fs);
fprintf(fs, "%s = %d",reset, 1);
fflush(fs);
fprintf(fs, "%s = %d",reset, 0);
fflush(fs);
//fprintf(fs, "%s = %d", start_enc, 0);
fclose(fs);


//***************************************************************************************************************//
//****************************************setting and clearing start bit*******************************************************//
char start_dec[] = "start_dec";
fs = fopen("/dev/Enc_dec", "w");
if (fs == NULL)
{
	printf("Cannot open /dev/Enc_dec for write\n");
	return -1;
}
fprintf(fs, "%s = %d", start_dec, 1);
fflush(fs);
fprintf(fs, "%s = %d",start , 1);
fflush(fs);
fprintf(fs, "%s = %d",start_dec, 0);
fflush(fs);
//fprintf(fs, "%s = %d", start, 0);
fclose(fs);
//*************************************************************************************************************//
//*************************************writing from bram1 to dec_txt**********************************************//
fb = open("/dev/bram1", O_RDWR|O_NDELAY);
	if(fb < 0)
	{
		printf("Cannot open /dev/bram1 for write\n");
		return -1;
	}
	decrypted_txt_array = (int*) malloc(MAX_BRAM_SIZE);
	g=(int*)mmap(0, MAX_BRAM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
	if(g == NULL) {
		printf("\ncouldnt mmap\n");
		return 0;
	}
	
	memcpy(decrypted_txt_array, g, MAX_BRAM_SIZE);
	munmap(g, MAX_BRAM_SIZE);
	printf("bram1 done\n");
	close(fb);
	if(fb < 0)
	{
		printf("cannot close /dev/bram1 for write\n");
		return -1;
	}
//************************************************************************************************************//
//*************************************writing encrypted data to file****************************************//
FILE *fv;
	fv = fopen("decrypted.txt", "w");
	if(fv==NULL)
	{
		printf("cannot open encrypted.txt\n");
		return -1;
	}

	printf("Otvoren fajl.\n");
	for(i = 0;i<=(txt_length*2)-1; i++)
	{
		fprintf(fv, "%c", (char)decrypted_txt_array[i]);
		fflush(fv);
	}
	fprintf(fv, "\n");
	printf("encrypted data writen\n");
	
	if(fclose(fv) == EOF)
	{
		printf("cannot close encrypted.txt\n");
		return -1;
	}
//******************************************************************************************************************//







	return 0;
}
