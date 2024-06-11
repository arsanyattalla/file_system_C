/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: bits.c
*
* Description: Bit helper functions
*
**************************************************************/

#include "bits.h"

void setBit(unsigned char a[], int k)
{
    a[k / 8] |= 1 << (k % 8);
}

void clearBit(unsigned char a[], int k)
{
    a[k / 8] &= ~(1 << (k % 8));
}

int testBit(unsigned char a[], int k)
{
    return ((a[k / 8] & (1 << (k % 8))) != 0);
}