/*
 * tcp_connection.cpp
 *
 *  Created on: Apr 13, 2013
 *      Author: montgomc
 */


#include "ss_client.h"
#include "ss_server.h"


namespace ss {

ss_client::ss_client(boost::asio::io_service& io_service, ss_server& server)
	: socket_(io_service),
	  server_(server)
{
	waiting_for_ = command;
	blob_size_ = 0;
}

boost::asio::ip::tcp::socket& ss_client::socket()
{
	return socket_;
}

void ss_client::start()
{
	//Start waiting for data
	socket_.async_read_some(boost::asio::buffer(buffer_),
			boost::bind(&ss_client::handle_read, shared_from_this(),
					boost::asio::placeholders::error,
					boost::asio::placeholders::bytes_transferred));
}

void ss_client::stop()
{
	socket_.close();
}

void ss_client::tell(ss_message msg)
{
	// Add the command
	to_send_ = msg.get_command_str() + "\n";
	// For each item in params, if it starts with an uppercase letter, it
	//  must have a header (e.g., "Name:").  The param keys are the same
	//  as the required header.  We'll simply append a ":" to these as
	//  they are inserted in to the response string

	for(unsigned int x = 0; x < msg.params.size(); x++)
	{
		int a = (int)'A';
		int z = (int)'Z';
		int cur = (int)msg.params[x].first[0];
		if(a <= cur && cur <= z)
		{
			// This param requires a header.  Append a colon, then the data
			//  and finally a \n
			to_send_ += msg.params[x].first + ':' + msg.params[x].second + '\n';
		}
		else
		{
			// If this param key does not start with an uppercase letter, it is
			//  a blob.  Append it, plus a \n, to the string without header
			to_send_ += msg.params[x].second + '\n';
		}
	}
	// Now that the output is ready, send it to the socket.  (send in chunks of 8192)
	//std::vector<boost::asio::const_buffer> to_send_buffs;
	//unsigned int pos = 0;
	//while(pos <= to_send_.length())
	//{
	//to_send_buffs.push_back(boost::asio::buffer(to_send_.substr(pos, 8192)));
//		pos += 8192;
	//}
	boost::asio::async_write(socket_, boost::asio::buffer(to_send_), boost::bind(&ss_client::handle_write,
			shared_from_this(), boost::asio::placeholders::error));
	//socket_.send(to_send);
}

void ss_client::handle_read(const boost::system::error_code& e,
		std::size_t bytes_transferred)
{
	if(!e)
	{
		// Grab a pointer to our buffer that we can manipulate
		char* bufPtr = buffer_.begin();
		// Loop through the data we received in the buffer, and add it to unused_
		while(bufPtr != buffer_.begin() + bytes_transferred)
		{
			char curChar = *bufPtr;
			if(curChar == '\r' && *(bufPtr + 1) == '\n')
			{
				//Skip it - coming from telnet
				bufPtr++;
			}
			// If we're not waiting for a blob, we're only interested in unused_
			//   if the current char is \n
			else if(waiting_for_ != blob && curChar == '\n')
			{
				// If the current line is a command, we want to reset the state,
				//  and start processing the new command
				//
				// Try the current line as a header - if it is, this method
				//  will set the appropriate cur_msg_type_ and waiting_for_ states
				//  and clear unused_ to wait for the next token.
				bool is_command = try_as_command();
				if(is_command)
				{
					// Nothing to do here - cur_msg_type_ and waiting_for_ have
					//  already been appropriately set.  Continue to process next
					//  character in buffer...
					bufPtr++;

				}
				// If we're waiting for a command, and the current line is not a command,
				//  drop unused_
				else if(waiting_for_ == command)
				{
					next_message_.command = ss_message::ERROR;
					tell(next_message_);
					ss_message newNext;
					std::swap(newNext, next_message_);
					unused_ = "";
					bufPtr++;
				}
				// If here, we're waiting for a message token - lets see what we find
				else
				{
					std::string wait_delim = get_waiting_for_delim();
					// See if the beginning of unused_ matches the delimiter
					//  Check first to make sure unused_ is not shorter than delim
					if(unused_.length() < wait_delim.length())
					{
						// Current line is to short - sending ERROR and
						//reset waiting_for_ state to waiting for command
						next_message_.command = ss_message::ERROR;
						tell(next_message_);
						ss_message newNext;
						std::swap(newNext, next_message_);
						waiting_for_ = command;

					}
					else
					{
						// We have sufficient length to test
						// See if the current line is what we're waiting for
						if(unused_.compare(0, wait_delim.length(), wait_delim) == 0)
						{
							// Here, the line begins with the delim.
							// Grab the content from the end of the delim to the \n
							std::string value = unused_.substr(wait_delim.length(), unused_.length() - 1);
							// Add the delim and value to the next_message_ as a param
							//  (trim the ":" from the delim before using as the key in params)
							next_message_.set(wait_delim.substr(0,wait_delim.length() - 1), value);
							// Finally, set the waiting_for_ state to look for the next required token.
							update_waiting_for();
							// And empty unused_
							unused_ = "";
							bufPtr++;
						}
						else
						{
							// If we didn't find the token we are expecting, send ERROR,
							//abandon the message, and begin to wait for a command, again
							next_message_.command = ss_message::ERROR;
							tell(next_message_);
							ss_message newNext;
							std::swap(newNext, next_message_);
							waiting_for_ = command;
							unused_ = "";
							bufPtr++;
							ss_message new_next;
							std::swap(next_message_, new_next);
						}
					}
				}
			}
			else if(curChar == '\n' || (waiting_for_ == blob && blob_size_ == unused_.length()))
			{
				// If here, we have a new line, but we are waiting for a blob
				//  check to see if the unused_ is the same length as blob_size_
				// If it is the same size, then put the blob (the only blob we receive on the server is
				//  a cell "content") in the next_message_
				if(unused_.length() == blob_size_)
				{
					next_message_.set("content", unused_);
					unused_ = "";
					update_waiting_for();
					bufPtr++;
				}
				else
				{
					// If not, wait for more...
					//(but keep the \n)
					unused_.push_back(*bufPtr++);
				}
			}
			else
			{
				// If here, the current character is not a \n.  Add the current char to unused, and continue
				unused_.push_back(*bufPtr++);
			}
		} /* End while more buffer data */
	}
	else
	{
		//Here, we got an error on the socket
		server_.remove_client(shared_from_this());
	}
	// Listen for more
	start();
}

void ss_client::handle_write(const boost::system::error_code& e)
{
	if(!e)
	{
		//Make sure we don't have any pending data to send

	}
	else
	{
		server_.remove_client(shared_from_this());
	}
}

bool ss_client::try_as_command()
{
	// All headers wait first for Name: token
	if(unused_ == "CREATE" || unused_ == "CREATE\r")
	{
		// Indicate we're now trying to parse a CREATE message,
		// we're waiting for a Name:, and clear unused_
		cur_msg_type_ = CREATE;
		waiting_for_ = name;
		// Reset the next_message_  and set the command appropriately
		ss_message new_next;
		std::swap(next_message_, new_next);
		next_message_.command = ss_message::CREATE;
		unused_ = "";
		return true;
	}
	else if(unused_ == "JOIN" || unused_ == "JOIN\r")
	{
		cur_msg_type_ = JOIN;
		waiting_for_ = name;
		ss_message new_next;
		std::swap(next_message_, new_next);
		next_message_.command = ss_message::JOIN;
		unused_ = "";
		return true;
	}
	else if(unused_ == "UNDO" || unused_ == "UNDO\r")
	{
		cur_msg_type_ = UNDO;
		waiting_for_ = name;
		ss_message new_next;
		std::swap(next_message_, new_next);
		next_message_.command = ss_message::UNDO;
		unused_ = "";
		return true;
	}
	else if(unused_ == "SAVE" || unused_ == "SAVE\r")
	{
		cur_msg_type_ = SAVE;
		waiting_for_ = name;
		ss_message new_next;
		std::swap(next_message_, new_next);
		next_message_.command = ss_message::SAVE;
		unused_ = "";
		return true;
	}
	else if(unused_ == "CHANGE" || unused_ == "CHANGE\r")
	{
		cur_msg_type_ = CHANGE;
		waiting_for_ = name;
		ss_message new_next;
		std::swap(next_message_, new_next);
		next_message_.command = ss_message::CHANGE;
		unused_ = "";
		return true;
	}
	else if(unused_ == "LEAVE" || unused_ == "LEAVE\r")
	{
		cur_msg_type_ = LEAVE;
		waiting_for_ = name;
		ss_message new_next;
		std::swap(next_message_, new_next);
		next_message_.command = ss_message::LEAVE;
		unused_ = "";
		return true;
	}
	return false;
}

std::string ss_client::get_waiting_for_delim()
{
	switch(waiting_for_)
	{
	case command:
		return "";
	case name:
		return "Name:";
	case password:
		return "Password:";
	case version:
		return "Version:";
	case cell:
		return "Cell:";
	case length:
		return "Length:";
	case blob:
		//Not really important, wont be used
		return "blob";
	}
	return "";
}

void ss_client::update_waiting_for()
{
	// Only called if current token was successful.

	ss_message new_next;
	switch(cur_msg_type_)
	{
	// CREATE and JOIN have the same format
	case CREATE:
	case JOIN:
		switch(waiting_for_)
		{
		case name:
			waiting_for_ = password;
			break;
		case password:
			// This is the end of this message format.
			// Send the message to dispatch, empty it, then reset waiting_for_
			server_.dispatch_request(shared_from_this(), next_message_);
			std::swap(next_message_, new_next);
			waiting_for_ = command;
			break;
		default:
			//We should never get here, but not resetting here will never let
			//  the state out of the broken state
			waiting_for_ = command;
			break;
		}
		break;
	// LEAVE and SAVE have the same format
	case SAVE:
	case LEAVE:
		// The only thing we need is name, which we already got - reset
		server_.dispatch_request(shared_from_this(), next_message_);
		std::swap(next_message_, new_next);
		waiting_for_ = command;
		break;
	case UNDO:
		switch(waiting_for_)
		{
		case name:
			waiting_for_ = version;
			break;
		case version:
			server_.dispatch_request(shared_from_this(), next_message_);
			std::swap(next_message_, new_next);
			waiting_for_ = command;
			break;
		default:
			//We should never get here, but not resetting here will never let
			//  the state out of the broken state
			waiting_for_ = command;
			break;
		}
		break;
	case CHANGE:
		switch(waiting_for_)
		{
		case name:
			waiting_for_ = version;
			break;
		case version:
			waiting_for_ = cell;
			break;
		case cell:
			waiting_for_ = length;
			break;
		case length:
			// We need to put the length of the CHANGE Content: token in
			//  blob_size_, then wait for content
			blob_size_ = boost::lexical_cast<int>(next_message_.get_val("Length"));
			waiting_for_ = blob;
			break;
		case blob:
			server_.dispatch_request(shared_from_this(), next_message_);
			std::swap(next_message_, new_next);
			waiting_for_ = command;
			break;
		default:
			//We should never get here, but not resetting here will never let
			//  the state out of the broken state
			waiting_for_ = command;
			break;
		}
		break;
	}
}

}
