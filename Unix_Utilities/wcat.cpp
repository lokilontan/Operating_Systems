#include <iostream>

#include <fcntl.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

int main(int argc, char *argv[]) {

  int fd;
  char buffer[4096];
  int ret;

  //check if there is no argument(file)
  if (argc == 1) {
    return 0;
  }
  else {
    
    for (int i = 1; i < argc; i++) {
      fd = open(argv[i], O_RDONLY);
      if (fd == -1) {
	cout << "wcat: cannot open file" << endl;
	return 1;
      }
      else {
	while( (ret = read(fd, buffer, 4096)) > 0 ) {
	  write(STDOUT_FILENO, buffer, ret);
	}
	close(fd);
      }
    }
  }

  return 0;
}
