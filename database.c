#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <stdbool.h> 
#include "utils.h"

// Okay. Python and Java have made me forget C. I'm very rusty with pointers and memory allocation and structs and the like
// Figuring this out as i go along and determined to write this in C

// VSCode debugger does not support taking params from stdin. GDB it is. 


//4kb. 
#define PAGE_SIZE 4096
#define MAX_PAGES 100


#define SIZE_USERNAME 50
#define SIZE_EMAIL 50

#define PAGEFILE "db.txt"
#define META_FILE "meta.info"


// The pager is the interface between the data file and the data 
typedef struct pager {
    void* pages[MAX_PAGES];
    FILE* file;
    long int filelength;
} Pager;

Pager *pager;

typedef struct Row {
    int id;
    char username[SIZE_USERNAME];
    char email[SIZE_EMAIL];
} Row;

typedef struct table {
    int count;
    Pager* pager;
} Table;

Table* table;

// A cursor points to the current row so we can perform operations
// We have a cursor for the start and end of the table
typedef struct cursor {

    // So we can deal with multiple tables later
    Table* table;

    // We need only a reference to the row in question. The actual contents are irrelevant to the cursor
    int row;

    // So we know if it's the start or end cursor
    bool isAtEnd;

} Cursor;

// https://stackoverflow.com/questions/3553296/sizeof-single-struct-member-in-c
const  size_t ID_SIZE = sizeof( ((Row*)0) -> id);
const  size_t USERNAME_SIZE = sizeof( ((Row*)0) -> username);
const  size_t EMAIL_SIZE = sizeof( ((Row*)0) -> email);
const  size_t ROW_SIZE = sizeof(Row);

const int ID_OFFSET = 0;
const int USERNAME_OFFSET = ID_OFFSET + ID_SIZE;
const int EMAIL_OFFSET = USERNAME_OFFSET + USERNAME_SIZE;

const int ROWS_PER_PAGE = PAGE_SIZE/ROW_SIZE;
const int MAX_ROWS = MAX_PAGES * ROWS_PER_PAGE;




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


// A holder for a statement and it's meta information. At thisn point I'm just re-inventing classes here
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


// We ride on naglfar and flush the pages to the disk
void naglfar(int pagenumber) {

    if(pager->pages[pagenumber] == NULL) {
        // Doing a lot of exits. Need to figure out a better flow
        printf("Passed null page to naglfr\n");
        exit(0);
    }

    if(fseek(pager->file, pagenumber * PAGE_SIZE, SEEK_SET) != 0) {
        printf("Can't seek\n");
        exit(0);
    }

    size_t written = fwrite(pager->pages[pagenumber], PAGE_SIZE, 1, pager->file);

    if(!written) {
        printf("Couldn't write to file\n");
        exit(0);
    }


}

// The end of all things
void ragnarok() {
    int numpages = table->count / ROWS_PER_PAGE;
    for(int i = 0; i <= numpages; i++) {
        if(pager->pages[i] == NULL) {
            continue;
        }

        naglfar(i);
        free(pager->pages[i]);
        pager->pages[i] = NULL;
    }

    FILE *meta = fopen(META_FILE, "w");

    fprintf(meta, "%d\n", table->count);

    
    fclose(pager->file);
    fclose(meta);
    free(pager);
    free(table);
}

void housekeeping(InputBuffer* buffer) {
    // Passing reference here
    free(buffer->input);
    ragnarok();

}

// We init the table. There is no free cause we nevre want to free this. Termination is our free
void init() {

    pager = malloc(sizeof(Pager));

    FILE* pagefile = fopen(PAGEFILE,"r+");

    FILE* metafile = fopen(META_FILE, "r+");
    // Nothing more to do. Quit. No operating soleley in memeory. We're not redis
    if(pagefile == NULL) {
        exit(0);
    }

    int i = 0;
    for(i = 0; i < MAX_PAGES; i++) {
        pager->pages[i] = NULL;
    } 

    fseek(pagefile, 0L, SEEK_END);
    long int length = ftell(pagefile);

    pager->file = pagefile;
    pager->filelength = length;

    table = malloc(sizeof(Table));
    table->count  = 0;
    table->pager = pager;

    int storedcount = 0;
    FILE *meta = fopen(META_FILE,"r");
    fscanf(meta,"%d",&storedcount);
    fclose(meta);
    if( storedcount > 0) {
        table->count = storedcount;
    }


  
}

void print(Row* result) {
    printf("ID \t Name \t Email \n");
    printf("%d \t %s \t %s \n", result->id, result->username, result->email);
}

// Allocate space for the page
void* page_index(int currentindex) {
    int pagenumber = currentindex / ROWS_PER_PAGE;

    // Out of memory exception
    if(pagenumber > MAX_PAGES) {
        exit(0);
    }
    
    if(pager->pages[pagenumber] == NULL) {

        // We don't have it in memory. Getting it from the disk.
        pager->pages[pagenumber] = malloc(PAGE_SIZE);
        int numpages = pager->filelength / PAGE_SIZE;
        
        if(pagenumber <= numpages) {
            fseek(pager->file, pagenumber * PAGE_SIZE, SEEK_SET);
            fread(pager->pages[pagenumber], PAGE_SIZE, 1, pager->file);
        }
    }

    
    return pager->pages[pagenumber] + (currentindex % ROWS_PER_PAGE) * ROW_SIZE;
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
        getdata(&result, page_index(i));
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
    sscanf(statement->statement, "insert %d %s %s", &(statement->row.id), &(statement->row.username), &(statement->row.email));
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
    init();
    make_cli();
    read_input();
}