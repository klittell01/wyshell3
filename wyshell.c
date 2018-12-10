/*
 * wyshell.c
 * Author: Kevin Littell
 * Date: 12-03-2018
 * COSC 3750, program 10
 * simple shell utility
 */
// possible 1270
// have      704
// availble  400
// (.80 + .93) / 2 = .86

// .86

#include "stdbool.h"
#include "unistd.h"
#include "errno.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "fcntl.h"
#include "wyscanner.h"
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>

struct Word{
    struct Word *next, *prev;
    char *command;
};

struct Node{
        struct Node *next, *prev;
        char *command;
        int count;
        bool background;
        struct Word *arg_list;
        int in, out, err;
        char *inFile, *outFile, *errFile;
};

char *tokens[]={ "QUOTE_ERROR", "ERROR_CHAR", "SYSTEM_ERROR",
               "EOL", "REDIR_OUT", "REDIR_IN", "APPEND_OUT",
               "REDIR_ERR", "APPEND_ERR", "REDIR_ERR_OUT", "SEMICOLON",
               "PIPE", "AMP", "WORD" };


/* Function to reverse the linked list */
static int reverse(struct Node** headRef)
{
    int count = 0;
    int nodeCount = 0;
    struct Word* prev   = NULL;
    struct Word* current = (*headRef)->arg_list;
    struct Word* next = NULL;

    struct Node* nodeCurrent = *headRef;
    struct Node* nodeNext = NULL;
    // loop through all the command nodes so we can reverse all the arg list
    do{
        nodeCount++;
        while (current != NULL){
            count++;
            // Store next
            next  = current->next;

            // Reverse current node's pointer
            current->next = prev;

            // Move pointers one position ahead.
            prev = current;
            current = next;
        }
        nodeCurrent->count = count;
        count = 0;
        nodeNext = nodeCurrent->next;
        nodeCurrent->arg_list = prev;
        prev = NULL;
        if(nodeNext != NULL){
            nodeCurrent = nodeNext;
        } else {
            return nodeCount;
        }
        current = nodeCurrent->arg_list;
    } while(nodeCurrent != NULL);

    //(*headRef)->arg_list = prev;
    return nodeCount;
}

/* Function to push a node */
void push(struct Node** headRef, char * data)
{
    struct Word* newWord =
        (struct Word*) calloc(1, sizeof(struct Word));
    newWord->command = strdup(data);
    newWord->next = (*headRef)->arg_list;
    (*headRef)->arg_list = newWord;
}

/* Function to print linked list */
void printList(struct Word *head)
{
  struct Word *tmp = head;
  while(tmp != NULL)
  {
      printf(" %s,", tmp->command);
      tmp = tmp->next;
  }
  printf("\n");
}

// pass a pointer to the head of our command nodes and this executes
// builds the argument list for each command and processes them.
void Executer(struct Node * node, int nodeCount){
    int count = 0;
    struct Node * myNode = (struct Node*) calloc(1, sizeof(struct Node));
    myNode = node;
    int stdin_copy = dup(STDIN_FILENO);
    int stdout_copy = dup(STDOUT_FILENO);
    while(count < nodeCount){
        count++;

        // build argument list for each command
        int argCount = myNode->count;
        char ** myArgv;
        myArgv = calloc((argCount + 2), sizeof(char*));
        myArgv[0] = strdup(myNode->command);
        struct Word *tmp = myNode->arg_list;
        for(int i = 0; i < argCount; i++){
            myArgv[i + 1] = strdup(tmp->command);
            tmp = tmp->next;
        }



        int status;
        pid_t childWait;
        pid_t myExec;
        pid_t frtn = fork();
        if(frtn == 0){
            // child process
            // begin executing and forking
            int fd0, fd1;
            if(myNode->inFile != NULL){
                printf("check one\n");
                if ((fd0 = open(myNode->inFile, O_RDONLY)) < 0){
                    perror(myNode->inFile);
                    break;
                }
                dup2(fd0, 0);
                close(fd0);
            }

            if(myNode->outFile != NULL){
                printf("check two\n");
                if ((fd1 = open(myNode->outFile, O_WRONLY | O_TRUNC |
                    O_CREAT, 00600)) < 0){
                    perror(myNode->outFile);
                    break;
                }
                printf("check three\n");
                dup2(fd1, 1);
                close(fd1);
            }

            if(myNode->errFile != NULL){
                printf("check four\n");
                if ((fd1 = open(myNode->outFile, O_WRONLY | O_TRUNC |
                    O_CREAT, 00600)) < 0){
                    perror(myNode->outFile);
                    break;
                }
                printf("check five\n");
                dup2(fd1, 1);
                close(fd1);
            }


            myExec = execvp(myNode->command, myArgv);
            myArgv = NULL;
            if(myExec == -1){
                printf("%s: command not found\n",myNode->command);
            }
            //exit(frtn);
        } else if (frtn > 0){
            if(myNode->background == true){
                childWait = waitpid(frtn, &status, WNOHANG);
                if(childWait == -1){
                    perror("waitpid");
                }
            } else {
                childWait = waitpid(frtn, &status, 0);
            }
            // parent process
        } else if(frtn == -1){
            perror("fork");
        }

        // close file descriptors if it is time //
        if(myNode->inFile != NULL){
            dup2(stdin_copy, 0);
        }

        if(myNode->outFile != NULL){
            dup2(stdout_copy, 1);
        }


        myNode = myNode->next; // increment our node pointer
    }
}

int main (int argc, char * argv[]){
    char buf[256];
    int rtn;
    char *rtpt;
    while(1){
        printf("$> ");
        rtpt = fgets(buf, 256, stdin);
        if(rtpt == NULL){
            if(feof(stdin)) {
                printf("test exiting after input\n");
                return 0;
            }
        }
        rtn = parse_line(buf);
        if(rtn == EOL){
            printf("Error: not a valid input\n");
            continue;
        }

        bool rdDefined = false;
        bool myError = false;
        bool beginningOfCommand = true;
        struct Node *node = (struct Node*) calloc(1, sizeof(struct Node));
        struct Node *head = node;
        while(rtn != EOL){
            switch(rtn){
                case WORD:
                    if(beginningOfCommand == true){
                        node->command = strdup(lexeme);
                        node->count = 0;
                        beginningOfCommand = false;
                        rdDefined = true;
                    } else {
                        push(&node, lexeme);
                        rdDefined = true;
                    }
                    break;
                case ERROR_CHAR:
                    printf("Error: character error\n" );
                    break;
                case SYSTEM_ERROR:
                    printf("test the exit on SYSTEM_ERROR\n");
                    exit(42);
                default:
                    // if it is a pipe character
                    if (strcmp(tokens[rtn %96], "PIPE") == 0){
                        if(beginningOfCommand == true){
                            break;
                            myError = true;
                        }
                        struct Node *newNode =
                            (struct Node*) calloc(1, sizeof(struct Node));
                        newNode->prev = node;

                        //assign the input file
                        if(node->inFile == NULL ){
                            newNode->inFile = strdup(node->command);
                        } else {
                            printf("Error: Redirection error\n");
                        }

                        // assign the output file
                        if(node->outFile == NULL ){
                            // get and assign outFile of the node
                            rtn = parse_line(NULL);
                            if(rtn != EOL){
                                node->outFile = strdup(lexeme);
                                newNode->command = strdup(lexeme);
                            } else {
                                printf("Error: Ambiguous redirection\n");
                                myError = true;
                                break;
                            }
                        } else {
                            printf("Error: Redirection error\n");
                        }
                        // assign the next pointer to the last node
                        node->next = newNode;
                        node = newNode;
                        beginningOfCommand = false;
                        rdDefined = true;
                    }
                    // if it is a semicolon character
                    else if(strcmp(tokens[rtn %96], "SEMICOLON") == 0){
                        struct Node *newNode =
                            (struct Node*) calloc(1, sizeof(struct Node));

                        rtn = parse_line(NULL);
                        if(rtn != EOL){
                            newNode->command = strdup(lexeme);
                            // assign the next pointer to the last node
                            node->next = newNode;
                            node = newNode;
                        } else {
                            break;
                        }
                        beginningOfCommand = false;
                        rdDefined = true;
                    }
                    // if it is a > character
                    else if(strcmp(tokens[rtn %96], "REDIR_OUT") == 0){
                        if(beginningOfCommand == true){
                            myError = true;
                            printf("Error: Ambiguous redirection\n");
                            break;
                        }
                        if(node->outFile == NULL){
                            // get and assign outFile of the node
                            rtn = parse_line(NULL);
                            if(rtn != EOL){
                                node->outFile = strdup(lexeme);
                            } else {
                                printf("Error: Ambiguous redirection\n");
                                myError = true;
                                break;
                            }
                        } else {
                            //if there is more than one attempt to redirect out
                            printf("Error: Ambiguous redirection\n");
                            myError = true;
                            break;
                        }
                        rdDefined = true;
                    }
                    // if it is a < character
                    else if(strcmp(tokens[rtn %96], "REDIR_IN") == 0){
                        if(beginningOfCommand == true){
                            myError = true;
                            printf("Error: Ambiguous redirection\n");
                            break;
                        }
                        if(node->inFile == NULL){
                            // get and assign inFile of the node
                            rtn = parse_line(NULL);
                            if(rtn != EOL){
                                node->inFile = strdup(lexeme);
                            } else {
                                printf("Error: Ambiguous redirection\n");
                                myError = true;
                                break;
                            }
                        } else {
                            //if there is more than one attempt to redirect in
                            printf("Error: Ambiguous redirection\n");
                            myError = true;
                            break;
                        }
                        rdDefined = true;
                    }
                    // if it is the 2>1 character
                     else if(strcmp(tokens[rtn %96], "REDIR_ERR_OUT") == 0){
                        if(beginningOfCommand == true){
                            myError = true;
                            printf("Error: Ambiguous redirection\n");
                            break;
                        }
                        if(node->errFile == NULL){
                            // get and assign errFile of the node
                            rtn = parse_line(NULL);
                            if(rtn != EOL){
                                node->errFile = strdup(lexeme);
                            } else {
                                printf("Error: Ambiguous redirection\n");
                                myError = true;
                                break;
                            }
                        } else {
                            //if there is more than one attempt to redirect err
                            printf("Error: Ambiguous redirection\n");
                            myError = true;
                            break;
                        }
                        rdDefined = true;
                    }
                    // if it is a >> character
                     else if(strcmp(tokens[rtn %96], "APPEND_OUT") == 0){
                        if(beginningOfCommand == true){
                            myError = true;
                            printf("Error: Ambiguous redirection\n");
                            break;
                        }
                        if(node->outFile == NULL){
                            // get and assign outFile of the node
                            rtn = parse_line(NULL);
                            if(rtn != EOL){
                                node->outFile = strdup(lexeme);
                            } else {
                                printf("Error: Ambiguous redirection\n");
                                myError = true;
                                break;
                            }
                        } else {
                            //if there is more than one attempt to redirect out
                            printf("Error: Ambiguous redirection\n");
                            myError = true;
                            break;
                        }
                        rdDefined = true;
                    }
                    // if it is a 2>> character
                    else if(strcmp(tokens[rtn %96], "APPEND_ERR") == 0){
                        if(beginningOfCommand == true){
                            myError = true;
                            printf("Error: Ambiguous redirection\n");
                            break;
                        }
                        if(node->errFile == NULL){
                            // get and assign errFile of the node
                            rtn = parse_line(NULL);
                            if(rtn != EOL){
                                node->errFile = strdup(lexeme);
                            } else {
                                printf("Error: Ambiguous redirection\n");
                                myError = true;
                                break;
                            }
                        } else {
                            //if there is more than one attempt to redirect err
                            printf("Error: Ambiguous redirection\n");
                            myError = true;
                            break;
                        }
                        rdDefined = true;
                    }
                    // if it is a 2> character
                    else if(strcmp(tokens[rtn %96], "REDIR_ERR") == 0){
                        if(beginningOfCommand == true){
                            myError = true;
                            printf("Error: Ambiguous redirection\n");
                            break;
                        }
                        if(node->errFile == NULL){
                            // get and assign errFile of the node
                            rtn = parse_line(NULL);
                            if(rtn != EOL){
                                node->errFile = strdup(lexeme);
                            } else {
                                printf("Error: Ambiguous redirection\n");
                                myError = true;
                                break;
                            }
                        } else {
                            //if there is more than one attempt to redirect err
                            printf("Error: Ambiguous redirection\n");
                            myError = true;
                            break;
                        }
                        rdDefined = true;
                    }
                    // if it is only an opening quote with no close
                    else if (strcmp(tokens[rtn %96], "QUOTE_ERROR") == 0){
                        printf("Error: unmatched quote\n");
                        myError = true;
                        break;
                    }
                    // if it is an & character
                    else if (strcmp(tokens[rtn %96], "AMP") == 0){
                        rtn = parse_line(NULL);
                        //printf("my values are: %d, %s\n", rtn, tokens[rtn%96]);
                        if(rtn == EOL){
                            node->background = true;
                        } else {
                            myError = true;
                            break;
                        }

                    }
                    //printf("%d: %s\n", rtn, tokens[rtn%96]);
            }
            if(myError == true){
                break;
            }
            //push(&node, lexeme);
            if(rtn != EOL){
                rtn = parse_line(NULL);
            }
            if(rtn == EOL){
                if(rdDefined == false){
                    printf("Error: No file defined for redirection\n");
                    myError = true;
                    break;
                }
            }
        }
        if(myError == false){
            int nodeCount = 0;
            nodeCount = reverse(&head);
            Executer(head, nodeCount);
        }
        //printList(node->arg_list);

    }

}
