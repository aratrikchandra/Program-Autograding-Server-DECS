#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <pthread.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>

const int BUFFER_SIZE = 1024;
const int MAX_FILE_SIZE_BYTES = 4;


void error(char *msg) {
  perror(msg);
  exit(1);
}

//Utility Function to receive a file of any size to the grading server
int recv_file(int sockfd, char* file_path)
//Arguments: socket fd, file name (can include path) into which we will store the received file
{
    char buffer[BUFFER_SIZE]; //buffer into which we read  the received file chars
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "wb");  //Get a file descriptor for writing received data into file
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }

	
	//buffer for getting file size as bytes
    char file_size_bytes[MAX_FILE_SIZE_BYTES];
    //first receive  file size bytes
    if (recv(sockfd, file_size_bytes, sizeof(file_size_bytes), 0) == -1)
    {
        perror("Error receiving file size");
        fclose(file);
        return -1;
    }
   
    int file_size;
    //copy bytes received into the file size integer variable
    memcpy(&file_size, file_size_bytes, sizeof(file_size_bytes));
    
    //some local printing for debugging
    // printf("File size is: %d\n", file_size);
    
    //now start receiving file data
    size_t bytes_read = 0, total_bytes_read =0;;
    while (true)
    {
    	  //read max BUFFER_SIZE amount of file data
        bytes_read = recv(sockfd, buffer, BUFFER_SIZE, 0);

        //total number of bytes read so far
        total_bytes_read += bytes_read;

        if (bytes_read <= 0)
        {
            perror("Error receiving file data");
            fclose(file);
            return -1;
        }

		//write the buffer to the file
        fwrite(buffer, 1, bytes_read, file);

	// reset buffer
        bzero(buffer, BUFFER_SIZE);
        
       //break out of the reading loop if read file_size number of bytes
        if (total_bytes_read >= file_size)
            break;
    }
    fclose(file);
    return 0;
}
//Utility Function to send a file of any size to the grading server
int send_file(int sockfd, char* file_path)
//Arguments: socket fd, file name (can include path)
{
    char buffer[BUFFER_SIZE]; //buffer to read  from  file
    bzero(buffer, BUFFER_SIZE); //initialize buffer to all NULLs
    FILE *file = fopen(file_path, "rb"); //open the file for reading, get file descriptor 
    if (!file)
    {
        perror("Error opening file");
        return -1;
    }	

	//now send the source code file 
    while (!feof(file))  //while not reached end of file
    {
    
    		//read buffer from file
        size_t bytes_read = fread(buffer, 1, BUFFER_SIZE -1, file);
        
     		//send to server
        if (send(sockfd, buffer, bytes_read+1, 0) == -1)
        {
            perror("Error sending file data");
            fclose(file);
            return -1;
        }
        
        //clean out buffer before reading into it again
        bzero(buffer, BUFFER_SIZE);
    }
    //close file
    fclose(file);
    return 0;
}

char* compile_command(int id, char* programFile, char* execFile) {

  char *s;
  char s1[20];
  
  s = (char *) malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));
  strcpy(s, "gcc -o ");
  strcat(s, execFile);
  strcat(s, "  ");
  strcat(s, programFile);
  strcat(s, " 2> compiler_err");
 	sprintf(s1, "%d", id);	
 	strcat(s, s1);	
  strcat(s, ".txt");
//   printf("%s\n",s);
  return s;
}
    
char* run_command(int id, char* execFileName) {

  char *s;
  char s1[20];
  
  s = (char *)malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));
 	sprintf(s1, "%d", id);	  

  strcpy(s, "./");
  strcat(s, execFileName);
  strcat(s, " > output");
 	strcat(s, s1);	
  strcat(s, ".txt");
  strcat(s, " 2> runtime_err");
 	strcat(s, s1);	
 	strcat(s, ".txt");	
//   printf("%s\n",s);
  return s;
}


char *makeProgramFileName(int id) {

  char *s;
  char s1[20];	
  
  s = (char *)malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));

  sprintf(s1, "%d", id);	  
  strcpy (s, "file");
  strcat (s, s1);
  strcat (s, ".c");
  return s;
}  
  
char *makeExecFileName(int id) {

  char *s;
  char s1[20];	
  
  s = (char *)malloc (200*sizeof(char));
  memset(s, 0, sizeof(s));
  memset(s1, 0, sizeof(s1));
  sprintf(s1, "%d", id);	  
  strcpy (s, "prog");
  strcat (s, s1);
  return s;
} 

// Function to construct the compiler error output file name
char *makeCompileErrorFilename(int id) {
    char *filename;
    char id_str[20];

    filename = (char *)malloc(200 * sizeof(char));
    memset(filename, 0, sizeof(filename));
    memset(id_str, 0, sizeof(id_str));

    // Construct the file name with the format "compiler_err<ID>.txt"
    strcpy(filename, "compiler_err");
    sprintf(id_str, "%d", id);
    strcat(filename, id_str);
    strcat(filename, ".txt");

    return filename;
}
// Function to construct the runtime error output file name
char *makeRuntimeErrorFilename(int id) {
    char *filename;
    char id_str[20];

    filename = (char *)malloc(200 * sizeof(char));
    memset(filename, 0, sizeof(filename));
    memset(id_str, 0, sizeof(id_str));

    // Construct the file name with the format "compiler_err<ID>.txt"
    strcpy(filename, "runtime_err");
    sprintf(id_str, "%d", id);
    strcat(filename, id_str);
    strcat(filename, ".txt");

    return filename;
}

// Function to generate output filename
char *makeOutputFilename(int requestID) {
    char *filename;
    filename= (char *)malloc(200 * sizeof(char));
    sprintf(filename, "output%d.txt", requestID);
    return filename;
}

// Function to generate output diff filename
char *makeOutputDiffFilename(int requestID) {
    char *filename = (char *) malloc(200 * sizeof(char));
    sprintf(filename, "output_diff%d.txt", requestID);
    return filename;
}

// Function to construct the output check command
char *output_check_command(int id, char *outputFile) {
    char *s = (char *) malloc(200 * sizeof(char));
    memset(s, 0, sizeof(s));

    // Construct the command to echo the expected output and compare it with the actual output
    sprintf(s, "echo '1 2 3 4 5 6 7 8 9 10' | diff -B -Z - %s > output_diff%d.txt", outputFile, id);

    return s;
}