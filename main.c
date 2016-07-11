#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>

/* TODO: We probably shouldn't hardcode lengths of these things... */
#define STACK_SLOTS 255
#define BUFSIZE 255

enum parse_state { NORMAL, SINGLE_QUOTE };

/* The global stack */
char STACK[STACK_SLOTS][BUFSIZE] = {'\0'};
/* Points to the current operable item on stack.  On empty stack, -1. */
int STACK_I = -1;

int num_stack_items()
{
	int num_items = STACK_I+1;
	if (num_items < 0) {
		fprintf(stderr, "%s:%d:%s(): number of items is less than 0 for some reason.  this shouldn't happen.\n", __FILE__,__LINE__,__func__);
		num_items = 0;
	}
	return num_items;
}

void stack_push(char *word)
{
	if (word == NULL) {
		fprintf(stderr, "%s:%d:%s(): word is NULL.  Should not be.\n", __FILE__,__LINE__,__func__);
		return;
	}
	else if (STACK_I+1 > STACK_SLOTS) {
		fprintf(stderr, "%s:%d:%s(): stack overflow error: number of words on stack would be greater than %d.\n", __FILE__,__LINE__,__func__,STACK_SLOTS);
		return;
	}

	++STACK_I;
	strncpy(STACK[STACK_I], word, BUFSIZE-2);
	STACK[STACK_I][BUFSIZE-1] = '\0';
}

/* stack_pop returns the address of its internal buffer.  If you use this
 * function for multiple items in succession, save your results so that they
 * don't get clobbered.
 */
char *stack_pop()
{
	if (num_stack_items() == 0) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return NULL;
	}

	return STACK[STACK_I--];
}

void stack_dot()
{
	char *printval = stack_pop();
	if (printval == NULL) return;
	printf("%s\n", printval);
}

void stack_show()
{
	for (int i = 0; i <= STACK_I; ++i) {
		printf("%s\t", STACK[i]);
	}
	printf("\n");
}

char *stack_peek()
{
	if (num_stack_items() == 0) return NULL;
	return STACK[STACK_I];
}

void stack_dropall()
{
	while (num_stack_items() > 0) stack_pop();
}

bool strisdigit(char *str)
{
	for (int i = 0; str[i] != '\0'; ++i) {
		if (!isdigit(str[i])) return false;
	}
	return true;
}

/* TODO: deduplicate stack_plus and stack_minus functions */
void stack_plus()
{
	if (num_stack_items() < 2) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return;
	}

	char *arg1 = STACK[STACK_I-1];
	char *arg2 = STACK[STACK_I];

	if (!( strisdigit(arg1) && strisdigit(arg2) )) {
		fprintf(stderr, "%s:%d:%s(): type error with given arguments: %s,%s.\n", __FILE__,__LINE__,__func__,arg1,arg2);
		return;
	}

	stack_pop();
	stack_pop();

	int arg1_converted;
	int arg2_converted;

	sscanf(arg1, "%d", &arg1_converted);
	sscanf(arg2, "%d", &arg2_converted);
	
	char retval[BUFSIZE] = {'\0'};
	snprintf(retval, BUFSIZE-2, "%d", arg1_converted + arg2_converted);
	retval[BUFSIZE-1] = '\0';
	stack_push(retval);
}

void stack_minus()
{
	if (num_stack_items() < 2) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return;
	}

	char *arg1 = STACK[STACK_I-1];
	char *arg2 = STACK[STACK_I];

	if (!( strisdigit(arg1) && strisdigit(arg2) )) {
		fprintf(stderr, "%s:%d:%s(): type error with given arguments: %s,%s.\n", __FILE__,__LINE__,__func__,arg1,arg2);
		return;
	}

	stack_pop();
	stack_pop();

	int arg1_converted;
	int arg2_converted;

	sscanf(arg1, "%d", &arg1_converted);
	sscanf(arg2, "%d", &arg2_converted);
	
	char retval[BUFSIZE] = {'\0'};
	snprintf(retval, BUFSIZE-2, "%d", arg1_converted - arg2_converted);
	retval[BUFSIZE-1] = '\0';
	stack_push(retval);
}

void stack_swap()
{
	if (num_stack_items() < 2) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return;
	}
	char arg1[BUFSIZE] = {'\0'};
	char arg2[BUFSIZE] = {'\0'};
	strcpy(arg1, stack_pop());
	strcpy(arg2, stack_pop());
	stack_push(arg1);
	stack_push(arg2);
}

void stack_dup()
{
	if (num_stack_items() < 1) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return;
	}

	char *topval = stack_peek();
	stack_push(topval);
}

void stack_over()
{
	if (num_stack_items() < 2) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return;
	}

	char *topval = stack_pop();
	stack_dup();
	stack_push(topval);
	stack_swap();
}

void stack_rot()
{
	if (num_stack_items() < 3) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return;
	}

	char *topval = stack_pop();
	stack_swap();
	stack_push(topval);
	stack_swap();
}

bool strisspace(char *str) {
	for (int i = 0; str[i] != '\0'; ++i) {
		if (!isspace(str[i]))
			return false;
	}
	return true;
}

/* stack operation */
void get_variable()
{
	char name[BUFSIZE] = {'\0'};
	strcpy(name, stack_pop());
	char *value = getenv(name);
	if (value == NULL)
		stack_push("()");
	else
		stack_push(value);
}

/* stack operation */
void set_variable()
{
	char name[BUFSIZE] = {'\0'};
	char value[BUFSIZE] = {'\0'};
	strcpy(name, stack_pop());
	if (name == NULL) return;

	strcpy(value, stack_pop());
	if (value == NULL) return;

	if (strcmp(value, "()") == 0)
		unsetenv(name);
	else
		setenv(name, value, 1);
}

/* Execute binary on the top of the stack with the rest of the stack as its
 * arguments. */
void stack_execute()
{
	if (num_stack_items() < 1) {
		fprintf(stderr, "%s:%d:%s(): stack underflow error.\n", __FILE__,__LINE__,__func__);
		return;
	}

	char prog[BUFSIZE] = {'\0'};;
	char *args[STACK_SLOTS] = {'\0'};
	for (int i = 0; i < STACK_SLOTS; ++i) {
		args[i] = malloc(BUFSIZE);
	}
	strcpy(prog, stack_pop());
	int wstatus;
	int child_exit_status;
	char child_exit_status_str[255];
	int retval;
	int i;

	/* Use entire stack for arguments now */
	strcpy(args[0], prog);
	for (i = 1; stack_peek() != NULL; ++i) {
		strcpy(args[i], stack_pop());
	}
	free(args[i]);
	args[i] = NULL;

	int pid = fork();
	switch (pid) {
		case 0:
			retval = execvp((const char *)prog, (char * const *)args);
			if (retval == -1) {
				fprintf(stderr, "%s:%d:%s(): error starting %s: ", __FILE__,__LINE__,__func__, prog);
				perror(NULL);
			}
			exit(1);
			break;
		case -1:
			fprintf(stderr, "%s:%d:%s(): error starting prog %s.\n", __FILE__,__LINE__,__func__, prog);
			break;

		default: {
				 int w = waitpid(pid, &wstatus, 0);
				 if (w == -1 && errno != ECHILD) {
					 perror("waitpid");
				 }
				 else if (WIFEXITED(wstatus)) {
					 child_exit_status = WEXITSTATUS(wstatus);
					 sprintf(child_exit_status_str, "%d", child_exit_status);
					 stack_push(child_exit_status_str);
				 }
				 else {
					 perror(NULL);
					 fprintf(stderr, "%s:%d:%s(): error:prog %s has not exited.\n", __FILE__,__LINE__,__func__, prog);
				 }
				 break;
			 }
		
	}
	for (int i = 0; i < STACK_SLOTS; ++i) {
		free(args[i]);
	}
}

char *get_next_token(char *line, size_t line_size, int *start_from)
{
	enum parse_state state = NORMAL;
	int i = *start_from;
	static char word[255] = {'\0'};
	int word_i = 0;

	/* No further tokens */
	if (*start_from >= line_size) return NULL;

	if (strisspace(line))
		return NULL;

	while (isspace(line[i]))
		++i;

	/* Comment */
	if (line[i] == '#') {
		return NULL;
	}

	if (line[i] == '$' && line[i+1] == '\'') {
		state = SINGLE_QUOTE;
		word[word_i++] = line[i++];
		word[word_i++] = line[i++];
	}
	else if (line[i] == '\'') {
		state = SINGLE_QUOTE;
		word[word_i++] = line[i++];
	}

	if (state == SINGLE_QUOTE) {
		for (; i < line_size; ++i) {
			/* single quote makes entire value have single quote */
			if (line[i] == '\'' && line[i+1] == '\'') {
				word[word_i++] = '\'';
				word[word_i] = '\'';
				++i; /* this will increment a second time during loopover to skip the second quote */
			}
			else if (line[i] == '\''){ /* terminate single-quoted string */
				word[word_i++] = line[i];
				word[word_i] = '\0';
				break;
			}
			else {
				word[word_i] = line[i];
			}
			++word_i;
		}
		if (i >= line_size) {
			fprintf(stderr, "Input ended unexpectedly\n");
			return NULL;
		}
		++i; /* Increment one last time to get to possible whitespace */

		if (!isspace(line[i])) {
			fprintf(stderr, "Input after quote is not whitespace.\n");
			return NULL;
		}
	}
	else if (state == NORMAL) {
		for (; i < line_size; ++i) {
			if (isspace(line[i])) {
				word[word_i] = '\0';
				break;
			}
			else {
				word[word_i] = line[i];
				++word_i;
			}
		}
	}

	/* Skip whitespace */
	while (isspace(line[i]))
		++i;

	*start_from = i;

	return word;
}

void program_loop()
{
	char *line = NULL;
	size_t line_size = 0;
	int start_from = 0;
	char *token = NULL;

	while(true) {
		printf(" > ");
		line_size = getline(&line, &line_size, stdin);
		if (line_size == -1) break;

		while (true) {
			token = get_next_token(line, line_size, &start_from);
			if (token == NULL) {
				break;
			}
			else if (strcmp(token, ".s") == 0) {
				stack_show();
				continue;
			}
			else if (strcmp(token, ".") == 0) {
				stack_dot();
				continue;
			}
			else if (strcmp(token, ".dropall") == 0) {
				stack_dropall();
				continue;
			}
			else if (strcmp(token, "+") == 0) {
				stack_plus();
				continue;
			}
			else if (strcmp(token, "-") == 0) {
				stack_minus();
				continue;
			}
			else if (strcmp(token, ".swap") == 0) {
				stack_swap();
				continue;
			}
			else if (strcmp(token, ".dup") == 0) {
				stack_dup();
				continue;
			}
			else if (strcmp(token, ".over") == 0) {
				stack_over();
				continue;
			}
			else if (strcmp(token, ".drop") == 0) {
				stack_pop();
				continue;
			}
			else if (strcmp(token, ".rot") == 0) {
				stack_rot();
				continue;
			}
			else if (strcmp(token, ".-rot") == 0) {
				stack_rot();
				stack_rot();
				continue;
			}
			else if (strcmp(token, "!") == 0) {
				set_variable();
				continue;
			}
			else if (strcmp(token, "$") == 0) {
				get_variable();
				continue;
			}
			else if (token[0] == '$' && token[1] == '$') {
				fprintf(stderr, "error: variable name can't begin with $.\n");
				continue;
			}
			else if (token[0] == '$' && token[1] == '.') {
				/* Put literals in for stack operations. */
				/* shift everything after $ over one */
				for (int i = 0; i < BUFSIZE-1; ++i) {
					token[i] = token[i+1];
				}
				stack_push(token);
				continue;
			}
			else if (token[0] == '$') {
				/* shift everything after $ over one */
				for (int i = 0; i < BUFSIZE-1; ++i) {
					token[i] = token[i+1];
				}
				stack_push(token);
				get_variable();
				continue;
			}
			else if (strcmp(token, ";") == 0) {
				stack_execute();
				continue;
			}
			else if (token[0] == '.' && strlen(token) > 1) {
				/* shift everything after . over one */
				for (int i = 0; i < BUFSIZE-1; ++i) {
					token[i] = token[i+1];
				}
				stack_push(token);
				stack_execute();
				continue;
			}
			stack_push(token);
		}
		start_from = 0;
	}
}

int main(int argc, char *argv[])
{
	program_loop();
	return 0;
}
