#include <iostream>
#include <iterator>
#include <fcntl.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fstream>

using namespace std;
char error_message[30] = "An error has occurred\n";
enum command_kind
{
    OTHER,
    EXIT,
    CD,
    PATH
};
vector<string> path;

//tokenizer should be able to separate each input command(single line) into tokens
//any number of any white-space character should be handled
//assume that '>' and '&' commands don't have to be surrounded by white-space, but they can be
vector<string> tokenize(string input_command)
{
    //define all the vars
    vector<string> in_tokens;  //for the final tokens
    string ws = " \t\n\v\f\r"; //will search NOT for any of these; ws for white-space
    string special = ">&";     //also will search for any of these
    size_t init_pos = 0;       //some other vars to manipulate wit the string
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

//this function returns the kind of the command passed into it
command_kind resolve_kind(string command)
{
    if (command.compare("exit") == 0)
    {
        return EXIT;
    }
    else if (command.compare("cd") == 0)
    {
        return CD;
    }
    else if (command.compare("path") == 0)
    {
        return PATH;
    }
    return OTHER;
}

//performs exit; no command in args
void exit_shell(vector<string> args)
{
    if (args.empty())
    {
        exit(0);
    }
    else
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}
//performs change of directory; no command in args
void cd_shell(vector<string> args)
{
    if (args.size() == 1)
    {
        if (chdir(args[0].c_str()) == -1)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    else
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
}

//performs change of path dir's; no command in args
void path_shell(vector<string> args)
{
    if (args.empty())
    {
        path.clear();
    }
    else
    {
        path.clear();
        for (int i = 0; i < args.size(); i++)
        {
            path.push_back(args[i]);
        }
    }
}

//returns: -1 - invalid statement; 0 - NO redirection; 1 - valid statement with redirection
int redirect_status(vector<string> args)
{
    int redirect_cmd_count = 0; //simply count '>'
    int files_count = 0;        //simply count args after first '>' found
    int out_status;

    for (int i = 0; i < args.size(); i++)
    {
        if (args[i].compare(">") == 0)
        {
            redirect_cmd_count++;
        }
        else if (redirect_cmd_count > 0)
        {
            files_count++;
        }
    }

    //-1: more than one arg(file) after '>', more than one '>'s, only '>' and no file
    //0:  no '>' - no files,respectively, let access() check if the cmd is legal
    //1: only one '>' and only one arg(file) after it
    if (((redirect_cmd_count > 1) || (files_count > 1)) ||
        ((redirect_cmd_count == 1) && (files_count == 0)) ||
        ((redirect_cmd_count == 1) && (files_count == 1) && (args.size() == 2)))
    {
        out_status = -1;
    }
    else if ((redirect_cmd_count == 1) && (files_count == 1))
    {
        out_status = 1;
    }
    else
    {
        out_status = 0;
    }
    return out_status;
}

//performs any other type of command; command passed in args
void other_shell(vector<string> args, bool isParallel)
{
    if (!args.empty())
    {
        int red_status = redirect_status(args);

        //if the command makese sense
        if (red_status != -1)
        {
            int red_loc;
            int size_to_allocate;

            //if there is an '>', get its location
            if (red_status == 1)
            {
                for (int i = 0; i < args.size(); i++)
                {
                    if (args[i].compare(">") == 0)
                    {
                        red_loc = i;
                    }
                }
            }

            //determine how much memory to allocate for the args
            if (red_status == 1)
            {
                size_to_allocate = red_loc;
            }
            else
            {
                size_to_allocate = args.size();
            }

            //put the args into a char **array
            char **new_args = new char *[size_to_allocate + 1];
            for (int i = 0; i < args.size(); i++)
            {
                if (args[i].compare(">") == 0)
                    break;
                new_args[i] = strdup(args[i].c_str());
            }
            //LAST element is nullptr
            new_args[size_to_allocate] = nullptr;

            //determine if the command is accessible
            bool accessible = false;
            for (int i = 0; i < path.size(); i++)
            {
                string abs_path = path[i] + '/' + args[0];
                if (access(abs_path.c_str(), X_OK) == 0)
                {
                    accessible = true;
                    new_args[0] = strdup(abs_path.c_str());
                    break;
                }
            }

            //now execute it or print out the error
            if (accessible)
            {
                pid_t ret = fork(); //new proccess
                if (ret < 0)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else if (ret == 0) //if this is a child proccess
                {
                    if (red_status == 1) //if redirection is valid
                    {
                        //open the specified file
                        int red_fd = open(args[red_loc + 1].c_str(), O_CREAT | O_TRUNC | O_RDWR, 0644);

                        //error check
                        if (red_fd != -1)
                        {
                            //STDERR and STDOUT are now linked to red_fd(file)
                            if ((dup2(red_fd, STDERR_FILENO) == -1) || (dup2(red_fd, STDOUT_FILENO) == -1))
                            {
                                write(STDERR_FILENO, error_message, strlen(error_message));
                            }
                            close(red_fd);
                        }
                        else
                        {
                            write(STDERR_FILENO, error_message, strlen(error_message));
                        }
                    }
                    //execute the command
                    execv(new_args[0], new_args);
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
                else
                {
                }
            }
            else
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }
        else
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }
    if (!isParallel)
    {
        pid_t wpid;
        while ((wpid = wait(NULL)) > 0)
            ;
    }
}

void execute(vector<string> commands)
{
    if (!commands.empty())
    {
        switch (resolve_kind(commands[0]))
        {
        case EXIT:
        {
            commands.erase(commands.begin());
            exit_shell(commands);
            break;
        }
        case CD:
        {
            commands.erase(commands.begin());
            cd_shell(commands);
            break;
        }
        case PATH:
        {
            commands.erase(commands.begin());
            path_shell(commands);
            break;
        }
        default:
        {
            //vector for parallel proccesses
            vector<string> cmd;
            for (int i = 0; i < commands.size(); i++)
            {
                //run a child proccess once you catch a '&'
                if (commands[i].compare("&") == 0)
                {
                    other_shell(cmd, true);
                    cmd.clear();
                }
                else
                {
                    cmd.push_back(commands[i]);
                }
            }
            //once the OLDEST parent will be done with collecting commands
            //it will call the function (PARALLELLY) for the last time, while other children still work on their commands
            //the cmd vector will hold anything after the last '&'. It can be empty, or just a simple command
            //cmd vector can also collect all the elements from initial vector commands if no '&' will be met
            other_shell(cmd, false);
            break;
        }
        }
    }
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
        execute(input_tokens);

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

        execute(input_tokens);

        // && (strcmp(input_tokens[0].c_str(), exit_command) != 0)
    } while ((newLinePos < (data_from_file.length())));
}

int main(int argc, char *argv[])
{

    int fd;
    string data_from_file;
    int ret;
    char buffer[4096];
    path.push_back("/bin");

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
