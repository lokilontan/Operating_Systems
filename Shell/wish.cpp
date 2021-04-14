#include <iostream>
 
#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;

vector<string> tokenize(string input_command) {
    vector<string> in_tokens;
    in_tokens.push_back(input_command);

    //TODO: implement this
    return(in_tokens);
}

int main(int argc, char *argv[]) {

    int fd;
    char error_message[30] = "An error has occurred\n";
    string input_command = "";
    vector<string> input_tokens;
    char exit_command[] = "exit";

    //choose the mode
    if (argc == 1) {
        //interactive mode (no file for batch)
        do{
            cout << "wish> ";
            getline(cin, input_command);
            input_tokens = tokenize(input_command);
        }
        while (strcmp(input_tokens[0].c_str(), exit_command) != 0);
        exit(0);
    } else if (argc == 2) {
        //batch mode
        fd = open(argv[1], O_RDONLY);
        
        //check if the file is valid
        if (fd < 0) {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        close(fd);
        exit(0);
    } else {
        //more than one argument(file) passed
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }

}
