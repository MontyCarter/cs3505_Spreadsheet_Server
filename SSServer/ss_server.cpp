/*
 * ss_server.cpp
 *
 *  Created on: Apr 13, 2013
 *      Author: montgomc
 */

#include "ss_server.h"
#include <signal.h>


namespace ss {

//Constructor - requires a port and a virtual directory root
ss_server::ss_server(int port, std::string& root_dir, std::string& index_file)
	: io_service_(),
	  acceptor_(io_service_, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
	  ss_manager_(root_dir, index_file)
{

	start_accept_client();
}

void ss_server::stop()
{
	std::cout << "Terminate Received.  Closing the server...\n";
	std::cout << "Closing all spreadsheet sessions...\n";
	std::map<std::string,ss_session*>::iterator sessIt;
	for(sessIt = sessions_.begin(); sessIt != sessions_.end(); sessIt++)
	{
		(*sessIt).second->close();
	}
	std::cout << "Closing all client connections...\n";
	std::set<ss_client_ptr>::iterator cliIt;
	for(cliIt = clients_.begin(); cliIt != clients_.end(); cliIt++)
	{
		(*cliIt)->stop();
	}
	std::cout << "All client connections closed.\n";
	std::cout << "Shutting down the server...\n";

	io_service_.stop();
}

//Starts the server
void ss_server::run()
{
	io_service_.run();
}

//Dispatches a message sent by a client (client is responsible for ensuring proper message formatting)
void ss_server::dispatch_request(ss_client_ptr requester, ss_message request)
{
	std::string reqName;
	switch(request.command)
	{
	case ss_message::CREATE:
		ss_manager_.handle_create_request(requester, request);
		break;
	case ss_message::JOIN:
		// Grab the name of spreadsheet requested to join
		reqName = request.get_val("Name");

		// See if there is not already a session
		if(!sessions_.count(reqName))
		{
			// See if we can get the spreadsheet (see if it exists)
			spreadsheet* curSS = ss_manager_.get_spreadsheet(reqName);
			// If curSS is a null pointer, it means the spreadsheet was not found
			// In this case, TODO
			if(curSS == NULL)
			{
				// Set up a response to go to the requester
				ss_message response;
				response.set("Name", reqName);
				response.command = ss_message::JOIN_FAIL;
				response.set("message", "The requested spreadsheet does not exist");
				requester->tell(response);
				return;
			}
			// Load the spreadsheet, and pass it in to a new ss_session
			curSS->load();
			// Add a new session for the spreadsheet to our map of sessions.
			sessions_[reqName] = new ss_session(reqName, curSS);

		}
		//We've either found or created a session...  lets try to add the requester
		if(sessions_[reqName]->add_client(requester, request.get_val("Password")))
		{
			// Get join ok message from session (session will fill message with appropraite values)
			ss_message* join_ok_msg = sessions_[reqName]->get_join_ok_msg();
			requester->tell(*join_ok_msg);
		}
		else
		{
			// The password did not match
			ss_message response;
			response.command = ss_message::JOIN_FAIL;
			response.set("Name", reqName);
			response.set("message", "The provided password did not match the requested password");
			requester->tell(response);
		}
		break;
	case ss_message::CHANGE:
		reqName = request.get_val("Name");

		// Check that the session exists - if not, send an appropriate response
		if(!sessions_.count(reqName))
		{
			ss_message response;
			response.command = ss_message::CHANGE_FAIL;
			response.set("Name", reqName);
			//response.set("Version", "");  //Removed to conform to updated spec
			response.set("message", "There is no session for the requested spreadsheet");
			requester->tell(response);
			return;
		}
		// The session is valid - pass the request on to the session
		sessions_[reqName]->handle_change_request(requester, request);
		break;
	case ss_message::UNDO:
		reqName = request.get_val("Name");

		// Check that the session exists - if not, send an appropriate response
		if(!sessions_.count(reqName))
		{
			ss_message response;
			response.command = ss_message::UNDO_FAIL;
			response.set("Name", reqName);
			//response.set("Version", "");  //Removed to conform to updated spec
			response.set("message", "There is no session for the requested spreadsheet");
			requester->tell(response);
			return;
		}
		// The session is valid - pass the request on to the session
		sessions_[reqName]->handle_undo_request(requester, request);
		break;
	case ss_message::SAVE:
		reqName = request.get_val("Name");

		// Check that the session exists - if not, send an appropriate response
		if(!sessions_.count(reqName))
		{
			ss_message response;
			response.command = ss_message::SAVE_FAIL;
			response.set("Name", reqName);
			response.set("message", "There is no session for the requested spreadsheet");
			requester->tell(response);
			return;
		}
		// The session is valid - pass the request on to the session
		sessions_[reqName]->handle_save_request(requester, request);
		break;
	case ss_message::LEAVE:
		reqName = request.get_val("Name");

		// Check that the session exists - if not, send an appropriate response
		if(sessions_.count(reqName))
		{
			sessions_[reqName]->drop_client(requester);
			if(sessions_[reqName]->empty())
			{
				// If no clients left, remove it from
				//  sessions, and delete it
				sessions_.erase(reqName);
			}
		}
		// Else, nothing to do...
		break;
	default:
		break;
	}
}

//Listens for incoming connections
void ss_server::start_accept_client()
{
	//Set up a new ss_client object, to attach the next
	//  incoming connection to.
	next_client_.reset(new ss_client(io_service_, *this));

	//Tell this->acceptor_ to start accepting asynchronously, and tell
	//  it to put the new socket in the tcp_connection we just created

	//Because the async_accept method requires a function OBJECT (as
	//  opposed to a function pointer) for the second parameter, we
	//  must use boost::bind to create a function object out of the
	//  handle_accept callback that will be called after a connection
	//  is accepted
	acceptor_.async_accept(next_client_->socket(),
			boost::bind(&ss_server::handle_accept_client, this,
					boost::asio::placeholders::error));
}

//handle_accept is the callback that is invoked after an incoming connection
//  is created.
void ss_server::handle_accept_client(const boost::system::error_code& error)
{
	// Start the client
	next_client_->start();
	// And add the client to the list of clients
	clients_.insert(next_client_);
	// Finally, listen for another connection
	start_accept_client();
}

void ss_server::remove_client(ss_client_ptr to_drop)
{
	clients_.erase(to_drop);
}
}
