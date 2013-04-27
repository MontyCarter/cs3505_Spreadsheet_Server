/*
 * SSServer.h
 *
 *  Created on: Apr 6, 2013
 *      Author: montgomc
 */

#ifndef SS_SERVER_H_
#define SS_SERVER_H_

#include "ss_client.h"
#include "ss_message.h"
#include "ss_session.h"
#include "spreadsheet_manager.h"
#include <string>
#include <set>
#include <map>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include <signal.h>



namespace ss {

//class spreadsheet_manager;
//class ss_session;
//class ss_client;


class ss_server
{
public:
	// Constructor - requires an io_service, a port, and a directory path
	ss_server(int port, std::string& root_dir, std::string& index_file);
	// Stops the server - shuts down gracefully
	void stop();
	// Start the server service
	void run();
	// Processes a message sent from an ss_client
	void dispatch_request(ss_client_ptr requester, ss_message message);
	// Stop tracking a client
	void remove_client(ss_client_ptr to_drop);

private:
	// -----Socket connections------
	// Start listening for an incoming socket connection
	void start_accept_client();
	// Callback invoked after receiving a socket connection
	void handle_accept_client(const boost::system::error_code& error);
	//  Called from process_request - creates a new spreadsheet

	// The boost io_service object - sits between OS sockets and asio sockets
	boost::asio::io_service io_service_;
	// The boost object that listens for socket connections
	boost::asio::ip::tcp::acceptor acceptor_;
	// The next connection to be accepted
	ss_client_ptr next_client_;
	// A list of all connections
	std::set<ss_client_ptr> clients_;
	// A map of sessions (name,session)
	std::map<std::string,ss_session*> sessions_;
	// The spreadsheet manager
	spreadsheet_manager ss_manager_;

};
}
#endif /* SS_SERVER_H_ */

