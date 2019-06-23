// We'll put our utils here. Apparanty converting a string to lowercase in C is more tedious than I remember. 

#include<ctype.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>

char* strtolower(char* string) {

    char* stringlwr = (char*) malloc (strlen(string) + 1);    
    strcpy(stringlwr, string);
    for(int i = 0; stringlwr[i]; i++) {
        stringlwr[i] = tolower(stringlwr[i]);
    }
    return stringlwr;
}

// Maybe use curses later
void clearscr() {
    #ifdef _WIN32
        system("cls");
    #elif defined(unix) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
        system("clear");
    #endif
}


char* ltrim(char* str, const char* seps) {
    size_t totrim;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    totrim = strspn(str, seps);
    if (totrim > 0) {
        size_t len = strlen(str);
        if (totrim == len) {
            str[0] = '\0';
        }
        else {
            memmove(str, str + totrim, len + 1 - totrim);
        }
    }
    return str;
}

char* rtrim(char* str, const char* seps) {
    int i;
    if (seps == NULL) {
        seps = "\t\n\v\f\r ";
    }
    i = strlen(str) - 1;
    while (i >= 0 && strchr(seps, str[i]) != NULL) {
        str[i] = '\0';
        i--;
    }
    return str;
}

char* trim(char* str, const char* seps) {
    return ltrim(rtrim(str, seps), seps);
}