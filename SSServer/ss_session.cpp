/*
 * ss_session.cpp
 *
 *  Created on: Apr 9, 2013
 *      Author: montgomc
 */

#include "ss_session.h"

namespace ss {


ss_session::ss_session(std::string ss_name, spreadsheet* ss)
	: ss_name_(ss_name),
	  ssheet_(ss),
	  version_(0),
	  password_(ssheet_->get_password())
{

}

void ss_session::handle_change_request(ss_client_ptr requester, ss_message change_request)
{
	// The response that will be sent
	ss_message response;
	response.set("Name", change_request.get_val("Name"));
	//Make sure the client is in the session
	if(clients_.find(requester) == clients_.end())
	{
		response.command = ss_message::CHANGE_FAIL;
		//Removed to conform to updated spec
		//response.set("Version", boost::lexical_cast<std::string>(version_));
		response.set("message", "Client not member of spreadsheet session");
		requester->tell(response);
		return;
	}

	// See if the version is correct
	if(boost::lexical_cast<int>(change_request.get_val("Version")) != version_ )
	{
		response.command = ss_message::CHANGE_WAIT;
		response.set("Version", boost::lexical_cast<std::string>(version_));
		//Removed to conform to updated spec
		//response.set("message", "Client spreadsheet version out of date");
		requester->tell(response);
		return;
	}

	// Push an undo struct on to the undo stack
	change chng;
	chng.cell = change_request.get_val("Cell");
	chng.old_contents = ssheet_->get_cell_contents(chng.cell);
	undo_stack_.push(chng);

	// The change is good - apply
	ssheet_->set_cell_contents(change_request.get_val("Cell"), change_request.get_val("content"));
	version_++;
	response.command = ss_message::CHANGE_OK;
	response.set("Version", boost::lexical_cast<std::string>(version_));



	// Tell the requester and inform the others
	requester->tell(response);
	send_updates(version_, change_request.get_val("Cell"), change_request.get_val("content"), requester);
}

void ss_session::close()
{

	std::cout << "Saving spreadsheet " << ss_name_ << " from open session...\n";
	ssheet_->save();
	delete ssheet_;
	std::cout << ss_name_ << " saved successfully.\n";
}

void ss_session::handle_undo_request(ss_client_ptr requester, ss_message undo_request)
{
	// The response that will be sent
	ss_message response;
	response.set("Name", undo_request.get_val("Name"));
	// Make sure the client is part of the session
	if(clients_.find(requester) == clients_.end())
	{
		response.command = ss_message::UNDO_FAIL;
		//Removed to conform to updated spec
		//response.set("Version", boost::lexical_cast<std::string>(version_));
		response.set("message", "Client not member of spreadsheet session");
		requester->tell(response);
		return;
	}

	// See if the version is correct
	if(boost::lexical_cast<int>(undo_request.get_val("Version")) != version_ )
	{
		response.command = ss_message::UNDO_WAIT;
		response.set("Version", boost::lexical_cast<std::string>(version_));
		//Removed to conform to updated spec
		//response.set("message", "Client spreadsheet version out of date");
		requester->tell(response);
		return;
	}

	// See if there are any unsaved changes to undo
	if(undo_stack_.empty())
	{
		response.command = ss_message::UNDO_END;
		response.set("Version", boost::lexical_cast<std::string>(version_));
		requester->tell(response);
		return;
	}

	// If here, we're good to go.  Let's undo.
	change undoing = undo_stack_.top();
	undo_stack_.pop();
	std::string cell = undoing.cell;
	std::string contents = undoing.old_contents;
	ssheet_->set_cell_contents(cell,contents);
	version_++;
	//Prepare the response for the requester
	response.command = ss_message::UNDO_OK;
	response.set("Version", boost::lexical_cast<std::string>(version_));
	response.set("Cell", cell);
	response.set("Length", boost::lexical_cast<std::string>(contents.length()));
	response.set("contents", contents);
	//Send the response to the requester, and then tell the others
	requester->tell(response);
	send_updates(version_, cell, contents, requester);

}

void ss_session::handle_save_request(ss_client_ptr requester, ss_message save_request)
{
	// The response that will be sent
	ss_message response;
	response.set("Name", save_request.get_val("Name"));
	// Make sure the client is part of the session
	if(clients_.find(requester) == clients_.end())
	{
		response.command = ss_message::SAVE_FAIL;
		response.set("message", "Client not member of spreadsheet session");
		requester->tell(response);
		return;
	}

	// Save the spreadsheet
	ssheet_->save();

	// Empty the undo stack
	std::stack<change> empty;
	undo_stack_ = empty;

	// And send the response
	response.command = ss_message::SAVE_OK;
	requester->tell(response);
}

// Just adds a client to the clients_ set
bool ss_session::add_client(ss_client_ptr new_client, std::string password)
{
	// Make sure the password matches
	if(password == password_)
	{
		clients_.insert(new_client);
		return true;
	}
	else
	{
		return false;
	}
}

// Just removes a client from the clients_ set
void ss_session::drop_client(ss_client_ptr old_client)
{
	// Remove the session
	clients_.erase(old_client);
	//If no clients remain, self-destruct
}

bool ss_session::empty()
{
	return clients_.empty();
}

ss_message* ss_session::get_join_ok_msg()
{
	ss_message* join_ok_msg = new ss_message();
	join_ok_msg->command = ss_message::JOIN_OK;
	join_ok_msg->set("Name", ss_name_);
	join_ok_msg->set("Version", boost::lexical_cast<std::string>(version_));
	std::string ss_xml;
	ssheet_->as_xml_string(ss_xml);
	join_ok_msg->set("Length", boost::lexical_cast<std::string>(ss_xml.length()));
	join_ok_msg->set("xml", ss_xml);
	return join_ok_msg;
}

void ss_session::send_updates(int version, std::string cell, std::string contents, ss_client_ptr initiator)
{
	// Build the update message
	ss_message update;
	update.command = ss_message::UPDATE;
	update.set("Name", ss_name_);
	update.set("Version", boost::lexical_cast<std::string>(version));
	update.set("Cell", cell);
	update.set("Length", boost::lexical_cast<std::string>(contents.length()));
	update.set("content", contents);

	std::set<ss_client_ptr>::iterator it;
	// And send the update message to each attached client
	for(it = clients_.begin(); it != clients_.end(); ++it)
	{
		// Send to all attached clients except the initiator
		if((*it) != initiator)
		{
			(*it)->tell(update);
		}
	}
}

}

