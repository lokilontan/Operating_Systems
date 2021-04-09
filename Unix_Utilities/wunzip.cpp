#include <iostream>                                                    
                                                                       
#include <fcntl.h>                                                     
#include <stdlib.h>                                                    
                                                                       
#include <sys/types.h>                                                 
#include <sys/uio.h>                                                   
#include <unistd.h>                                                    
#include <vector>

using namespace std;

void stdout_unzip(vector<int> freq, vector<char> chars) {
  
    for (unsigned int i = 0; i < freq.size(); i++) {
      write(STDOUT_FILENO, string(freq[i], chars[i]).c_str() , freq[i]);
    }
    
}

int main(int argc, char *argv[]) {

  int fd;
  char charBuffer[1];
  vector<char> charVector;
  int intBuffer[1];
  vector<int> intVector;
  int ret;
  
  //check if there is no arguments(files)                              
  if (argc == 1) {
    cout << "wunzip: file1 [file2 ...]" << endl;
    return 1;
  }
  else {

    for (int i = 1; i < argc; i++) {
      fd = open(argv[i], O_RDONLY);
      if (fd == -1) {
        cout << "wunzip: cannot open file" << endl;
        return 1;
      }
      else {
	while( (ret = read(fd, intBuffer, 4)) > 0 ) {
	  intVector.push_back(*intBuffer);
	  read(fd, charBuffer, 1);
	  charVector.push_back(*charBuffer);
	}
        close(fd);
      }
    }

    stdout_unzip(intVector, charVector);
   
  }
  return 0;
}

