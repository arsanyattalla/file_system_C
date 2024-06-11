/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: de_s.c
*
* Description: Directory entry helper functions
*
**************************************************************/

#include <stdio.h>

#include "de_s.h"

void printDe(const de_s de)
{
    printf("\nName: %s\n", de.name);
    printf("Size: %i\n", de.size);
    printf("Block location: %i\n", de.location);
    printf("Is directory: %i\n", de.isDir);
    printf("Created at: %li\n", de.createdAt);
    printf("Modified at: %li\n", de.modifiedAt);
    printf("Accessed at: %li\n", de.accessedAt);
}