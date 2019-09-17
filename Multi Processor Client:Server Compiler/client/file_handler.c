#include "common.h"

/*
 * Function: files_exist
 * --------------------
 * Checks if files exit given an array of filenames
 *
 * char *file_names[50]: file names.
 * int size: number of filenames
 *
 * Returns: 1 if all files exist, 0 if at least 1 does not.
 */
int files_exist(char *file_names[50],int size){

  // Attempt to open and read each file
  for(int i=0;i<size;i++){
    FILE *fptr;
    fptr = fopen(file_names[i],"r");

    if(fptr == NULL){
      // printf("File: %s could not be opened\n", file_names[i]);
      return 0;
    }
    fclose(fptr);
  }
  return 1;
}
