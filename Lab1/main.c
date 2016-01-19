#include <unistd.h> 
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
//////
/// dont wanna use static, ruins thread safety
int verboseFlag;
int errFlag;
int numFds;
int numThreads;
int* fds;
pid_t* threads;
int maxFds;
int maxThreads;
int flags = 0;
//////

int parseArgs(int* expected_arg_num,int i, int argc,char** dest_arg_arr, char** argv) {
  int counter = 1; //starts at 1 because optarg contains the first required argument
  while (i < argc) {
    if (strstr(argv[i],"--")) {
      break; //if we hit a -- option
    }
    if (counter == *expected_arg_num) {
      *expected_arg_num = *expected_arg_num * 2;
      dest_arg_arr = realloc(dest_arg_arr, (*expected_arg_num + 1) * sizeof(char*));
    }
    
    dest_arg_arr[counter] = argv[i]; //maybe strcopy?
    i++; counter++;
  }
  optind = i;
  dest_arg_arr[counter]='\0';
  return counter;
}

void checkMem(){
  if(numFds >= maxFds){
    maxFds*=2;
    ///     if ((fds = realloc(fds, maxFds)) == NULL){
    fds = realloc(fds, maxFds*sizeof(int));
    if (fds== NULL){
      fprintf(stderr,"Error Reallocating Memory of Fds\n");
      exit(EXIT_FAILURE);
    }
  }
  if(numThreads >= maxThreads){
    maxThreads*=2;
    threads = realloc(threads, maxThreads);
    if (threads== NULL){
      fprintf(stderr,"Error Reallocating Memory of Threads\n");
      exit(EXIT_FAILURE);
    }

  }
}

int OpenFile(int c){
  if (verboseFlag) {
    //this is a complicated print
    printf("--%sonly %s\n", ((c=='r') ? "rd" : "wr"),optarg);
  }
    //check existence for errors
    //use a switch on c to create  a string R_OK, W_OK, etc to use in access, calling it s???
    //seemslike a lot of ppl juts use F_OK though
  //if (access(optarg, s) != -1
  //switch(c){ case'r':
    if (access(optarg, F_OK) == -1) {
      fprintf(stderr, "Error: %s does not exist\n", optarg);
      return 0;
    }
    fds[numFds] = open(optarg, flags);
    if (fds[numFds] < 0) {
      fprintf(stderr,"Error opening : %s\n", optarg);
      return 0;
    }
    numFds++; 
    checkMem();
    return numFds-1;
}
int checkFD(int fd) {
  if (fd < 0 || fd > numFds) return 0;
  return (fcntl(fds[fd], F_GETFD) != -1);
}

int main (int argc, char **argv){
  ///might want to make these static globals so that we can easily write checkmem; EDIT: moved them
  numFds = 0;
  numThreads = 0; ///proccesses
  fds = malloc(100*sizeof(int)); /// 50 fd's good? 100? 500?
  threads = malloc(100*sizeof(int));
  if ((fds == NULL) || (threads == NULL)){
    fprintf(stderr,"Error Allocating Initial Memory\n");
  }
  maxFds = 100;
  maxThreads = 100;
  while(1){
    static struct option long_opts[] = 
      {
	{"verbose", no_argument, NULL, 'v'},
	{"command", required_argument, NULL, 'c'},
	{"rdonly",required_argument,NULL, 'r'},
	{"wronly",required_argument,NULL,'w' },
	//more options go here
	{0,0,0,0}
      };
    int x = getopt_long(argc, argv,"",long_opts,NULL); //Not null, &some_option_index
    if (x== -1)
      break;
    switch (x) {

    case 'v': {
      verboseFlag=1;
      break;
    }
      //          case rdonly f:
      //open file f for reading
      /////////////
    case 'r': {
      if (optarg)
	{
	  //optarg is our string
	  flags |= O_RDONLY;
	  int fd= OpenFile(x);
	  if ((fd < 0) || !checkFD(fd)){
	    fprintf(stderr,"Error in opening file %s\n",optarg);
	    errFlag = 1;
	  }
	  flags = 0;
	}
      else {
	fprintf(stderr,"No arguments provided for --rdonly\n");
	errFlag = 1;
      }
      break;
    }
      //          case wronly f:
      //open file f for writing
    case 'w': {
      if (optarg)
	{
	  //optarg is our string
	  flags|= O_WRONLY;
	  int fd= OpenFile(x);
	  if ((fd < 0) || !checkFD(fd)){
	    fprintf(stderr,"Error in opening file %s",optarg);
	    errFlag = 1;
	  }
	  flags= 0;
	}
      else {
	fprintf(stderr,"No arguments provided for --wronly\n");      
	errFlag = 1;
      }
      break;
    }
    case 'c': {
      int numArgs = 4; 

      int _stdin, _stdout, _stderr;
      char** arg_array = malloc((numArgs+1) * sizeof(char*));
      if (optarg == NULL) {
	fprintf(stderr,"Not enough arguments for --command, found 0 need 4");
	errFlag = 1;
	break;
      }
      arg_array[0] = optarg;
      char** command_arg = arg_array+3;
      //parse args, if correct apply to optind for next options
      int returnVal = parseArgs(&numArgs,optind,argc,arg_array,argv);
      if (returnVal < 4) {
	fprintf(stderr,"Not enough arguments for --command, found %d need 4",returnVal);
	errFlag = 1;
	break;
      }
      if (verboseFlag) {
	printf("--command");
	for (int i = 0; i < returnVal; i++){
	  printf(" %s",arg_array[i]);
	}
	printf("\n");
        fflush(stdout);
      }
      //error checking //check if fd's are valid
      sscanf(arg_array[0],"%d",&_stdin);
      sscanf(arg_array[1],"%d",&_stdout);
      sscanf(arg_array[2],"%d",&_stderr);
      if (!checkFD(_stdin) || !checkFD(_stdout) || !checkFD(_stderr))
	{
	  fprintf(stderr,"ERROR in File Descriptor Options: %d %d %d\n",_stdin,_stdout,_stderr);
	errFlag = 1;
	}
      char * file = arg_array[3];
      pid_t child_pid = fork();
      if (child_pid==0) {
	//ChildProcess
	dup2(fds[_stdin],0); // actually go into file descriptor array
	dup2(fds[_stdout],1); // same for these
	dup2(fds[_stderr],2);
	execvp(file,command_arg);
	//print error if this comes back
	fprintf(stderr,"ERROR in command: %s\n",file);
	errFlag = 1;
	exit(EXIT_FAILURE);
      }
      else if (child_pid==-1) {
	fprintf(stderr,"ERROR in command: %s\n",file);
	errFlag = 1;
      }
      else {
	threads[numThreads++] = child_pid;
	checkMem();
      }
      free(arg_array);
      break;
    }
    case '?':
      break;
    }
  }
  free(fds);
  free(threads);
  for (int k = 0; k < numFds; k++){
  	close(fds[numFds]);
  	numFds--;
  }
  return errFlag;
}

  