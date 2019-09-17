#include "common.h"

/*
 * Function: duplicate
 * --------------------
 * forks a program
 *
 *
 * Returns: pid
 */
int duplicate(){
  pid_t pid = fork();
  return pid;
}
