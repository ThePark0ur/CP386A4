/*
Repository URL: https://github.com/ThePark0ur/CP386A4

Authors:
Kamran Tayyab       170432010   Git: Kamran14
Matthew Dietrich    170462520   Git: ThePark0ur
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

typedef struct client // Represents a single thread
{
    int clientNum; 
    int orderNum; 
    pthread_t threadHandle;
    int returnVal; 
} Client;

int ReadFile (char* fileName);
void Request(char* command);
void SilentRequest(char* command);
void Release(char* command);
void SilentRelease(char* command);
void Star();
void Run(Client** clients);
int SafetyAlgorithm();
void* threadExec(void* t);
int InputParser(char* input);

int ClientCount; //Number of clients
int ResourceCount; //Number of resources
char **CommandLineArgs = NULL; //To allow access to command line arguments globally

int *Available = NULL, **Max = NULL, **Allocation = NULL, **Need = NULL;//Matrices which will hold resource information in relation to clients
int *safeSeq = NULL, *tempSeq = NULL, *work = NULL, *finish = NULL; //For use in safe sequence processing
Client* clients = NULL; //Stores array of clients as given in input text
pthread_mutex_t mutex; //Mutex lock


int main(int argc, char **argv){

	clients = (Client*) malloc(sizeof(Client)*ClientCount); //Size out the clients array to hold each line
	char *command = (char*) malloc(50);
	if (argc < 2){
		printf("Error: No maximum resource amounts given.\n");
		return -1;
	}

	ResourceCount = argc-1;
	CommandLineArgs = argv;
	ReadFile("sample4_in.txt");

    //Print out known info
	printf("Number of Clients: %d\n", ClientCount);
	printf("Currently Available Resources: ");
	for (int i = 0; i < ResourceCount; i++){
		printf("%d ", Available[i]);
	}
    printf("\n");

	//Get user input until run is entered.
    int finish = 0; //Used to determine when to exit while loop
	while (finish == 0){
		printf("Enter Request: ");
		fgets(command, 25, stdin); //Take user input
		finish = InputParser(command);//Parse the input and call relevant function
	}

	return 0;
}

int InputParser(char* input){
    int clientID;

    if (memcmp("RQ", input, 2) == 0){ //Requesting Resources
        Request(input);
    }
    else if (memcmp("RL", input, 2) == 0){ //Releasing Resources
        Release(input);
    }
    else if (memcmp("*", input, 1) == 0){ //Report on resource usage
        Star();
    }
    else if (memcmp("Run", input, 3) == 0){ //Begin execution of threads
		Run(&clients);
        return 1;
    }
    else{
        printf("Unexpected Command. Please re-enter.\n");
    }
    fflush(stdin); //Clear input buffer of anything that might be clogging it
    return 0;
}


int ReadFile(char* fileName){ //Takes input from the sample input file

	ClientCount = 0;
	FILE *fh = fopen(fileName, "r");
	char fPointer;

	if(!fh){
		printf("Error: File not found\n");
		return -1;
	}

	//Determine number of clients in the input file
	fPointer = fgetc(fh);
	while (fPointer != EOF){
        if (fPointer == '\n'){
            ClientCount++; //Increase number of clients (rows in file)
        }
        fPointer = fgetc(fh); //Move to the next character.
    }
	ClientCount++;

	//Set up informational matrices for use with Safety Algorithm
	Available = (int *)malloc(ResourceCount * sizeof(int*)); //Shows resources available
	Max = (int **)malloc(ClientCount * sizeof(int *)); //Shows max required resources 
    Allocation = (int **)malloc(ClientCount * sizeof(int *)); //Shows how many resources are currently allocated
    Need = (int **)malloc(ClientCount * sizeof(int *)); //Shows the remaining resources needed by each function

    //Create second dimension
	for (int i=0; i < ClientCount; i++){
         Max[i] = (int *)malloc(ResourceCount * sizeof(int));
         Allocation[i] = (int *)malloc(ResourceCount * sizeof(int));
         Need[i] = (int *)malloc(ResourceCount * sizeof(int));
	}

    //Set up known array values:
	//Move given resource amounts into proper variable
	for (int i=1; i <= (ResourceCount); i++){
		Available[i-1] = atoi(*(CommandLineArgs+i));
	}
    //Allocation array will begin at 0
	for (int i = 0; i < ClientCount; i++){
		for (int j = 0; j < ResourceCount; j++){
			Allocation[i][j] = 0;
		}
	}

	fseek(fh, 0, SEEK_SET);

	char *temp;
    int i = 0, j = 0;
	fPointer = fgetc(fh);
    
	while (fPointer != EOF){
		if (fPointer != ',' && fPointer != '\n'){
			temp = &fPointer;

            //Max and need will begin at the same number (Allocation is at 0 across all Clients)
			Max[i][j] = atoi(temp);
			Need[i][j] = atoi(temp);
			j++;

			if (j == ResourceCount){
				j = 0;
				i++;
			}
		}
		fPointer = fgetc(fh);
	}
	
	return 0;
}

void Request(char* command){
	char* CommandBackup = (char*) malloc(50);
	strcpy(CommandBackup, command); //Save copy of command for use in release if needed

	char *token = strtok(command, " "); //Seperate RQ from informational sections of command
	int cID = atoi(strtok(NULL, " ")); //Retrieve the ClientID
	int resourceVal; //Initialize variable to hold resource value being modified
	int valid = 0; //Verifies if request is able to be fulfilled (Does not include Safe State Check)

	if (cID >= ClientCount){
		printf("Error: Invalid Client Number.\n");
		return;
	}

    int j = 0;
	token = strtok(NULL, " "); //Retrieve next resource value being requested
	while (token != NULL){
		resourceVal = atoi(token);
        //Allocate the resource amount to the thread
        Allocation[cID][j] = Allocation[cID][j] + resourceVal;
		Available[j] = Available[j] - resourceVal;
		Need[cID][j] = Need[cID][j] - resourceVal;
        //Process whether allocation is possible
		if (Need[cID][j] < 0){ //Too many resources were requested, thread overcapacity. Will not be filled.
			valid = -1;
		}
		if (Available[j] < 0){ //Not enough resources are available. Will not be filled.
			valid = -2;
			
		}
		j++;
		token = strtok(NULL, " ");
	}

	switch (valid){
		case -1:
			printf("Error: Thread requested more resources than maximum allowed. Request denied\n");
			break;
		case -2:
			printf("Error: Thread requested more resources than currently available. Request denied\n");
			break;
	}

    if (valid != 0) {
		SilentRelease(CommandBackup); //Release resources and return to previous state
		return;
	}

	int safe = SafetyAlgorithm(); //Checks if safe state is possible. 0 if able, -1 otherwise
	if (safe == -1){
		printf("Error: No safe state possible. Request denied.\n");
		SilentRelease(CommandBackup); //Release resources back to previous safe state
	}else{
		printf("Request valid and safe. Request Accepted. \n");
	}
}

//Similar to request, but does not print confirmations or denials. Use to reverse changes
void SilentRequest(char* command){
	char *token = strtok(command, " "); //Seperate RQ from informational sections of command
	int cID = atoi(strtok(NULL, " ")); //Retrieve the ClientID
	int resourceVal; //Initialize variable to hold resource value being modified
    int j = 0;
	token = strtok(NULL, " "); //Retrieve next resource value being requested
	while (token != NULL){
		resourceVal = atoi(token);
        //Allocate the resource amount to the thread
        Allocation[cID][j] = Allocation[cID][j] + resourceVal;
		Available[j] = Available[j] - resourceVal;
		Need[cID][j] = Need[cID][j] - resourceVal;
		j++;
		token = strtok(NULL, " ");
	}
}

void Release(char* command){
	char* CommandBackup = (char*) malloc(50);
	strcpy(CommandBackup, command); //Save copy of command for use in request if needed
	char *token = strtok(command, " "); //Remove unnecessary parts of the request
	int cID = atoi(strtok(NULL, " ")); //Retrieve the ClientID
	int j = 0;
	int error = 0;
	int resourceVal; //Initialize variable to hold resource value being modified

	if (cID >= ClientCount){
		printf("Error: Invalid Client Number.\n");
		return;
	}

    token = strtok(NULL, " "); //Get resource value to modify by
	while (token != NULL){
		resourceVal = atoi(token);
		Allocation[cID][j] = Allocation[cID][j] - resourceVal; //Remove resources from amount allocated
		Need[cID][j] = Need[cID][j] + resourceVal; //Resources must be added back to the needed counter
        Available[j] = Available[j] + resourceVal; //Add the value back to globally available resources

		if (Allocation[cID][j] < 0){ //If request would attempt to release more resources than the client currently holds, deny it
			error = 1;
		}
		j++;
		token = strtok(NULL, " ");
	}
	if (error){
		printf("Error: More resource being released than client has allocated. Release denied.\n");
		SilentRequest(CommandBackup);
	}else{
		printf("Release request is valid. Release Accepted\n");
	}
}

//Similar to release, but does not print confirmations or denials. Use to reverse changes
void SilentRelease(char* command){
	char *token = strtok(command, " "); //Remove unnecessary parts of the request
	int cID = atoi(strtok(NULL, " ")); //Retrieve the ClientID
	int j = 0;
	int resourceVal; //Initialize variable to hold resource value being modified

    token = strtok(NULL, " "); //Get resource value to modify by
	while (token != NULL){
		resourceVal = atoi(token);
		Allocation[cID][j] = Allocation[cID][j] - resourceVal; //Remove resources from amount allocated
		Need[cID][j] = Need[cID][j] + resourceVal; //Resources must be added back to the needed counter
        Available[j] = Available[j] + resourceVal; //Add the value back to globally available resources
		j++;
		token = strtok(NULL, " ");
	}
	return;
}

void Star(){
	printf("Available Resources: \n");
	for (int j = 0; j < ResourceCount; j++){
		printf("%d ", Available[j]);
	}

	printf("\nMaximum Resources by Client: ");
	for (int i = 0; i < ClientCount; i++){
        printf("\nClient %d\t", i);
		for (int j = 0; j < ResourceCount; j++){
			printf("%d ", Max[i][j]);
		}
	}

	printf("\nCurrently Allocated Resources by Client: ");
	for (int i = 0; i < ClientCount; i++){
        printf("\nClient %d\t", i);
		for (int j = 0; j < ResourceCount; j++){
			printf("%d ", Allocation[i][j]);
		}
	}

	printf("\nCurrent Resources Needed by Client: ");
	for (int i = 0; i < ClientCount; i++){
        printf("\nClient %d\t", i);
		for (int j = 0; j < ResourceCount; j++){
			printf("%d ", Need[i][j]);
		}
	}
	printf("\n");
}


int SafetyAlgorithm(){
	return 0;
}

void Run(Client **clients){
	return;
}