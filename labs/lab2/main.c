/*
* Description: Breaking a string provided via getline() into a series of
* character tokens using the strtok() function.
*
* Author: Lukas Hanson
*
* Date: 2020/10/09
*
* Notes: None
*/

/*-------------------------Preprocessor Directives---------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*---------------------------------------------------------------------------*/

/*-----------------------------Program Main----------------------------------*/
int main() {
	/* Main Function Variables */
	char* line = (char *) malloc(BUFSIZ * sizeof(char));
	size_t n = BUFSIZ;
	char* token;
	int token_counter;
	int exit_flag = 0;

	while (1) {
		printf(">>> ");
		getline(&line, &n, stdin);

		token_counter = 0;
		token = strtok(line, " ");

		if (*token == '\n') {
			continue;
		}

		while (token != NULL) {
			printf("\n");

			if (token == NULL) {
				break;
			}

			if (strncmp(token, "exit", 4) == 0) {
				exit_flag = 1;
				break;
			}

			printf("T%d: %s", token_counter++, token);
			token = strtok(NULL, " ");
		}

		if (exit_flag) {
			break;
		}
	}

	free(line);

	return 1;
}
/*-----------------------------Program End-----------------------------------*/
