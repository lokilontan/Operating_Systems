#include <iostream>
#include <iterator>
#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

using namespace std;
char error_message[30] = "An error has occurred\n";

//tokenizer should be able to separate each input command(single line) into tokens
//any number of any white-space character should be handled
//assume that '>' and '&' commands don't have to be surrounded by white-space, but they can be
vector<string> tokenize(string input_command)
{   
    //define all the vars
    vector<string> in_tokens;  //for the final tokens
    string ws = " \t\n\v\f\r";   //will search NOT for any of these; ws for white-space
    string special = ">&";      //also will search for any of these
    size_t init_pos = 0;        //some other vars to manipulate wit the string
    size_t final_pos;
    string temp_string;
    size_t pos_special = 0;
    string temp_string_special = "";

    while (init_pos != string::npos)
    {   
        //look for any char, but not any from ws
        init_pos = input_command.find_first_not_of(ws, init_pos);

        //if there is something
        if (init_pos != string::npos)
        {
            //look for the next char which is this time FROM the ws
            //get substring from init_pos
            //init_pos should be updated, so next time program doesn't search in the previous indices 
            final_pos = input_command.find_first_of(ws, init_pos + 1);
            temp_string = input_command.substr(init_pos, final_pos - init_pos);
            init_pos = final_pos;
            
            //the chunk of code is good with whitespaces
            //now we need to check if there are any '>' or '&' in a separate command
            pos_special = 0;

            //loop until there is a special command
            while ((temp_string.find(">") != string::npos) || (temp_string.find("&") != string::npos))
            {   
                //if '>' comes before '&' then 
                if (temp_string.find(">") < temp_string.find("&"))
                {
                    //find the position of '>'
                    pos_special = temp_string.find(">");
                    //get the substing from 0 to '>'
                    temp_string_special = temp_string.substr(0, pos_special);
                    //sometimes there are empty strings because of the substr(0,0)
                    //do not include them
                    if (!temp_string_special.empty())
                    {
                        in_tokens.push_back(temp_string_special);
                    }
                    //do not forget '>'
                    in_tokens.push_back(">");
                    //erase the command before '>' and erase '>' too
                    temp_string.erase(0, pos_special + 1);
                }
                else
                {   
                    //the same process here with '&'
                    pos_special = temp_string.find("&");
                    temp_string_special = temp_string.substr(0, pos_special);
                    if (!temp_string_special.empty())
                    {
                        in_tokens.push_back(temp_string_special);
                    }
                    in_tokens.push_back("&");
                    temp_string.erase(0, pos_special + 1);
                }
            }
            //after the '>' or '&' cases there are cases where temp_string remains empty
            //do not need it
            if (!temp_string.empty())
            {
                in_tokens.push_back(temp_string);
            }
        }
    }
    //finally
    return (in_tokens);
}

void interactive()
{
    string input_command;
    //char exit_command[] = "exit";
    vector<string> input_tokens;

    do
    {
        cout << "wish> ";
        getline(cin, input_command);
        input_tokens = tokenize(input_command);

        //test
        for (vector<string>::const_iterator i = input_tokens.begin(); i != input_tokens.end(); ++i)
        {
            cout << *i << endl;
        }
        //strcmp(input_tokens[0].c_str(), exit_command) != 0
    } while (cin);
}

void batch(string data_from_file)
{
    string input_command;
    size_t newLinePos = 0;
    size_t oldPos = 0;
    vector<string> input_tokens;
    //char exit_command[] = "exit";

    //look trhough the data
    do
    {
        //get the position of first found \n, always start searching from the point stopped before
        newLinePos = data_from_file.find_first_of("\n", newLinePos);

        //get the substring from the oldPos to the next \n, grab one more character which is \n
        input_command = data_from_file.substr(oldPos, newLinePos - oldPos + 1);

        //increment newLinePos to start looking for \n AFTER last found \n
        newLinePos++;

        //update oldPos for the next input_command
        oldPos = newLinePos;

        input_tokens = tokenize(input_command);

        //test
        for (vector<string>::const_iterator i = input_tokens.begin(); i != input_tokens.end(); ++i)
        {
            cout << *i << endl;
        }

        // && (strcmp(input_tokens[0].c_str(), exit_command) != 0)
    } while ((newLinePos < (data_from_file.length())));
}

int main(int argc, char *argv[])
{

    int fd;
    string data_from_file;
    int ret;
    char buffer[4096];

    //choose the mode
    if (argc == 1)
    {
        //interactive mode (no file for batch)
        interactive();
        exit(0);
    }
    else if (argc == 2)
    {
        //batch mode
        fd = open(argv[1], O_RDONLY);

        //check if the file is valid
        if (fd < 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }

        //read everything from the file using a buffer to the string "data"
        while ((ret = read(fd, buffer, 4096)) > 0)
        {
            data_from_file.append(buffer, ret);
        }

        //check if the file was empty
        if (data_from_file.length() != 0)
        {
            batch(data_from_file);
        }

        close(fd);

        exit(0);
    }
    else
    {
        //more than one argument(file) passed
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(1);
    }
}
