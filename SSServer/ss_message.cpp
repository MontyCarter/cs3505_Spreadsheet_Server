/*
 * ss_message.cpp
 *
 *  Created on: Apr 20, 2013
 *      Author: montgomc
 */

#include "ss_message.h"

namespace ss {

ss_message::ss_message()
{

}

ss_message::ss_message(ss_message& orig)
{
	this->params = orig.params;
	this->command = orig.command;
}

void ss_message::set(std::string key, std::string val)
{
	int idx = find(key);
	if(idx == -1)
	{
		//It doesn't exist.  Append it
		params.push_back(std::make_pair<std::string,std::string>(key,val));
	}
	else
	{
		//It does exist - update val
		params[idx].second = val;
	}
}

int ss_message::find(std::string key)
{
	for(unsigned int x = 0; x < params.size(); x++)
	{
		if(params[x].first == key)
		{
			return x;
		}
	}
	return -1;
}

std::string ss_message::get_val(std::string key)
{
	int idx = find(key);
	if(idx == -1)
	{
		return "";
	}
	else
	{
		return params[idx].second;
	}
}


std::string ss_message::get_command_str()
{
	switch(command)
	{
	case CREATE:
		return "CREATE";
	case CREATE_OK:
		return "CREATE OK";
	case CREATE_FAIL:
		return "CREATE FAIL";
	case JOIN:
		return "JOIN";
	case JOIN_OK:
		return "JOIN OK";
	case JOIN_FAIL:
		return "JOIN FAIL";
	case CHANGE:
		return "CHANGE";
	case CHANGE_OK:
		return "CHANGE OK";
	case CHANGE_WAIT:
		return "CHANGE WAIT";
	case CHANGE_FAIL:
		return "CHANGE FAIL";
	case UNDO:
		return "UNDO";
	case UNDO_OK:
		return "UNDO OK";
	case UNDO_END:
		return "UNDO END";
	case UNDO_WAIT:
		return "UNDO WAIT";
	case UNDO_FAIL:
		return "UNDO FAIL";
	case UPDATE:
		return "UPDATE";
	case SAVE:
		return "SAVE";
	case SAVE_OK:
		return "SAVE OK";
	case SAVE_FAIL:
		return "SAVE FAIL";
	case LEAVE:
		return "LEAVE";
	case ERROR:
		return "ERROR";
	}
	throw new std::exception();
}

}


