#include <iostream>

#include <stdlib.h>
#include <stdio.h>

#include "HttpService.h"
#include "ClientError.h"

using namespace std;

HttpService::HttpService(string pathPrefix) {
  this->m_pathPrefix = pathPrefix;
}

User *HttpService::getAuthenticatedUser(HTTPRequest *request)  {
  // TODO: implement this function
  if (!request->hasAuthToken()) {
    throw ClientError::badRequest();  //if this function was called, then there should be an auth_token
  } 

  string new_auth_token = request->getAuthToken();
  map<std::string, User *>::iterator it;
  it = m_db->auth_tokens.find(new_auth_token);    //if no such key, returns an iterator to map::end
  if (it == m_db->auth_tokens.end()) {
    throw ClientError::unauthorized();    //there is a token, but we did not see it before
  }

  //at this point there must be a user with the auth_token provided.
  User *authenticated_user = m_db->auth_tokens.at(new_auth_token);
  
  return authenticated_user;
}

string HttpService::pathPrefix() {
  return m_pathPrefix;
}

void HttpService::head(HTTPRequest *request, HTTPResponse *response) {
  cout << "HEAD " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::get(HTTPRequest *request, HTTPResponse *response) {
  cout << "GET " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::put(HTTPRequest *request, HTTPResponse *response) {
  cout << "PUT " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::post(HTTPRequest *request, HTTPResponse *response) {
  cout << "POST " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

void HttpService::del(HTTPRequest *request, HTTPResponse *response) {
  cout << "DELETE " << request->getPath() << endl;
  throw ClientError::methodNotAllowed();
}

