/*
 * Wisconsin Shell (Wish) 
 * 
 * A basic shell environment that handles a few commands.
 *
 * Author: Alejandro Aguina
 */

/* Import Statements */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


/* Global Declarations */
char *userIn;
char *sourceLine;
char *token; 
char *builtIn1 = "exit";
char *builtIn2 = "cd";
char *builtIn3 = "path";
char *cmdLine = "wish> ";
char *delim = " \t\r\n\f\v";
char *errMsg = "An error has occurred\n";
char *argL[32]; //initial argument list for commands
char *PATH = "/bin";
char *redir = ">";
char *batchF;
char *execPath;

/* Specific Globals for condMd() */
char *notCond = "!=";
char *eqCond = "==";
char *cond = "if";
char *thCond = "then";
char *endCond = "fi";

/* int globals */
int isRedir;
int cmpVal;
int isBatch;
int isIFSTATE;
int hasRedir;

/* unique globals */
size_t inputSize = 4;
size_t line;
size_t cmdCount;
pid_t  pid;

FILE *fp;

int accessChk(char *argA[]) {
    //if the PATH is NULL we just exit
    if (PATH == NULL) {
        return -1;
    }

    //first is to create function variables
    int result = -1; 
    char *tmpStr;
    char *pathCpy = strdup(PATH);
    char *dlim = " \n";
    char *cmd;

    //tokenize the copy
    tmpStr = strtok(pathCpy, dlim);
    
    //bounds check, if temp is NULL (or empty " ") exit
    if (tmpStr == NULL) {
        return -1;
    }

    //concat file bracket first
    cmd = strdup(tmpStr);
    strcat(cmd, "/");

    while (tmpStr != NULL) {
        //concatentate
        strcat(cmd, argA[0]);

        //see if printed
        //printf("concat str is -> %s\n", cmd);

       // printf("tok is -> %s\n", tmpStr);


        //real butter of this function
        result = access(cmd, X_OK);

        //printf("Result is -> %d| for cmd -> %s|\n", result, cmd);
        
        //if the result = 0, we set argL[0] to the cmd
        if (result == 0) {
            execPath = cmd;
            
            return result;
        }

        //tokenize
        tmpStr = strtok(NULL, dlim);
        
        //ensure we aren't writing NULL to cmd (seg fault bound)
        if (tmpStr != NULL) {
            cmd = strdup(tmpStr);
            strcat(cmd, "/");
        }
    }

    //checking if execPath is now cmd before return
    //printf("execPath in list is now -> %s\n", execPath);

    return result;
}


void cmdPath(char *path, int reset) {
    //create a specific delim
    char *dlim = " ";

    strsep(&path, dlim);
    
    //set PATH to new value
    PATH = path;

    //printf("NEW PATH is -> %s\n", PATH);

    //free dlim
    dlim = NULL;
    free(dlim);
}

void cmdChd(char *path) {
    //take the given path and cd to it
    int cdVal = chdir(path);
    
    //if cd != 0 return err
    if (cdVal != 0) {
        write(STDERR_FILENO, errMsg, strlen(errMsg));
    }


    //printf("cdVal returned -> %d\n", cdVal);
}

void cmdExit() {
    //free absolutely all globals
    //TODO FILLLATER sysFree();


    exit(0);

}

void resetCmd() {
    cmdCount = 0;
    cmpVal = 0;

    isRedir = 0;
    isIFSTATE = 0;
    hasRedir = 0;

    //since argL is fixed size, just iterate through entire arr
    for (int i = 0; i < 32; i++) {
        argL[i] = NULL;
    }

    execPath = NULL;

}


void scnLine() {
    //for safety copy line
    char *tmpLine = strdup(sourceLine);

    //if we see > then we set redir = 1
    if (strchr(tmpLine, '>') != NULL) {
        hasRedir = 1;
        
        //printf("user in -> %s\n", userIn);

        //printf("FOUND REDIR\n");
    }

}

void redirectMd(int mode, char *argT[]) {
    //first, create a new arr to store commands for file write to
    char *argF[32];
    
    /* vars to use */
    int cmp;
    int fnext;
    char *file;
    int canWrite = 0;

    //if mode is 1 (smashed > in line) then we do unique run
    if (mode == 1) {
        /* unique vars for 1 */
        char *cpyLine = strdup(sourceLine);
        char *tok;
        char *strChk;
        char *dlim = "\n ";

        printf("sourceline -> %s\n", sourceLine);
        
        tok = strtok(cpyLine, dlim);
        
        while (tok != NULL) {
            int i;
            
            printf("tok is -> %s\n", tok);

            if(strchr(tok, '>') == NULL) {
                //get the args
                argF[i] = tok;
                i++;
                
                printf("stored -> %s\n", tok);


                tok = strtok(NULL, dlim);
            }
            else {
                printf("tok is currently (else) %s\n", tok);

                char *next = strtok(tok, ">");

                printf("next is -> %s\n", next);

                if(argF[i] == NULL) {
                    argF[i] = next;
                }

                strChk = strstr(tok, ">");

                printf("strchk is -> %s\n", strChk);

                char *dlm = ">";

                char *tokF = strtok(strChk, dlm);

                file = tokF;

                //ensure file wasn't added to the args
                if ((cmp = (strcmp(argF[i], file))) == 0) {
                    argF[i] = NULL;
                }

                //break 
               // break;
            }
        }
    }
    else {
        for (int i = 0; i < 32; i++) {
            //if the element is null, exit
            if (argT[i] == NULL) {
                break;
            }
            //test
            //printf("current arg is -> %s\n", argL[i]);

            cmp = strcmp(argT[i], redir);
        
            //if the arg at i is > set fnext to 1
            if (cmp == 0) {
                //simple check
                //printf("next item is -> %s\n", argT[i +1]);
                //check first if > is next again
                if (argT[i + 1] == NULL || (cmp = strcmp(argT[i + 1],redir)) == 0) {
                    //set can run to -1
                    canWrite = -1;
                    //write(STDERR_FILENO,errMsg, strlen(errMsg));
                    //return;
                }
                else {
                    //printf("fnext on, argL + 1 = %s\n", argT[i+1]);
                    fnext = 1;
                    continue;
                }
            }
            else {
                if (fnext == 1) {
                    if (argL[i+1] != NULL) {
                        canWrite = -1;
                    }
                    else{
                        file = argT[i];
                    }

                }   
                else {
                    argF[i] = argT[i];
                }
            }

            //printf("argF is -> %s\n", argF[i]);
            //printf("argL is -> %s\n", argL[i]);


        }



    }

    //printf("cw is -> %d\n", canWrite); 
    //printf("file is -> %s\n", file);

    pid_t outPid;

    //int canRun = accessChk();
    
    //check the cmd
    //printf("arg2 -> %s\n", argL[2]);
    //printf("path is -> %s\n", argL[0]);


    if (canWrite != 0) {
        write(STDERR_FILENO, errMsg, strlen(errMsg));
    }
    else {
    
        outPid = fork();
        //printf("fork is -> %d\n", outPid);

        if (outPid == 0) {
            //example redirect (use man gcc)
           /* char *test[3] =  {
                "/bin/man",
                "javac",
                NULL
            };*/

            int fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0744);
            
                //dup2(fd, fileno(stdout));
                //dup2(fd, fileno(stderr));


            if (fd == -1) {
                write(STDERR_FILENO,errMsg, strlen(errMsg));

            }

            else {
                dup2(fd, fileno(stdout));
                dup2(fd, fileno(stderr));
                
                //check if we can use accesschk here
                int run = accessChk(argF);

                if (run != 0) {
                    write(STDERR_FILENO,errMsg, strlen(errMsg));
                    close(fd);
                    
                    //if we can't run exit the child process in err
                   // exit(1);
                }
                else{
                //ensure argF = argL0
                //argF[0] = argL[0];

                //close here?
                close(fd);

                //exec command
                execv(execPath, argF);

              /* if (result != 0) {
                    printf("error with exec\n");
                }
                else {
                    printf("accessed cmd\n");
                }*/

                //close(fd);
                }
            }

        }
         
            int status;
            waitpid(outPid, &status, 0);
            


    }

    //printf("I'm redirect!\n");
}

/*
 * A recursive method that runs through any line that uses the "if" syntax 
 * CLI just scans for the if value and enters. if the cli value at 1 is not if
 * we exit.
 *
 * Takes an integer value (specifically for recursion) to track index across recursive calls
 * (tracks index of ArgL used across shell)
 *
 * void return.
 */
void condMd(int val) {
    int ind = val;
    int cmpr = 0;
    int isEq = 0;
    int isNot = 0;
    unsigned int expected = 0;
    int actual = 0;
    int argNum = 0;
    int aChk = 0;
    pid_t cndPid = 0;
    pid_t finalPid = 0;
    
    //create new arg arr for specific calls
    char *argCnd[5];

    //upon each entry of this mode, reset the values (recursion destroys the bytes in the array?) - seg fault bounds
    for (int i = 0; i < 5; i++) {
        argCnd[i] = NULL;
        //printf("Arg at i is -> %s\n", argCnd[i]);
    }
    
   //printf("index is -> %d\n", ind);
    

    //base case for entire method -> if argL[0] isn't 'if' error out and return
    if ((cmpr = (strcmp(argL[0], cond))) != 0) {
        write(STDERR_FILENO, errMsg, strlen(errMsg));
        return;
    }


    //enter the while
    while (argL[ind] != NULL) {
        //since the 0th is always if we can skip
        if (ind == 0) {
            ind++;
            continue;
        }
        //for recursion, check if the values are if or then to skip
        else if ((cmpr = (strcmp(argL[ind], cond))) == 0 || (cmpr = (strcmp(argL[ind], thCond))) == 0) {
            ind++;
            continue;
        }
        

        //print the current arg
        //printf("argL -> %s\n", argL[ind]);
        
        //check if the current item is one of our conditions, continue if so
        if ((cmpr = (strcmp(argL[ind], eqCond))) == 0) {
            isEq = 1;
            ind++;
            continue;
        }
        else if ((cmpr = (strcmp(argL[ind], notCond))) == 0) {
            isNot = 1;
            ind++;
            continue;
        }

        //if we've met the cond value, we know the next item is the value to test
        if (isNot == 1 || isEq == 1) {
            //set the expected to argL
            expected = atoi(argL[ind]);
            //increment since the expected is considered an arg
            ind++;

            //printf("expt is -> %d\n", expected);

            //in wish.c utilize accessChk to confirm we can run
            //int canRun = accessChk();
            //if canRun = 0, then we go, if not we exit with error
            aChk = accessChk(argCnd);
            
            //printf("args are %s | %s | %s | %s | %s\n", argCnd[0], argCnd[1], argCnd[2], argCnd[3], argCnd[4]);

            //pid_t cndPid = 0;
            
            //if aChk = 0 we can go, else we exit
            if (aChk == 0) {
            
                cndPid = fork();

                //for some reason we can't fork exit
                if (cndPid < 0) {
                    //write(STDERR_FILENO, errMsg, strlen(errMsg));
                    return;
                }
                else if (cndPid == 0) {
                    //enter the exec
                    execv(execPath, argCnd);
                }else {
                    int status;
                    waitpid(cndPid, &status, 0);
                
                    //printf("status is -> %d\n", status);

                    //grab the value
                    if(WIFEXITED(status)) {
                        actual = WEXITSTATUS(status);
                    }

                }
            }
            //error
            else {
                write(STDERR_FILENO, errMsg, strlen(errMsg));
                return;
            }

            //printf("status is -> %d\n", status);
            //printf("first run complete\n");
            //printf("actual is -> %d\n", actual);

            //if isNot flag is 1, we compare values to be not equal, run if true
            if (isNot == 1) {
                if (actual != expected) {
                    //rescursion area
                    
                    //if the next value is then we can enter
                    if ((cmpr = (strcmp(argL[ind], thCond))) == 0) {
                        //if the next value is if, recurse
                        if ((cmpr = (strcmp(argL[ind + 1], cond))) == 0) {
                            condMd(ind);
                            return;
                        }
                        //else, we can now run the final command
                        else {
                            //new pid
                            //pid_t finalPid;

                            //reset argNum
                            argNum = 0;

                            //iterate breaks only when arg is null
                            while(argL[ind] != NULL) {
                                //if we simply reach fi, return
                                if ((cmpr = (strcmp(argL[ind], endCond))) == 0) {
                                    return;
                                }
                                
                                
                                    //printf("arg is -> %s\n", argL[ind]);

                                //if its then or if continue
                                if ((cmpr = (strcmp(argL[ind], thCond))) == 0 || (cmpr = (strcmp(argL[ind], cond))) == 0) {
                                    //printf("arg is -> %s\n", argL[ind]);
                                    ind++;
                                    continue;
                                }
                                else {
                                    //(seg bounds) if the next item isNULL error
                                    if (argL[ind + 1] == NULL) {
                                        write(STDERR_FILENO, errMsg, strlen(errMsg));
                                        return;
                                    }
                                    
                                    argCnd[argNum] = argL[ind];

                                    //branch again if we don't reach fi
                                    if ((cmpr = (strcmp(argL[ind + 1], endCond))) != 0) {
                                        char *check = strstr(argL[ind + 1], "fi");

                                        if (check != NULL){
                                            write(STDERR_FILENO, errMsg, strlen(errMsg));
                                            return;
                                        }


                                        argNum++;
                                        ind++;
                                        continue;
                                    }
                                    else {
                                       
                                        //printf("argNum and argL are %s | %s\n", argCnd[argNum], argL[ind]);
                                        argCnd[argNum] = argL[ind];
                                    
                                        //(seg bounds) if the next item isNULL error
                                        if (argL[ind + 1] == NULL) {
                                            write(STDERR_FILENO, errMsg, strlen(errMsg));
                                            return;
                                        }

                                        //branch again if we don't reach fi
                                        if ((cmpr = (strcmp(argL[ind + 1], endCond))) != 0) {
                                            argNum++;
                                            ind++;
                                            continue;
                                        }
                                        else {
                                            if (argL[ind + 2] != NULL) {
                                                char *check = strstr(argL[ind + 2], "fi");

                                                if ((cmpr = (strcmp(check, endCond))) != 0) {
                                                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                    return;
                                                }
                                            }

                                            /*
                                            //printf("argl curr -> %s | argl next -> %s|\n", argL[ind], argL[ind + 2]);
                                            if (argL[ind + 2] != NULL) {
                                                if ((cmpr = (strcmp(argL[ind + 2], endCond))) == 0) {
                                                    break;
                                                }
                                                else {
                                                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                    return;
                                                }

                                            }*/

                                            //printf("args are %s | %s | %s | %s | %s\n", argCnd[0], argCnd[1], argCnd[2], argCnd[3], argCnd[4]);
                                            //implement ability to use builtins.
                                            if((cmpr = (strcmp(argCnd[0], builtIn1))) == 0) {
                                                //exit so don't have to do anything
                                                cmdExit();
                                            }
                                            else if ((cmpr = (strcmp(argCnd[0], builtIn2))) == 0) {
                                                //printf("cd -> %s\n", argCnd[1]);
                                                //chd then return
                                                cmdChd(argCnd[1]);
                                                return;
                                            }
                                            else if ((cmpr = (strcmp(argCnd[0], builtIn3))) == 0) {
                                            
                                                cmdPath(argCnd[1], 1);
                                            }
                                            /* not a built - in check for access and run */
                                            else{
                                                //check access
                                                aChk = accessChk(argCnd);

                                                if (aChk == 0) {
                                                    //hit the final run
                                                    finalPid = fork();
                                        
                                                    //pid doesn't work write to err
                                                    if (finalPid < 0) {
                                                        //write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                        return;
                                                    }
                                                    else if (finalPid == 0) {
                                                        //run final command
                                                        execv(execPath, argCnd);
                                                    }
                                                    else {
                                                        int status;
                                                        waitpid(finalPid, &status, 0);
                                                    }
                                                }
                                                //error
                                                else {
                                                    //no errors to be written
                                                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                    return;
                                                }
                                                //printf("we've finished the else branch, coming home\n");
                                                //upon completion return when we get here (no need to continue)
                                            }

                                            //printf("we've finished the else branch, coming home\n");
                                            //upon completion return when we get here (no need to continue)
                                            return;
                                            }
                                    }
                                }
                            }
                        }
                    }
                    //if its not then we error out
                    else {
                        write(STDERR_FILENO, errMsg, strlen(errMsg));
                        return;
                    }
                } 
                else {
                    //if its not equal immediately return
                   // write(STDERR_FILENO, errMsg, strlen(errMsg));
                    return;
                }
            }
            //copy of above
            else if(isEq == 1) {
                //printf("Entered isEq\n");

                if (actual == expected) {
                    //rescursion area
                    //printf("actual and expected are equal\n");
                    //printf("the next value is -> %s\n", argL[ind + 1]);
                    //printf("the second next is -> %s\n", argL[ind + 2]);

                    //if the next value is then we can enter
                    if ((cmpr = (strcmp(argL[ind], thCond))) == 0) {
                        //if the next value is if, recurse
                        if ((cmpr = (strcmp(argL[ind + 1], cond))) == 0) {
                            //printf("stepping into recursion\n");
                            condMd(ind);
                            return;
                        }
                        //else, we can now run the final command
                        else {
                            //printf("entered final area\n");
                            //new pid
                            //pid_t finalPid;

                            //reset argNum
                            argNum = 0;
                            
                            //check the prints
                            //printf("args are %s | %s | %s | %s | %s\n", argCnd[0], argCnd[1], argCnd[2], argCnd[3], argCnd[4]);
                            

                            //iterate breaks only when arg is null
                            while(argL[ind] != NULL) {
                                //test print statement
                                //printf("arg is now -> %s\n", argL[ind]);
                                //if we simply reach fi, return
                                if ((cmpr = (strcmp(argL[ind], endCond))) == 0) {
                                    return;
                                }
                                
                                //if its then or if continue
                                if ((cmpr = (strcmp(argL[ind], thCond))) == 0 || (cmpr = (strcmp(argL[ind], cond))) == 0) {
                                    ind++;
                                    continue;
                                }
                                else {
                                    //(seg bounds) if the next item isNULL error
                                    if (argL[ind + 1] == NULL) {
                                        write(STDERR_FILENO, errMsg, strlen(errMsg));
                                        return;
                                    }
                                    

                                    argCnd[argNum] = argL [ind];
                                    
                                    //branch again if we don't reach fi
                                    if ((cmpr = (strcmp(argL[ind + 1], endCond))) != 0) {
                                        //check if the string has fi, if so return
                                        char *check = strstr(argL[ind + 1], "fi");
                                        
                                        if (check != NULL) {
                                            write(STDERR_FILENO, errMsg, strlen(errMsg));
                                            return;
                                        }


                                        argNum++;
                                        ind++;
                                        continue;
                                    }
                                    else {
                                        //check if arg + 1 is fi 
                                       /* char *check = strstr(argL[ind + 1], "fi");

                                        if ((cmpr = (strcmp(check, endCond))) != 0) {
                                            write(STDERR_FILENO, errMsg, strlen(errMsg));
                                        }*/


                                       // printf("argl curr -> %s | argl next -> %s\n", argL[ind], argL[ind + 2]);
                                       //enter if the final arg (recursive only) is NOT NULL
                                        if (argL[ind + 2] != NULL) {
                                            

                                            char *check = strstr(argL[ind + 2], "fi");

                                            //print test to see if printed
                                            //printf("check is now -> %s\n", check);
                                            
                                            //cmp value to endCond
                                            if ((cmpr = (strcmp(check, endCond))) != 0) {
                                                write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                return;
                                            }


                                        }


                                       /* if(argL[ind + 2] != NULL) {
                                            if((cmpr = (strcmp(argL[ind + 2], endCond))) == 0) {
                                                printf("cmpr -> %d with %s\n", cmpr, argL[ind +2]);

                                                break;
                                            }
                                            else {
                                                write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                return;
                                            }
                                        }*/
                                        
                                        //printf("either made it or broke on doulbe fi\n");

                                        /*if((cmpr = (strcmp(argL[ind + 2], endCond))) != 0 || argL[ind + 2] != NULL) {
                                            write(STDERR_FILENO, errMsg, strlen(errMsg));
                                            return;
                                        }*/

                                        
                                        //printf("args are %s | %s | %s | %s | %s\n", argCnd[0], argCnd[1], argCnd[2], argCnd[3], argCnd[4]);
                                        //implement ability to use builtins.
                                        if((cmpr = (strcmp(argCnd[0], builtIn1))) == 0) {
                                            //exit so don't have to do anything
                                            cmdExit();
                                        }
                                        else if ((cmpr = (strcmp(argCnd[0], builtIn2))) == 0) {
                                            //printf("cd -> %s\n", argCnd[1]);
                                            //chd then return
                                            cmdChd(argCnd[1]);
                                            return;
                                        }
                                        else if ((cmpr = (strcmp(argCnd[0], builtIn3))) == 0) {
                                            
                                            cmdPath(argCnd[1], 1);
                                        }
                                        /* not a built - in check for access and run */
                                        else{
                                            //printf("redir vals are hasR: %d | isR: %d\n", hasRedir, isRedir);
                                            
                                        //printf("args are %s | %s | %s | %s | %s\n", argCnd[0], argCnd[1], argCnd[2], argCnd[3], argCnd[4]);


                                            //if we have redirection enter
                                            /*if (hasRedir == 1 || isRedir == 1) {
                                                                                         
                                                //if hasRedir but not isRedir, enter with 1
                                                if (hasRedir == 1 && isRedir == 0) {
                                                    redirectMd(1, argCnd);
                                                    return;
                                                }
                                                else if (isRedir == 1) {
                                                    redirectMd(0, argCnd);
                                                    return;
                                                }   
                                                */


                                            //}
                                            //else {
                                                //check access
                                                aChk = accessChk(argCnd);

                                                if (aChk == 0) {
                                                

                                                    //hit the final run
                                                    finalPid = fork();
                                        
                                                    //pid doesn't work write to err
                                                    if (finalPid < 0) {
                                                        //write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                        return;
                                                    }
                                                    else if (finalPid == 0) {
                                                        //run final command
                                                        execv(execPath, argCnd);
                                                    }
                                                    else {
                                                        int status;
                                                        waitpid(finalPid, &status, 0);
                                                    }

                                                    //if we make it here we return
                                                    return;
                                                }
                                                //error
                                                else {
                                                    //no errors to be written
                                                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                                                    return;
                                                }
                                            //printf("we've finished the else branch, coming home\n");
                                            //upon completion return when we get here (no need to continue)
                                            //}
                                            return;
                                        }

                                        //printf("we've finished the else branch, coming home\n");
                                        //upon completion return when we get here (no need to continue)
                                    
                                    }
                                        
                                }
                            }
                         }
                         //test return here
                         return;
                       } 
                    }
                    
                    //if its not then we error out
                    else {
                        //printf("exiting ifp\n");
                        //write(STDERR_FILENO, errMsg, strlen(errMsg));
                        return;
                    }
                } 
                else {
                    //if its not equal immediately return
                    //write(STDERR_FILENO, errMsg, strlen(errMsg));
                    return;
                }
            }


        //bottom case -> if nothing can enter branches, means we can store a argument/command
        else {
            //printf("Setting %s arg into the argCnd # %d\n", argL[ind], argNum);

            //set arg to argL
            argCnd[argNum] = argL[ind];
            ind++;
            argNum++;
        }


    }    


}

int condParse() {
    int ret = 0;

    //int i = 0;

    //while (argL[i] != NULL) {
    for (int i = 0; i < 32; i++) {
        //if value is null continue.
        if (argL[i] == NULL) {
            continue;
        }
        //special case if last value met
        if (argL[i + 1] == NULL) {
            //if cmp is not eq set err to -1
            if ((cmpVal = (strcmp(argL[i], endCond))) != 0) {
                ret = -1;
            }
        }
        

    

       //printf("argL is -> %s\n", argL[i]);
        

        //create str
        char *check;

        //1st case if
        if ((check = strstr(argL[i], cond)) != NULL) {
            //if the cmp is not equal set err to -1
            if ((cmpVal = (strcmp(argL[i], cond))) != 0) {
                ret = -1;
            }
        }
        //2nd case then
        else if ((check = strstr(argL[i], thCond)) != NULL) {
            //if cmp is not eq set err to -1
            if ((cmpVal = (strcmp(argL[i], thCond))) != 0) {
                ret = -1;
            }
        }
        //last case fi
        else if ((check = strstr(argL[i], endCond)) != NULL) {
            //if cmp is not eq set err to -1
            if ((cmpVal = (strcmp(argL[i], endCond))) != 0) {
                ret = -1;
            }
        }
        //special looking for redirection
        else {
            if ((check = strstr(argL[i], redir)) != NULL) {
                //set has redir
                hasRedir = 1;

                //cmp for if its not smashed
                if ((cmpVal = (strcmp(argL[i], redir))) == 0) {
                    isRedir = 1;
                }
            }
        }
        
        
        //printf("argL is -> %s\n", argL[i]);
        //if it's null, ret
        if (argL[i] == NULL) {
            return ret;    
        }
    }

    return ret;
}

/*
 * Dev - Note, attempted to incorporate into interactive mode but seems easier to do it here
 *
 * batch mode mirrors interactive mode, except userinput is from a file rather than user input
 * system will run commands until EOF.
 *
 * Update @ 10/11 01:52 "Mistakes were made."
 */
void batchMd() {
        fp = fopen(batchF, "r");
        
        //if for some reason file can't open, exit
        if (!fp) {
            write(STDERR_FILENO, errMsg, strlen(errMsg));
            exit(1);
        }

    //essentially infinite loop
    while ((line = getline(&userIn, &inputSize, fp)) != -1) {
        //printf("%s", cmdLine);
        
        //print the line for checking
        //printf("line from text is -> %s\n", userIn);
       
        sourceLine = strdup(userIn);
        
        //before completing anything, scnLine for >
        scnLine();


       
        int isBlank;

        //test blank bounds
        if (userIn == NULL || (isBlank = (strcmp(userIn, "\n"))) == 0 ) {
            continue;
        }
        
        //grab the first token
        token = strtok(userIn, delim);

        //try while
        while (token != NULL) {
            //if the token val is '>' = redirection
            if ((cmpVal = (strcmp(token, redir))) == 0) {
                //we'll handle error checking in the function redirectMd
                isRedir = 1;
                
            }
            
            //if the token is 'if' = condMd
            if ((cmpVal = (strcmp(token, cond))) == 0) {
               isIFSTATE = 1; 
            }
            //begin storing commands
            argL[cmdCount] = token;


            token = strtok(NULL, delim);
            
            //printf("argL is -> %s\n", argL[cmdCount]);

            cmdCount++;
        }
        
        //blank line bounds check, argL is NULL we continue
        if (argL[0] == NULL) {
            resetCmd();
            continue;
        }
        //check line here
        //printf("sourceLine -> %s\n", sourceLine);
        //printf("ArgL is -> %s\n", argL[0]);
        
        /* Priority of values if Cond -> Redircts -> Built Ins -> reg cmds */
        
        /* if the isIFSTATE flag is on, enter condMd() */
        if (isIFSTATE == 1) {
            //singular error check here, check the sourceline for != or ==
            char *notEq = strstr(sourceLine, "!=");
            char *eq = strstr(sourceLine, "==");

            if (notEq != NULL || eq != NULL) {
                int result = condParse();

                if (result == -1) {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                }
                else {
                    condMd(0);
                }

                
            }
            else {
                write(STDERR_FILENO, errMsg, strlen(errMsg));
            }

        }
        /* We check if there's redirection requirements */
        else if (isRedir == 1 || hasRedir == 1) {
            //error check if the first argument is > we error instead of redirect
            if ((cmpVal = (strcmp(argL[0], redir))) == 0) {
                write(STDERR_FILENO, errMsg, strlen(errMsg));
            }
            else {
                //if hasRedir but not isRedir, enter with 1
                if (hasRedir == 1 && isRedir == 0) {
                   redirectMd(1, argL); 
                }
                else if (isRedir == 1) {
                    redirectMd(0, argL);
                }
                //shouldn't get here but im insane
                else {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                }

            }
        }
        /* This runs when 1.) no redirection AND 2.) no parallel command structures */
        else {
            //first, check if our command is a built in
            if ((cmpVal = (strcmp(argL[0], builtIn1))) == 0) {
                //if any argument is passed to exit, error
                if (argL[1] != NULL) {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
            
                } else {
                    fclose(fp);
                    cmdExit();    
                }

            }
            //check for cd
            else if ((cmpVal = (strcmp(argL[0], builtIn2))) == 0) {
                if (argL[1] == NULL || argL[2] != NULL) {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                } else {
                    cmdChd(argL[1]);
                }


            }
            //check for PATH
            else if ((cmpVal = (strcmp(argL[0], builtIn3))) == 0) {
                //path can take 0 or more arguments so just enter path func
                cmdPath(sourceLine, 1);
            
            } else {
        
                //first, we ensure access to run a command
                int canRun = accessChk(argL);
                
                //if accessChk returns -1 we error out (cannot run cmd)
                if (canRun != 0) {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                }
                //else run the command
                else {
                    pid = fork();
                    
                    //printf("pid is -> %d\n", pid);
                    //printf("Args are -> %s | %s | %s\n", argL[0], argL[1], argL[2]);

                    //if for some reason the fork fails, print error
                    if (pid < 0) {
                        write(STDERR_FILENO,errMsg, strlen(errMsg));
                    }
                    else if (pid == 0) {
                        execv(execPath, argL);
                    }   

                    int status;
                    waitpid(pid, &status, 0);

                }
        

            }
        }

        /* Reset Function */
        resetCmd();
    }   
   
    //close file if no exit cmd found
    fclose(fp);
    
    //once done with file, exit gracefully
    exit(0);
}

void interactiveMd() {

    //essentially infinite loop
    while (1) {
        printf("%s", cmdLine);
        
        //get the command line from user    
        line = getline(&userIn, &inputSize, stdin);
        
        sourceLine = strdup(userIn);
        
        //before we even do anything, enter scnLine()
        scnLine();

        //get the command line from user    
        //line = getline(&userIn, &inputSize, stdin);
        
        //printf("userIn is -> %s | and source is -> %s |\n", userIn, sourceLine);

        int isBlank;

        //test blank bounds
        if (userIn == NULL || (isBlank = (strcmp(userIn, "\n"))) == 0 ) {
            continue;
        }

        //grab the first token
        token = strtok(userIn, delim);

        //try while
        while (token != NULL) {
            //if the token is '>' = equals redirection (no smashed tokens)
            if ((cmpVal = (strcmp(token, redir))) == 0) {
                //we'll handle error checking in the function redirectMd
                isRedir = 1;
                
            }
            
            //if the token is 'if' = equals condMd()
            if ((cmpVal = (strcmp(token, cond))) == 0) {
                isIFSTATE = 1;
            }

            //begin storing commands
            argL[cmdCount] = token;


            token = strtok(NULL, delim);

            cmdCount++;
        }
        
        /* Enter condMd if the isIFSTATE flag is on (error checks in function) */
         if (isIFSTATE == 1) {
                        

            //singular error check here, check the sourceline for != or ==
            char *notEq = strstr(sourceLine, "!=");
            char *eq = strstr(sourceLine, "==");

            if (notEq != NULL || eq != NULL) {
                int result = condParse();

                //if -1 error
                if (result == -1) {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                }
                //else enter mode
                else {
                    condMd(0);
                }
            }
            else {
                write(STDERR_FILENO, errMsg, strlen(errMsg));
                
            }
        }
        /* We check if there's redirection requirements */
        else if (isRedir == 1 || hasRedir == 1) {
            //error check if the first argument is > we error instead of redirect
            if ((cmpVal = (strcmp(argL[0], redir))) == 0) {
                write(STDERR_FILENO, errMsg, strlen(errMsg));
            }
            else {
                //if hasRedir but not isRedir, enter with 1
                if (hasRedir == 1 && isRedir == 0) {
                   redirectMd(1, argL); 
                }
                else if (isRedir == 1) {
                    redirectMd(0, argL);
                }
                //shouldn't get here but im insane
                else {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                }

            }
        }
        /* This runs when 1.) no redirection AND 2.) no parallel command structures */
        else {
            //first, check if our command is a built in
            if ((cmpVal = (strcmp(argL[0], builtIn1))) == 0) {
                //if any argument is passed to exit, error
                if (argL[1] != NULL) {
                    printf("arg1 is -> %s\n", argL[1]); 

                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                } else {
                    cmdExit();    
                }

            }
            //check for cd
            else if ((cmpVal = (strcmp(argL[0], builtIn2))) == 0) {
                if (argL[1] == NULL || argL[2] != NULL) {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                } else {
                    cmdChd(argL[1]);
                }


            }
            //check for PATH
            else if ((cmpVal = (strcmp(argL[0], builtIn3))) == 0) {
                //path can take 0 or more arguments so just enter path func
                cmdPath(sourceLine, 1);
            
            } else {
        
                //first, we ensure access to run a command
                int canRun = accessChk(argL);
        
                //if accessChk returns -1 we error out (cannot run cmd)
                if (canRun != 0) {
                    write(STDERR_FILENO, errMsg, strlen(errMsg));
                }
                //else run the command
                else {
                    pid = fork();

                    //if for some reason the fork fails, print error
                    if (pid < 0) {
                        write(STDERR_FILENO,errMsg, strlen(errMsg));
                    }
                    else if (pid == 0) {
                        execv(execPath, argL);
                    }   

                    int status;
                    waitpid(pid, &status, 0);

                }
        

            }
        }

        /* Reset Function */
        resetCmd();
        
    }
}


int main (int argc, char *argv[]) {
    //first, if argc is > 2, error out
    if (argc > 2) {
        write(STDERR_FILENO, errMsg, strlen(errMsg));
        exit(1);
    }

    userIn = (char *)malloc(sizeof(char) * inputSize);

    if (userIn == NULL) {
        printf("%s", errMsg);
        exit(0);
    }
    
    //if wish has 1 arg, enter batch mode, else interactive mode
    if (argc > 1) {
        batchF = argv[1];

        batchMd();
    } else {
        interactiveMd();

    }





    
   //exit gracefully once completed
   return 0;


}
