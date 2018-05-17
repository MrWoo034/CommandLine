/* Author: Leif Brockman, 664734715, lbroc3@uis.edu
    Compile: gcc main.c -o shell.out

    Brief Description: A simple command line interpretter; it takes
	a single command and parameters and executes
	the command as a child process.

	The command prompt also stores a history of up
	too 10 commands.  User can access commands with
	the special command !!, or !n where n represents
	the nth command entered.

    History is written to a file called history.txt to maintain
    history between sessions.

    V 2.0.0 Now includes the most frequently used command (mfu)

    Commands are displayed including their occurrence rate when
    mfu is entered.  These are stored between sessions in a file
    occurrence.txt in the same location as the shell.out file.
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_LINE 80 /* The maximum length command */
#define MAX_ARGS 10 /* The maximum number of arguments */
#define MAX_HISTORY 10 /*The maximum number of commands to store in history */


// function prototypes
void readCommand(char **_cmdPtr);

void parseCommand(char **_argsPtr, char *_cmdPtr);

void insertHistory(char **_cmdHistory, char *_cmdPtr);

void readHistory(char **_cmdHistory);

void writeHistToFile(const char *_fileName, char **_cmdHistory);

void readHistoryFile(const char *filename, char **_cmdHistory);

void resizeCmdRecord(void);


// global vars

const char CMD_EXIT[] = "exit\n";
const char CMD_RECENT[] = "recent\n";
const char OCCUR_FILEPATH[] = "occurence.txt";
const char HIST_FILEPATH[] = "history.txt";
const char CMD_MFU[] = "mfu\n";

int cmd_record_index = 0;
int numCmds = MAX_HISTORY;
int session_started = 0;

// OCCURENCE STRUCTURE

struct cmd_record {
    char the_command[MAX_LINE];
    int count;
} **pCmd_record;

// FUNCTION PROTOTYPES RELYING ON STRUCT

void readOccurrenceFile(const char *filename);

void writeOccurrenceToFile(const char *filename);

void allocStruct(struct cmd_record ***_pCmd_record, int _numCmds);

void deallocStruct(struct cmd_record ***_pCmd_record, int _numCmds);

void updateOccurrence(char *_theCommand);

void printOccurrences(void);

void sortOccurrence(void);

/**
 * Main Program.
 * @return
 */
int main(void) {
    char *args[MAX_ARGS] = {(char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                            (char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                            (char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                            (char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                            (char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE)};

    int should_run = 1; /* flag to determine if the program should exit */
    int child_status = -1;
    pid_t child_pid = -1, wait_pid = -2;

    // Initialize the input holder to
    // an area of heap memory the size of
    // MAX_LINE characters
    char *commandInput = (char *) malloc(MAX_LINE * sizeof(char));

    char *cmdHistory[MAX_HISTORY] = {(char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                                     (char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                                     (char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                                     (char *) malloc(sizeof(char) * MAX_LINE), (char *) malloc(sizeof(char) * MAX_LINE),
                                     (char *) malloc(sizeof(char) * MAX_LINE),
                                     (char *) malloc(sizeof(char) * MAX_LINE)};

    // Initialize the history buffer from
    // the file history.txt
    readHistoryFile(HIST_FILEPATH, cmdHistory);
    // initialize the occurrence struct
    allocStruct(&pCmd_record, numCmds);
    // read history for mfu into the struct
    readOccurrenceFile(OCCUR_FILEPATH);

    while (should_run) {
        printf("COMMAND-> ");
        fflush(stdout);

        // Reads input using fgets() and stores
        // the input into the character pointer
        // commandInput

        readCommand(&commandInput);

        // check if the command entered was
        // exit, if so set flag to 0, and
        // free memory from allocated pointers

        if (strcasecmp(CMD_EXIT, commandInput) == 0) {
            should_run = 0;
            writeHistToFile(HIST_FILEPATH, cmdHistory);
            writeOccurrenceToFile(OCCUR_FILEPATH);
            continue;
        }

            // Check if command was recent, then
            // we should print the history of cmds

        else if (strcasecmp(CMD_RECENT, commandInput) == 0) {
            readHistory(cmdHistory);
            continue;

        } else if (strcasecmp(CMD_MFU, commandInput) == 0) {
            printOccurrences();
            continue;
        } else {

            /*
            * If the command starts with !, we
            * will load a command from history.
            */

            if (*(commandInput + 0) == '!') {
                int cmdIndex = 1;
                // if the second char is '!'
                if (*(commandInput + 1) == '!') {
                    cmdIndex = 0;
                } else {
                    // stores the index
                    // of the desired place
                    // in history buffer
                    if (!isdigit(*(commandInput + 1))) {
                        printf("Invalid number.  Please try again\n");
                        continue;
                    }

                    if (*(commandInput + 1) == '1') {
                        if (*(commandInput + 2) == '0')
                            cmdIndex = 10;
                        else if (isdigit(*(commandInput + 2))) {
                            printf("Invalid number.  Please try again\n");
                            continue;
                        }
                    }


                    // convert second char to an int
                    // to be used to pull the index
                    // from the cmdHistory
                    cmdIndex = *(commandInput + 1) - '0';
                    cmdIndex--; // remove one from the index for array addressing


                }

                if (**(cmdHistory + cmdIndex) == '\0') {
                    printf("There is no recent command number %i\n", cmdIndex + 1);
                    continue;
                }

                // overload the command input
                // and parse the command input
                // back into the args array.

                memcpy(commandInput, *(cmdHistory + cmdIndex), sizeof(char) * MAX_LINE);
                printf("COMMAND-> %s", commandInput);

            }

            parseCommand(args, commandInput);

            child_pid = fork();
            fflush(stdout);

            if (child_pid == 0) {
                fflush(stdout);
                execvp(*args, args);
                printf("Unknown Command.\n");
                exit(2);
            } else if (child_pid < 0) {
                printf("\nFork Failed");
                exit(1);
            } else {
                while (wait_pid != child_pid)
                    wait_pid = wait(&child_status);
            }

            if (child_status == 0) {
                session_started = 1;
                insertHistory(cmdHistory, commandInput);
                updateOccurrence(commandInput);
            }
        }


    }

    free(commandInput);
    deallocStruct(&pCmd_record, numCmds);

    int i;

    for (i = 0; i < MAX_ARGS; i++) {
        free(*(args + i));
    }

    for (i = 0; i < MAX_HISTORY; i++) {
        free(*(cmdHistory + i));
    }

    return 0;
}

/**
	Reads and returns up to 81 characters entered
	by the user as the command.
	
	Requires a pointer to a pointer.  Then references
	the value of the pointer, which is the address of
	the pointer contained in main.  This can then be
	passed to the next function, parse command.
	
*/

void readCommand(char **_cmdPtr) {

    fgets(*_cmdPtr, MAX_LINE, stdin);
    fflush(stdin);

    return;
}

/**
	Parses the text in _cmdPtr into a maximum
	of ten different command line arguments.
	
	The command line arguments will be updated 
	in main, so they may be passed back to the
	the original function and used for to fork
	new processes.
	
*/

void parseCommand(char **_argsPtr, char *_cmdPtr) {

    int i = 0, argIndex = 0, wordPosition = 0, count = 0;

    // loop through the array

    for (i = 0; i < MAX_LINE; i++) {




        // if the current char in array is
        // a space or null 0, then it is ready
        // to add to the args array.  We start
        // by allocating memory in the args array
        // equal to the total count of chars we need, increment argsPtr to
        // next argument, and continue loop
        // reset our arg char index, j, to 0

        if (_cmdPtr[i] == ' ' || _cmdPtr[i] == '\0' || _cmdPtr[i] == '\n') {


            // CHECK TO SEE IF ARGS IS NULL (MEANING NO MEMORY ALLOCATED)
            // IF SO WE MUST REALLOCATE MEMORY

            if (*_argsPtr == (char *) NULL) {
                *_argsPtr = (char *) malloc(sizeof(char) * MAX_LINE);
            }

            // first check to see if the current char is '\0'
            // if it is, then we must set the current char[]
            // to null.  NULL is required as the last argument
            // to commands such as LS to work properly.
            // Allocating memory results in char[]='\0', which
            // will cause LS to fail.

            if (_cmdPtr[i] == '\0') {
                *_argsPtr = (char *) NULL;
                break;
            }

            // Initialize the current array of chars in memory
            // for the args pointer to match the size of the current
            // word plus one

            //*(_argsPtr) = (char*)malloc(count+1*sizeof(char));

            // set the wordPosition relative to the full
            // command string.  This is the position we
            // we want to start extracting from the full
            // command string.
            wordPosition = i - count;

            // counting variable to iterate onto the new string
            int j = 0;
            while (j < count) {

                // if character is '\n' we need to break
                // the loop.

                if (*(_cmdPtr + wordPosition) == '\n') {
                    *(*(_argsPtr) + j) = *(char *) NULL;
                    break;
                }

                // set the new string (char array) equal to the
                // value stored
                // in the sentence at the current word position
                // and increment both forward.

                // j = char position in array
                // wordPosition = char position in full command

                *(*(_argsPtr) + j) = *(_cmdPtr + wordPosition);

                j++;
                wordPosition++;
            }

            // place a null character at the end of the
            // current char array to mark string end.
            if (count > 0)
                *(*(_argsPtr) + count) = '\0';

            // Increment the _argsPtr to the next array
            // element
            // e.g. equivalent to myArray[index++]
            // reset counting and position variables.
            _argsPtr++;
            count = 0;
            wordPosition = 0;
            continue;
        }

        // keep count of the number of letters
        // in the current arg, so we can initialize
        // that arg.

        count++;

    }

    return;

}

/**
 * Inserst the current _cmdPtr into
 * the running _cmdHistory
 *
 * @param _cmdHistory  This is the 'buffer'
 *                      of commands stored.
 * @param _cmdPtr       This it the command
 *                      to be stored in the
 *                      buffer.
 */

void insertHistory(char **_cmdHistory, char *_cmdPtr) {

    int i;

    for (i = MAX_HISTORY - 1; i > 0; i--) {
        memcpy(*(_cmdHistory + i), *(_cmdHistory + (i - 1)), sizeof(char) * MAX_LINE);
    }

    memcpy(*(_cmdHistory), _cmdPtr, sizeof(char) * MAX_LINE);

    return;
}

/**
 * Reads the full history of commands
 * entered to this point.
 *
 * @param _cmdHistory The array of history
 * commands the user has entered.
 *
 */

void readHistory(char **_cmdHistory) {
    int i = 0;

    for (i = 0; i < MAX_HISTORY; i++) {
        if (i == 0 && (*(_cmdHistory) == NULL)) {
            printf("No recent commands\n");
            break;
        } else if (*(_cmdHistory + i) == NULL || *(*(_cmdHistory + i)) == '\0') {
            break;
        } else {
            printf("%i %s", i + 1, *(_cmdHistory + i));
        }
    }
}

/**
 * Writes an occurrence of a
 * @param filename
 */
void writeOccurrenceToFile(const char *filename) {

    FILE *fptr;  // pointer to the file
    int i;

    if ((fptr = fopen(filename, "wb")) == NULL) {  // open a binary file for writing (overwrite contents)
        printf("fopen [wb] error!\n");  // file open failed
        //exit(1);
    }

    for (i = 0; i < cmd_record_index; i++) {  // write 5 samples
        fwrite(pCmd_record[i], sizeof(struct cmd_record), 1, fptr); // write one structure into the file
    }

    fclose(fptr); // close the file
}

// a sample function to write structure data into a file
void writeHistToFile(const char *filename, char **_cmdHistory) {
    FILE *fptr;  // pointer to the file
    int i;

    if ((fptr = fopen(filename, "wb")) == NULL) {  // open a binary file for writing (overwrite contents)
        printf("fopen [wb] error!\n");  // file open failed
        exit(1);
    }

    // writes the history buffer to the
    // history.txt file.

    for (i = 0; i < MAX_HISTORY; i++) {
        // if the command is null, then
        // we are at the end of the buffer
        // break the loop
        if (*(_cmdHistory + i) == (char *) NULL) {
            break;
        }
        // else we write this command
        fwrite(*(_cmdHistory + i), sizeof(char) * strlen(*(_cmdHistory + i)), 1,
               fptr); // write one structure into the file
    }
    fclose(fptr); // close the file
}

// a sample function to read structure data from a file
void readOccurrenceFile(const char *filename) {

    FILE *fptr;  // pointer to the file

    if ((fptr = fopen(filename, "rb")) == NULL) {  // open a binary file for reading
        if (session_started) {
            printf("fopen [rb] error!\n");  // file open failed
            return;
        } else {
            printf("Occurrence File not found.  File will be created on exit.\n");
            return;
        }
    }

    // read the file in fptr, reading a single struct
    // into the pCmd_record.
    // If no more structs in file, the loop will break;

    // temp struct to store read data into.
    struct cmd_record *p = malloc(sizeof(struct cmd_record));

    while (fread(p, sizeof(struct cmd_record), 1, fptr) ==
           1) {

        // perform a deep copy of the temp
        // to our global var.
        pCmd_record[cmd_record_index]->count = p->count;
        strcpy(pCmd_record[cmd_record_index]->the_command, p->the_command);

        cmd_record_index++;

        /*
         * If our index is past our num of commands
         * we need to update the number of commands
         * and resize our pointer so we have enough
         * memory allocated to hold the file contents
         */

        if (cmd_record_index > (numCmds - 1)) {
            numCmds *= 2;
            resizeCmdRecord();
        }


        //printf("The command [%s] appeared %d times.\n", *pCmd_.cmd, entry.count);
    }

    if (feof(fptr)) {
        // TODO
        // printf("fread read past the end of file!\n");
    }

    fclose(fptr); // close the file
    free(p);

}

void readHistoryFile(const char *filename, char **_cmdHistory) {

    FILE *fptr;  // pointer to the file

    if ((fptr = fopen(filename, "rb")) == NULL) {  // open a binary file for reading
        if (session_started) {
            printf("fopen %s [rb] error!\n", filename);  // file open failed
            return;
        } else {
            printf("History File not found.  File will be created on exit.\n");
            return;
        }
    }

    // read the file in fptr, reading a single struct
    // into the pCmd_record.
    // If no more structs in file, the loop will break;
    int cmdIndex = 0;
    int brkLoop = 0;
    size_t length = 0;
    ssize_t reader = 0;

    while ((reader != -1) & (brkLoop == 0)) {

        reader = getline(&_cmdHistory[cmdIndex], &length, fptr);

        // DEBUG CODE
        //printf("Read the command: %s", (_cmdHistory[cmdIndex]));

        cmdIndex++;
        /*
         * Check to see if the index has passed
         * the MAX history line.  If so, we need
         * to break the loop, as there should be
         * no more history.
         */
        if (cmdIndex > (MAX_HISTORY - 1)) {
            brkLoop = 1;
        }
    }

    fclose(fptr); // close the file

}

/**
 * Resizes the cmd_record ptr to
 * the updated number of cmd_records
 * (numCmds).  Does a deep copy,
 * deletes, and then reinitializes the
 * struct.
 */

void resizeCmdRecord(void) {

    // temp struct pointer to copy
    // the original into.
    struct cmd_record **ptr_record_temp;
    // allocate memory for temp struct
    allocStruct(&ptr_record_temp, cmd_record_index);

    int index;
    // copy contents from global struct
    // into the temp struct
    for (index = 0; index < cmd_record_index; index++) {
        ptr_record_temp[index]->count = pCmd_record[index]->count;
        memcpy(ptr_record_temp[index]->the_command, pCmd_record[index]->the_command, sizeof(char) * MAX_LINE);
    }
    // free the global, then reallocate
    // with
    deallocStruct(&pCmd_record, cmd_record_index);
    allocStruct(&pCmd_record, numCmds);

    // copy from temp back to global struct
    for (index = 0; index < cmd_record_index; index++) {
        pCmd_record[index]->count = ptr_record_temp[index]->count;
        memcpy(pCmd_record[index]->the_command, ptr_record_temp[index]->the_command, sizeof(char) * MAX_LINE);
    }

    // free the temp struct
    deallocStruct(&ptr_record_temp, cmd_record_index);

}

/**
 * Allocates the struct into memory
 * using global variables.
 *
 */
void allocStruct(struct cmd_record ***_pCmd_record, int _numCmds) {

    //_pCmd_record = (struct cmd_record *) malloc(_numCmds * sizeof(struct cmd_record));
    *_pCmd_record = malloc(_numCmds * sizeof(struct cmd_record));

    int i;
    for (i = 0; i < _numCmds; i++) {
        *((*_pCmd_record) + i) = malloc(sizeof(struct cmd_record));
    }
}

/**
 * Reclaims the memory from the
 * pCmd_record ** so it can be
 * reused later in the program.
 */

void deallocStruct(struct cmd_record ***_pCmd_record, int _numCmds) {

    int i;
    // free each pointer in the array
    for (i = 0; i < _numCmds; i++) {
        free(*(*(_pCmd_record) + i));
    }
    // free the pointer to the array
    free(*_pCmd_record);

}

/**
 *
 * @param _pCmd_record
 * @param _theCommand
 */
void updateOccurrence(char *_theCommand) {

    // first we look to see if this command has occurred
    // so we can update occurrence count and return
    int i;

    for (i = 0; i < cmd_record_index; i++) {
        if (strcmp(pCmd_record[i]->the_command, _theCommand) == 0) {
            pCmd_record[i]->count++;
            sortOccurrence();
            return;
        };
    }

    // if we continue, we need to move our index along
    // then check to resize, and if required we will
    // Then we add a new struct to our struct array

    stpcpy(pCmd_record[cmd_record_index]->the_command, _theCommand);
    pCmd_record[cmd_record_index]->count = 1;

    cmd_record_index++;
    // Check to see if we require more memory
    if (cmd_record_index > numCmds - 1) {
        resizeCmdRecord();
    }

    sortOccurrence();

    return;

}

void sortOccurrence(void) {
    struct cmd_record *temp = malloc(sizeof(struct cmd_record));
    int i, j;

    for (i = 0; i < cmd_record_index; i++) {
        for (j = i + 1; j < cmd_record_index; j++) {
            // if j > i Occurrence Count
            // Swap j / i
            if (pCmd_record[j]->count > pCmd_record[i]->count) {
                // store i into temp
                temp->count = pCmd_record[i]->count;
                strcpy(temp->the_command, pCmd_record[i]->the_command);
                // store j into i
                pCmd_record[i]->count = pCmd_record[j]->count;
                strcpy(pCmd_record[i]->the_command, pCmd_record[j]->the_command);
                // store temp into j (i into j)
                pCmd_record[j]->count = temp->count;
                strcpy(pCmd_record[j]->the_command, temp->the_command);
            }
        }
    }


}

/**
 * Prints occurrences in descending order
 * Limits to five occurrences
 */

void printOccurrences() {
    int i;

    for (i = 0; i < cmd_record_index; i++) {
        if (i > 4) {
            break;
        }

        int j;

        printf("\"");

        for (j = 0; j < (strlen(pCmd_record[i]->the_command) - 1); j++) {
            printf("%c", pCmd_record[i]->the_command[j]);
        }

        if (pCmd_record[i]->count < 2) {
            printf("\"\t\t\t(%i Occurrence)\n", pCmd_record[i]->count);
        } else {
            printf("\"\t\t\t(%i Occurrences)\n", pCmd_record[i]->count);
        }
    }
}
