//COP4610
//Project 1
//Joshua Rosenfield, Barrett McKinney, Ryan Kenney
#include <stdio.h>
#include <string.h> 	//strcmp
#include <stdlib.h> 	//getenv
#include <unistd.h> 	//getlogin_r, gethostname, getcwd
#include <sys/stat.h> 	//check for files
#include <stdbool.h> 	//bool


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
char*  pathResolution(char* cmd);

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
        //char* test = resolveShortcut("input.txt");
	//printf("%s\n",test);
	while (1) {
		getcwd(cwdarr, 256);
		printf("%s@%s : %s>", loginarr, hostarr, cwdarr);
		int invalid_path = 0; //Set if resolveShortcut finds a bad path
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
		
		int j = 0;
		int i = 0;
		//check for . and / in each token to determine if path; if so expand it
		for(i = 0; i < instr.numTokens; ++i){
			for(j = 0; j < strlen(instr.tokens[i]); ++j){
				if(instr.tokens[i][j] == '/' || instr.tokens[i][j] == '.'){
					instr.tokens[i] = resolveShortcut(instr.tokens[i]);
					if(strcmp(instr.tokens[i]," \0") == 0)
						invalid_path = 1;
					break;
				}
			}
		}

		addNull(&instr);
		//Only execute instruction if it has a valid path
		if(invalid_path == 0)
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
	else{
		instr_ptr->tokens = (char**) realloc(instr_ptr->tokens, (instr_ptr->numTokens+1) * sizeof(char*));
	}
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
	bool isRedirect, inputRedirect, outputRedirect;
	isRedirect = false;
        inputRedirect = false;
        outputRedirect = false;
	//print tokens
//	printf("Printing Tokens:\n");
//	for (i = 0; i < instr_ptr->numTokens; i++) {
//		if ((instr_ptr->tokens)[i] != NULL)
//			printf("%s\n", (instr_ptr->tokens)[i]);
//	}
//	printf("End print tokens\n");
	//end print tokens
	
	//char * redirect_path = NULL;
	char * input_path = NULL;
	char * output_path = NULL;

	//execution for <> -- cat PATH NULL > output.txt
	//6 tokens
	if(instr_ptr->numTokens == 6 && (instr_ptr->tokens[1][0] == '<') && (instr_ptr->tokens[3][0] == '>')){
		inputRedirect = true;
        	outputRedirect = true;
		
		input_path = resolveShortcut(instr_ptr->tokens[2]);
		if(*input_path == '\0')
                	return;
		instr_ptr->tokens[1] = input_path;
                instr_ptr->tokens[2] = NULL;
		
		output_path = resolveShortcut(instr_ptr->tokens[4]);
		if(*output_path == '\0')
                	return;
		instr_ptr->tokens[3] = NULL;
                instr_ptr->tokens[4] = NULL;
             
		instr_ptr->numTokens = instr_ptr->numTokens - 3;				
	}
	//execution for >< 
	else if(instr_ptr->numTokens == 6 && (instr_ptr->tokens[1][0] == '>') && (instr_ptr->tokens[3][0] == '<')){
        	inputRedirect = true;
                outputRedirect = true;

		output_path = resolveShortcut(instr_ptr->tokens[2]);
                if(*output_path == '\0')
                        return;
                instr_ptr->tokens[3] = NULL;
                instr_ptr->tokens[2] = NULL;
		
                input_path = resolveShortcut(instr_ptr->tokens[4]);
                if(*input_path == '\0')
                        return;
                instr_ptr->tokens[1] = input_path;
                instr_ptr->tokens[4] = NULL;

                instr_ptr->numTokens = instr_ptr->numTokens - 3;	
	}

	//check for Missing name for redirect
	//execute simple < or > command
	for (i = 0; i < instr_ptr->numTokens - 1; i++) {
                if (instr_ptr->tokens[i][0] == '>' || instr_ptr->tokens[i][0] == '<' || instr_ptr->tokens[i][0] == '|'){
                	isRedirect = true;
			//error checking
			if(isRedirect == true && instr_ptr->numTokens < 4){
         			printf("Missing name for redirect.\n");
                		return;
        		}
			if(instr_ptr->tokens[i][0] == '<'){
                                inputRedirect = true;	
				input_path = resolveShortcut(instr_ptr->tokens[i+1]);
				//if file for redirection doesnt exit 
				if(*input_path == '\0')
					return;
				instr_ptr->tokens[i] = input_path;
				instr_ptr->tokens[i+1] = NULL;
				instr_ptr->numTokens = instr_ptr->numTokens - 1;
			}
			else if(instr_ptr->tokens[i][0] == '>'){
				outputRedirect = true;
				output_path = resolveShortcut(instr_ptr->tokens[i+1]);
				//if file for redirection doesnt exit 
				if(*output_path == '\0')
                                        return;
                                instr_ptr->tokens[i] = NULL;
                                instr_ptr->tokens[i+1] = NULL;
                                instr_ptr->numTokens = instr_ptr->numTokens - 2;
			}
		}
        }

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

	//$PATH resolution
	strcpy(instr_ptr->tokens[0], pathResolution(instr_ptr->tokens[0]));
	
	//built ins
	if(strcmp(instr_ptr->tokens[0], "exit") == 0){
		printf("Exiting...\n");
		printf("\tCommands executed: %d\n",instr_ptr->exitTotal);
		exit(1); 
	}
	// cd not finished
	else if(strcmp(instr_ptr->tokens[0], "cd") == 0){
                if(chdir(instr_ptr->tokens[1]) != 0)
                        printf("%s: No such file or directory.\n", instr_ptr->tokens[1]);
        }
	//do we neeed to even do this?
	/*else if(strcmp(instr_ptr->tokens[0], "echo") == 0){
		for(i = 1;i < instr_ptr->numTokens;i++){
                	if ((instr_ptr->tokens)[i] != NULL)
                        	printf("%s ", (instr_ptr->tokens)[i]);
        	}
		printf("\n");
	}
	*/
	//forks and execv 
	else{
	//char *const parmList[] = {"/bin/cat", "/home/majors/rosenfie/cop4610/proj1/input.txt", NULL};	//shows needed fromat...used for testing only
		int fd0, fd1;
		pid_t pid;
		pid = fork();
		if(inputRedirect){
			fd0 = open(input_path);
		}
		if(outputRedirect){
			fd1 = creat(output_path);
		}
		if(pid == -1){
        		perror("fork error");
		}
		else if(pid == 0){ 
			if(inputRedirect && outputRedirect){
				close(STDOUT_FILENO);
                               	dup(fd1);
				close(fd1);
				
				close(STDIN_FILENO);
				dup(fd0);
				close(fd0);	
			} 
			else if(inputRedirect){
				close(STDIN_FILENO);
				dup(fd0);
				close(fd0);
			
			}
			else if(outputRedirect){
				close(STDOUT_FILENO);
				dup(fd1);	
				close(fd1);
			}
			//execv(parmList[0], parmList); //used for testing only
			execv(instr_ptr->tokens[0], instr_ptr->tokens);
			printf("%s: Command not found\n", instr_ptr->tokens[0]);	
			return;
		}
		else{
			if(inputRedirect)
				close(fd0);
			if(outputRedirect)
                                close(fd1);
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
}

char* resolveShortcut(char* path){
	//printf("%s\n",path);
	int i;
	instruction instr;
	instr.tokens = NULL;
	instr.numTokens = 0;
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
			//printf("%d %s\n", instr.numTokens,instr.tokens[instr.numTokens-1]);
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
				strcpy(cwd, " \0");
				return cwd;				}
		}
		//move up a directory if possible when .. is found, else error
		else if(strcmp(instr.tokens[i],"..\0") == 0){
			int j = strlen(cwd);
			if(j <= 1){
				printf("Error: Cannot go past root directory\n");
                                strcpy(cwd, " \0");
                                return cwd;
			}
			while(cwd[j] != '/'){
				cwd[j] = '\0';
				--j;
			}
			if(j != 0)
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
					strcpy(cwd," \0");
					return cwd;
				}
			}
			//misc token / unknown name
			else{
				printf("Error: File / directory '%s' not found\n",instr.tokens[i]);
				strcpy(cwd, " \0");
				return cwd;
			}
			free(temp);
			buffer.st_mode = 0;
		}
		//printf("%s\n",cwd);

	}
	return cwd;
}	

char* pathResolution(char* cmd){
	//printf("cmd = %s\n", cmd);
	char **paths_array;	//array for possible paths 
	int total_colon, path_length, counter, i;
	total_colon = 0;
	counter = 0;

	//char * env_path = getenv("PATH");

	char * test_path = getenv("PATH");
	int test_path_length = strlen(test_path);
	char env_path[test_path_length];
	strcpy(env_path, test_path);
        
	//printf("env path = %s\n", env_path);
	path_length = strlen(env_path);
	//printf("%d", char_length_path);
	//add up amount of : (paths)
	for(i = 0; i < path_length;i++){
		if(env_path[i] == ':'){
			env_path[i] = '\0';
			total_colon++;	
		}
	}
	//printf("colon count = %d\n" , total_colon);
	//env_path = path with \0 instead of :
	if(total_colon == 0)
		return cmd;	
	paths_array = malloc((total_colon + 1) * sizeof(*paths_array));
	//fill paths array
	paths_array[0] = env_path;
	for(i = 0; i < path_length;i++){
		if(env_path[i] == '\0'){
			paths_array[++counter] = env_path + i + 1;
            }
	}	

	//for(i = 0;i < total_colon;i++){
	//printf("%s\n", paths_array[i]);
	//}

	//need to add cmd to the end of every path then check them if it exist: else error
	//what do i need instead of 400?
	char temp[2056];
	//char *temp = malloc(sizeof(char) * 400);
	for(i = 0; i < total_colon;i++){
		strcpy(temp, paths_array[i]);
		strcat(temp, "/");
		strcat(temp, cmd);
		//printf("%s\n",temp);
		if(access(temp, F_OK) != -1){
			cmd = temp;
			free(paths_array);
			return cmd;
		}		
	}
	//printf("doesnt exist\n");
	free(paths_array);
	return cmd;
}






