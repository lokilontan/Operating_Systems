# This is a basic Unix Shell

# Usage:
./wish - for interactive mode
./wish batch.txt - for batch mode, where batch.txt contains commands on each line

Built-in commands:
- exit
- cd {path}
- path {NO_path, path_1, ..., path_n}  - can accept 0 or more paths.   
    From the start contains '/bin'. If 0 paths passed then path variable is empty

Supports:
- Redirection through symbol '>'
- Parallelism through symbol '&'
- Whitespaces handling