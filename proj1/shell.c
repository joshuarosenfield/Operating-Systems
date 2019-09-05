//COP4610
//Project 1
//Joshua Rosenfield, Barrett McKinney, Ryan Kenney
#include <stdio.h>
#include <string.h> //strcmp
#include <stdlib.h> //getenv
#include <unistd.h> //getlogin_r, gethostname, getcwd
#include <sys/stat.h> //check for files

typedef struct
{
	char** tokens;
	int numTokens;
	int exitTotal;	//exit command
} instruction;

void addToken(instruction* instr_ptr, char* tok);
void executeTokens(instruction* instr_ptr);
void clearInstruction(instruction* instr_ptr);
void addNull(instruction* instr_ptr);
char* resolveShortcut(char* path);

int main() {
	char* token = NULL;
	char* temp = NULL;

	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;
	instr.exitTotal = 0;	//initial exit 

	//user login
	char loginarr[256];
	char hostarr[256];
	char cwdarr[256];
 	getlogin_r(loginarr, 256);
	gethostname(hostarr, 256);
	getcwd(cwdarr, 256);
	//tests function for shortcuts
        //char* test = resolveShortcut("/home/kenney/./Downloads/../test.c");
	//printf("%s\n",test);
	while (1) {
		getcwd(cwdarr, 256);
		printf("%s@%s : %s>", loginarr, hostarr, cwdarr);
		//printf("Please enter an instruction: ");

		// loop reads character sequences separated by whitespace
		do {
			//scans for next token and allocates token var to size of scanned token
			scanf("%ms", &token);
			temp = (char*)malloc((strlen(token) + 1) * sizeof(char));

			int i;
			int start = 0;
			for (i = 0; i < strlen(token); i++) {
				//pull out special characters and make them into a separate token in the instruction
				if (token[i] == '|' || token[i] == '>' || token[i] == '<' || token[i] == '&') {
					if (i-start > 0) {
						memcpy(temp, token + start, i - start);
						temp[i-start] = '\0';
						addToken(&instr, temp);
					}

					char specialChar[2];
					specialChar[0] = token[i];
					specialChar[1] = '\0';

					addToken(&instr,specialChar);

					start = i + 1;
				}
			}

			if (start < strlen(token)) {
				memcpy(temp, token + start, strlen(token) - start);
				temp[i-start] = '\0';
				addToken(&instr, temp);
			}

			//free and reset variables
			free(token);
			free(temp);

			token = NULL;
			temp = NULL;
		} while ('\n' != getchar());    //until end of line is reached

		addNull(&instr);
		executeTokens(&instr);
		clearInstruction(&instr);
		instr.exitTotal += 1;
	}

	return 0;
}

//reallocates instruction array to hold another token
//allocates for new token within instruction array
void addToken(instruction* instr_ptr, char* tok)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**) malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	//allocate char array for new token in new slot
	instr_ptr->tokens[instr_ptr->numTokens] = (char *)malloc((strlen(tok)+1) * sizeof(char));
	strcpy(instr_ptr->tokens[instr_ptr->numTokens], tok);

	instr_ptr->numTokens++;
}

void addNull(instruction* instr_ptr)
{
	//extend token array to accomodate an additional token
	if (instr_ptr->numTokens == 0)
		instr_ptr->tokens = (char**)malloc(sizeof(char*));
	else
		instr_ptr->tokens = (char**)realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));

	instr_ptr->tokens[instr_ptr->numTokens] = (char*) NULL;
	instr_ptr->numTokens++;
}

void executeTokens(instruction* instr_ptr)
{
	int i;
	//print tokens
	printf("Printing Tokens:\n");
	for (i = 0; i < instr_ptr->numTokens; i++) {
		if ((instr_ptr->tokens)[i] != NULL)
			printf("%s\n", (instr_ptr->tokens)[i]);
	}
	printf("End print tokens\n");
	//end print tokens
	//start of env implementation
	for(i = 0; i < instr_ptr->numTokens - 1;i++){
		if(instr_ptr->tokens[i][0] == '$'){
			if(getenv(instr_ptr->tokens[i] + 1) == NULL){
				//strcpy(instr_ptr->tokens[1], strcat(instr_ptr->tokens[i]+1,  ": Undefined variable."));	
				printf("%s: Undefined variable.\n", instr_ptr->tokens[i]);
				return;	//end this command
				//TODO::make this only print
			}
			else{	//copys $*** to its token 
				strcpy(instr_ptr->tokens[i],  getenv(instr_ptr->tokens[i] + 1));
				//printf("%s\n", instr_ptr->tokens[i]);
			}	
		}
	}
	//built ins
	if(strcmp(instr_ptr->tokens[0], "exit") == 0){
		printf("Exiting...\n");
		printf("\tCommands executed: %d\n",instr_ptr->exitTotal);
		exit(EXIT_SUCCESS); 
	}
	// cd not finished
	else if(strcmp(instr_ptr->tokens[0], "cd") == 0){
                if(chdir(instr_ptr->tokens[1]) != 0)
                        printf("%s: No such file or directory.\n", instr_ptr->tokens[1]);
        }
	//do we neeed to even do this?
	else if(strcmp(instr_ptr->tokens[0], "echo") == 0){
		for(i = 1;i < instr_ptr->numTokens;i++){
                	if ((instr_ptr->tokens)[i] != NULL)
                        	printf("%s ", (instr_ptr->tokens)[i]);
        	}
		printf("\n");
	}
	//forks and execv 
	else{
	//char *const parmList[] = {"/bin/ls", "-ls", NULL};	//shows needed fromat...used for testing only
		pid_t pid;
		pid = fork();
		if(pid == 0){
			//execv(parmList[0], parmList); //used for testing only
			execv(instr_ptr->tokens[0], instr_ptr->tokens);
			printf("%s: Command not found\n", instr_ptr->tokens[0]);	
		}
		else{
			 while(wait(NULL) != pid);
		}
	}
}

void clearInstruction(instruction* instr_ptr)
{
	int i;
	for (i = 0; i < instr_ptr->numTokens; i++)
		free(instr_ptr->tokens[i]);
	free(instr_ptr->tokens);
	instr_ptr->tokens = NULL;
	instr_ptr->numTokens = 0;
	//instr_ptr->exitTotal = 0;
}

char* resolveShortcut(char* path){
	printf("%s\n",path);
	int i;
	instruction instr;

	int start = 0; //saves location of / in path
	//skip first / in absolute path
	if(path[0] == '/')
		start = 1;

	for(i = start; i <= strlen(path); ++i){
		//stop at every / found in path; save word between start and i as a token
		if(path[i] == '/' || i == strlen(path)){
			char* temp = (char*)malloc((i-start + 1) * sizeof(char));
			int j;
			//extract letters into string
			for(j = 0; j < i-start; ++j)
				temp[j] = path[start+j];
			temp[i-start] = '\0';
			start = i+1;
			if(strcmp(temp, "\0") != 0){
				addToken(&instr,temp);
			}
			printf("%d %s\n", instr.numTokens,instr.tokens[instr.numTokens-1]);
			free(temp);
		}
	}
	addNull(&instr);

	int absolute = 0;
	char* cwd = (char*)malloc(256*sizeof(char));
	if(path[0] == '/'){
		absolute = 1;
		cwd[0] = '/';
	}
	else{
		getcwd(cwd,255);
	}
	//go token by token
	for(i = 0; i < instr.numTokens-1; ++i){
		//return to $HOME if ~ found as first token, else error
		if(strcmp(instr.tokens[i],"~\0") == 0){
			if(i == 0){
				memset(cwd, 0, 256);
				strcpy(cwd,getenv("HOME"));
			}
			else{
				printf("Error: '~' can only be used at the start of the path\n");
				return "\0";				}
		}
		//move up a directory if possible when .. is found, else error
		else if(strcmp(instr.tokens[i],"..\0") == 0){
			int j = strlen(cwd);
			while(cwd[j] != '/'){
				if(j == 1){
					printf("Error: Cannot go past root directory\n");
					return "\0";	
				}
				cwd[j] = '\0';
				--j;
			}
			cwd[strlen(cwd)-1] = '\0';
		}
		//check if word is a file / directory
		else if(strcmp(instr.tokens[i],".\0") != 0){
			struct stat buffer;
			char* temp = (char*)malloc(256*sizeof(char));
			strcpy(temp,cwd);
			if(absolute == 0){
				strcat(temp,"/");
			}
			else{
				absolute = 0;
			}
			strcat(temp,instr.tokens[i]); //build path from cwd + / + token
			stat(temp, &buffer);
			if(S_ISDIR(buffer.st_mode)){
				strcpy(cwd,temp);
			}
			//check if is file and if filename is last token
			else if(S_ISREG(buffer.st_mode)){
				if(i == instr.numTokens-2){
					strcpy(cwd,temp);
				}
				else{
					printf("Error: Files can only appear at the end of a path\n");
					return "\0";
				}
			}
			//misc token / unknown name
			else{
				printf("Error: File / directory '%s' not found\n",instr.tokens[i]);
				return "\0";
			}
			free(temp);
			buffer.st_mode = 0;
		}
		//printf("%s\n",cwd);

	}
	return cwd;
}	
