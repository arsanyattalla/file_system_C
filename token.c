/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: token.c
*
* Description: String tokenization helper functions
*
**************************************************************/

#include <stdio.h>
#include <string.h>

#include "token.h"

char* getLastToken(const char *s, const char *delim)
{
    char *str = strdup(s);
    char *token;
    char *nextToken = NULL;
    char *saveptr;
    token = strtok_r(str, delim, &saveptr);
    nextToken = strtok_r(NULL, delim, &saveptr);
    while (nextToken != NULL) {
        token = nextToken;
        nextToken = strtok_r(NULL, delim, &saveptr);
    }
    return token;
}