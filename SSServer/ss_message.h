/*
 * ss_message.h
 *
 *  Created on: Apr 14, 2013
 *      Author: montgomc
 */

#ifndef SS_MESSAGE_H_
#define SS_MESSAGE_H_

#include <vector>
#include <string>

// As socket data is parsed, an ss_message is filled.
// Once a message is complete, it gets sent to the
//   server's processRequest method
namespace ss {

// An ordered list of parameters and values from the message
typedef std::pair<std::string,std::string> kvp;

class ss_message {
public:
	ss_message();
	ss_message(ss_message& orig);

	// The first line in a protocol message
	enum _command
	{
		CREATE,
		CREATE_OK,
		CREATE_FAIL,
		JOIN,
		JOIN_OK,
		JOIN_FAIL,
		CHANGE,
		CHANGE_OK,
		CHANGE_WAIT,
		CHANGE_FAIL,
		UNDO,
		UNDO_OK,
		UNDO_END,
		UNDO_WAIT,
		UNDO_FAIL,
		UPDATE,
		SAVE,
		SAVE_OK,
		SAVE_FAIL,
		LEAVE,
		ERROR
	} command;

	// Holds the params
	std::vector<kvp> params;

	void set(std::string key, std::string val);

	std::string get_val(std::string key);

	std::string get_command_str();

private:
	int find(std::string key);

};

//std::string ss_message::get_command_str(ss_message::_command command)


}

#endif /* SS_MESSAGE_H_ */

