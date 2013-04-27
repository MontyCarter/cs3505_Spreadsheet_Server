/*
 * spreadsheet_manager.h
 *
 *  Created on: Apr 14, 2013
 *      Author: montgomc
 */

#ifndef SPREADSHEET_MANAGER_H_
#define SPREADSHEET_MANAGER_H_

#include <iostream>
#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <boost/lexical_cast.hpp>
#include "ss_message.h"
#include "ss_client.h"
#include "spreadsheet.h"

namespace ss {


// Spreadsheet manager uses an index file to abstract spreadsheet names
//  from files.  The index file is an xml file where there is an "index" root
//  and multiple "ss" nodes.  Each ss node has an attribute that is the
//  spreadsheet filename, and its contents is the spreadsheet name
//  presented to the user.

// A spreadsheet's "name" can be any string (not containing \n).
//  These are stored in files whose names are basically serial numbers
//  (1.ss, 2.ss).  These spreadsheets will store their names, in case
//  the index is damaged or lost

// The manager is responsible for
//    -responding to client create message
//    -responding to ss_session get spreadsheet requests
//    -indexing spreadsheets

class spreadsheet_manager {
public:
	// Creates a spreadsheet manager
	spreadsheet_manager(std::string root_dir, std::string indexFile);

	// Called by dispatch to process a CREATE message
	void handle_create_request(ss_client_ptr requester, ss_message create_request);

	// Returns the requested spreadsheet - ss_session responsible for destroying spreadsheet
	//  Returns null if spreadsheet was not found
	spreadsheet* get_spreadsheet(std::string ss_name);
private:
	// Creates a new spreadsheet file - returns false if not data was written
	std::string* new_spreadsheet(std::string filename, std::string password);

	// Returns the filename of the spreadsheet - null if not found
	std::string* find_spreadsheet(std::string ss_name);

	// The index full file name
	std::string index_fullfile_;

	// The index XML document
	xmlDocPtr index_xml_;

	// The root index node
	xmlNodePtr index_;

	// The xpath search context
	xmlXPathContextPtr xpathCtx_;

	// The root directory
	std::string root_dir_;

	// The next spreadsheet file serial number
	int next_file_id_;

};

class SSFileIOException : public std::exception
{
public:

   SSFileIOException(std::string ss) : s(ss) {}
   ~SSFileIOException() throw() {}
   virtual const char* what() const throw() { return s.c_str(); }
private:
   std::string s;
};

} /* namespace ss */
#endif /* SPREADSHEET_MANAGER_H_ */
