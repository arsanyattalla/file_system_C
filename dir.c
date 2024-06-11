/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: dir.c
*
* Description: Directory helper functions
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dir.h"
#include "mfs.h"
#include "de_s.h"
#include "fsLow.h"
#include "freespace.h"
#include "token.h"

int createDir(de_s *parentDirectory)
{
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Malloc space for the array of directory entries
    de_s *directoryEntryArray = malloc(directoryEntriesPerDirectory * sizeof(de_s));
    // Initialize each directory entry to a free state
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        strcpy(directoryEntryArray[i].name, "");
        directoryEntryArray[i].size = 0;
        directoryEntryArray[i].location = 0;
        directoryEntryArray[i].isDir = -1;
        directoryEntryArray[i].createdAt = 0;
        directoryEntryArray[i].modifiedAt = 0;
        directoryEntryArray[i].accessedAt = 0;
    }
    // Allocate space for directory entries
    int newDirectoryLBA = allocateFreeSpace(blocksNeededForDirectoryEntries);
    if (newDirectoryLBA == -1) {
        return -1;
    }
    // Initialize "." directory
    strcpy(directoryEntryArray[0].name, ".");
    directoryEntryArray[0].size = directoryEntriesPerDirectory * sizeof(de_s);
    directoryEntryArray[0].location = newDirectoryLBA;
    directoryEntryArray[0].isDir = 1;
    directoryEntryArray[0].createdAt = time(NULL);
    directoryEntryArray[0].modifiedAt = time(NULL);
    directoryEntryArray[0].accessedAt = time(NULL);
    printDe(directoryEntryArray[0]);
    // Initialize ".." directory
    strcpy(directoryEntryArray[1].name, "..");
    if (parentDirectory == NULL) { // Root directory, ".." should be the same as "."
        directoryEntryArray[1].size = directoryEntryArray[0].size;
        directoryEntryArray[1].location = directoryEntryArray[0].location;
        directoryEntryArray[1].isDir = directoryEntryArray[0].isDir;
        directoryEntryArray[1].createdAt = directoryEntryArray[0].createdAt;
        directoryEntryArray[1].modifiedAt = directoryEntryArray[0].modifiedAt;
        directoryEntryArray[1].accessedAt = directoryEntryArray[0].accessedAt;
    } else {
        directoryEntryArray[1].size = parentDirectory->size;
        directoryEntryArray[1].location = parentDirectory->location;
        directoryEntryArray[1].isDir = parentDirectory->isDir;
        directoryEntryArray[1].createdAt = parentDirectory->createdAt;
        directoryEntryArray[1].modifiedAt = parentDirectory->modifiedAt;
        directoryEntryArray[1].accessedAt = parentDirectory->accessedAt;
    }
    printDe(directoryEntryArray[1]);
    // Write directory
    LBAwrite(directoryEntryArray, blocksNeededForDirectoryEntries, newDirectoryLBA);
    // Clean up
    free(directoryEntryArray);
    // Return directory LBA
    return newDirectoryLBA;
}

int deleteDirectory(de_s *directoryToDelete)
{
    // Check preconditions
    if (directoryToDelete == NULL) {
        printf("error: null directory\n");
        return 1;
    }
    if (directoryToDelete[0].location == vcb->rootLBA) {
        printf("error: cannot remove root directory\n");
        return 1;
    }
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Check that directory is empty
    for (int i = 2; i < directoryEntriesPerDirectory; i++) {
        if (directoryToDelete[i].isDir != -1) {
            printf("error: cannot remove directory, directory is not empty\n");
            return 1;
        }
    }
    // Remove directory from parent
    de_s *parentDirectory = malloc(directoryEntriesPerDirectory * sizeof(de_s));
    LBAread(parentDirectory, blocksNeededForDirectoryEntries, directoryToDelete[1].location);
    int deleted = 0;
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (parentDirectory[i].location == directoryToDelete[0].location) {
            strcpy(parentDirectory[i].name, "");
            parentDirectory[i].size = 0;
            parentDirectory[i].location = 0;
            parentDirectory[i].isDir = -1;
            parentDirectory[i].createdAt = 0;
            parentDirectory[i].modifiedAt = 0;
            parentDirectory[i].accessedAt = 0;
        }
    }
    LBAwrite(parentDirectory, blocksNeededForDirectoryEntries, directoryToDelete[1].location);
    free(parentDirectory);
    // Deallocate directory
    deallocateFreeSpace(directoryToDelete[0].location, blocksNeededForDirectoryEntries);
    // Finish
    return 0;
}

de_s* getDir(const char *pathname, int parseFinalToken)
{
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Malloc space for the array of directory entries
    de_s *tempDirectory = malloc(directoryEntriesPerDirectory * sizeof(de_s));

    // Parse pathname
    // If absolute path, start from root. Else, start from current working directory
    if (pathname[0] == '/') {
        LBAread(tempDirectory, blocksNeededForDirectoryEntries, vcb->rootLBA);
        if (strcmp(pathname, "/") == 0) { // Special case for root pathname
            return tempDirectory;
        }
    } else {
        LBAread(tempDirectory, blocksNeededForDirectoryEntries, currentWorkingDirectoryLBA);
    }
    // Tokenize path
    char *path = strdup(pathname);
    char *token;
    char *nextToken = NULL;
    const char *delim = "/";
    char *saveptr;
    token = strtok_r(path, delim, &saveptr);
    if (token == NULL || token[0] == '\n') {
        printf("error: no path provided\n");
        return NULL;
    }
    nextToken = strtok_r(NULL, delim, &saveptr);
    // Iterate through the path until the entire path has been parsed (except for the final token if specified)
    while ((parseFinalToken == 0 && nextToken != NULL) || (parseFinalToken != 0 && token != NULL)) {
        int found = 0;
        for (int i = 0; i < directoryEntriesPerDirectory; i++) {
            if (strcmp(token, tempDirectory[i].name) == 0 && tempDirectory[i].isDir == 1) {
                LBAread(tempDirectory, blocksNeededForDirectoryEntries, tempDirectory[i].location);
                found = 1;
                break;
            }
        }
        if (found == 0) {
            printf("error: invalid path, could not find directory \"%s\"\n", token);
            return NULL;
        }
        token = nextToken;
        if (token != NULL) {
            nextToken = strtok_r(NULL, delim, &saveptr);
        }
    }
    return tempDirectory;
}

de_s* getFile(const char *pathname, int create)
{
    de_s *ret = NULL;
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Get parent directory of file
    de_s *parentDirectory = getDir(pathname, 0);
    if (parentDirectory == NULL) {
        printf("error: failed to get file \"%s\"\n", pathname);
        free(parentDirectory);
        return NULL;
    }
    // Find file
    char *filename = getLastToken(pathname, "/");
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(filename, parentDirectory[i].name) == 0 && parentDirectory[i].isDir == 0) {
            // Update file directory entry
            parentDirectory[i].accessedAt = time(NULL);
            LBAwrite(parentDirectory, blocksNeededForDirectoryEntries, parentDirectory[0].location);

            // Deep copy directory entry for return value
            ret = malloc(sizeof(de_s));
            strcpy(ret->name, parentDirectory[i].name);
            ret->size = parentDirectory[i].size;
            ret->location = parentDirectory[i].location;
            ret->isDir = parentDirectory[i].isDir;
            ret->createdAt = parentDirectory[i].createdAt;
            ret->modifiedAt = parentDirectory[i].modifiedAt;
            ret->accessedAt = parentDirectory[i].accessedAt;
            break;
        }
    }
    // If file was not found and create parameter is set, create the file
    if (ret == NULL && create != 0) {
        ret = createFile(pathname, 0);
    }
    // Clean up and return
    free(parentDirectory);
    return ret;
}

de_s* createFile(const char *pathname, int bytes)
{
    de_s *ret = NULL;
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Get parent directory of file
    de_s *parentDirectory = getDir(pathname, 0);
    if (parentDirectory == NULL) {
        printf("error: failed to create file \"%s\"\n", pathname);
        free(parentDirectory);
        return NULL;
    }
    // Check that file does not already exist
    char *filename = getLastToken(pathname, "/");
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(filename, parentDirectory[i].name) == 0 && parentDirectory[i].isDir == 0) {
            printf("error: file \"%s\" already exists\n", pathname);
            free(parentDirectory);
            return NULL;
        }
    }
    // Create file entry
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (parentDirectory[i].isDir == -1) {
            // Create file on volume
            int blocks = getBlocksNeededForBytes(bytes);
            int fileLocation = allocateFreeSpace(blocks);
            if (fileLocation == -1) {
                printf("error: not enough space to store file %s\n", pathname);
                free(parentDirectory);
                return NULL;
            }
            char *buf = malloc(bytes);
            LBAwrite(buf, blocks, fileLocation);
            free(buf);
            buf = NULL;

            // Save file in directory entries
            strcpy(parentDirectory[i].name, filename);
            parentDirectory[i].size = bytes;
            parentDirectory[i].location = fileLocation;
            parentDirectory[i].isDir = 0;
            parentDirectory[i].createdAt = time(NULL);
            parentDirectory[i].modifiedAt = time(NULL);
            parentDirectory[i].accessedAt = time(NULL);
            LBAwrite(parentDirectory, blocksNeededForDirectoryEntries, parentDirectory[0].location);

            // Deep copy directory entry for return value
            ret = malloc(sizeof(de_s));
            strcpy(ret->name, parentDirectory[i].name);
            ret->size = parentDirectory[i].size;
            ret->location = parentDirectory[i].location;
            ret->isDir = parentDirectory[i].isDir;
            ret->createdAt = parentDirectory[i].createdAt;
            ret->modifiedAt = parentDirectory[i].modifiedAt;
            ret->accessedAt = parentDirectory[i].accessedAt;
            break;
        }
    }
    if (ret == NULL) {
        printf("error: could not create file %s, no more directory entries remaining\n", pathname);
    }
    // Clean up and return
    free(parentDirectory);
    return ret;
}

int updateFile(const char *pathname, de_s *de)
{
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Get parent directory of file
    de_s *parentDirectory = getDir(pathname, 0);
    if (parentDirectory == NULL) {
        printf("error: failed to get file \"%s\"\n", pathname);
        free(parentDirectory);
        return 1;
    }
    // Find file
    char *filename = getLastToken(pathname, "/");
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(filename, parentDirectory[i].name) == 0 && parentDirectory[i].isDir == 0) {
            // Update file directory entry
            strcpy(parentDirectory[i].name, de->name);
            parentDirectory[i].size = de->size;
            parentDirectory[i].location = de->location;
            parentDirectory[i].isDir = de->isDir;
            parentDirectory[i].createdAt = de->createdAt;
            parentDirectory[i].modifiedAt = de->modifiedAt;
            parentDirectory[i].accessedAt = de->accessedAt;
            LBAwrite(parentDirectory, blocksNeededForDirectoryEntries, parentDirectory[0].location);
            free(parentDirectory);
            return 0;
        }
    }
    // Clean up and return
    free(parentDirectory);
    printf("error: failed to update file %s, file not found\n", pathname);
    return 1;
}

de_s* getDirByDirp(fdDir *dirp)
{
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Malloc space for the array of directory entries
    de_s *tempDirectory = malloc(directoryEntriesPerDirectory * sizeof(de_s));
    // Read directory
    LBAread(tempDirectory, blocksNeededForDirectoryEntries, dirp->directoryStartLocation);
    return tempDirectory;
}

int getNumBlocksNeededForDirectoryEntries()
{
    // Calculate how many blocks are needed to store directory entries for one directory
    int blocksNeededForDirectoryEntries = sizeof(de_s) * MIN_DE_PER_DIR / vcb->blockSize;
    if (sizeof(de_s) * MIN_DE_PER_DIR > blocksNeededForDirectoryEntries * vcb->blockSize) { // Account for truncating from integer division
        blocksNeededForDirectoryEntries++;
    }
    return blocksNeededForDirectoryEntries;
}

int getNumDirectoryEntriesPerDirectory()
{
    // Calculate how many directory entries can fit in the required blocks (using unused space if any is available)
    int directoryEntriesPerDirectory = getNumBlocksNeededForDirectoryEntries() * vcb->blockSize / sizeof(de_s);
}

int getBlocksNeededForBytes(int bytes)
{
    if (bytes == 0) {
        return 1;
    }
    int blocksNeeded = bytes / vcb->blockSize;
    if (bytes > blocksNeeded * vcb->blockSize) {
        blocksNeeded++;
    }
    return blocksNeeded;
}