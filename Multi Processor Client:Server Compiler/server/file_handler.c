#include "common.h"
#include "file_handler.h"

// Makes a dir if force or dir does not exist
int makeDir(char *dir,int force){
  if(force || (dirExists(dir)!=1) ){
    printf("%s\n",dir);
    rmdir(dir);
    mkdir(dir, 0700);
    return 1;
  }
  else{
    return 0;
  }
}

// Checks if dir exists
int dirExists(char *dir){
  struct stat st = {0};

  if (stat(dir, &st) == -1) {
    return 0;
  }
  else{
    return 1;
  }
}

// Append to file
int appendFile(FILE *fptr,char *str){
  fputs(str, fptr);
  return 1;
}

// Checks if a file exists
int fileExists(char *file_name){
  FILE *fptr;
  fptr = fopen(file_name,"r");

  if(fptr == NULL){
    printf("File: %s could not be opened\n", file_name);
    fclose(fptr);
    return 0;
  }else{
    fclose(fptr);
    return 1;
  }

}
