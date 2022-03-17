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
int main(int argc, char** argv) {
	/* Main Function Variables */
	char* line = (char *) malloc(BUFSIZ * sizeof(char));
	char* token;
	size_t n = BUFSIZ;
	int token_counter;
	int exit_flag = 0;

	FILE* infile = fopen(argv[1], "r");
	FILE* outfile = fopen("output.txt", "w");

	while (1) {
		getline(&line, &n, infile);

		token_counter = 0;
		token = strtok(line, " ");

		if (*token == '\n') {
			continue;
		}

		while (token != NULL) {
			fprintf(outfile, "\n");

			if (token == NULL) {
				break;
			}

			if (strncmp(token, "exit", 4) == 0) {
				exit_flag = 1;
				break;
			}

			fprintf(outfile, "T%d: %s", token_counter++, token);
			token = strtok(NULL, " ");
		}

		if (exit_flag) {
			break;
		}
	}

	fclose(infile);
	fclose(outfile);
	free(line);

	return 1;
}
/*-----------------------------Program End-----------------------------------*/
