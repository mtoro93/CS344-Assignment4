/*
 * Name: Matthew Toro
 * Class: CS344 Operatng Systems
 * Assignment Program 4
 * Due Date: 8/18/2017
 * Description: utility program that generates random key files
 * */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

const char* keyArray[] = {"A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K",
"L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z", " "};

int main(int argc, char* argv[])
{
   // seed the random generator
   srand(time(0));
   // get the command line arg and convert to int
   char* intStr = argv[1];
   int arrayLength = atoi(intStr);
   // arrayLength +  2: 1 for a newline, 1 for a null terminator 
   char* key = malloc((arrayLength + 2) * sizeof(char));
   memset(key, '\0', sizeof(key)); 
   int index;
   // iterate through array assigning keyArray[rand() % 27] to key[index]
   for (index = 0; index < arrayLength; index++)
   {
	if (index == 0)
	   strcpy(key, keyArray[rand() % 27]);
	else
	   strcat(key, keyArray[rand() % 27]);
   }
   strcat(key, "\n");
   write(1, key, strlen(key));
   return 0;
}
