/*
 * Name: Matthew Toro
 * Class: CS344 Operating Systems
 * Assignment: Program 4
 * Due Date: 8/18/2017
 * Description: A daemon that accepts cipherText and a key to decrypt it to plaintext to send back.
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <sys/ioctl.h>

int main(int argc, char* argv[])
{
   // set up the server information, borrowed from server.c from lecture
   struct sockaddr_in serverAddress;
   memset((char*)&serverAddress, '\0', sizeof(serverAddress));
   int portNum = atoi(argv[1]);
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_port = htons(portNum);
   serverAddress.sin_addr.s_addr = INADDR_ANY;

   // create the socket
   int listenSocketFD = socket(AF_INET, SOCK_STREAM, 0);
   if (listenSocketFD < 0)
   {
	fprintf(stderr, "Error: error opening socket");
	exit(2);
   }
   
   // bind the socket
   if (bind(listenSocketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
   {
	fprintf(stderr, "Error: error on binding");
	exit(2);
   }
   // begin listening
   listen(listenSocketFD, 5);

   // start the infinite loop here
   while(1)
   {
	// accept a connection
	// if not valid, send a message to stderr
	// else fork off a child with a copy of the file descriptor
	struct sockaddr_in clientAddress; 
	socklen_t sizeOfClientInfo = sizeof(clientAddress); 
	int establishedConnectionFD = accept(listenSocketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
	if (establishedConnectionFD < 0)
	{
	   fprintf(stderr, "Error: error on accept");
	}
	else
	{
	   // else fork off here
	   pid_t spawnPid = -5;
	   int childPID_actual;
	   int childExitMethod = -5;
	   spawnPid = fork();
	   switch (spawnPid)
	   {
	   	case -1:
	   	{
		   fprintf(stderr, "ERROR: fork failed\n");
		   close(establishedConnectionFD);
	   	   break;
	   	}
	   	case 0:
	   	{
			// begin fork child code
			// recv message from client
	   		char completeMessage[150000], readBuffer[10];
	   		memset(completeMessage, '\0', sizeof(completeMessage));
	   		int charsRead;
			// i use a terminating sequence of "@@" to know when the message ends
	   		while (strstr(completeMessage, "@@") == NULL)
	   		{
				memset(readBuffer, '\0', sizeof(readBuffer));
				charsRead = recv(establishedConnectionFD, readBuffer, sizeof(readBuffer) - 1, 0);
				if (charsRead < 0)
				{
	   	   			fprintf(stderr, "error reading from socket");
	   	   			break ;
				}
				strcat(completeMessage, readBuffer);
	   		}
	   		int terminalLocation = strstr(completeMessage, "@@") - completeMessage;
	   		completeMessage[terminalLocation] = '\0';

	   		// tokenize the message on \n
	   		// this lets me parse the message for relevant data
	   		// the first token is whether the code is 'd' or not
	   		// if it is 'd' then continue, else, send a rejection message back to the client
	   		char * token = strtok(completeMessage, "\n");
	   		if (token[0] == 'd')
	   		{
				// the second token is the cipher text
				token = strtok(NULL, "\n");
				char * cipherTextStr = malloc(strlen(token) * sizeof(char));
				memset(cipherTextStr, '\0', sizeof(cipherTextStr));
				strcpy(cipherTextStr, token);

				// the third token is the key
				token = strtok(NULL, "\n");
				char * keyStr = malloc(strlen(token) * sizeof(char));
				memset(keyStr, '\0', sizeof(keyStr));
				strcpy(keyStr, token);
  
				// set up variables for the encryption algorithm
				// i add 5 characters to the cipher text for a code 'd-' and a terminating sequence '@@\n' 
				char * plainText = malloc(((int) strlen(cipherTextStr) + 5) * sizeof(char));
				memset(plainText, '\0', sizeof(plainText));
				strcpy(plainText, "d-");
				char letters[27] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
				char merge[2];

				// begin the loop
				int i;
				for (i = 0; i < strlen(cipherTextStr); i++)
				{
		   			int cipherTextIndex, keyIndex, trueIndex;
					// if a space, set to 0; else, subtract 64 from ascii value
		   			if (cipherTextStr[i] == 32)
						cipherTextIndex = 0;
		   			else if (cipherTextStr[i] >= 65 && cipherTextStr[i] <= 90)
						cipherTextIndex = cipherTextStr[i] - 64;
					// do the same here with the key
		   			if (keyStr[i] == 32)
						keyIndex = 0;
		   			else if (keyStr[i] >= 65 && keyStr[i] <= 90)
						keyIndex = keyStr[i] - 64;

					// if the combined indices are greater than 26, add 27
					// else just subtract
		   			if (cipherTextIndex - keyIndex < 0)
						trueIndex = (cipherTextIndex - keyIndex) + 27;
		   			else
						trueIndex = cipherTextIndex - keyIndex;
		   			memset(merge, '\0', sizeof(merge));
		   			merge[0] = letters[trueIndex];
		   			strcat(plainText, merge);
				}
				// cat the terminating sequence
				strcat(plainText, "@@\0");
  
				// right here i send a message back to the client
				// this message tells the client we have recv and decrypted
				// now the client can move on to recv
				char go[3];
				memset(go, '\0', sizeof(go));
				strcpy(go, "go");
				send(establishedConnectionFD, go, sizeof(go), 0);

				// set up the loop to send the cipher text to the client
				int totalBytes = strlen(cipherTextStr) + 5;
				int currentBytes = 0;
				int leftoverBytes = totalBytes;
				while(currentBytes < totalBytes)
				{
					charsRead = send(establishedConnectionFD, plainText + currentBytes, leftoverBytes, 0);
					if (charsRead < 0)
					{
						fprintf(stderr, "Server: error writing to socket\n");
						break;
					}
					currentBytes += (int) charsRead;
					leftoverBytes -= (int) charsRead;
				}
				//so I was running into a weird bug where the connection would fail in the client
				//it turns out I was closing this socket before it was finished send, so now I force it to wait
				//and then close 
				int checkSend = -5;
				do
				{
					ioctl(establishedConnectionFD, TIOCOUTQ, &checkSend);
				} while(checkSend > 0); 
				close(establishedConnectionFD);
				// exit the child
				exit(0); 
	   		}
	   		else
	   		{
				// if the message received from the client is anything but a 'd' then I send wrong program error message
				char errMsg[19];
				memset(errMsg, '\0', sizeof(errMsg));
				strcpy(errMsg, "w-Wrong Program@@\n");
				int totalBytes = strlen(errMsg);
				int currentBytes = 0;
				int leftoverBytes = totalBytes;
				int charsRead;
				while(currentBytes < totalBytes)
				{
					charsRead = send(establishedConnectionFD, errMsg + currentBytes, leftoverBytes, 0);
					if (charsRead < 0)
					{
						fprintf(stderr, "Server: error writing to socket\n");
						break;
					}
					currentBytes += (int) charsRead;
					leftoverBytes -= (int) charsRead;
				}
				close(establishedConnectionFD); 
				exit(1);
	   		}
	   	  	break;
	   	}
	   	default:
	   	{
			// parent skips to here
			// close  parent file descriptor
			// waitpid nohang so we don't end up with lots of zombies
			close(establishedConnectionFD);
			childPID_actual = waitpid(-1, &childExitMethod, WNOHANG);
	   	  	break;
	   	}	  
	   }
	}
   }
   close(listenSocketFD); 
   return 0;
}
