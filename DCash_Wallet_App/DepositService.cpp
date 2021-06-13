#define RAPIDJSON_HAS_STDSTRING 1

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "DepositService.h"
#include "Database.h"
#include "ClientError.h"
#include "HTTPClientResponse.h"
#include "HttpClient.h"

#include "rapidjson/document.h"
#include "rapidjson/prettywriter.h"
#include "rapidjson/istreamwrapper.h"
#include "rapidjson/stringbuffer.h"

using namespace rapidjson;
using namespace std;

DepositService::DepositService() : HttpService("/deposits") {}

void DepositService::post(HTTPRequest *request, HTTPResponse *response)
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
    // Let's check if they are the right ones(amount, stripe_token)
    // In addition check if they are empty(apparently someone else handles it (error code: 500))
    // in both cases this is a badRequest
    WwwFormEncodedDict decoded_body = request->formEncodedBody();
    if (decoded_body.get("amount").empty() || decoded_body.get("stripe_token").empty())
    {
        throw ClientError::badRequest();
    }

    // The exception wasn't thrown
    string amount = decoded_body.get("amount");
    string stripe_token = decoded_body.get("stripe_token");

    // Amount cannnot be less than 50
    int amountInt = stoi(amount);

    if (amountInt < 50)
    {
        throw ClientError::badRequest();
    }

    // Create and encode the body for the request
    WwwFormEncodedDict body;
    body.set("amount", amountInt);
    body.set("currency", "usd");
    body.set("source", stripe_token);
    string encoded_body = body.encode();

    // From the gunrock server to Stripe (AUTH)
    HttpClient client("api.stripe.com", 443, true);
    client.set_basic_auth(m_db->stripe_secret_key, "");

    // Get the JSON from Stripe
    HTTPClientResponse *client_response = client.post("/v1/charges", encoded_body);

    // Check the Response for error codes
    // Anyway stripe sends back an JSON object, I just do different things depepending on client_response->success()
    if (!client_response->success())
    {
        response->setStatus(client_response->status()); // and that should be it
    }
    else
    {
        // At this point I definitely received a nice JSON
        // Get the data from it(I need User, amount, stripe_charge_id for the DB)
        Document *d = client_response->jsonBody();
        int amount_charged = (*d)["amount"].GetInt();
        string charge_id = (*d)["id"].GetString();
        delete d;

        // Update the DB(deposits)
        Deposit *new_deposit = new Deposit();
        new_deposit->to = authenticated_user;
        new_deposit->amount = amount_charged;
        new_deposit->stripe_charge_id = charge_id;

        m_db->deposits.push_back(new_deposit);

        // New resource created
        // response->setStatus(201);

        // Update the User's balance
        authenticated_user->balance += amount_charged;

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

        for (unsigned int i = 0; i < m_db->deposits.size(); i++)
        {
            if (m_db->deposits[i]->to == authenticated_user)
            {
                // add an object to our array
                Value to;
                to.SetObject();
                to.AddMember("to", m_db->deposits[i]->to->username, a);
                to.AddMember("amount", m_db->deposits[i]->amount, a);
                to.AddMember("stripe_charge_id", m_db->deposits[i]->stripe_charge_id, a);
                array.PushBack(to, a);
            }
        }
        // and add the array to our return object
        o.AddMember("deposits", array, a);

    // now some rapidjson boilerplate for converting the JSON object to a string
    document.Swap(o);
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    document.Accept(writer);

    // set the return object
    response->setContentType("application/json");
    response->setBody(buffer.GetString() + string("\n"));
    }
}

void DepositService::get(HTTPRequest *request, HTTPResponse *response)
{
}
