#include <iostream>

#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

//this function compresses a string with data,
//returns compressed string
string compress(string data){

  string compressed = "";
  int counter;

  if (!data.empty()) {

    counter = 1;
    
    for (unsigned int i = 1; i < data.length(); i++) {
      if (data[i] == data[i-1]) {
	counter++;
      }
      else {
	compressed += to_string(counter) + data[i-1];
	counter = 1;
      }	
    }
    compressed += to_string(counter) + data[data.length()-1];
  }
  
  return compressed;

}

//this procedure converts each number to int and writes its raw(4 byte) version to the STDOUT_FILENO and each symbols raw data is printed to STDOUT_FILENO as well
void stdoutRaw(string data){
  int tempInt;
  string tempString;
  char character;
  for (unsigned int i = 0; i < data.length(); i++) {
    if (isdigit(data[i])) {
      //if digit then append to the tempString
      tempString += data[i];
    }
    else {
      //ith char is alphabetic
      //convert string to int and print frequences with letter to the stdout
      tempInt = atoi (tempString.c_str());
      tempString = "";
      character = data[i];
      write(STDOUT_FILENO, &tempInt, 4);
      write(STDOUT_FILENO, (void *)&character, 1);
    }
  }
}

int main(int argc, char *argv[]) {

  int fd;
  char buffer[4096];
  int ret;
  string dataCollector;

  //check if there is no arguments(files)
  if (argc == 1) {
    cout << "wzip: file1 [file2 ...]" << endl;
    return 1;
  }
  else {
    
    for (int i = 1; i < argc; i++) {
      fd = open(argv[i], O_RDONLY);
      if (fd == -1) {
	cout << "wzip: cannot open file" << endl;
	return 1;
      }
      else {
	  while( (ret = read(fd, buffer, 4096)) > 0 ) {
	    dataCollector.append(buffer, ret);
	  }
	close(fd);
      }
    }
    dataCollector = compress(dataCollector);
    stdoutRaw(dataCollector);    
  }
  return 0;
}
