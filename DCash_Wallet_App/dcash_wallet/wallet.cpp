#define RAPIDJSON_HAS_STDSTRING 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <iomanip>

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include "WwwFormEncodedDict.h"
#include "HttpClient.h"
#include "StringUtils.h"

#include "rapidjson/document.h"

using namespace std;
using namespace rapidjson;

//function declarations
void interactive();
void batch(const char *file_path);
void execute(string full_command);
void auth(vector<string> command_tokens);
void balance(vector<string> command_tokens);
void deposit(vector<string> command_tokens);
void send(vector<string> command_tokens);
void logout(vector<string> command_tokens);
vector<string> splitWithDelimiter(string str, char delimiter);
vector<string> split(string str, char delimiter);

char error_message[30] = "Error\n";
int API_SERVER_PORT = 8080;
string API_SERVER_HOST = "localhost";
string PUBLISHABLE_KEY = "";

string auth_token;
string user_id;

int main(int argc, char *argv[])
{
  stringstream config;
  int fd = open("config.json", O_RDONLY);
  if (fd < 0)
  {
    cout << "could not open config.json" << endl;
    exit(1);
  }
  int ret;
  char buffer[4096];
  while ((ret = read(fd, buffer, sizeof(buffer))) > 0)
  {
    config << string(buffer, ret);
  }
  Document d;
  d.Parse(config.str());
  API_SERVER_PORT = d["api_server_port"].GetInt();
  API_SERVER_HOST = d["api_server_host"].GetString();
  PUBLISHABLE_KEY = d["stripe_publishable_key"].GetString();

  //choose the mode
  if (argc == 1)
  {
    //interactive mode (no file for batch)
    interactive();
  }
  else if (argc == 2)
  {
    //batch mode
    batch(argv[1]);
  }
  else
  {
    //more than one argument(file) passed
    write(STDOUT_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  return 0;
}

//function for interactive mode
void interactive()
{
  string input_command;
  do
  {
    cout << "D$> ";
    getline(cin, input_command);
    execute(input_command);
  } while (cin);
}

void batch(const char *file_path)
{
  int fd;
  string data_from_file;
  int ret;
  char buffer[4096];
  fd = open(file_path, O_RDONLY);
  vector<string> batch_lines;

  //check if the file is valid
  if (fd < 0)
  {
    write(STDOUT_FILENO, error_message, strlen(error_message));
    exit(1);
  }

  //read everything from the file using a buffer to the string "data_from_file"
  while ((ret = read(fd, buffer, 4096)) > 0)
  {
    data_from_file.append(buffer, ret);
  }

  //check if the file was empty
  if (data_from_file.length() != 0)
  {
    batch_lines = split(data_from_file, '\n');
    for (int i = 0; i < (int)batch_lines.size(); i++)
    {
      execute(batch_lines[i]);
    }
  }

  close(fd);
}

void execute(string full_command)
{
  try
  {
    // Check if the command is empty
    if (full_command.empty())
    {
      throw "ERROR: command is empty!";
    }

    // Tokenize it; NOTE: not very robust with the whitespaces except '\n' and ' '
    vector<string> command_tokens = StringUtils::split(full_command, ' ');
    string command = command_tokens[0];

    if (command.compare("auth") == 0)
    {
      auth(command_tokens);
    }
    else if (command.compare("balance") == 0)
    {
      balance(command_tokens);
    }
    else if (command.compare("deposit") == 0)
    {
      deposit(command_tokens);
    }
    else if (command.compare("send") == 0)
    {
      send(command_tokens);
    }
    else if (command.compare("logout") == 0)
    {
      logout(command_tokens);
    }
    else
    {
      throw "ERROR: command does not exist!";
    }
  }
  catch (const char *error)
  {
    //cout << error << endl;
    write(STDOUT_FILENO, error_message, strlen(error_message));
  }
}

void auth(vector<string> command_tokens)
{
  // check for the correct number of args; should be 4 in total
  if (command_tokens.size() != 4)
  {
    throw "ERROR: wrong number of attributes!";
  }

  // create an HTTP request to the GunRock Server
  // Create and encode the body for the request
  WwwFormEncodedDict body_post_auth;
  body_post_auth.set("username", command_tokens[1]);
  body_post_auth.set("password", command_tokens[2]);
  string encoded_body_post_auth = body_post_auth.encode();

  // From the wallet to the GunRock server (AUTH)
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

  // Get the JSON from GunRock
  HTTPClientResponse *client_response = client.post("/auth-tokens", encoded_body_post_auth);

  // Check the Response for error codes
  if (!client_response->success())
  {
    throw "ERROR: in POST /auth-token response";
  }

  // At this point I definitely received a nice JSON
  // Get the data from it(I need auth_token, user_id)
  Document *d = client_response->jsonBody();
  string auth_token_received = (*d)["auth_token"].GetString();
  string user_id_received = (*d)["user_id"].GetString();

  // Check if someone is already logged in in the wallet
  // If so, delete the auth_token from the server
  if (!auth_token.empty())
  {
    // From the wallet to the GunRock server (AUTH)
    HttpClient client_2(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

    // Set header for the future calls
    client_2.set_header("x-auth-token", auth_token);

    // Get the JSON from GunRock
    string path = "/auth-tokens/" + auth_token;
    client_response = client_2.del(path);

    // Check the Response for error codes
    if (!client_response->success())
    {
      throw "ERROR: in DEL /auth-token/user_id response";
    }
  }
  // At this point I must update auth_token and user_id inside the wallet, anyway
  auth_token = auth_token_received;
  user_id = user_id_received;

  // Now the new or the same user is logged in with the FRESH auth tokens
  // Update the email address

  // create an HTTP request to the GunRock Server
  // Create and encode the body for the request
  WwwFormEncodedDict body_put_account;
  body_put_account.set("email", command_tokens[3]);
  string encoded_body_put_account = body_put_account.encode();

  // From the wallet to the GunRock server (ACCOUNT)
  HttpClient client_3(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

  // Set header
  client_3.set_header("x-auth-token", auth_token);

  // Get the JSON from GunRock
  string path = "/users/" + user_id;
  client_response = client_3.put(path, encoded_body_put_account);

  // Check the Response for error codes
  if (!client_response->success())
  {
    throw "ERROR: in PUT /users/user_id response";
  }

  // At this point I definitely received a nice JSON
  // Get the data from it(I need the balance)
  d = client_response->jsonBody();
  double balance_received = (*d)["balance"].GetDouble();
  delete d;

  cout << fixed;
  cout << std::setprecision(2);
  cout << "Balance: $" << balance_received / 100 << endl;
}

void balance(vector<string> command_tokens)
{
  // Check for the correct number of args; should be 1 in total
  if (command_tokens.size() != 1)
  {
    throw "ERROR: wrong number of attributes!";
  }

  // Ideally, I need to check if an user is authenticated in the wallet(I have non-empty credentials)
  // But the API server should handle this
  // Hopefully, setting an empty header value will work. And it worked...

  // From the wallet to the GunRock server (ACCOUNT)
  HttpClient client_3(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

  // Set header
  client_3.set_header("x-auth-token", auth_token);

  // Get the JSON from GunRock
  string path = "/users/" + user_id;
  HTTPClientResponse *client_response = client_3.get(path);

  // Check the Response for error codes
  if (!client_response->success())
  {
    throw "ERROR: in GET /users/user_id response";
  }

  // At this point I definitely received a nice JSON
  // Get the data from it(I need the balance)
  Document *d = client_response->jsonBody();
  double balance_received = (*d)["balance"].GetDouble();
  delete d;

  cout << fixed;
  cout << std::setprecision(2);
  cout << "Balance: $" << balance_received / 100 << endl;
}

void deposit(vector<string> command_tokens)
{
  // Check for the correct number of args; should be 6 in total
  if (command_tokens.size() != 6)
  {
    throw "ERROR: wrong number of attributes!";
  }

  // Now it's time to tokenize the provided card with Stripe

  // from the dcash wallet to Stripe
  HttpClient client("api.stripe.com", 443, true);
  client.set_header("Authorization", string("Bearer ") + PUBLISHABLE_KEY);

  // Create and encode the body for the request
  // Stripe docs show the request example where values are all int's
  // Passing the strings worked fine
  WwwFormEncodedDict body;
  body.set("card[number]", command_tokens[2]);
  body.set("card[exp_month]", command_tokens[4]);
  body.set("card[exp_year]", command_tokens[3]);
  body.set("card[cvc]", command_tokens[5]);
  string encoded_body = body.encode();

  // Get the JSON from Stripe
  HTTPClientResponse *client_response = client.post("/v1/tokens", encoded_body);

  // Check the Response for error codes
  // Anyway stripe sends back an JSON object, I just do different things depepending on client_response->success()
  if (!client_response->success())
  {
    throw "ERROR: in POST /v1/tokens Stripe response!";
  }

  // At this point I definitely received a nice JSON
  // Get the data from it(I need User, amount, stripe_charge_id for the DB)
  Document *d = client_response->jsonBody();
  string card_token_id = (*d)["id"].GetString();
  delete d;

  // Make a deposit request to the API server
  // From the wallet to the GunRock server (DEPOSIT)
  HttpClient client_2(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

  // Set header
  client_2.set_header("x-auth-token", auth_token);

  // Create and encode the body for the request
  WwwFormEncodedDict body_post_deposits;
  body_post_deposits.set("amount", stod(command_tokens[1]) * 100); //??????? BE CAREFUL
  body_post_deposits.set("stripe_token", card_token_id);
  string encoded_body_post_deposits = body_post_deposits.encode();

  // Get the JSON from GunRock
  client_response = client_2.post("/deposits", encoded_body_post_deposits);

  // Check the Response for error codes
  if (!client_response->success())
  {
    throw "ERROR: in POST /deposits response";
  }

  // At this point I definitely received a nice JSON
  // Get the data from it(I need the balance)
  d = client_response->jsonBody();
  double balance_received = (*d)["balance"].GetDouble();
  delete d;

  cout << fixed;
  cout << std::setprecision(2);
  cout << "Balance: $" << balance_received / 100 << endl;
}

void send(vector<string> command_tokens)
{
  // Check for the correct number of args; should be 3 in total
  if (command_tokens.size() != 3)
  {
    throw "ERROR: wrong number of attributes!";
  }

  // Create a transfer request to the API server
  // From the wallet to the GunRock server (TRANSFER)
  HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

  // Set header
  client.set_header("x-auth-token", auth_token);

  // Create and encode the body for the request
  WwwFormEncodedDict body;
  body.set("amount", stod(command_tokens[2]) * 100);
  body.set("to", command_tokens[1]);
  string encoded_body = body.encode();

  // Get the JSON from GunRock
  HTTPClientResponse *client_response = client.post("/transfers", encoded_body);

  // Check the Response for error codes
  if (!client_response->success())
  {
    throw "ERROR: in POST /transfers response";
  }

  // At this point I definitely received a nice JSON
  // Get the data from it(I need the balance)
  Document *d = client_response->jsonBody();
  double balance_received = (*d)["balance"].GetDouble();
  delete d;

  cout << fixed;
  cout << std::setprecision(2);
  cout << "Balance: $" << balance_received / 100 << endl;
}

void logout(vector<string> command_tokens)
{
  // Check for the correct number of args; should be 1 in total
  if (command_tokens.size() != 1)
  {
    throw "ERROR: wrong number of attributes!";
  }

  // Check if someone is already logged in in the wallet
  // If so, delete the auth_token from the server
  if (!auth_token.empty())
  {
    // From the wallet to the GunRock server (AUTH)
    HttpClient client(API_SERVER_HOST.c_str(), API_SERVER_PORT, false);

    // Set header for the future calls
    client.set_header("x-auth-token", auth_token);

    // Get the JSON from GunRock
    string path = "/auth-tokens/" + auth_token;
    HTTPClientResponse *client_response = client.del(path);

    // Check the Response for error codes
    if (!client_response->success())
    {
      throw "ERROR: in DEL /auth-token/user_id response";
    }
  }

  exit(0);
}

// Modified method from StringUtils, which includes empty string too
// Needed to pass test case 17 for wallet
vector<string> splitWithDelimiter(string str, char delimiter) {
  vector<string> result;

  int start = 0;
  int idx;
  for (idx = 0; idx < (int) str.length(); idx++) {
    // if we're at the delimiter
    if (str[idx] == delimiter) {
     
      result.push_back(str.substr(start, idx-start));
      
      string s;
      s.push_back(delimiter);
      result.push_back(s);
      start = idx + 1;
    }
  }

  //result.push_back(str.substr(start, idx-start));

  return result;
}

// The same method provided from StringUtils
vector<string> split(string str, char delimiter) {
  vector<string> result;
  vector<string> initial = splitWithDelimiter(str, delimiter);
  string d;
  d.push_back(delimiter);

  // filter out the delimiter tokens
  for (int idx = 0; idx < (int) initial.size(); idx++) {
    if (initial[idx] != d) {
      result.push_back(initial[idx]);
    }
  }

  return result;
}