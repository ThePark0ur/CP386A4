/*
Repository URL: https://github.com/ThePark0ur/CP386A4

Authors:
Kamran Tayyab       170432010   Git: Kamran14
Matthew Dietrich    170462520   Git: ThePark0ur
*/

//Include Statements
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
//#include <pthread.h>
//#include <semaphore.h>

void readFile(char* fileName, int** Customer, int CC);
int countLines(char* fileName);

int main(int argc, char* argv[]){
    if (argc < 2){
        printf("Error: No maximum resource amounts given.\n");
        return -1;
    }
    for (int i = 0; i < argc; i++){
        printf("%s\n", argv[i]);
    }
    //Customer* cust = NULL;
    //int CustomerCount = readFile("Sample4_in.txt",&cust);
    int CustomerCount = countLines("Sample4_in.txt");
    int Customer[CustomerCount][argc-1];
    int* CustomerPointer = &Customer[0][0];
    printf("We out here\n");
    readFile("Sample4_in.txt", &CustomerPointer, CustomerCount);


    printf("not brokeennnn\n");
    return 0;
}

//Reads input file and creates 2D matrix of customer resource maximums
void readFile(char* fileName, int** Customer, int CC){ 
    //Check the file exists
	FILE *in = fopen(fileName, "r");
	if(!in){
		printf("Error: Couldn't open file.\n");
		return;
	}

    //Copy contents of file into memory and close file
	struct stat st;
	fstat(fileno(in), &st);
	char* fileContent = (char*)malloc(((int)st.st_size+1)* sizeof(char));
	fileContent[0]='\0';	
	while(!feof(in)){
		char line[100];
		if(fgets(line,100,in)!=NULL)
		{
			strncat(fileContent,line,strlen(line));
		}
	}
	fclose(in);

    //Split each customer into resource values
	char* lines[CC];
	char* command = NULL;
	int i=0;
	command = strtok(fileContent,"\r\n");
	while(command!=NULL){
		lines[i] = malloc(sizeof(command)*sizeof(char));
		strcpy(lines[i],command);
		i++;
		command = strtok(NULL,"\r\n");
	}

    //Track resource maximum values for each customer
	for(int k=0; k<CC; k++){
		char* token = NULL;
        int j = 0;
		token =  strtok(lines[k],",");
		while(token!=NULL)
		{
            Customer[k][j] = atoi(token);
			j++;
			token = strtok(NULL,",");
		}
	}
	return;
}


int countLines(char* fileName){
    //Check the file exists
	FILE *in = fopen(fileName, "r");
	if(!in){
		printf("Error: Couldn't open file.\n");
		return -1;
	}

    //Copy contents of file into memory and close file
	struct stat st;
	fstat(fileno(in), &st);
	char* fileContent = (char*)malloc(((int)st.st_size+1)* sizeof(char));
	fileContent[0]='\0';	
	while(!feof(in)){
		char line[100];
		if(fgets(line,100,in)!=NULL)
		{
			strncat(fileContent,line,strlen(line));
		}
	}
	fclose(in);

    //Count lines and begin allocating memory for data struct
	char* command = NULL;
	int CustomerCount = 0;
	command = strtok(fileContent,"\r\n");
	while(command!=NULL){
		CustomerCount++;
		command = strtok(NULL,"\r\n");
	}

    return CustomerCount;
}