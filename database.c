#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "utils.h"

// Okay. Python and Java have made me forget C. I'm very rusty with pointers and memory allocation and structs and the like
// Figuring this out as i go along and determined to write this in C

// Statuses for executibng meta commands
typedef enum metastatuses {EXEC_SUCCESS, EXEC_FAILURE, UNRECOGNIZED} MetaStatus;

// Statementtypes. DDL and DML here
typedef enum statementtype {INSERT_STATEMENT, SELECT_STATEMENT, DELETE_STATEMENT, CREATE_STATEMENT, UPDATE_STATEMENT} StatementType;

MetaStatus exec_meta_command(char*);
MetaStatus prepare_statement(char*);

// VSCode debugger does not support taking params from stdin. GDB it is. 

// I haven't initialised the buffer. Or done an malloc. Still works for getline
// Too much java and python have made me forget my C. Need to brush up
typedef struct InputBufferStruct {
    char* input;
    size_t input_size, buffer_size;
} InputBuffer;

InputBuffer buffer;


// A holder for a statement and it's meta information. At thisn point I'm just reventing classes here
typedef struct Statement {
    StatementType statementtype;
    char* statement;
} Statement;


// Make this into a file later. 
void print_help() {
    printf("\nList of valid commands\n");
    printf(".help - Help. You're here right now\n");
    printf(".exit - Exit\n");
    printf(".clear - Clears the screen\n");
    printf("Yeah. Not very helpful, is this?\n\n");
}

void make_cli() {
    printf("\n>");
}

void housekeeping(InputBuffer* buffer) {
    // Passing reference here
    free(buffer->input);
}

void read_input() {
    while(1) {
        size_t inputlength = getline(&buffer.input, &buffer.input_size, stdin);

        if(buffer.input == NULL)  {
            perror("Error: " );
            exit(1);
        }

        if(inputlength <= 0) {
            printf("\n Error in reading input");
            exit(1);
        }

        // https://www.freebsd.org/cgi/man.cgi?query=strcspn&sektion=3
        buffer.input[strcspn(buffer.input, "\n")] = 0;

        if(strcmp("help", strtolower(buffer.input)) == 0) {
            printf("Type .help for help. Meta commands start with a (.) dot\n");
        }
        else if (buffer.input[0] == '.') {
            switch(exec_meta_command(buffer.input)) {

                // Pipe result here
                case EXEC_SUCCESS:
                    printf("Executed command %s \n", buffer.input);
                    break;

                // Pipe failure reason here
                case EXEC_FAILURE:
                    printf("Execution failed for command %s \n", buffer.input);
                    break;

                case UNRECOGNIZED:
                    printf("The command %s is unrecognized. Type .help for a list of valid commands\n", buffer.input);
                    break;
            }
        }
        else {
            switch(prepare_statement(buffer.input)) {
                case EXEC_SUCCESS:
                    printf("Statement executed\n");
                    break;
                case EXEC_FAILURE:
                    printf("Error in executing statement\n");
                    break;
                case UNRECOGNIZED:
                    printf("The statement is unrecognized\n");
                    break;
            }
        }
        make_cli();
    }    
}

MetaStatus prepare_statement(char* input_statement) {
    Statement* statement = malloc(sizeof(Statement));
    // Append this to a log here
    statement->statement = strtolower(trim(input_statement, NULL));

    // Strtok() this maybe
    if(strncmp("insert", statement->statement, 6) == 0) {
        statement->statementtype = INSERT_STATEMENT;
        printf("The insert statement is of type %d \n", statement->statementtype);
        return EXEC_SUCCESS;
    }
    else if(strncmp("delete", statement->statement, 6) == 0) {
        statement->statementtype = DELETE_STATEMENT;
        printf("The delete statement is of type %d \n", statement->statementtype);
        return EXEC_SUCCESS;
    } 
    else {
        return UNRECOGNIZED;
    }

    return EXEC_FAILURE;
}


MetaStatus exec_meta_command(char* command) {
       
    if(strcmp(".exit", command) == 0) {
        housekeeping(&buffer);
        printf("\nbye, bye\n");
        exit(0);
    } 
    else if(strcmp(".help", command) == 0) {
        print_help();
        return EXEC_SUCCESS;
    }
    else if(strcmp(".clear", command) == 0 ) {
        clearscr();
        return EXEC_SUCCESS;
    }
    else {
        return UNRECOGNIZED;
    }

    return EXEC_FAILURE;
}

int main(int argc, char* argv[]) {
    make_cli();
    read_input();
}