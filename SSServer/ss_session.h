/*
 * ss_session.h
 *
 *  Created on: Apr 9, 2013
 *      Author: montgomc
 */

#ifndef SS_SESSION_H_
#define SS_SESSION_H_

#include "ss_client.h"
#include "spreadsheet.h"
#include <set>
#include <string>
#include <stack>
#include <boost/lexical_cast.hpp>


namespace ss {


class ss_session {
public:
	//Create a new spreadsheet session for the spreadsheet filename
	ss_session(std::string ss_name, spreadsheet* ss);

	//Destroys a spreadsheet session
	void close();


	//Processes a change cell request
	//Invoked by dispatch after receiving a CHANGE request
	void handle_change_request(ss_client_ptr requester, ss_message change_request);

	//Processes an undo request
	//Invoked by dispatch after receiving an UNDO request
	void handle_undo_request(ss_client_ptr requester, ss_message undo_request);

	// Processes a SAVE request
	void handle_save_request(ss_client_ptr requester, ss_message save_request);

	// Adds a client to the session returns false if password does not match
	bool add_client(ss_client_ptr new_client, std::string password);

	// Removes a client from the session
	void drop_client(ss_client_ptr old_client);

	// Returns whether the session has clients
	bool empty();

	// Returns a properly formatted JOIN OK message, to be sent to client
	//  (called by server - server is responsible for join/leave handling)
	ss_message* get_join_ok_msg();

private:
	void send_updates(int version, std::string cell, std::string contents, ss_client_ptr initiator);

	// A list of string sockets representing participants in the session
	std::set<ss_client_ptr> clients_;

	// The name of the spreadsheet this session is for
	std::string ss_name_;

	// The spreadsheet object
	spreadsheet* ssheet_;

	// The session version of the spreadsheet
	int version_;

	// The password for the spreadsheet
	std::string password_;

	// Items for the undo stack
	struct change
	{
		std::string cell;
		std::string old_contents;
	};
	//The undo stack
	std::stack<change> undo_stack_;
};
}
#endif /* SS_SESSION_H_ */
