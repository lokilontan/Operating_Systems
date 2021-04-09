#include <iostream>

#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

void findMatches(int fd, string term){

  int ret;
  char buffer[4096];
  string data;
  string tempLine;
  size_t newLinePos = 0;
  size_t oldPos = 0;

  //read everything from the file using a buffer to the string "data"
  while( (ret = read(fd, buffer, 4096)) > 0 ) {
    data.append(buffer, ret);
  }

  //look trhough the data
  while ( newLinePos < (data.length()-1) ) {

    //get the position of first found \n, always start searching from the point stopped before
    newLinePos = data.find_first_of("\n", newLinePos);

    //get the substring from the oldPos to the next \n, grab one more character which is \n
    tempLine = data.substr(oldPos, newLinePos-oldPos+1);

    //increment newLinePos to start looking for \n AFTER last found \n
    newLinePos++;

    //reusing oldPos here
    oldPos = tempLine.find(term);

    //if there is a search term, then its positiob will be returned
    //otherwise, string::npos is returned
    if (oldPos != string::npos){

      //write to stdout the line with the term found
      write(STDOUT_FILENO, tempLine.c_str(), tempLine.length());
    }

    //update oldPos for the next tempLine
    oldPos = newLinePos;
  }
  
}

int main(int argc, char *argv[]) {

  int fd;
  string searchTerm;

  //check if there is no arguments(file and search term)
  if (argc == 1) {
    cout << "wgrep: searchterm [file ...]" << endl;
    return 1;
  }
  else
    //check if there is only one argument
    //then need to identify is it a search term or a file name
    if (argc == 2) {
      fd = open(argv[1], O_RDONLY);
      //if fd == -1, then this is a search term, but there is no file
      //use STDIN_FILENO
      if ( fd == -1 ) {
	fd = STDIN_FILENO;
	searchTerm = argv[1];
	findMatches(fd, searchTerm);
	close(fd);
      }
      //fd != -1, then this is a file, but no search term
      //match NO lines
      else {
	return 0;
      }
    }
  //there are 3 or more arguments
  //1st-search term; 2nd,3rd,...-files
    else {
      for (int i = 2; i < argc; i++) {
	fd = open(argv[i], O_RDONLY);
	if (fd == -1) {
	  cout << "wgrep: cannot open file" << endl;
	  return 1;
	}
	else {
	  searchTerm = argv[1];
	  findMatches(fd, searchTerm);
	  close(fd);
	}
      }
    }
  return 0;
}
