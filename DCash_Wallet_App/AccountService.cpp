#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "AccountService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace std;
using namespace rapidjson;

AccountService::AccountService() : HttpService("/users") {
  
}

void AccountService::get(HTTPRequest *request, HTTPResponse *response) {
    // TODO: implement this function
    // Get authenticated user
    User *authenticated_user = getAuthenticatedUser(request);

    // Get the user_id
    string request_user_id;
    try {
    request_user_id = request->getPathComponents().at(1);
    } catch (exception &err)
    {
        throw ClientError::badRequest();
    }

    // Check if the user_id provided belongs to the authenticated_user
    if (authenticated_user->user_id.compare(request_user_id) != 0) {
        throw ClientError::forbidden();
    }
    
    // At this point we are sure the request is ok
    // Ready to output the Response object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();

    // add a key value pair directly to the object
    o.AddMember("balance", authenticated_user->balance, a);
    o.AddMember("email", authenticated_user->email, a);

    // now some rapidjson boilerplate for converting the JSON object to a string
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}

void AccountService::put(HTTPRequest *request, HTTPResponse *response) {
    // TODO: implement this function
    // Get authenticated user
    User *authenticated_user = getAuthenticatedUser(request);

    // Get the user_id
    string request_user_id;
    try {
    request_user_id = request->getPathComponents().at(1);
    } catch (exception &err)
    {
        throw ClientError::badRequest();
    }

    // Check if the user_id provided belongs to the authenticated_user
    if (authenticated_user->user_id.compare(request_user_id) != 0) {
        throw ClientError::forbidden();
    }

    // Check if the user_id provided belongs to the authenticated_user
    if (authenticated_user->user_id.compare(request_user_id) != 0) {
        throw ClientError::badRequest();
    }

    // There should be only one argument(email)
    string raw_body = request->getBody();
    vector<string> attribute_pairs = StringUtils::split(raw_body, '&');
    if (attribute_pairs.size() != 1)
    {
        throw ClientError::badRequest();
    }

    // Check if the attribute is "email" and not something else
    WwwFormEncodedDict decoded_body = request->formEncodedBody();
    string request_email = decoded_body.get("email");
    if (request_email.empty()) {
        throw ClientError::badRequest();
    }
    
    // At this point we are sure the request is ok and there is some value for email attribute
    // email itself is not checked for correctness
    authenticated_user->email = request_email;

    // Ready to output the Response object
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();

    // add a key value pair directly to the object
    o.AddMember("balance", authenticated_user->balance, a);
    o.AddMember("email", authenticated_user->email, a);

    // now some rapidjson boilerplate for converting the JSON object to a string
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}
