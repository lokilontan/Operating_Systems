#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "TransferService.h"
#include "ClientError.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

TransferService::TransferService() : HttpService("/transfers") {}

void TransferService::post(HTTPRequest *request, HTTPResponse *response)
{
    // TODO: implement this function
    // Get authenticated user
    User *authenticated_user = getAuthenticatedUser(request);

    // Get the body and check for the right number of attributes
    // attributes are separated with '&'
    string raw_body = request->getBody();
    vector<string> attribute_pairs = StringUtils::split(raw_body, '&');
    if (attribute_pairs.size() != 2)
    {
        throw ClientError::badRequest();
    }

    // at this point there are only 2 attributes
    // Let's check if they are the right ones(amount, to)
    // In addition check if they are empty(apparently someone else handles it (error code: 500))
    // in both cases this is a badRequest
    WwwFormEncodedDict decoded_body = request->formEncodedBody();
    if (decoded_body.get("amount").empty() || decoded_body.get("to").empty())
    {
        throw ClientError::badRequest();
    }

    // The exception wasn't thrown
    string amount_to_transfer_str = decoded_body.get("amount");
    // Check if amount value is an int
    int amount_to_transfer;
    try
    {
        amount_to_transfer = stoi(amount_to_transfer_str);
    }
    catch (exception &err)
    {
        throw ClientError::badRequest();
    }
    string transfer_to_username = decoded_body.get("to");

    // Check if the transfer_to_username exists
    map<std::string, User *>::iterator it;
    it = m_db->users.find(transfer_to_username); //if no such key, returns an iterator to map::end
    if (it == m_db->users.end())
    {
        throw ClientError::notFound();
    }

    // Check if the tranfer will produce a negative authenticated_user's balance
    // Or if the amount_to_transfer is negative
    if ( ((authenticated_user->balance - amount_to_transfer) < 0) || (amount_to_transfer < 0) )
    {
        throw ClientError::forbidden();
    }

    // At this point I am ready to initiate the transfer between Users
    Transfer *new_transfer = new Transfer();
    new_transfer->from = authenticated_user;
    new_transfer->to = m_db->users.at(transfer_to_username);
    new_transfer->amount = amount_to_transfer;

    m_db->transfers.push_back(new_transfer);

    // New resource created
    // response->atus(201);

    // Update the balances
    authenticated_user->balance -= amount_to_transfer;
    m_db->users.at(transfer_to_username)->balance += amount_to_transfer;

    // Prepare the response
    Document document;
    Document::AllocatorType &a = document.GetAllocator();
    Value o;
    o.SetObject();

    // add a key value pair directly to the object
    o.AddMember("balance", authenticated_user->balance, a);
    // create an array
    Value array;
    array.SetArray();

    for (unsigned int i = 0; i < m_db->transfers.size(); i++)
    {
        if ( (m_db->transfers[i]->to == authenticated_user) || 
             (m_db->transfers[i]->from == authenticated_user) )
        {
            // add an object to our array
            Value to;
            to.SetObject();
            to.AddMember("from", m_db->transfers[i]->from->username, a);
            to.AddMember("to", m_db->transfers[i]->to->username, a);
            to.AddMember("amount", m_db->transfers[i]->amount, a);
            array.PushBack(to, a);
        }
    }
    // and add the array to our return object
    o.AddMember("transfers", array, a);

    // now some rapidjson boilerplate for converting the JSON object to a string
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
}

void TransferService::get(HTTPRequest *request, HTTPResponse *response)
{
}
