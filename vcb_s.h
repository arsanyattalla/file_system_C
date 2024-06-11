/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: vcb_s.h
*
* Description: Volume control block structure
*
**************************************************************/

#ifndef _VCB_S_H
#define _VCB_S_H

#define VCB_MAGIC_NUMBER 0x13371337 // The magic number signalling VCB presence on disk.

typedef struct vcb_s
{
    int signature; // Magic number, signals presence of VCB on disk when present
    int blockCount; // Number of blocks in volume
    int blockSize; // Size of blocks in volume
    int rootLBA; // Position of root directory in volume
    int freeSpaceLBA; // Position of free space bitmap in volume
} vcb_s;

#endif