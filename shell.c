/**
 * shell.c
 *
 * PURPOSE:		This is a simple shell which provides the option where 
 *				shell variables may be defined. These variables may be 
 *                  used in the shell, to replace other values/commands.
 *				There is also support of preloading and executing "set"
 *				commands from a text file.
 *
 * EXAMPLE:		set $list=ls	//to set variable
 *                  $list		//this actually runs ls (instead of typing 'ls')
 *
 * Proper use of "set":				
 *		set $variablename=value
 *
 * Note: Only one value can be assigned to a single variable.
 *
 * These are not valid uses of "set" (these will give "Invalid command", since not proper syntax):
 *		set
 *		set $
 *		set $variablename
 *		set $variablename=value value2 (not allowed to assign more than one value)
 *					
 *
 * TO COMPILE:		gcc -Wall shell.c -o prog
 * TO RUN:		./prog
 *
 * NOTE:	The .shell_init.txt file is in this folder, but it is hidden
 *		for some reason.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_LENGTH 50
#define ARG_LENGTH 10

enum BOOL{
	false = 0,
	true = 1
};
typedef enum BOOL boolean;

struct ARGUMENT{
	char *var_name;
	char *var_value;
};

typedef struct ARGUMENT Argument;


/**
 * PURPOSE: Search for the variable's actual value. Then replace the variable with
 *		  it's value in the args[]. This is the args[] that will be run in execvp().
 *		  The user has the option of combining multiple variables together: 
 *		  ie. In commandline: $variable1 $variable2 $variable3
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user.
 *		arg_count		The number of arguments.
 * 		ptr:			A pointer to the array of Arugment structs.
 *        s_index:		The size of the array of Argument structs.
 * 
 * OUTPUT PARAMETERS:
 *		None
 *
 */
void process_variable_replacement(char *args[], int arg_count, Argument *ptr, int s_index){


	boolean to_replace = false;
	
	char *variable;
	int arg_index;
	int row;
	int i;
	int index;
	
	if(args[0] != NULL){
		if(strcmp(args[0], "set") != 0){
	
			//find which argument the variable is
			for(row = 0; row < arg_count; row ++){
				if(args[row][0] == '$'){

					variable = strdup(args[row]);
					arg_index = row;
			
					//search the value that corresponds with our variable
					for(i = 0; i < s_index; i ++){
						if(strcmp(ptr[i].var_name, variable) == 0){
						index = i;
						to_replace = true;

						}

					}
					if(to_replace == true){
						//replace the variable with it's specified value as part of arguments
						args[arg_index] = strdup(ptr[index].var_value);
						to_replace = false;
					}
				}
			}
		
		}
	}

}

/**
 * PURPOSE: Processes "set" commands. If command is valid it gets stored in arguments
 *          for future reference.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user.
 *		arguments:	A pointer to the array of Arugment structs.
 *        s_index:		The size of the array of Argument structs.
 * 
 * OUTPUT PARAMETERS:
 *		s_index:		Pass back the size of array of Argument since it 
 *                       may have been updated.
 *
 */
int process_set_command(char *args[], Argument *arguments, int s_index){

	char variable[MAX_LENGTH];
	char value[MAX_LENGTH];
	int variable_index = 0;
	int value_index = 0;
	int col = 0;

	boolean append_variable = true;
	boolean append_value = false;
	boolean invalid = false;
	
	//main processing
	if(args[0] != NULL && args[1] != NULL && args[2] == NULL){
		if(strcmp(args[0], "set") == 0 && args[1][0] == '$'){
	
			int length = strlen(args[1]);
			//if someone wrote "set $" is invalid
			if(length == 1){

				invalid = true;
			}
			col = 0;
			variable_index = 0;
			value_index = 0;
			
			//run through arg[1] which contains both variable and value assigned to it
			for(col = 0; col < length; col ++){
				
				if(append_variable == true){
					variable[variable_index] = args[1][col];
					variable_index ++;
				}
				if(append_value == true){

					value[value_index] = args[1][col];
					value_index ++;
				}
				
				if(args[1][col] == '='){
					variable[col] = '\0';
					append_variable = false;
					append_value = true;
				}
			

			}
			value[value_index] = '\0';
			
			if(append_value == false){
				
				invalid = true;
			}
			
			if(invalid == false){
				//put all values in a struct for reference later
				arguments[s_index].var_name = strdup(variable);
				arguments[s_index].var_value = strdup(value);
				s_index ++;
			}
			else{
				printf("Invalid command\n>");
			}
		}
		else{
			printf("Invalid command\n>");
		}
	}
	else{
		printf("Invalid command\n>");
	}

	return s_index;
	
}

/**
 * PURPOSE: Tokenize a line of text from a file or from a user.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user or from file. Arguments 
 *					are to be stored here.
 *		input[]:		The line of text from a file or from a user.
 * 
 * OUTPUT PARAMETERS:
 *		arg_count:	The number of arguments.
 *
 */
int tokenize_input(char *args[], char input[]){

	int length;
	int count = 0;
	int arg_count = 0;
	
	//need to remove newline at the end of the command
	length = strlen(input);
	if(input[length-1] == '\n'){
		input[length-1] = '\0';
	}

	args[0] = strtok(input," ");

	while (args[count] != NULL) {
		count++;
		args[count] = strtok(NULL, " ");
		arg_count++;

	
	} 
	
	//did this to cut off any extra commands from a previous command
	//that was longer *this is very important
	count++;
	args[count] = '\0';
	

	return arg_count;
}

/**
 * PURPOSE: Fork off a child to execute a command.
 * NOTE:    This was placed in it's own function otherwise code would be duplicated.
 * 
 * INPUT PARAMETERS:
 *		args[]:		The arguments typed in by user. These arguments are
 * 					to be executed by child.
 *
 * OUTPUT PARAMETERS:
 *		None
 *
 */
void fork_off(char *args[]){

	int pid;
	int rc;

	pid = fork();
	if(pid != 0){
		wait(NULL);
	}
	else{
		rc = execvp(args[0],args);
		if(rc != 0){
			if(args[0] != '\0' && strcmp(args[0], "set") != 0){
				printf("Invalid command\n>");
			}
		}
		exit(0);
	}

}


int main(int argc, char *argv[]){

	char input_user[MAX_LENGTH];
	char input_file[MAX_LENGTH];
	char *args[ARG_LENGTH];
	int arg_count;
	
	//an array of structs, this keeps a history of variables and their values
	Argument arguments[MAX_LENGTH];	
	int s_index = 0;
	
	FILE *in;
	in = fopen(".shell_init.txt", "r");
	
	if(in == NULL){
		printf("\n\nUnable to open .shell_init\n\n\n");
		exit(EXIT_FAILURE);
	}
	else{
		printf("\n\nProcessing .shell_init\n\n\n");
	}
	
	//preload commands in .shell.init
	//preloading is done by putting commands in array of structs
	while(fgets(input_file, MAX_LENGTH, in) != NULL){
	
		printf("preloading command: %s", input_file);
		arg_count = tokenize_input(args, input_file);
		//preload commands
		s_index = process_set_command(args, arguments, s_index);

	
	}
	
	//execute command variables
	int i;
	for(i = 0; i < s_index; i ++){
		
		printf("\n\nexecuted command: %s\n", arguments[i].var_name);	//execute this command
		args[0] = strdup(arguments[i].var_value);	//get the actual value to execute
		args[1] = '\0';
		
		//execute
		fork_off(args);
	
	}
	

	printf("\nCompleted executing commands. Control given to user.");
	printf("\n>");
	while(fgets(input_user, MAX_LENGTH, stdin) != NULL){

		printf(">");

		arg_count = tokenize_input(args, input_user);
		
		
		//show usage
		if(args[0] != NULL && args[1] == NULL){
			if(strcmp(args[0], "set") == 0){
				printf("Usage: set $variablename=value\n>");	
			}
		
		}
		if(args[0] != NULL && args[1] != NULL){
			if(strcmp(args[0], "set") == 0){
				s_index = process_set_command(args, arguments, s_index);
			}
		}
		//process possible variables, not a set command
		if(args[0] != NULL){
			if(strcmp(args[0], "set") != 0){		
				process_variable_replacement(args, arg_count, arguments, s_index);
			}
		}
		
		//don't want to execute a "set" command because we are the ones to process that
		if(args[0] != '\0' && strcmp(args[0], "set") != 0){
			fork_off(args);
		}
		
	}

	











	return 0;

}

