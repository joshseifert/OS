#include <stdio.h>
#include <stdlib.h>		//rand
#include <string.h>		//atoi
#include <time.h>		//time

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		printf("Usage: keygen keylength\n");
		exit(1);
	}

	/*Seed for pseudo-random numbers*/
	srand(time(NULL));
	
	/*For loop controller*/
	int i;
	
	/*The length of the key to generate, passed in as command line argument.*/
	int keyLength = atoi(argv[1]);
	
	int randomNum;
	
	for (i=0; i < keyLength; i++)
	{
		/*Generate a random number 0 - 26*/
		randomNum = rand() % 27;
		if(randomNum == 26)
		{
			printf(" ");
		}
		else
		{
			//65 is the ASCII value of 'A'. Converts 0 to A, 1 to B ... 25 to Z
			printf("%c",randomNum + 65);
		}
	}
	/*Cap off key with a newline, to standardize formatting.*/
	printf("\n");
	exit(0);
}