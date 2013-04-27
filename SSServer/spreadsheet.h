/*
 * Spreadsheet.h
 *
 *  Created on: Apr 13, 2013
 *      Author: montgomc
 */

#ifndef SPREADSHEET_H_
#define SPREADSHEET_H_

#include <string>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

namespace ss {

// This is the in-memory model of a spreadsheet.
// One spreadsheet object should exist per ss_session

// Spreadsheets will be stored as:
//   <server_ss>
//      <ssName>TheEndUserName</ssName>
//      <password>PASS101</password>
//      <spreadsheet version="1">
//         <cell>
//            <name>A1</name>
//            <contents>=A2+3</contents>
//         </cell>
//      </spreadsheet>
//   </server_ss>
//
// In other words, a slight modification has been made to the format,
//   so that password is included at a higher level.  The as_xml_string
//   method should only return the <spreadsheet> node.

class spreadsheet {
public:
	// Creates a spreadsheet object
	spreadsheet(std::string filename, std::string root_dir);

	// Destroy the spreadsheet
	~spreadsheet();

	// Loads a spreadsheet from disk
	void load();

	// Saves a spreadsheet to disk
	void save();

	// Returns the password listed in the spreadsheet file - returns null if ss not "load()"ed
	std::string get_password();

	// Returns the spreadsheet state in simplified xml
	//   (no comments, metadata, etc)
	void as_xml_string(std::string& xml_out);

	// Sets the contents of a cell - ss_session is in charge of version
	void set_cell_contents(std::string cell, std::string contents);

	// Gets the contents of a cell - returns empty string if cell not defined
	std::string get_cell_contents(std::string cell);

	// Sets the version attribute on the spreadsheet node
	void set_version(std::string new_ver);

	// Gets the version attribute on the spreadsheet node
	std::string get_version();

private:
	// The full file path of the
	std::string full_filename_;

	// The password
	std::string password_;

	// Spreadsheet contents are stored in XML format, in memory
	xmlDocPtr ss_;

	//xPath context for searching
	xmlXPathContextPtr xpathCtx_;
};
}
#endif /* SPREADSHEET_H_ */
