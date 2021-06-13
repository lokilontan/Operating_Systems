#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AuthService.h"
#include "StringUtils.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AuthService::AuthService() : HttpService("/auth-tokens")
{
}

void AuthService::post(HTTPRequest *request, HTTPResponse *response)
{
    // TODO: implement this function
    // First, get the body and check for the right number of attributes
    // attributes are separated with '&'
    string raw_body = request->getBody();
    vector<string> attribute_pairs = StringUtils::split(raw_body, '&');
    if (attribute_pairs.size() != 2)
    {
        throw ClientError::badRequest();
    }

    // at this point there are only 2 attributes
    // Let's check if they are the right ones(username, password)
    // In addition check if they are empty(apparently someone else handles it (error code: 500))
    // in both cases this is a badRequest
    WwwFormEncodedDict decoded_body = request->formEncodedBody();
    if (decoded_body.get("username").empty() || decoded_body.get("password").empty())
    {
        throw ClientError::badRequest();
    }

    // Now check for the username to have only lowercase letters
    string username = decoded_body.get("username");
    string password = decoded_body.get("password");
    int i = 0;
    while (username[i])
    {
        if (isupper(username[i]))
        {
            throw ClientError::badRequest();
        }
        i++;
    }

    // At this point we can check if the user exist
    map<std::string, User *>::iterator it;
    it = m_db->users.find(username); //if no such key, returns an iterator to map::end
    if (it != m_db->users.end())
    {
        // there is a User with such username, compare the passwords
        User *user = m_db->users.at(username);
        if (user == NULL)
        { // I don't know how, but let's check. I mean the key can be there, but the value can be NULL.
            throw ClientError::notFound();
        }
        if (password.compare(user->password) != 0)
        { //passwords don't match
            throw ClientError::forbidden();
        }
    }
    else
    {
        // there is no User with such username, create a User with username
        User *new_user = new User();
        new_user->email = "";
        new_user->username = username;
        new_user->password = password;
        new_user->user_id = StringUtils::createUserId();
        new_user->balance = 0;

        // update the DB (users)
        m_db->users[username] = new_user;

        //update the response status to "Success and created a new resource"
        response->setStatus(201);
    }

    // At this point we already had a User with username in the DB and password matched OR
    // there was no such User and we created it and it is in the DB now
    // Ready to submit a response
    // For the response object we need: auth_token and user_id

    // Create auth_token
    string response_auth_token = StringUtils::createAuthToken();

    // Get the User*
    User *user = m_db->users[username];

    // Update DB (auth_tokens)
    m_db->auth_tokens[response_auth_token] = user;

    // Get the user_id
    string response_user_id = user->user_id;

    // Create Response Object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();

    // add a key value pair directly to the object
    o.AddMember("auth_token", response_auth_token, a);
    o.AddMember("user_id", response_user_id, a);

    // now some rapidjson boilerplate for converting the JSON object to a string
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}

void AuthService::del(HTTPRequest *request, HTTPResponse *response)
{
    // TODO: implement this function
    // Get authenticated user
    User *authenticated_user = getAuthenticatedUser(request);

    // Get the auth_token to delete
    string auth_token_to_delete = request->getPathComponents()[1];

    // Try to find the token in the DB(auth_tokens)
    map<std::string, User *>::iterator it;
    it = m_db->auth_tokens.find(auth_token_to_delete); //if no such key, returns an iterator to map::end
    if (it != m_db->auth_tokens.end()){
        // Get the user of the auth_token to delete
        User *user_found = m_db->auth_tokens.at(auth_token_to_delete);

        // If the authenticated user and user found match, then delete the auth_token
        // Otherwise, throw 403
        if (authenticated_user == user_found) {
            m_db->auth_tokens.erase(auth_token_to_delete);
        } else {
            throw ClientError::forbidden();
        }
    } else {
        // In case if the specified auth_token is not found in the DB(auth_tokens)
        throw ClientError::notFound();
    }

}
