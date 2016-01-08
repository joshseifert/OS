/*
OTP_DEC_D
Author: Josh Seifert
Filename: otp_dec_d.c
Date Last Modified: 06/04/2105
Description: This program is a daemon (runs in the background as a server) to
	accept files from client processes containing text. It reads in encrypted
	text, and a randomly generated key. Using the key as a one-time pad, it
	decrypts the text and returns it to the client.
*/

#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>

char *encrypt(char *ciphertext, char *key, int length);

int main(int argc, char *argv[])
{
	if(argc != 2)
	{
		fprintf(stderr, "Usage: otp_dec_c portNumber\n");
		exit(1);
	}
	
	int sockfd;						/*File descriptor for the socket*/
	int newsockfd;					/*File descriptor for the child process*/
	int port;						/*Port on which to listen for client. Passed via command line*/
    int pid;
    int status;
	struct sockaddr_in server;		/*Holds info the server's connection*/
	struct sockaddr_in client;		/*Info about the client's connection*/
	int numBytes;
	
	/*As in client process, I'm choosing to sacrifice some memory efficiency by declaring
	multiple buffers, to increase the clarity of the code.*/
	
	char buffer[256];
	char ciphertextBuffer[256];					/*Holds chunks of the ciphertext sent from client*/
	char keyBuffer[256];						/*Holds corresponding chunks of the key, sent from client*/
	char *plaintextBuffer;						/*Points to ciphertextBuffer after encryption, given separate name for clarity*/
	
	/*Message to send to server, ensure right client communicates with right server.
	Idea from Robert Sparks on the class message board.*/
	char validate[] = "ACCEPT_DEC_SERVER";
    
    port = atoi(argv[1]);
 
	/*The following section is heavily based on the C socket tutorial at
	http://www.linuxhowtos.org/C_C++/socket.htm*/
 
    /*Create a socket on the client*/
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		perror("socket");
		exit(1);
	}
	
	/*Configure server*/
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);

	/*Bind socket to a port*/
	if(bind(sockfd, (struct sockaddr *) &server, sizeof(server)) == -1)
	{
		perror("bind call failed");
		exit(1);
	}

	/*Listen on the port for connections. Per program requirements, up to 5 connections may be
	made at once. Since each client process, in quick succession, sends 2 messages to the server,
	I'm upping it to 10, to err on the side of safety.*/
	if(listen(sockfd, 10) == -1)
	{
		perror("listen");
		exit(1);
	}

	/*Once the server is set up, we want it to run indefinitely in the background*/
    while (1)
	{
		int addressLength = sizeof(client);
		
		/*Try to accept requests from client server*/
		if((newsockfd = accept(sockfd, (struct sockaddr*) &client, &addressLength)) == -1)
		{
			perror("accept");
			/*Don't exit here. May want to keep accepting new connections*/
		}
		
		/*Create new process for each client request*/
        pid = fork();
		
		if(pid == -1)
		{
			perror("fork");	
			exit(1);			
		}
		/*If pid == 0, is child process.*/
		else if(pid == 0)
		{	
			/*Close the socket in the child process. The child inherits the server functionality of its
			parent, but we only want the parent to act as the server, not all of its children.*/
			close(sockfd);
			
			memset(buffer, '\0', sizeof(buffer));
			
			/*First message from the client is an authorization code.*/
			if(read(newsockfd, buffer, sizeof(buffer)-1) == -1)
			{
				perror("read");
				exit(1);
			}
			
			/*If the client has the incorrect authorization code, send back rejection message*/
            if (strcmp(buffer, "ACCEPT_DEC_CLIENT") != 0)
			{
                printf("Error: %s cannot find otp_enc_d",argv[0]);
                strcpy(validate, "REJECT_DEC_SERVER");
                write(newsockfd, validate, sizeof(validate));
                exit(1);
            }
			/*If the client has the correct authorization code, send back accept message*/
			else
			{
                write(newsockfd, validate, sizeof(validate));
            }
            
            memset(buffer, '\0', sizeof(buffer)); 
            
			/*This runs while the client continues to send data to the server. */
			while(1)
			{
				/*read returns the number of bytes received from the client*/
				if((numBytes = read(newsockfd, ciphertextBuffer, 255)) == -1)
				{
					perror("read");
					exit(1);
				}
				
				/*if numBytes is 0, the client has finished sending data, may terminate loop.*/
				if(numBytes == 0)
				{
					break;
				}
				ciphertextBuffer[numBytes] = '\0';
				
				/*Next, the client sends the same length of key*/
				if(read(newsockfd, keyBuffer, numBytes) == -1)
				{
					perror("read");
					exit(1);
				}
				keyBuffer[numBytes] = '\0';
				
				/*The corresponding sections of text and key are sent to be decoded,
				and the decoded text is returned.*/
				plaintextBuffer = encrypt(ciphertextBuffer, keyBuffer, numBytes);
				
				/*Write the decoded text back to the client*/
				if(write(newsockfd, plaintextBuffer, numBytes) == -1)
				{
					perror("write");
					exit(1);
				}
			}			
            exit(0);
		}
		/*Parent process*/
		else
		{
			/*Reap zombies*/
            close(newsockfd);
            do {
                pid = waitpid(-1, &status, WNOHANG);
            } while (pid > 0);			
		}
    }
    close(sockfd);
    return 0;
}
 
/*Takes a length of encoded text, and an equal length of pseudo-randomly
generated cipher, and deciphers it into readable text. Decodes the cipher "in-place",
the encrypted text is replaced a letter at a time in the original char array,
just because it seems cool in my mind to have the code transform into text right
before your eyes.*/
char *encrypt(char *ciphertext, char *key, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		/*Convert the ciphertext to values 0 - 26, where A = 0, B = 1 ... Z = 25, ' ' = 26*/
		if(ciphertext[i] == ' ')
		{
			ciphertext[i] = 26;
		}
		else
		{
			ciphertext[i] -= 'A';
		}
		/*Convert the key, as above*/
		if(key[i] == ' ')
		{
			key[i] = 26;
		}
		else
		{
			key[i] -= 'A';
		}
		/*The decode cipher is subtract key from value, make sure it is positive, and mod the value*/
		ciphertext[i] = (ciphertext[i] - key[i] + 27) % 27;
		
		/*Then, reconvert the ASCII values to equivalent letters*/
		if(ciphertext[i] == 26)
		{
			ciphertext[i] = ' ';
		}
		else
		{
			ciphertext[i] += 'A';
		}
	}
	/*Cipher also converts the newline char at the end, but we want to 
	keep that, so restore it.*/
	ciphertext[i - 1] = '\n';
	
	return ciphertext;
}