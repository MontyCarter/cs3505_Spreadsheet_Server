/*
 * tcpconnection.h
 *
 *  Created on: Apr 6, 2013
 *      Author: montgomc
 */

#ifndef SS_CLIENT_H_
#define SS_CLIENT_H_

#include <vector>
#include <iostream>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include "ss_message.h"


namespace ss {


class ss_server;

//A tcp connection represents a tcp connection.  It contains a socket
//Inherit enable_shared_from_this, so this object can be treated as
//  a shared pointer.  When no code has a reference to the shared pointer,
//  this tcp_connection is automatically deleted
class ss_client : public boost::enable_shared_from_this<ss_client>, private boost::noncopyable
{
public:
	//A new tcp_connection
	ss_client(boost::asio::io_service& io_service, ss_server& server);

	//Returns a reference to the socket attached to this tcp_connection
	boost::asio::ip::tcp::socket& socket();

	// Start async i/o
	void start();

	// Stop outstanding async i/o
	void stop();

	// Sends a message to the client
	void tell(ss_message msg);

private:
	//Callback from async read
	void handle_read(const boost::system::error_code& e,
			std::size_t bytes_transferred);

	//Callback from async write
	void handle_write(const boost::system::error_code& e);

	//A buffer for incoming data
	boost::array<char, 8192> buffer_;

	//A string to be sent
	std::string to_send_;

	//The asio tcp socket
	boost::asio::ip::tcp::socket socket_;

	// Contains unused information received from socket (not yet part of message)
	std::string unused_;

	// The type of message the parser is currently trying to process
	enum msg_type {
		JOIN,
		CREATE,
		CHANGE,
		SAVE,
		LEAVE,
		UNDO
	} cur_msg_type_;

	// Indicates the type of token we're waiting for
	enum waiting_for {
		command,
		name,
		password,
		version,
		cell,
		length,
		blob
	} waiting_for_;

	// When waiting_for_ is blob, this tells how much to wait for
	unsigned int blob_size_;

	// Determines whether unused_ is a command token
	bool try_as_command();

	// Returns the Token delimiter needed for the current waiting_for_
	//  state
	std::string get_waiting_for_delim();

	// Sets the proper waiting_for_ state to begin searching for the
	//  next message
	void update_waiting_for();

	// A message to build as we parse
	ss_message next_message_;

	// A reference to the server
	ss_server& server_;
};

typedef boost::shared_ptr<ss_client> ss_client_ptr;

}
#endif /* SS_CLIENT_H_ */
