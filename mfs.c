/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: mfs.c
*
* Description: Directory functions
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mfs.h"
#include "de_s.h"
#include "fsLow.h"
#include "dir.h"
#include "token.h"
#include "freespace.h"
#include "stack.h"

int fs_mkdir(const char *pathname, mode_t mode)
{
    printf("creating directory...\n");
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Get working directory from pathname
    de_s *tempDirectory = getDir(pathname, 0);
    if (tempDirectory == NULL) {
        printf("error: failed to create directory\n");
        free(tempDirectory);
        return -1;
    }
    // Get new directory name from pathname
    char *path = strdup(pathname);
    char *token;
    char *nextToken = NULL;
    const char *delim = "/";
    char *saveptr;
    token = strtok_r(path, delim, &saveptr);
    if (token == NULL || token[0] == '\n') {
        printf("error: no path provided\n");
        free(tempDirectory);
        return -1;
    }
    nextToken = strtok_r(NULL, delim, &saveptr);
    while (nextToken != NULL) {
        token = nextToken;
        if (token != NULL) {
            nextToken = strtok_r(NULL, delim, &saveptr);
        }
    }
    // Check that a directory with the same name doesn't already exist
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(tempDirectory[i].name, token) == 0 && tempDirectory[i].isDir == 1) {
            printf("error: directory \"%s\" already exists\n", token);
            free(tempDirectory);
            return -1;
        }
    }
    // Find the next available directory entry in the temporary working directory
    int unusedDeIndex = -1;
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (tempDirectory[i].isDir == -1) {
            unusedDeIndex = i;
            break;
        }
    }
    if (unusedDeIndex == -1) {
        printf("error: directory \"%s\" is out of available directory entries\n", token);
        free(tempDirectory);
        return -1;
    }
    // Create new directory
    int newDirectoryLBA = createDir(&tempDirectory[0]);
    if (newDirectoryLBA == -1) {
        printf("error: not enough free space to create directory\n");
    }
    // Update parent directory
    strcpy(tempDirectory[unusedDeIndex].name, token);
    tempDirectory[unusedDeIndex].size = directoryEntriesPerDirectory * sizeof(de_s);
    tempDirectory[unusedDeIndex].location = newDirectoryLBA;
    tempDirectory[unusedDeIndex].isDir = 1;
    LBAwrite(tempDirectory, blocksNeededForDirectoryEntries, tempDirectory[0].location);

    // Clean up
    free(tempDirectory);
    // Return new directory LBA
    return newDirectoryLBA;
}

int fs_rmdir(const char *pathname)
{
    printf("removing directory...\n");
    // Get directory from pathname
    de_s *directoryToRemove = getDir(pathname, 1);
    if (directoryToRemove == NULL) {
        printf("error: failed to remove directory \"%s\"\n", pathname);
        free(directoryToRemove);
        return -1;
    }
    // Delete directory
    int res = deleteDirectory(directoryToRemove);
    // Clean up
    free(directoryToRemove);
    // Return result
    return res;
}

fdDir * fs_opendir(const char *name)
{
    fdDir *dirp = malloc(sizeof(fdDir));
    // Get directory from path
    de_s *tempDirectory;
    tempDirectory = getDir(name, 1);
    if (tempDirectory == NULL) {
        printf("error: failed to open directory \"%s\"\n", name);
        return NULL;
    }
    dirp->d_reclen = getNumDirectoryEntriesPerDirectory();
    dirp->directoryStartLocation = tempDirectory[0].location;
    dirp->dirEntryPosition = 0;
    // Clean up
    free(tempDirectory);
    return dirp;
}

fs_diriteminfo *fs_readdir(fdDir *dirp)
{
    fs_diriteminfo *di = malloc(sizeof(fs_diriteminfo));
    // Get directory from dirp
    de_s *tempDirectory = getDirByDirp(dirp);
    if (tempDirectory == NULL) {
        printf("error: failed to read directory\n");
        return NULL;
    }
    // Find next valid directory entry
    while (dirp->dirEntryPosition < dirp->d_reclen) {
        if (tempDirectory[dirp->dirEntryPosition].isDir != -1) {
            break;
        }
        dirp->dirEntryPosition++;
    }
    if (dirp->dirEntryPosition >= dirp->d_reclen) {
        return NULL;
    }
    strcpy(di->d_name, tempDirectory[dirp->dirEntryPosition].name);
    di->d_reclen = tempDirectory[dirp->dirEntryPosition].size;
    di->fileType = tempDirectory[dirp->dirEntryPosition].isDir;
    // Increment directory entry position
    dirp->dirEntryPosition++;
    // Clean up
    free(tempDirectory);
    return di;
}

int fs_closedir(fdDir *dirp)
{
    free(dirp);
    dirp = NULL;
    return 0;
}

char * fs_getcwd(char *buf, size_t size)
{
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Malloc space for the array of directory entries
    de_s *tempDirectory = malloc(directoryEntriesPerDirectory * sizeof(de_s));
    // Read directory
    LBAread(tempDirectory, blocksNeededForDirectoryEntries, currentWorkingDirectoryLBA);
    // Traverse to root, keeping track of paths
    struct StackNode *stack = NULL;
    while (tempDirectory[0].location != vcb->rootLBA) {
        push(&stack, tempDirectory[0].location);
        LBAread(tempDirectory, blocksNeededForDirectoryEntries, tempDirectory[1].location);
    }
    // Concatenate working directory name
    size_t curChar = 0;
    strncpy(&buf[curChar], "/", size - curChar);
    curChar += 1;
    int popped;
    while (!isEmpty(stack)) {
        popped = pop(&stack);
        for (int i = 0; i < directoryEntriesPerDirectory; i++) {
            if (tempDirectory[i].location == popped) {
                strncpy(&buf[curChar], tempDirectory[i].name, size - curChar);
                curChar += strlen(tempDirectory[i].name);
                if (!isEmpty(stack)) {
                    strncpy(&buf[curChar], "/", size - curChar);
                    curChar += 1;
                }
                LBAread(tempDirectory, blocksNeededForDirectoryEntries, popped);
                break;
            }
        }
    }
    buf[size - 1] = '\0';
    // Clean up
    free(tempDirectory);
    return buf;
}

int fs_setcwd(char *buf)
{
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Malloc space for the array of directory entries
    de_s *tempDirectory = malloc(directoryEntriesPerDirectory * sizeof(de_s));
    // Find new working directory
    tempDirectory = getDir(buf, 1);
    if (tempDirectory == NULL) {
        printf("error: failed to change working directory\n");
        return -1;
    }
    currentWorkingDirectoryLBA = tempDirectory->location;
    return 0;
}

int fs_isFile(char *path)
{
    // Get parent directory from path
    de_s *tempDirectory = getDir(path, 0);
    if (tempDirectory == NULL) {
        return 0;
    }
    // Get name of directory of interest from path
    const char *pathname = path;
    char *directoryName = getLastToken(pathname, "/");
    // Find directory entry with matching name and check if is directory
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(tempDirectory[i].name, directoryName) == 0 && tempDirectory[i].isDir == 0) {
            free(tempDirectory);
            return 1;
        }
    }
    // Clean up
    free(tempDirectory);
    return 0;
}

int fs_isDir(char *path)
{
    // Get parent directory from path
    de_s *tempDirectory = getDir(path, 0);
    if (tempDirectory == NULL) {
        return 0;
    }
    // Get name of directory of interest from path
    const char *pathname = path;
    char *directoryName = getLastToken(pathname, "/");
    // Find directory entry with matching name and check if is directory
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(tempDirectory[i].name, directoryName) == 0 && tempDirectory[i].isDir == 1) {
            free(tempDirectory);
            return 1;
        }
    }
    // Clean up
    free(tempDirectory);
    return 0;
}

int fs_delete(char *filename)
{
    printf("removing file...\n");
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Get filename
    char *fn = getLastToken(filename, "/");
    // Get parent directory of file
    de_s *parentDirectory = getDir(filename, 0);
    // Find directory entry of file
    int res = 1;
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(parentDirectory[i].name, fn) == 0 && parentDirectory[i].isDir == 0) {
            deallocateFreeSpace(parentDirectory[i].location, parentDirectory[i].size / vcb->blockSize);
            strcpy(parentDirectory[i].name, "");
            parentDirectory[i].size = 0;
            parentDirectory[i].location = 0;
            parentDirectory[i].isDir = -1;
            parentDirectory[i].createdAt = 0;
            parentDirectory[i].modifiedAt = 0;
            parentDirectory[i].accessedAt = 0;
            res = 0;
        }
    }
    // Update parent directory
    if (res == 0) {
        LBAwrite(parentDirectory, blocksNeededForDirectoryEntries, parentDirectory[0].location);
    }
    // Clean up
    free(parentDirectory);
    // Return result
    return res;
}

int fs_move(char *source, char *dest)
{
    printf("moving file...\n");
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Get filename
    char *fn = getLastToken(source, "/");
    char *newFilename = getLastToken(dest, "/");
    const int MAX_FILENAME_LENGTH = 256;
    if (strnlen(newFilename, MAX_FILENAME_LENGTH) <= 0 || strnlen(newFilename, MAX_FILENAME_LENGTH) > MAX_FILENAME_LENGTH - 1) {
        printf("error: destination filename length out of range\n");
        return 1;
    }
    // Get parent directory of file
    de_s *parentDirectory = getDir(source, 0);
    if (parentDirectory == NULL) {
        printf("error: source file does not exist\n");
        return 1;
    }
    // Find parent directory entry of file
    int res = 1;
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(parentDirectory[i].name, fn) == 0 && parentDirectory[i].isDir == 0) {
            // Find destination parent directory of file
            de_s *destinationDirectory = getDir(dest, 0);
            if (destinationDirectory) {
                // Handle special case if destination and source directory are the same
                if (destinationDirectory[0].location == parentDirectory[0].location) {
                    // Update file in parent directory
                    strncpy(parentDirectory[i].name, newFilename, MAX_FILENAME_LENGTH);
                    parentDirectory[i].modifiedAt = time(NULL);
                    // Update filesystem
                    LBAwrite(parentDirectory, blocksNeededForDirectoryEntries, parentDirectory[0].location);
                    // Clean up and return
                    free(destinationDirectory);
                    free(parentDirectory);
                    return 0;
                } else {
                    // Find next available directory entry in destination directory
                    for (int j = 0; j < directoryEntriesPerDirectory; j++) {
                        if (destinationDirectory[j].isDir == -1) {
                            // Move file to destination directory
                            strncpy(destinationDirectory[j].name, newFilename, MAX_FILENAME_LENGTH);
                            destinationDirectory[j].size = parentDirectory[i].size;
                            destinationDirectory[j].location = parentDirectory[i].location;
                            destinationDirectory[j].isDir = parentDirectory[i].isDir;
                            destinationDirectory[j].createdAt = parentDirectory[i].createdAt;
                            destinationDirectory[j].modifiedAt = time(NULL);
                            destinationDirectory[j].accessedAt = parentDirectory[i].size;
                            // Remove file from parent directory
                            strcpy(parentDirectory[i].name, "");
                            parentDirectory[i].size = 0;
                            parentDirectory[i].location = 0;
                            parentDirectory[i].isDir = -1;
                            parentDirectory[i].createdAt = 0;
                            parentDirectory[i].modifiedAt = 0;
                            parentDirectory[i].accessedAt = 0;
                            // Update filesystem
                            LBAwrite(parentDirectory, blocksNeededForDirectoryEntries, parentDirectory[0].location);
                            LBAwrite(destinationDirectory, blocksNeededForDirectoryEntries, destinationDirectory[0].location);
                            // Clean up and return
                            free(destinationDirectory);
                            free(parentDirectory);
                            return 0;
                        }
                    }
                }
                // If reached, then destination directory does not have any available directory entries
                free(destinationDirectory);
                free(parentDirectory);
                printf("error: destination directory is full\n");
                return 1;
            }
            // If reached, then destination directory does not exist
            free(destinationDirectory);
            free(parentDirectory);
            printf("error: destination is not a directory or does not exist\n");
            return 1;
        }
    }
    // If reached, then parent directory does not contain source file
    free(parentDirectory);
    printf("error: source file does not exist\n");
    return 1;
}

int fs_stat(const char *path, struct fs_stat *buf)
{
    // Get required values
    int directoryEntriesPerDirectory = getNumDirectoryEntriesPerDirectory();
    int blocksNeededForDirectoryEntries = getNumBlocksNeededForDirectoryEntries();
    // Get filename
    char *fn = getLastToken(path, "/");
    // Get parent directory of file
    de_s *parentDirectory = getDir(path, 0);
    // Find directory entry of file
    int res = 1;
    for (int i = 0; i < directoryEntriesPerDirectory; i++) {
        if (strcmp(parentDirectory[i].name, fn) == 0) {
            buf->st_size = (off_t) parentDirectory[i].size;
            buf->st_blksize = (blksize_t) vcb->blockSize;
            buf->st_blocks = (blkcnt_t) getBlocksNeededForBytes(parentDirectory[i].size);
            buf->st_accesstime = parentDirectory[i].accessedAt;
            buf->st_modtime = parentDirectory[i].modifiedAt;
            buf->st_createtime = parentDirectory[i].createdAt;
            res = 0;
        }
    }
    // Clean up
    free(parentDirectory);
    // Return result
    return res;
}