/*
OTP_DEC
Author: Josh Seifert
Filename: otp_dec.c
Date Last Modified: 06/04/2105
Description: This program accepts a file containing encoded text, along with a file
	containing a key to decrypt that text, and passes those files via a socket
	to a server, so that they may be decoded. Upon receipt, the decoded text
	is printed.
*/

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

int validateInput(int inputFile);

int main(int argc, char *argv[])
{
	if(argc != 4)
	{
		fprintf(stderr, "Usage: otp_dec plaintext key port\n");
		exit(1);
	}
	
    int ciphertext;							/*File descriptor for ciphertext file*/
    int key;								/*File descriptor for key file*/
    int ciphertextLength;					/*Length of ciphertext char array*/
	int keyLength;							/*Length of key char array*/
    int sockfd;								/*File descriptor for socket on server*/
    int port;								/*Port on which to send to server. Passed via command line*/
	int numBytes;							/*Keeps track of read/write sizes*/
    
    struct sockaddr_in server;				/*Info about the server*/
    struct hostent* serverAddress;			/*Address of the server*/
	
	/*These buffers could be consolidated and reused to be more 
	space efficient, but are separated out for clarity of code*/
	
	char plaintextBuffer[256];				/*Buffer used to store chunks of uncoded text*/
	char keyBuffer[256];					/*Buffer used to store chunks of cipher key*/
	char ciphertextBuffer[256];				/*Buffer used to store chunks of coded text*/
	
	/*Message to send to server, ensure right client communicates with right server.
	Idea from Robert Sparks on the class message board.*/
	char validate[] = "ACCEPT_DEC_CLIENT";
	
	/*Open the text files, passed in via command line*/
	if((ciphertext = open(argv[1], O_RDONLY)) == -1)
	{
		perror("open");
		exit(1);	
	}
	
	if((key = open(argv[2], O_RDONLY)) == -1)
	{
		perror("open");
		exit(1);	
	}
    
	port = atoi(argv[3]);
	
	/*Check input files for invalid chars. Returns length of input, or -1 if invalid.*/
	ciphertextLength = validateInput(ciphertext);
	keyLength = validateInput(key);
	
	/*Verify that files do not contain invalid input*/
	if (ciphertextLength == -1 || keyLength == -1)
	{
		fprintf(stderr,"%s error: input contains bad characters\n",argv[0]);
		exit(1);
	}
	
	/*Checks that key is at least as long as the ciphertext file*/
	if(ciphertextLength > keyLength)
	{
		printf("Error: key \'%s\' is too short\n",argv[2]);
		exit(1);
	}	
	
	/*The following section is heavily based on the C socket tutorial at
	http://www.linuxhowtos.org/C_C++/socket.htm*/
	
	/*Get address of the server. This is between processes on a single server,
	so the server is "localhost"*/
	if ((serverAddress = gethostbyname("localhost")) == NULL)
	{
		perror("server address");
		exit(1);
	}
    
    /*Create a socket on the client*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}	

	/*Configure server*/
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	memcpy(&server.sin_addr, serverAddress->h_addr, serverAddress->h_length);
		
	/*Connect client socket to server*/
	if (connect(sockfd, (struct sockaddr *) &server, sizeof(server)) == -1)
	{
		perror("connect");
		exit(1);
	}
    
	/*Write message to server to validate that it is connecting to correct place*/
	if(write(sockfd, validate, sizeof(validate)) == -1)
	{
		perror("write");
		exit(1);
	}
	
	/*Example linked to in the lecture uses bzero, but man pages say this is deprecated,
	to use memset instead, to clear buffers.*/
    memset(validate, '\0', sizeof(validate));
	
	/*Read message back from server approving connection*/
	if(read(sockfd, validate, sizeof(validate)) == -1)
	{
		perror("read");
		exit(1);
	}
	
	/*Check that the client connected to the correct server*/
    if (strcmp(validate, "ACCEPT_DEC_SERVER") != 0)
	{
//        perror("port");
        exit(1);
    }
	
	/*Loop runs until entire ciphertext file is read and translated*/
	while(1)
	{
		/*numBytes returns the number of bytes read from the ciphertext file*/
		if((numBytes = read(ciphertext, ciphertextBuffer, 255)) == -1)
		{
			perror("read");
			exit(1);
		}

		/*If numBytes == 0, at end of file, done reading*/			
		if(numBytes == 0)
		{
			break;
		}
		
		ciphertextBuffer[numBytes] = '\0';
		
		/*Read the same number of bytes from the key file*/
		if(read(key, keyBuffer, numBytes) == -1)
		{
			perror("read");
			exit(1);
		}
		keyBuffer[numBytes] = '\0';
		
		/*Write the corresponding ciphertext and key chunks to the server*/
		if(write(sockfd, ciphertextBuffer, numBytes) == -1)
		{
			perror("write");
			exit(1);
		}
		
		if(write(sockfd, keyBuffer, numBytes) == -1)
		{
			perror("write");
			exit(1);
		}
		
		/*Server returns the decoded text*/
		if(read(sockfd, plaintextBuffer, numBytes) == -1)
		{
			perror("read");
			exit(1);
		}
		
		plaintextBuffer[numBytes] = '\0';
		
		/*This is our decoded text*/
		printf("%s",plaintextBuffer);
		
		/*Reset all buffers*/
		memset(plaintextBuffer, '\0', sizeof(plaintextBuffer));
		memset(keyBuffer, '\0', sizeof(keyBuffer));
		memset(ciphertextBuffer, '\0', sizeof(ciphertextBuffer));
	}
    
    close(sockfd);
    
    return 0;
}

/*
This function serves two purposes. The first is to validate the input.
Input files must contain only letters A-Z, whitespace or newlines. Lowercase 
letters and punctuation are disallowed. The second purpose to get the length of
the input files, so that the program can easily determine if the key is long 
enough.
*/
int validateInput(int inputFile)
{
	/*Read the input files into the buffer a single char at a time*/
	char oneChar[1];
	/*Counts the number of chars, including endline*/
	int fileLength = 0;
	
	while (read(inputFile, oneChar, 1) != 0)
	{
		/*Coded text must be uppercase A-Z, or whitespace. Newlines are handled later*/
		if((oneChar[0] < 'A' || oneChar[0] > 'Z') && oneChar[0] != ' ' && oneChar[0] != '\n')
		{
			/*If encounter an invalid char, return immediately with -1, to indicate error*/
            return -1;
		}
		else
		{
			fileLength++;
		}
    }
	
	/*Reset file pointers back to the beginning of the file*/
	lseek(inputFile, 0, SEEK_SET);
	
	/*If no bad chars detected, return length of the file*/
	return fileLength;
}