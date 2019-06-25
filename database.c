#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include "utils.h"

// Okay. Python and Java have made me forget C. I'm very rusty with pointers and memory allocation and structs and the like
// Figuring this out as i go along and determined to write this in C

// VSCode debugger does not support taking params from stdin. GDB it is. 


//4kb. 
#define PAGE_SIZE 4096
#define MAX_PAGES 100


#define SIZE_USERNAME 50
#define SIZE_EMAIL 50

typedef struct Row {
    uint64_t id;
    char username[SIZE_USERNAME];
    char email[SIZE_EMAIL];
} Row;

typedef struct table {
    int count;
    void* pages[MAX_PAGES];
} Table;

Table* table;

// https://stackoverflow.com/questions/3553296/sizeof-single-struct-member-in-c
const  size_t ID_SIZE = sizeof( ((Row*)0) -> id);
const  size_t USERNAME_SIZE = sizeof( ((Row*)0) -> id);
const  size_t EMAIL_SIZE = sizeof( ((Row*)0) -> id);
const  size_t ROW_SIZE = sizeof(Row);

const int ID_OFFSET = 0;
const int USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const int EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

const int ROWs_PER_PAGE = PAGE_SIZE/ROW_SIZE;
const int MAX_ROWS = MAX_PAGES * ROWs_PER_PAGE;




// Statuses for executibng meta commands
typedef enum metastatuses {EXEC_SUCCESS, EXEC_FAILURE, UNRECOGNIZED, SIZE_OVERFLOW} MetaStatus;

// Statementtypes. DDL and DML here
typedef enum statementtype {INSERT_STATEMENT, SELECT_STATEMENT, DELETE_STATEMENT, CREATE_STATEMENT, UPDATE_STATEMENT} StatementType;





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
    Row row;
} Statement;


MetaStatus exec_meta_command(char*);
MetaStatus prepare_statement(char*);
MetaStatus exec_insert(Statement*);
MetaStatus exec_select(Statement*);

// Move these to a different file. Can't figure out how to work  with extern variables properly.
void putdata(Row* row, void* destination) {
    memcpy(destination + ID_OFFSET, &(row->id), ID_SIZE);
    memcpy(destination + USERNAME_OFFSET, &(row->username), USERNAME_SIZE);
    memcpy(destination + EMAIL_OFFSET, &(row->email), EMAIL_SIZE);
}

void getdata(Row* row, void* source) {
    memcpy(&(row->id), source + ID_OFFSET, ID_SIZE);
    memcpy(&(row->username), source + USERNAME_OFFSET, USERNAME_SIZE);
    memcpy(&(row->email), source + EMAIL_OFFSET, EMAIL_SIZE);
}


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

// We init the table. There is no free cause we nevre want to free this. Termination is our free
void init_table() {
    table = malloc(sizeof(Table));
    table->count  = 0;
    int i = 0;
    for(i = 0; i < MAX_PAGES; i++) {
        table->pages[i] = NULL;
    } 
}

void print(Row* result) {
    printf("ID\tName\tEmail\n");
    printf("%d\t%s\t%s\n", result->id, result->username, result->email);
}

// Allocate space for the page
void* page_index(int currentindex) {
    int pagenumber = currentindex / ROWs_PER_PAGE;
    void* page = table->pages[pagenumber];
    if(page == NULL) {
        table->pages[pagenumber] = malloc(PAGE_SIZE);
    }
    return table->pages[pagenumber] + (currentindex % ROWs_PER_PAGE) * ROW_SIZE;
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
                case SIZE_OVERFLOW:
                    printf("Not enough memory\n.");
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
        return exec_insert(statement);
    }
    else if(strncmp("select", statement->statement, 6) == 0) {
        statement->statementtype = SELECT_STATEMENT;
        return exec_select(statement);
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

// Add formatting and specific parameters here. For now it's a scan of everything
MetaStatus exec_select(Statement* statement) {
    Row result;
    for (int i = 0; i < table->count; i++) {
        getdata(page_index(i), &result);
        print(&result);
  }
  return EXEC_SUCCESS;
}

MetaStatus exec_insert(Statement* statement) {
    
    if(table->count >= MAX_ROWS) {
        return SIZE_OVERFLOW;
    }
    //strtok later. For now a simple sscanf
    // Structure 
    // insert into table id username email 1 rohit rohit@rohit.com
    sscanf(statement->statement, "insert %jd %s %s", &(statement->row.id), &(statement->row.username), &(statement->row.email));
    putdata(&(statement->row), page_index(table->count));
    table->count = table->count + 1;

    return EXEC_SUCCESS;
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
    init_table();
    make_cli();
    read_input();
}