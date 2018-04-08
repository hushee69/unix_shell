/*
 * Authors : CISSE Demba
 * 			JANDU Harry
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define BUFFER_ALLOC				8
#define QUIT						"quit"
#define EXIT						"exit"

static volatile int ne_ferme_jamais = 1;

void die_error(char *err_msg)
{
	perror(err_msg);
	exit(-1);
}

void free_memory(void **mem)
{
	if( (*mem) )
	{
		free(*mem);
		*mem = NULL;
	}
	
	return;
}


void ctrl_c_handler(int signum)
{
	if( signum == SIGINT )
	{
		printf("Caught Ctrl-C signal\n");
	}
	
	exit(-1);
}

int get_current_string_size(char *str_start, int delim)
{
	int cur_size = 0;
	
	while( *str_start != delim && *str_start != '\0' && *str_start != '\n' && *str_start != '\n' )
	{
		cur_size++;
		*str_start++;
	}
	
	return cur_size;
}

// lit en tant que chaine n'est pas 'quit' ou 'exit'
int get_delimited_input(char **data, int delim, FILE *fp)
{
	int bytes_written = 0, bytes_read = 0, resize = BUFFER_ALLOC;
	
	char *buffer = *data, *temp = NULL;
	char c;					// to read one byte in loop
	
	if( fp == NULL )
	{
		fp = stdin;
	}
	
	if( buffer == NULL )
	{
		buffer = (char*)malloc(sizeof(char) * BUFFER_ALLOC);
	}
	
	do
	{
		if( bytes_written >= ( resize - 1 ) )
		{
			resize = resize + bytes_written + BUFFER_ALLOC;
			
			temp = (char*)realloc(buffer, resize);
			if( temp == NULL )
			{
				die_error("get_delimited_input realloc");
			}
			buffer = temp;
		}
		
		bytes_read = fread(&c, sizeof(char), 1, fp);
		buffer[bytes_written] = c;
		
		bytes_written = bytes_written + bytes_read;
	}while( (int)c != delim && (int)c != EOF && bytes_read > 0 );
	
	buffer[bytes_written - 1] = '\0';
	bytes_written = bytes_written - 1;
	
	*data = buffer;
	
	return bytes_written;
}

char **fill_arguments(char *input_str, int *args_len)
{
	int i = 0, j = 0;
	int argument_count = 0, resize = BUFFER_ALLOC;
	
	char **arguments_array = (char**)calloc(resize, sizeof(char*));
	int len = 0;
	
	while( *input_str )
	{
		len = get_current_string_size(input_str, (int)' ');
		
		if( len > 0 )
		{
			if( argument_count >= ( resize - 1 ) )
			{
				
			}
			
			arguments_array[argument_count] = (char*)malloc(sizeof(char) * (len + 1));
			memset(arguments_array[argument_count], '\0', (len + 1));
			
			strncpy(arguments_array[argument_count], input_str, (len));
			argument_count++;
			input_str = input_str + len;
		}
		else
		{
			*input_str++;
		}
	}
	
	arguments_array[argument_count] = NULL;
	
	if( *args_len )
	{
		*args_len = argument_count - 1;
	}
	
	return arguments_array;
}

int read_commands(FILE *fp)
{	
	int init_buff = BUFFER_ALLOC;
	char *buf = NULL;
	char **args_array = NULL;
	
	// flag pour background running des applications avec '&'
	char quit_msg[5] = {0}, flag = 0, is_cd_command = 0;
	int bytes_read = 0, args_len;
	int i = 0;			// pour free args_array
	
	pid_t fils, fils_2;			// fils_2 pour background process
	
	if( fp == NULL )
	{
		fp = stdin;
	}
	
	char *cur_dir = NULL;
	int ps_status;			// status of processes
	
	do
	{		
		cur_dir = getcwd(cur_dir, 0);
		printf("@%s > ", cur_dir);
		bytes_read = get_delimited_input(&buf, (int)'\n', stdin);
		
		// if flag, background execution vrai
		flag = ( buf[bytes_read - 1] == '&' );
		
		args_array = fill_arguments(buf, &args_len);
		is_cd_command = !strncmp(args_array[0], "cd", 2);		// returns 0 if is not cd command, not(0) if is cd command
		
		if( flag )
		{
			int taille = strlen(args_array[0]);
			
			args_array[0][taille - 1] = '\0';
		}
		
		if( bytes_read >= 4 )
		{
			memset(quit_msg, 0, sizeof(quit_msg));
			strncpy(quit_msg, buf, 4);
			
			// ca marchera tjrs si les premiers 4 lettres sont 'exit' ou 'quit'
			// c-a-d meme pour 'exittttt', ca marchera
			if( !strncmp(quit_msg, QUIT, 4) || !strncmp(quit_msg, EXIT, 4) )
			{
				break;
			}
		}
		
		fils = fork();
		if( fils < 0 )
		{
			die_error("fils");
		}
		else if( fils == 0 )
		{
			if( is_cd_command )
			{
				return 0;
			}
			if( flag )				// pour background proecessus
			{
				fils_2 = fork();
				if( fils_2 < 0 )
				{
					die_error("fils_2");
				}
				else if( fils_2 == 0 )
				{
					execvp(args_array[0], args_array);
					
					return 0;
				}
				else
				{
					exit(-1);
				}
			}
			
			execvp(args_array[0], args_array);
			
			return 0;
		}
		else
		{
			if( is_cd_command )
			{
				if( chdir(args_array[1]) == -1 )
				{
					perror("cd");
				}
			}
		}
		
		free_memory((void**)&buf);
		free_memory((void**)&cur_dir);
		for( i = 0; i < args_len; ++i )
		{
			free_memory((void**)&args_array[i]);
		}
		
		if( fils > 0 )
		{
			waitpid(fils, NULL, 0);
		}
		
	}while( bytes_read > 0 && ne_ferme_jamais );
}

int main(int argc, char **argv)
{
	struct sigaction sa;
	
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &ctrl_c_handler;
	
	sigaction(SIGINT, &sa, NULL);
	
	read_commands(stdin);
		
	return 0;
}

