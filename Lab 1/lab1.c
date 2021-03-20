/**
 * \file Lab1.c
 * \Operating Systems - Lab 1
 * \Program that makes a caesar-ciphered copy of an existing file and decrypts it in standard output
 */

#include <stdio.h>
#include <stdlib.h>      
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <ctype.h>

/* Enum for encryption mode */
typedef enum {
	ENCRYPT,
	DECRYPT
} encrypt_mode;

#define STREQUAL(x, y) ( strncmp((x), (y), strlen(y) ) == 0 )

/* File Descriptors for stdin and stdout */
#define FD_STDIN 0
#define FD_STDOUT 1

/* Arbitrary buffer size */
#define BUFFER_SIZE 10

/* User read-write, group read, others read */
#define PERMS 0644

/* Caesar Encryption Algorithm */
char caesar(unsigned char ch, encrypt_mode mode, int key)
{
    if (ch >= 'a' && ch <= 'z') {
        if (mode == ENCRYPT) {
            ch += key;
            if (ch > 'z') ch -= 26;
        } else {
            ch -= key;
            if (ch < 'a') ch += 26;
        }
        return ch;
    }

    if (ch >= 'A' && ch <= 'Z') {
        if (mode == ENCRYPT) {
            ch += key;
            if (ch > 'Z') ch -= 26;
        } else {
            ch -= key;
            if (ch < 'A') ch += 26;
        }
        return ch;
    }

    return ch;
}

/* Function to check if a number is between 0 and 26 */
_Bool is_between(int x) {
	if(x < 0 || x > 26)
		return 0;
    return 1; 
}

/* Function to check if given key is a valid key or not */
_Bool is_valid_key(char *s) {
	for (int i = 0; i < strlen(s); i++) 
		if (isdigit(s[i]) == 0) 
			return 0;
	if(! is_between(atoi(s)))
		return 0;
	return 1; 
} 

/* Function to store a string of unknown length */
char *inputString(FILE* fp, size_t size) {
	char *str;
	int ch;
	size_t len = 0;
	str = realloc(NULL, sizeof(char)*size);
	if(!str)
		return str;
	while(EOF != (ch = fgetc(fp)) && ch != '\n') {
		str[len++] = ch;
		if(len == size) {
			str = realloc(str, sizeof(char)*(size += 16));
			if(!str)
				return str;
		}
	}
	str[len++] = '\0';
	return realloc(str, sizeof(char)*len);
}		

/* Input data problem handler */
int fix(const char *x) {
	char *missing_input;
	printf("We encountered an issue with the %s you entered.\n", x);
	do {
		if(STREQUAL(x, "Input")) {
			char *what_to_do;
			int file_descriptor;
			printf("If you' d rather try again type: file. Otherwise, to insert text straight in standard input type: stdin.\n");
			what_to_do = inputString(stdin, 10);
			if(STREQUAL(what_to_do, "file")) {
				printf("OK. You chose to give a file path again.\n");
				do {
					printf("Type the absolute path to the file you want to work on:\n");
					missing_input = inputString(stdin, 10);
					file_descriptor = open(missing_input, O_RDONLY);
					if(file_descriptor == -1)
						printf("%s is not a valid path. Please, try again.\n", missing_input);
					else {
						printf("OK. Absolute path [%s] was registered successfully.\n", missing_input);
						free(missing_input);
						free(what_to_do);
						return file_descriptor;
					}
				} while(1);
			}
			else if(STREQUAL(what_to_do, "stdin")) {
				printf("OK. You chose to insert text from standard input.\n");
				char buffer[BUFFER_SIZE];
				int temp_read, temp_write;
				printf("Please, type the text you want me to work on:\n");
				file_descriptor = open("Temporary file.txt", O_CREAT  | O_RDWR | O_TRUNC, PERMS);
				if(file_descriptor == -1) {
					perror("Error while creating temporary file");
					exit(-1);
				}
				do {
					temp_read = read(FD_STDIN, buffer, BUFFER_SIZE);
					if(temp_read == -1) {
						perror("Error while reading from standard output");
						exit(-1);
					}
					temp_write = write(file_descriptor, buffer, temp_read);
					if (temp_write < temp_read) {
						perror("Error while writing in newlly created input file.\n");
						exit(-1);
					}
				} while(temp_read > 0);
				close(file_descriptor);
				return open("Temporary file.txt", O_RDONLY, PERMS);
			}
			else
				printf("%s is not a valid answer [file/stdin]. Please, try again.\n", what_to_do);
		}
		else {
			printf("Type the cipher key you want to use in the encryption:\n");
			missing_input = inputString(stdin, 10);
			if(!is_valid_key(missing_input))
				printf("%s isn't a number between 0 and 26. Please, try again.\n", missing_input);
			else {
				printf("OK. Cipher key [%d] was registered successfully.\n", atoi(missing_input));
				return atoi(missing_input);
			}
		}	
	} while (1);
}

int main(int argc, char** argv) {

	int chosen_path = -1;
	int chosen_key = -1;
	int n_read, n_create, n_write, fd_create, status;
	char buffer[BUFFER_SIZE];
	
	/* ======================  Check for input validity  ===================== */
	for(int i = 1; i < argc; i++) {
		if(STREQUAL(argv[i], "--input"))
			if(i == argc - 1)	// path was passed as the last arguement
				chosen_path = fix("Input");
			else {  		       // argv[i+1] = absolute path
				chosen_path = open(argv[i+1], O_RDONLY);
				if(chosen_path == -1) 
					chosen_path = fix("Input");
				}
		if(STREQUAL(argv[i], "--key"))
			if(i == argc - 1)	// key was passed as the last arguement
				chosen_key = fix("Key");
			else {  		       // argv[i+1] = cipher key
				if(is_valid_key(argv[i+1]))
					chosen_key = atoi(argv[i+1]);
				else
					chosen_key = fix("Key");
			}
	}
	if(chosen_key == -1)		// "--key" string may never have been written
		chosen_key = fix("Key");
	if(chosen_path == -1)		// "--input" string may never have been written
		chosen_path = fix("Input");
	/* ================================================================= */
	
	pid_t c1 = fork();
	if(c1 == -1) {
		perror("Error while bearing first child");
		exit(-1);
	}
	if(c1 == 0) {	// encryptor child code
		fd_create = open("encrypted.txt", O_CREAT | O_WRONLY | O_TRUNC, PERMS);
		if(fd_create == -1) { 
			perror("Error while creating the encryption file");
			exit(-1);
		}
		do {
			n_read = read(chosen_path, buffer, BUFFER_SIZE);	// n_read: bytes written in buffer
			if(n_read == -1) {
				perror("Error while reading given input file");
				exit(-1);
			}
			for(int i = 0; i < n_read; i++) 
				buffer[i] = caesar(buffer[i], ENCRYPT, chosen_key);
			n_create = write(fd_create, buffer, n_read);		// copy encrypted text in encryption file
			if(n_create == -1) {
				perror("Error while writting in encryption file");
				exit(-1);
			}
		} while(n_read > 0); // why? --> so that no matter how big out message is, it will all be written (encrypted)
		close(fd_create);
		close(chosen_path);
	exit(0);
	}
	wait(&status);
	
	pid_t c2 = fork();
	if(c2 == -1) {
		perror("Error while bearing second child");
		exit(-1);
	}
	if(c2 == 0) {	// decryptor child code
		int fd_encrypted = open("encrypted.txt", O_RDONLY);
		if (fd_encrypted == -1) {
			perror("Error while opening encrypted file");
			exit(-1);
    		}
    		printf("\n========================== Decrypted Text ==========================\n");
		do {
			n_read = read(fd_encrypted, buffer, BUFFER_SIZE);	// n_read: bytes written in buffer
			if(n_read == -1) {
				perror("Error while reading the encrypted file.");
				exit(-1);
			}
			for(int i = 0; i < n_read; i++)
				buffer[i] = caesar(buffer[i], DECRYPT, chosen_key);
			n_write = write(FD_STDOUT, buffer, n_read);		// Write the decrypted text in standard output
			if(n_write < n_read) {
				perror("Error while writting in standard output.");
				exit(-1);
			}
		} while(n_read > 0); // why? ---> so that no matter how big out message is, it will all be written out
		close(fd_encrypted);
		exit(0);
	}
	wait(&status);
	remove("Temporary file.txt");
	printf("\n");
}
