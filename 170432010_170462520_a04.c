#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct Client {
  int clientNum; 
  int orderNum; 
  pthread_t threadHandle;
  int returnVal; 
} Client;

int ReadFile(char *fileName);
void RQ(char *command);
void RL(char *command);
void Asterisk();
void Run(Client **clients);
int safetyAlgo();
void *ClientExec(void *t);

int done = 0;
int *available =
    NULL; // 1D vector for resources, which stores the number of available
          // resources of each type. If available [j] = k, there are k instances
          // of resource type Rj available.
int **max = NULL; // 2D array (number of processes by number of resources). If
                  // Max [i,j] = k, then process Ti may request at most k
                  // instances of resource type Rj.
int **allocation = NULL; // 2D array (number of processes by number of
                         // resources). If Allocation[i,j] = k then Ti is
                         // currently allocated k instances of Rj.
int **need =
    NULL; // 2D array (number of processes by number of resources). If Need[i,j]
          // = k, then Ti will need k more instances of Rj to complete its task.
int varArgc;
char **varArgv = NULL;
int *safeSeq = NULL; // 1D vector to store safe sequence
int *work = NULL;   // 1D vector for SafetyAlgorithm(); stores current available
                    // resource instances
int *done = NULL; // 1D vector for SafetyAlgorithm(); stores which processes
                    // have finished execution
Client *clients = NULL; // Client object to store all clients
int *sequenceTemp = NULL;    // 1D vector to store temporary safe sequence for run()
pthread_mutex_t mutex;  // Declare a mutex

int rowNum;    // Number of rowNum in the matrix (# of clients/processes).
int ResourceCount; // Number of ResourceCount in the matrix (# of instances per resource).

// The vectors/matrices will be defined here as global variables.

int main(int argc, char **argv) {

  // Initiate clients object so that it's big enough to store all clients
  clients = (Client *)malloc(sizeof(Client) * rowNum);

  char *prelude = (char *)malloc(
      SIZE); // This will be used to get rid of the tokens in the fgets string.
  char *command = (char *)malloc(SIZE);
  if (argc < 2) {
    printf("No initial resources provided, exiting with error code -1. \n");
    return -1;
  }
  // printf("%d", atoi(argv[1]) * 2); This is a test and this works.
  varArgc = argc;
  varArgv = argv;
  ReadFile("sample4_in.txt");
  // Prompt the user for input here. We should assume that using the run command
  // ends the program.
  while (done == 0) {
    printf("Input your command: ");
    fgets(command, SIZE, stdin); // We must use fgets to get the user's input,
                                 // since their input will have spaces.
    // Run a function based on the user's input.
    if (strncmp(command, "RQ", 2) == 0) {
      RQ(command);
    } else if (strncmp(command, "RL", 2) == 0) {
      RL(command);
    } else if (strncmp(command, "*", 1) == 0) {
      Asterisk();
    } else if (strncmp(command, "Run", 3) == 0) {
      done = 1;
      Run(&clients);
    } else {
      printf("Invalid input; please input a valid command. \n");
    }
    while (prelude != NULL) {
      prelude = strtok(NULL, " ");
    }
  }
  // Frees all resources at end of the program
 
  return 0;
}

int ReadFile(char *fileName) // Reads the input file and sets up the
                             // vectors/matrices. Presumably complete.
{
  int processes;
  int resources; // Resources should equal (commas+lines)/lines.
  FILE *in = fopen(fileName, "r");
  char filePointer;
  int lines;
  int commas;

  if (!in) {
    printf(
        "Child A: Error in opening input file...exiting with error code -1\n");
    return -1;
  }
  // How many lines/commas and by extension, resources are in the file?
  filePointer = fgetc(in);
  while (filePointer != EOF) {
    if (filePointer == '\n') {
      lines = lines + 1; // Increment the line count.
    }
    if (filePointer == ',') {
      commas = commas + 1; // Increment the comma count.
    }
    // Move to the next character.
    filePointer = fgetc(in);
  }
  processes = lines;
  resources =
      (commas + lines) / lines; // This works, for sample4_in.txt, it printed 4.
  rowNum = processes;
  ResourceCount = resources;
  // Next, dynamically set up the vectors/matrices.
  Available = (int *)malloc(resources * sizeof(int)); // Create the Available vector.
  max = (int **)malloc(processes * sizeof(int *));
  for (int i = 0; i < processes; i++) {
    max[i] = (int *)malloc(resources * sizeof(int)); // Create the max matrix.
  }
  Allocation = (int **)malloc(processes * sizeof(int *));
  for (int i = 0; i < processes; i++) {
    allocation[i] = (int *)malloc(resources * sizeof(int)); // Create the allocation matrix.
  }
  need = (int **)malloc(processes * sizeof(int *));
  for (int i = 0; i < processes; i++) {
    need[i] = (int *)malloc(resources * sizeof(int)); // Create the need matrix.
  }
  // Finally, fill the contents of those vectors/matrices.
  // Set up available from argv.
  for (int i = 1; i < (varArgc); i++) {
    available[i - 1] = atoi(*(varArgv + i));
  }
  fseek(in, 0, SEEK_SET);
  // Set up max/need (which are the same to start).
  int i = 0;
  int j = 0;
  char *temp;
  filePointer = fgetc(in);
  // Print some basic information.
  printf("Number of Customers: %d", processes);
  printf("\n");
  printf("Currently Available Resources: ");
  for (j = 0; j < resources; j++) {
    printf("%d ", available[j]);
  }
  i = 0;
  j = 0; // Reset i and j.
  printf("\n");
  printf("Maximum resources from file: \n");
  while (filePointer != EOF) {
    printf("%c", filePointer);
    if (filePointer != '\n' && filePointer != ',') {
      temp = &filePointer;
      max[i][j] = atoi(temp);
      need[i][j] = atoi(temp);
      j = j + 1;
      if (j == resources) {
        j = 0;
        i = i + 1;
      }
    }
    filePointer = fgetc(in);
  }
  // Set up allocation (every element in allocation starts as 0).
  i = 0;
  j = 0;
  for (i = 0; i < processes; i++) {
    for (j = 0; j < resources; j++) {
      allocation[i][j] = 0;
    }
  }
  return 0;
}

void RQ(char *command) {
  char *save = (char *)malloc(SIZE);
  strcpy(save, command); // Just in case we shouldn't RQ, we can use this same
                         // command to perform a RL.
  save[1] = 'L';
  char *token = strtok(command, " "); // Get rid of the RQ bit in the command.
  int i = atoi(strtok(NULL, " ")); // Which row of the matrix should we look at?
  int j = 0;
  int number;    // What is the token as a number?
  int valid = 0; // Is this request valid? 0 if it is, -1 if it isn't. This will
                 // be checked by making sure that we don't exceed the maximum
                 // requestable resources for a Client.
  int safe = 0;  // Is this request safe? 0 if it is, -1 if it isn't.
  // Gives more information on why input is invalid
  int error1 = 0;
  int error2 = -1;
  token = strtok(NULL, " ");
  while (token != NULL) {
    number = atoi(token);
    available[j] = available[j] - number;
    allocation[i][j] = allocation[i][j] + number;
    need[i][j] = need[i][j] - number;
    if (need[i][j] < 0) { // This means that the Client has requested more
                          // resources than it needs.
      valid = -1;
      error1 = -1;
    }
    if (available[j] < 0) { // This means that you don't have enough resources
                            // to give to the Client.
      valid = -1;
      error2 = j;
    }
    j++;
    token = strtok(NULL, " ");
  }
  safe = SafetyAlgorithm();
  if (valid == -1 || safe == -1) {
    if (valid == -1) {
      printf("Invalid.\n");
    }
    if (error1 == -1) {
      printf("         Client requested more resources than it needs.\n");
    }
    if (error2 != -1) {
      printf("         Not enough resources available.\n");
    }
    if (safe == -1) {
      printf("Unsafe state.\n");
    }
    printf("RQ Request denied; reversing the process with RL. \n");
    // If request denied, release allocated resources to reverse process and
    // return to previous state
    RL(save);
  } else {
    printf("RQ Request granted. \n");
  }
}

void RL(char *command) {
  char *save = (char *)malloc(SIZE);
  strcpy(save, command); // Just in case we shouldn't RL, we can use this same
                         // command to perform a RQ.
  save[1] = 'Q';
  char *token = strtok(command, " "); // Get rid of the RL bit in the command.
  int i = atoi(strtok(NULL, " ")); // Which row of the matrix should we look at?
  int j = 0;
  int number;    // What is the token as a number?
  int valid = 0; // Is this request valid? 0 if it is, -1 if it isn't. This will
                 // be checked by making sure that we don't give a Client less
                 // than 0 resources.
  token = strtok(NULL, " ");
  while (token != NULL) {
    number = atoi(token);
    available[j] = available[j] + number;
    allocation[i][j] = allocation[i][j] - number;
    need[i][j] = need[i][j] + number;
    if (allocation[i][j] <
        0) { // This means that the Client has more resources than it needs...
      valid = -1;
    }
    j++;
    token = strtok(NULL, " ");
  }
  if (valid == -1) {
    printf("RL Request denied; reversing the process with RQ. \n");
    RQ(save);
  } else {
    printf("RL Request granted. \n");
  }
}

void Asterisk() {
  int i; // Placeholder iterator variables.
  int j;
  printf("Currently Available Resources: ");
  printf("\n");
  for (j = 0; j < ResourceCount; j++) {
    printf("%d ", available[j]);
  }
  printf("\n");
  printf("Maximum Allocatable Resources (each row is a process): ");
  printf("\n");
  for (i = 0; i < rowNum; i++) {
    for (j = 0; j < ResourceCount; j++) {
      printf("%d ", max[i][j]);
    }
    printf("\n");
  }
  printf("\n");
  printf("Currently Allocated Resources (each row is a process): ");
  printf("\n");
  for (i = 0; i < rowNum; i++) {
    for (j = 0; j < ResourceCount; j++) {
      printf("%d ", allocation[i][j]);
    }
    printf("\n");
  }
  printf("\n");
  printf("Currently Needed Resources (each row is a process): ");
  printf("\n");
  for (i = 0; i < rowNum; i++) {
    for (j = 0; j < ResourceCount; j++) {
      printf("%d ", need[i][j]);
    }
    printf("\n");
  }
  printf("\n");
}

void *ClientExec(
    void *t) { // Function invoked by a new Client created by pthread_create()

  // Lock the mutex for the following critical section
  // If mutex not available, the Client will sleep until it becomes available
  pthread_mutex_lock(&mutex);

  // Critical section starts here

  // Create arrays to store matrix values
  char *charArray = (char *)malloc(SIZE);
  int *intArray = (int *)malloc(SIZE);
  int number;

  // Put each element in Client i's row of the need matrix into intArray
  for (int i = 0; i < ResourceCount; i++) {
    int number = need[((Client *)t)->index][i];
    intArray[i] = number;
  }

  // Initialize charArray so that it contains 'RQ' and the Client index
  charArray[0] = 'R';
  charArray[1] = 'Q';
  charArray[2] = ' ';
  charArray[3] = ((Client *)t)->index + '0';
  charArray[4] = ' ';

  // Put the need resources from intArray in charArray
  int j = 0;
  for (int i = 5; i < 12; i = i + 2) {
    charArray[i] = intArray[j] + '0';
    charArray[i + 1] = ' ';
    j++;
  }

  printf("        Client %i has started\n", ((Client *)t)->index);

  // Request needed number of resources using charArray
  printf("        Request all needed resources\n");
  printf("        ");
  RQ(charArray);

  // After the request is granted, find the new need matrix, allocation matrix,
  // and available matrix for the given Client
  printf("        New need array:   ");
  for (int j = 0; j < ResourceCount; j++) {
    printf(" %i", need[((Client *)t)->index][j]);
  }
  printf("\n");
  printf("        New allocation array:   ");
  for (int j = 0; j < ResourceCount; j++) {
    printf(" %i", allocation[((Client *)t)->index][j]);
  }
  printf("\n");
  printf("        New available array:   ");
  for (int j = 0; j < ResourceCount; j++) {
    printf(" %i", available[j]);
  }
  printf("\n");

  // Check that the state remains safe after all needed resources are granted
  printf("        State still safe: ");
  int safe = SafetyAlgorithm();
  if (safe == 0) {
    printf(" Yes\n");
  } else {
    printf(" No\n");
  }

  printf("        Client %i has finished\n", ((Client *)t)->index);

  // After Client finishes execution, create another char array and int array to
  // store values of allocated resources
  char *charArrayRL = (char *)malloc(SIZE);
  int *intArray2 = (int *)malloc(SIZE);
  for (int i = 0; i < ResourceCount; i++) {
    number = allocation[((Client *)t)->index][i];
    intArray2[i] = number;
  }

  // Initiate charArrayRL so that it contains 'RL' and the Client index
  charArrayRL[0] = 'R';
  charArrayRL[1] = 'L';
  charArrayRL[2] = ' ';
  charArrayRL[3] = ((Client *)t)->index + '0';
  charArrayRL[4] = ' ';

  // Put the allocated resources from intArray2 to charArrayRL
  j = 0;
  for (int i = 5; i < 12; i = i + 2) {
    charArrayRL[i] = intArray2[j] + '0';
    charArrayRL[i + 1] = ' ';
    j++;
  }

  // Release all allocated resources for the given Client
  printf("        Client is releasing resources\n");
  printf("        ");

  RL(charArrayRL);

  // Display the new available matrix (all resource instances should now be
  // available)
  printf("        Now available:");
  for (int i = 0; i < ResourceCount; i++) {
    printf(" %i", available[i]);
  }
  printf("\n");

  // Critical section ends here

  // Unlock the mutex so that the next Client can access it
  pthread_mutex_unlock(&mutex);

  // Upon completion, terminate the Client
  pthread_exit(0);
  return 0;
}
void Run(Client **clients) { 
  if (pthread_mutex_init(&mutex, NULL) != 0) {
    printf("fail mutex init \n");
    return;
  }

  int isSafe = safetyAlgo();
  print("\nCurrent State:");
  switch(safetyAlgo()){
    case -1:
      print(" Not safe\n");
      print("Currently not in safe state; please enter a different set of "
            "resources"\n);
    case 0:
      print(" Safe\n");
    default:
      printf("Safe Sequence is: < ");
      for (int i = 0; i < rowNum; i++) {
        printf("%i ", safeSeq[i]);
        (*clients)[i].index = safeSeq[i];
        (*clients)[i].order = i;
      }
      printf(">\n");

      sequenceTemp = safeSeq;

      (*clients)[i].retVal = pthread_create(&((*clients)[i].handle), NULL,
                                            threadExec, &((*clients)[i]));
      printf("Now going to execute the clients:\n\n");
      for (int i = 0; i < rowNum; i++) {
        printf("----------------------------------------\n");
        printf("--> Customer/Client %i\n", sequenceTemp[i]);
        // How many resources the Client currently needs
      runloop(collNums, need[sequenceTemp[i]][j]), "Need:");
      runloop(collNums, allocation[sequenceTemp[i]][j], "Allocated resources");
      printf("        Available:   ");
      for (int j = 0; j < ResourceCount; j++) {
        printf(" %i", available[j]);
      }
      printf("\n");

      (*clients)[i].returnVal = pthread_create(&((*clients)[i].handle), NULL,
                                               threadExec, &((*clients)[i]));

      pthread_join((*clients)[i].handle, NULL);
      }

      // After all clients have executed, destroy the mutex
      pthread_mutex_destroy(&mutex);
  }
}
void runloop(int x; int needAlloc; char list[]) {
  printf("%s", list);
  for (int i = 0; i < x; i++){
    print("        %i   ", needAlloc);
  }
  printf("\n");
}

int safetyAlgo(int safeResources, int amountLeft, int done) {
  int safe = 0, resNum = 0, collNs = 0, isFound = 1;
  //safeResources = (int *)malloc(4 * sizeof(int)); // 4 is rowNum
  for (int i = 0; i < 4; i++) {
    safeResources[i] = -1;
  }

  // amountLeft = (int *)malloc(ResourceCount * sizeof(int));
  for (int i = 0; i < ResourceCount; i++) {

    amountLeft[i] = available[i];
  }

 // done = (int *)malloc(rowNum * sizeof(int));

  for (int i = 0; i < rowNum; i++) {
    done[i] = FALSE;
  }

  while(isLocated = 1 && isSafe = 0){
    for (int i = 0; i < rowNum;i++){
      if(!done[i]){
        ResourceCount = 0;
        int j = 0;
        while(j<rowNum){
          if(need[i][j] < 0 || amountLeft[j] < 0){
            isSafe -= 2;
            break;
          }
          if(need[i][j] > amountLeft[j]){
            break;
          }else{
            columnCount++:
          }
          if(columnCount == ResourceCount){
            safeResources[sequenceNum] = i;
            sequenceNum += 1, done[i] = 1, found = 1;
            for (int g = 0; g < ResourceCount; g++){
              amountLeft[g] += allocation[i][g];
            }
          }
        }
      }
    }
  }
 
  for (int i = 0; i < rowNum; i++) {

   
    if (done[i] == FALSE) {
      safe = -1;
    }
  }
  // printf("\n");

  return safe;
}
