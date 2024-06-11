/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: stack.c
*
* Description: Stack data structure
*
**************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include "stack.h"

struct StackNode* newNode(int data)
{
    struct StackNode* stackNode = (struct StackNode*) malloc(sizeof(struct StackNode));
    stackNode->data = data;
    stackNode->next = NULL;
    return stackNode;
}

int isEmpty(struct StackNode* root)
{
    return !root;
}

void push(struct StackNode** root, int data)
{
    struct StackNode* stackNode = newNode(data);
    stackNode->next = *root;
    *root = stackNode;
}

int pop(struct StackNode** root)
{
    if (isEmpty(*root)) {
        printf("warning: popping from empty stack");
        return 0;
    }
    struct StackNode* temp = *root;
    *root = (*root)->next;
    int popped = temp->data;
    free(temp);

    return popped;
}

int peek(struct StackNode* root)
{
    if (isEmpty(root)) {
        printf("warning: peeking from empty stack");
        return 0;
    }
    return root->data;
}

void freeStack(struct StackNode* root)
{
    while (!isEmpty(root)) {
        pop(&root);
    }
}