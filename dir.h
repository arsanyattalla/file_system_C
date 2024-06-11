/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: dir.h
*
* Description: Directory helper functions
*
**************************************************************/

#ifndef _DIR_H
#define _DIR_H
#include "de_s.h"
#include "mfs.h"

/**
 * @brief Creates a new directory.
 *
 * @param parentDirectory Pointer to directory entries of parent directory. Pass a NULL pointer to create the root directory.
 */
int createDir(de_s *parentDirectory);

/**
 * @brief Deletes a directory.
 *
 * @param directoryToDelete Pointer to directory entries of directory to delete.
 */
int deleteDirectory(de_s *directoryToDelete);

/**
 * @brief Gets directory from path.
 *
 * @param pathname Directory pathname.
 * @param parseFinalToken Whether to parse the final token of the given pathname.
 * @return An array of directory entries. Returns NULL if directory is not found.
 */
de_s* getDir(const char *pathname, int parseFinalToken);

/**
 * @brief Gets file from path. Optionally creates file if file does not exist.
 *
 * @param pathname File pathname.
 * @param create Whether to create a file if the passed file does not exist.
 * @return The directory entry of the found file.
 */
de_s* getFile(const char *pathname, int create);

/**
 * @brief Creates a new file at the given path.
 *
 * @param pathname File pathname.
 * @param bytes Size of the new file in bytes.
 * @return The directory entry of the new file.
 */
de_s* createFile(const char *pathname, int bytes);

/**
 * @brief Updates a file at the given path.
 *
 * @param pathname File pathname.
 * @param de Directory entry to update file to.
 * @return 0 on success, 1 on error.
 */
int updateFile(const char *pathname, de_s *de);

/**
 * @brief Gets directory from dirp.
 *
 * @param dirp Directory descriptor.
 * @return An array of directory entries.
 */
de_s* getDirByDirp(fdDir *dirp);

/**
 * @brief Returns number of blocks required to store directory entries.
 */
int getNumBlocksNeededForDirectoryEntries();

/**
 * @brief Returns number of directory entries per directory.
 */
int getNumDirectoryEntriesPerDirectory();

/**
 * @brief Returns number of blocks needed to store given amount of bytes.
 */
int getBlocksNeededForBytes(int bytes);

#endif