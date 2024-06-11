/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: fd_s.h
*
* Description: Directory entry node structure
*
**************************************************************/

#ifndef _DE_S_H
#define _DE_S_H
#include <time.h>

typedef struct de_s
{
    char name[256]; // Directory/file name
    int size; // Directory/file size in bytes // TODO: use size_t
    int location; // Directory/file block position
    int isDir; // Whether entry is directory or file
    time_t createdAt; // Directory/file creation, Unix time
    time_t modifiedAt; // Directory/file last modification, Unix time
    time_t accessedAt; // Directory/file last access, Unix time
} de_s;

void printDe(const de_s de);

#endif