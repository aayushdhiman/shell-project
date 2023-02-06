#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include "vect.h"
#include "tokens.h"
#include <assert.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int MAX_LINE = 255;
int should_run = 1;

void doExecInputOutput(vect_t* v, int* in, int* out, int* cloneIn, int *cloneOut);
void doExecOutput(vect_t* v, int* out, int *cloneOut);
void doExecInput(vect_t *v, int* in, int *cloneIn);
int openReadFile(int *in, char* path);
int openWriteFile(int *out, char* path);
void decideExec(vect_t *v, int* in, int* out, int *cloneIn, int *cloneOut);
void handleNonBuiltIn(int *looper, char **current, vect_t *v, int *in, int *out, vect_t *assemblePrev, vect_t *prev);
void handleBuiltIn(int *looper, char **current, vect_t *v, int *in, int *out, vect_t *assemblePrev, vect_t *prev);
int isBuiltInToken(char** current);
void hasBuiltIn(int *looper, char **current, vect_t *v, int* cloneIn, int* cloneOut, vect_t *assemblePrev, vect_t *prev);
void runLoop(vect_t *v);
void vectToArray(vect_t *input, char **charArray);
void doExec(vect_t *input);
void doExecSource(int *looper, char **current, vect_t *v, int* cloneIn, int* cloneOut, vect_t *assemblePrev, vect_t *prev);
void doExecPrev(int *looper, char **current, vect_t *v, int* cloneIn, int* cloneOut, vect_t *assemblePrev, vect_t *prev);

//starts the shell loop
int main(int argc, char **argv) {
  printf("Welcome to mini-shell.\n");
  // TODO: Implement your shell's main
  vect_t *v = vect_new();
  //vect_t *p = vect_new();
  runLoop(v);
  printf("\nBye bye.\n");
  return 0;
}

//runs the main shell loop
void runLoop(vect_t *v){
  // Loop counter and easy way to break out of loop
  int looper = 1;
  int cloneIn = dup(0);
  int cloneOut = dup(1);
  vect_t *assemblePrev = vect_new();
  vect_t *prev = vect_new();
  while(looper) { //main shell loop, gets input, processes, and repeats
    // Prints "shell $ " at the start of every iteration of loop
    printf("\nshell $");
    // Uses global constant to create an input array - enforces max line size
    char input[MAX_LINE];
    // Populates input array
    fgets(input, MAX_LINE, stdin);

    char **tokens = get_tokens(input);
    assert(tokens != NULL);

    char **current = tokens;
    //    printf("\ncurrent: %s", *current);
    v = vect_new();
    //printf("\nright after vectnew");
    
    //printf("\n coming to has built in");
    hasBuiltIn(&looper, current, v, &cloneIn, &cloneOut, assemblePrev, prev);
  }
}

//figures out if the next command is built on or not
void hasBuiltIn(int *looper, char **current, vect_t *v, int* cloneIn, int* cloneOut, vect_t *assemblePrev, vect_t *prev) {
  //printf("\nin hasbuiltin");
  //printf("\n current hasnbuiltin: %s", *current);
  if(feof(stdin)){
    //printf("\nfeof reached in hasbuiltin");
    vect_delete(v);
    *looper = 0;
  } else if(isBuiltInToken(current) == 1) {
    handleBuiltIn(looper, current, v, cloneIn, cloneOut, assemblePrev, prev);
  }else {
    //printf("\napproaching handle non build");////////////////////////////////;
    handleNonBuiltIn(looper, current, v, cloneIn, cloneOut, assemblePrev, prev);
  }
}

int isBuiltInToken(char **current) {
  if(strcmp(*current, "help") == 0
     || strcmp(*current, "prev") == 0
     || strcmp(*current, "cd") == 0
     || strcmp(*current, "source") == 0
     || strcmp(*current, "pwd") == 0) {
    //printf("\nincorrect isBuilt");////////////////////////////////////////////
    return 1;
  }
  return 0;
}

//Executes non-special commands
void doExec(vect_t *input) {
  //printf("\n insnide regular exec");//////////////////////////////
  char* charArray[1 + vect_size(input)];
  vectToArray(input, charArray); //convert vect to str array

  pid_t child = fork();//fork
  if(child == 0) {
    if(execvp(charArray[0], charArray) == -1) {//if it not works
      printf("\n%s", charArray[0]); //command not found
      printf(": command not found");
    }
    //printf("command worked");
    //otherwise it should work (prayge)
  }else if(child > 0) {
    wait(NULL);//wait for child to finish?
  } else  {
    perror("\nfork failed");
    exit(1);
  }
}


//considering adding this to clean the while(1)
// agreed, we have to rework while(1)
//comment it out if u don't like it or just ignore

void handleNonBuiltIn(int *looper, char *current[], vect_t *v, int *cloneIn, int *cloneOut, vect_t *assemblePrev, vect_t *prev) {
  int input = 0;
  int output = 1;
  //printf("approaching while of nonBuiltIn");///////////////////////////////////////////////
  while(1) {
    //printf("\nin while for non built in");
    //printf("\n current: %s", *current);
    //printf("\n freezes here");
    if(feof(stdin)){
      printf("got to feof");
      vect_delete(v);
      *looper = 0;
      break;
    } else if(*current == NULL) {
      //printf("\n null curent");////////////////////////////
      decideExec(v, &input, &output, cloneIn, cloneOut);//picks which exe to run, frees v
      prev = assemblePrev;
      assemblePrev = vect_new();
      break;
    } else if(strcmp(*current, "exit") == 0) {
      //printf("\n freedom time");//////////////////////////
      vect_delete(v);//free v before we go :)
      *looper = 0;//exits shell
      break;
    } else if(strcmp(*current, ";") == 0) {
      decideExec(v, &input, &output, cloneIn, cloneOut);//picks which exe to run
      v = vect_new(v);//remakes v (exe frees memory);
      vect_add(assemblePrev, *current);
      ++current;//next token
      vect_add(assemblePrev, *current);
      hasBuiltIn(looper, current, v, cloneIn, cloneOut, assemblePrev, prev);//checks if next expresion is built in
      break;//we're done with this loop
    }else if(strcmp(*current, "<") == 0) {
      vect_add(assemblePrev, *current);
      ++current;//get file token
      vect_add(assemblePrev, *current);
      if(openReadFile(&input, *current) == -1) {//if cannot open input file for read
        printf("Failed to read file");
        break;
      }
      current++;//next token after file
      vect_add(assemblePrev, *current);
    }else if(strcmp(*current, ">") == 0) {
      vect_add(assemblePrev, *current);
      ++current;//get file token
      if(openWriteFile(&output, *current) == -1) {//if cannot open output file for write
        printf("Failed to write to file");
        break;
      }
      ++current;//token after file token
      vect_add(assemblePrev, *current);
    }else if(strcmp(*current, "|") == 0) {
      output = fileno(tmpfile());////////////////////////////////
      doExecInputOutput(v, &input, &output, cloneIn, cloneOut);//run exe with input and output
      vect_delete(v);//free v
      vect_new(v);//remake v
      vect_add(assemblePrev, *current);
      ++current;
      vect_add(assemblePrev, *current);
      if(fcntl(input, F_GETFD) == -1) {
        printf("USELESS FILE");
      }
    }else {
      //printf("\nadded to vector: %s", *current);/////////////////////////////////
      vect_add(assemblePrev, *current);
      //printf("\n assemblePrev size in notbuiltin: %d", vect_size(assemblePrev));
      vect_add(v, *current);//add to command
      ++current;
    }
  }
}

//decides which exec to use depending on if an input and output are chosen
void decideExec(vect_t *v, int *input, int *output, int *cloneIn, int *cloneOut) {
  //printf("\nexe decision making");//////////////////////////////////////
  //printf("\n decide input %d", *input);////////
  //printf("\n decide output %d", *output);//////////
  if(*input == 0 && *output == 1) {
    //printf("\n heading to vanilla exe");////////////////////
    doExec(v);
  }else if(*input == 0) {
    //printf("\n heading to output exe");////////////
    doExecOutput(v, output, cloneOut);
  }else if(*output == 1) {
    //printf("\n heading to input exe");/////////////
    doExecInput(v, input, cloneIn);
  }else {
    //printf("\n heading to in out exe");////////////////
    doExecInputOutput(v, input, output, cloneIn, cloneOut);
  }
  vect_delete(v);
}

//helper that attempts to open a read file, and returns -1 if cannot for some reason
int openReadFile(int *input, char *path) {

  if(*input != 0) {
    return -1;
  }

  *input = open(path, O_RDONLY);
  if(*input == -1) {
    return -1;
  }
  return 0;
}

//helper that attempts to open a write file, and returns -1 if it cannot for some reason
int openWriteFile(int *output, char *path) {
  if(*output != 1) {
    close(*output);
  }
  *output = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if(*output == -1) {
    return -1;
  }
  return 0;
}

//ame as last ocmment
//essentially the same as handle non built ins but output and pipe not needed
void handleBuiltIn(int *looper, char **current, vect_t *v, int *cloneIn, int *cloneOut, vect_t *assemblePrev, vect_t *prev) {
  //printf("\nhandle built in");
  //printf("\n current is %s", *current);
  while(1){
    if(feof(stdin)){
      vect_delete(v);
      *looper = 0;
      break;
    } else if(*current == NULL){
      //printf("\n null current wooo");
      //vect_add(assemblePrev, *current);
      prev = assemblePrev;
      assemblePrev = vect_new();
      break;
    } else if(strcmp(*current, "exit") == 0 || feof(stdin)) {
      //printf("\n exit reached");
      vect_delete(v);
      *looper = 0;
      break;
    } else if(strcmp(*current, "cd") == 0) {
      //printf("\ngot a cd");
      vect_add(assemblePrev, *current);
      ++current;
      vect_add(assemblePrev, *current);
      //printf("\n current (should be path): %s", *current);
      if(chdir(*current) == -1){
	//printf("Error in chdir");
	if(*current == NULL){
	  if(chdir("..") == -1){
	    printf("\n%s: Directory not found", *current);
	  }
	}
      }
    } else if(strcmp(*current, "source") == 0){
      //printf("\n got a source");
      vect_add(assemblePrev, *current);
      ++current; // sets current to the script file
      vect_add(assemblePrev, *current);
      doExecSource(looper, current, v, cloneIn, cloneOut, assemblePrev, prev);
    } else if(strcmp(*current, "prev") == 0) {
      //printf("\n got a prev");
      //printf("\nassemblePrev size: %d", vect_size(assemblePrev));
      for(int i = 0; i < vect_size(assemblePrev); i++){
	vect_add(prev, vect_get(assemblePrev, i));
      }
      
      doExecPrev(looper, current, v, cloneIn, cloneOut, assemblePrev, prev);
      ++current;
    } else if(strcmp(*current, "help") == 0){
      //printf("\n got a help");
      printf("\n Built-in commands: \ncd <path>: changes the current working directory to specified path.");
      printf("\nsource <filename>: executes the specified file as a script.");
      printf("\nprev: executes the previously run command");
      printf("\npwd: prints the current working directory");
      printf("\nhelp: prints this message.");
      vect_add(assemblePrev, *current);
      ++current;
      vect_add(assemblePrev, *current);
      // print help function lol
    } else if(strcmp(*current, "pwd") == 0){
      //printf("\n got to pwd\n");
      printf("%s\n", getenv("PWD"));
      vect_add(assemblePrev, *current);
      ++current;
      vect_add(assemblePrev, *current);
      
    } else {
      //printf("add to vector");
      vect_add(v, *current);
      vect_add(assemblePrev, *current);
      ++current;
    }
  }
}

void doExecSource(int *looper, char **filename, vect_t *v, int *cloneIn, int *cloneOut, vect_t *assemblePrev, vect_t *prev){
  pid_t child = fork();
  if(child == 0){
    int fd = open(*filename, O_RDONLY);
    if(fd == -1){
      perror("Error - error with fd open");
      exit(0);
    }
    //printf("\nnot sure what to do after fd");
    dup2(fd, 0);

    // run shell on in
    // tokenize stdin
    int looperSource = 1;
    vect_t *vsource = vect_new();
    vect_t *assemblePrevSource = vect_new();
    vect_t *prevSource = vect_new();
    int cloneInSource = dup(0);
    int cloneOutSource = dup(1);
    while(looperSource){  
      char inputSource[MAX_LINE];
      fgets(inputSource, MAX_LINE, stdin);
      char **tokensSource = get_tokens(inputSource);
      assert(tokensSource != NULL);
      char **currentSource = tokensSource;

      // this needs to break out by itself - it does using the EOF
      hasBuiltIn(&looperSource, currentSource, vsource, &cloneInSource, &cloneOutSource, assemblePrevSource, prevSource);
    }
    
    dup2(0, *cloneIn);
    vect_delete(vsource);
    vect_delete(assemblePrevSource);
    vect_delete(prevSource);
  } 
}

void doExecPrev(int *looper, char **current, vect_t *v, int *cloneIn, int *cloneOut, vect_t *assemblePrev, vect_t *prev){
  //printf("\nin prev");
  // looper = exiting shell
  // current = tokenized prev
  // v = existing vector
  // clone in and clone out = existing stdin/out
  // assembleprev = new vect
  // prev = old assembleprev

  char inputPrev[MAX_LINE];

  int i = 0;
  //printf("\nprevsize: %d", vect_size(prev));
  while(i < vect_size(prev)) {
    inputPrev[i] = *vect_get(prev, i);
    ++i;
  }
  //  inputPrev[vect_size(prev)] = NULL;

  //printf("%s", inputPrev);
  
  char** tokensPrev = get_tokens(inputPrev);
  assert(tokensPrev != NULL);
  char **currentPrev = tokensPrev;
  v = vect_new();
  //printf("\nERROR RIGHT HERE");
  hasBuiltIn(looper, currentPrev, v, cloneIn, cloneOut, assemblePrev, prev);  
}



//runs command using *input as the input to it
void doExecInput(vect_t *command, int *input, int *cloneIn) {
  //printf("\n in exec input");///////////////////////////
  char* charArray[1+vect_size(command)];
  vectToArray(command, charArray);

  pid_t child = fork();
  if(child == 0) {
    //printf("\nvalue of input %d", *input);///////////////
    dup2(*input, 0);
    //printf("\n post dup");/////////////////////
    //int fd = open(file, O_RDONLY);
    //printf("\n input is %d", *input);/////////////////////
    if(execvp(charArray[0], charArray) == -1) {
      printf("\n%s", charArray[0]);
      printf(" : command not found");
    }
  }else if(child > 0) {
    wait(NULL);
    //printf("\n post null");////////////
    close(*input);
    //printf("\n post clone");/////////////
    *input = dup2(*cloneIn,0);
    //printf("\n post dup");//////////////
  }else {
    perror("FORK FAILED");
    exit(1);
  }
}

//runs a command using *output as the commands output
void doExecOutput(vect_t *command, int *output, int *cloneOut) {
  //printf("\n in exec output");
  char* charArray[1+vect_size(command)];
  vectToArray(command, charArray);

  pid_t child = fork();
  if(child == 0) {
    //int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC , 0644);
    dup2(*output, 1);
    if(execvp(charArray[0], charArray) == -1) {
      printf("\n%s",charArray[0]);
      printf(" : command not found");
    }
  }else if(child > 0) {
    wait(NULL);
    close(*output);
    *output = dup2(*cloneOut, 1);//sus code
  } else {
    perror("FAILED TO FORK");
    exit(1);
  }
}

//runs a command using *input and *output as it's input and output 
void doExecInputOutput(vect_t *command, int* input, int* output, int *cloneIn, int* cloneOut) {

  char* com[1+vect_size(command)];
  vectToArray(command, com);

  //printf("\n in pipe is %d", *input);///////////////
  //printf("\n out pipe is %d", *output);//////////////
  pid_t child = fork();

  if(child == 0) {
    dup2(*input, 0);
    dup2(*output, 1);

    if(execvp(com[0], com) == -1) {
      printf("\n%s", com[0]);
      printf(" : command not found");
    }

  }else if(child > 0) {
    wait(NULL);

    close(*input);

    rewind(fdopen(*output, "rw"));
    *input = *output;
    //printf("\n input is now output???? %d", *input);//////////
    //printf("\n output is now %d", *output);//////////
    *output = dup2(*cloneOut, 1);
    dup2(*cloneIn, 0);

    //printf("\n double checking input %d ", *input);/////////////
    //printf("\n double checking output %d ", *output);////////////
  }else {
    perror("ERROR FORKING TO CHILD");
    exit(1);
  }
}


//converts a vector into an array that can be passed into 
void vectToArray(vect_t *input, char** charArray) {
  int i = 0;
  while(i < vect_size(input)) {
    charArray[i] = (char* ) vect_get(input, i);
    ++i;
  }
  charArray[vect_size(input)] = NULL;
}
