/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: stack.h
*
* Description: Stack data structure
*
**************************************************************/

#ifndef _STACK_H
#define _STACK_H

struct StackNode
{
    int data;
    struct StackNode* next;
};

int isEmpty(struct StackNode* root);
void push(struct StackNode** root, int data);
int pop(struct StackNode** root);
int peek(struct StackNode* root);
void freeStack(struct StackNode* root);

#endif