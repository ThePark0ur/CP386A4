/*
Repository URL: https://github.com/ThePark0ur/CP386A4

Authors:
Kamran Tayyab       170432010   Git: Kamran14
Matthew Dietrich    170462520   Git: ThePark0ur
*/

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct client //Represents a single client
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
int SafetyAlgo();
void* ClientExec(void* t);
int InputParser(char* input);


int ClientCount; //Number of clients
int ResourceCount; //Number of resources
char **CommandLineArgs = NULL; //To allow access to command line arguments globally

int *Available = NULL, **Max = NULL, **Allocation = NULL, **Need = NULL;//Matrices which will hold resource information in relation to clients
int *safeResources = NULL, *sequenceTemp = NULL, *AmountLeft = NULL, *done = NULL; //For use in safe sequence processing
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

    //printf out known info
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

	int safe = SafetyAlgo(); //Checks if safe state is possible. 0 if able, -1 otherwise
	if (safe == -1){
		printf("Error: No safe state possible. Request denied.\n");
		SilentRelease(CommandBackup); //Release resources back to previous safe state
	}else{
		printf("Request valid and safe. Request Accepted. \n");
	}
}

//Similar to request, but does not printf confirmations or denials. Use to reverse changes
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

//Similar to release, but does not printf confirmations or denials. Use to reverse changes
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


void *ClientExec(void *t) { //Called from pthread

	//Mutex lock to determine critical section order
	pthread_mutex_lock(&mutex);

	//Crit Section...
	char *InputArray = (char *)malloc(50);
	int *intArray = (int *)malloc(50);
	int number;

	//Establish new need array
	for (int i = 0; i < ResourceCount; i++) {
		int number = Need[((Client *)t)->clientNum][i];
		intArray[i] = number;
	}

	//Create a request string for the client
	InputArray[0] = 'R';
	InputArray[1] = 'Q';
	InputArray[2] = ' ';
	InputArray[3] = ((Client *)t)->clientNum + '0';
	InputArray[4] = ' ';

	//Insert needed amount of resource into request string
	int j = 0;
	for (int i = 5; i < 12; i = i + 2) {
		InputArray[i] = intArray[j] + '0';
		InputArray[i + 1] = ' ';
		j++;
	}

	printf("\tClient %i has started\n", ((Client *)t)->clientNum);

	//Request remaining resources
	printf("\tRequest all needed resources\n        ");
	SilentRequest(InputArray);

	//Print out updated matrices
	printf("New Need Array:   ");
	for (int j = 0; j < ResourceCount; j++) {
		printf(" %i", Need[((Client*)t)->clientNum][j]);
	}

	printf("\n\tNew Allocation Array:   ");
	for (int j = 0; j < ResourceCount; j++) {
		printf(" %i", Allocation[((Client*)t)->clientNum][j]);
	}

	printf("\n\tNew Available Array:   ");
	for (int j = 0; j < ResourceCount; j++) {
		printf("%i ", Available[j]);
	}

	//Verify state is safe
	printf("\n\tState still safe: ");
	int safe = SafetyAlgo();
	(safe == 0) ? printf("Yes\n") : printf("No\n");

	printf("\tClient %i has finished\n", ((Client *)t)->clientNum);
	int *intArray2 = (int *)malloc(50);
	for (int i = 0; i < ResourceCount; i++) {
		number = Allocation[((Client *)t)->clientNum][i];
		intArray2[i] = number;
	}

	//Initiate array to hold the release request
	InputArray[0] = 'R';
	InputArray[1] = 'L';
	InputArray[2] = ' ';
	InputArray[3] = ((Client *)t)->clientNum + '0';
	InputArray[4] = ' ';

	j = 0;
	for (int i = 5; i < 12; i += 2) {
		InputArray[i] = intArray2[j] + '0';
		InputArray[i + 1] = ' ';
		j++;
	}

	//Release resources now that client is complete
	printf("\tClient is releasing resources\n");
	SilentRelease(InputArray);

	//print out updated matrices
	printf("\tNow available:");
	for (int i = 0; i < ResourceCount; i++) {
		printf(" %i", Available[i]);
	}
	printf("\n");

	//End the crit section 

	//open  mutex lock for next client
	pthread_mutex_unlock(&mutex);

	//all done, time to exit!
	pthread_exit(0);
	return 0;
}

void Run(Client **clients) { 
	if (pthread_mutex_init(&mutex, NULL) != 0) {
		printf("Failed mutex init \n");
		return;
	}

	printf("\nCurrent State:");
	switch(SafetyAlgo()){
		case -1:
			printf("Not safe\n");
			printf("Currently not in safe state.\n");
			break;
		case 0:
			printf("Safe\n");
			break;
		default:
			break;
	}
	printf("Safe Sequence is: < ");
	for (int i = 0; i < ClientCount; i++) {
		printf("%i ", safeResources[i]);
		(*clients)[i].clientNum = safeResources[i];
		(*clients)[i].orderNum = i;
	}
	printf(">\n");

	sequenceTemp = safeResources;

	printf("Now going to execute the clients:\n\n");
	for (int i = 0; i < ClientCount; i++) {
		printf("----------------------------------------\n");
		printf("--> Customer/Client %i\n", sequenceTemp[i]);

		//List number of resources the Client currently needs
		printf("\tNeed:   ");
		for (int j = 0; j < ResourceCount; j++) {
			printf(" %i", Need[sequenceTemp[i]][j]);
		}
		printf("\n");

		printf("\tAllocated resources:   ");
		for (int j = 0; j < ResourceCount; j++) {
			printf(" %i", Allocation[sequenceTemp[i]][j]);
		}
		printf("\n");
		
		printf("        Available:   ");
		for (int j = 0; j < ResourceCount; j++) {
			printf(" %i", Available[j]);
		}
		printf("\n");

		(*clients)[i].returnVal = pthread_create(&((*clients)[i].threadHandle), NULL, ClientExec, &((*clients)[i]));

		pthread_join((*clients)[i].threadHandle, NULL);
	}

	//Clean up mutex locks now that we're done
	pthread_mutex_destroy(&mutex);
}


int SafetyAlgo() {
	int safe = 0, isLocated = 1, columnCount = ResourceCount, sequenceNum = 0;

	safeResources = (int *)malloc(ClientCount * sizeof(int));
	AmountLeft = (int *)malloc(ResourceCount * sizeof(int));
	done = (int *)malloc(ClientCount * sizeof(int));

	for (int i = 0; i < ClientCount; i++) {
		safeResources[i] = -1;
		done[i] = 0;
	}

	for (int i = 0; i < ResourceCount; i++) {
		AmountLeft[i] = Available[i];
	}

	while(isLocated && !safe){
		isLocated = 0;
		for (int i = 0; i < ClientCount;i++){
			if(!done[i]){
				columnCount = 0;
				int j = 0;
				while(j<ResourceCount){
					if(Need[i][j] < 0 || AmountLeft[j] < 0){
						safe = -1;
						break;
					}
					if(Need[i][j] > AmountLeft[j]){
						break;
					}else{
						columnCount++;
					}
					if(columnCount == ResourceCount){
						safeResources[sequenceNum] = i;
						sequenceNum += 1, done[i] = 1, isLocated = 1;
						for (int g = 0; g < ResourceCount; g++){
							AmountLeft[g] += Allocation[i][g];
						}
					}
					j++;
				}
			}
		}
	}


	for (int i = 0; i < ClientCount; i++) {
		if (!done[i]) {
			safe = -1;
		}
	}

	return safe;
}
