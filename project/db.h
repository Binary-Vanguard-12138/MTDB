#ifndef DB_H_
#define DB_H_

#include <pthread.h>

typedef struct node {
    char *name;
    char *value;
    struct node *lchild;
    struct node *rchild;
} node_t;

extern node_t head;

node_t *search(char *name, node_t *parent, node_t **parentp);

/**
  * The db_query() function calls search() to retrieve the node associated with the 
  * given key. If such a node is found, the function retrieves the value stored in 
  * that node and returns it. 
  */
void db_query(char *name, char *result, int len);

/**
  * db_add() uses search() to determine if the given key is already in the database. 
  * If the key is not in the database, the function creates a new node with the given 
  * key and value and inserts this node into the database as a child of the parent node 
  * returned by search().
  * Returns 1 on success and 0 on failure
  */
int db_add(char *name, char *value);

/**
  * The db_remove() function calls search() to retrieve the node associated with the given 
  * key. If such a node is found, the function must delete it while preserving the tree 
  * ordering constraints. There are three cases that may occur, depending on the children of 
  * the node to be removed:
  * 	- Both children are NULL: In this case, the function simply deletes the node and sets 
  *	the corresponding pointer of the parent node to NULL.
  *	- One child is NULL: In this case, the function replaces the node with its non-NULL child.
  *	- Neither child is NULL: In this case, the function finds the leftmost child of the node's 
  *	right subtree and replaces the removed node with this one. Since the replacement node has no 
  * 	left child, it is easy to remove it from its current position, and since it is the leftmost 
  * 	child of its subtree it can occupy the position of the deleted node and satisfy the tree's 
  *	ordering constraints.
  */
int db_remove(char *name);

/** 
  * The interpret_command() function gets called by the server to interpret a command from a client, 
  * call database functions, and store the response.
  */
void interpret_command(char *command, char *response, int resp_capacity);

/**
  * The db_print() function performs a pre-order traversal of the tree, printing each  node's 
  representation and then recursively printing its left and right subtrees. It will attempt 
  * to print to a file with the given filename, or stdout if none is provided. 
  * Returns 0 on success or -1 on failure (invalid file)
  */
int db_print(char *filename);

/**
  * The db_cleanup() function frees all dynamically-allocated nodes in the database. This function 
  * should be used in server.c to clean up the database before exiting. You should only do this when 
  * you are certain that no other threads are currently using or will be using the database. You should 
  * check the variables in the server_control_t struct located near the top of server.c to ensure that all 
  * threads are terminated before you call db_cleanup in your main thread.
  */
void db_cleanup(void);

#endif  // DB_H_
