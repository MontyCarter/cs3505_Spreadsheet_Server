/*
 * Spreadsheet.cpp
 *
 *  Created on: Apr 13, 2013
 *      Author: montgomc
 */

#include "spreadsheet.h"
#include <string>
#include <boost/algorithm/string.hpp>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/xmlIO.h>
#include <libxml/xinclude.h>
#include <libxml/xmlschemastypes.h>
#include <iostream>
#include <cstdio>

namespace ss {

spreadsheet::spreadsheet(std::string filename, std::string root_dir)
	: full_filename_(root_dir + filename)
{
}

spreadsheet::~spreadsheet()
{
	xmlFreeDoc(ss_);
}

// Throws saving errors - to be caught by ss_session, to know to send
//  SAVE FAIL
void spreadsheet::save()
{
	// We are letting errors pass up the stack, to ss_session
	const char* file = full_filename_.c_str();
	const char* enc = "UTF-8";
	int res = xmlSaveFormatFileEnc(file, ss_, enc, 1);

	if(res == -1)
	{
		// TODO figure out how to pass a message
		throw new std::exception();
	}
}

// Throws loading errors - to be caught by ss_session, to know to send
//   JOIN FAIL
void spreadsheet::load()
{
	// We are letting exceptions pass up the stack, to ss_session

	// Load the xml from disk (c library xml2 requires c strings)
	ss_ = xmlReadFile(full_filename_.c_str(), NULL, 0);

	if(ss_ == NULL)
		throw new std::exception();

	// Fill the xPath context
	xpathCtx_ = xmlXPathNewContext(ss_);

	// Grab the password node

	xmlXPathObjectPtr xp_pass = xmlXPathEvalExpression((const xmlChar*)"/server_ss/password", xpathCtx_);
	xmlNodePtr n_pass = xp_pass->nodesetval->nodeTab[0];

	// Grab the password
    password_ = (char*)(xmlNodeGetContent(n_pass));

}


void spreadsheet::set_cell_contents(std::string cell, std::string contents)
{
	// Set up a query and search for the cell.
	std::string curCell = boost::to_upper_copy(cell);
	std::string query =  "/server_ss/spreadsheet/cell[name='" + curCell + "']/contents";
	xmlXPathObjectPtr xp_cells = xmlXPathEvalExpression((const xmlChar*)(query.c_str()), xpathCtx_);


	//Make sure we got something
	if(xp_cells != NULL && (xp_cells->nodesetval))
	{
		xmlNodePtr cellContents = xp_cells->nodesetval->nodeTab[0];
		xmlNodeSetContent(cellContents, (const xmlChar*)(contents.c_str()));
	}
	else
	{
		// If here, means the cell was not yet defined
		// Get the spreadsheet node, and create a new Cell with the requested params
		xmlXPathObjectPtr xp_ss = xmlXPathEvalExpression((const xmlChar*)"/server_ss/spreadsheet", xpathCtx_);
		if(xp_ss->nodesetval->nodeNr != 0)
		{
			// Get the spreadsheet node
			xmlNodePtr ss_root = xp_ss->nodesetval->nodeTab[0];
			// Create the new cell
			xmlNodePtr newCell = xmlNewChild(ss_root, NULL, (const xmlChar*)"cell", NULL);
			// And fill it with name and contents
			xmlNewChild(newCell, NULL, (const xmlChar*)"name", (const xmlChar*)(curCell.c_str()));
			xmlNewChild(newCell, NULL, (const xmlChar*)"contents", (const xmlChar*)(contents.c_str()));
		}
		else
		{
			std::cerr << "No spreadsheet node in spreadsheet " + full_filename_ + " XML tree" << std::endl;
			throw new std::exception();
		}
	}
}

std::string spreadsheet::get_cell_contents(std::string cell)
{
	// Set up a query and search for the cell.
	std::string curCell = boost::to_upper_copy(cell);
	std::string query =  "/server_ss/spreadsheet/cell[name='" + curCell + "']/contents";
	xmlXPathObjectPtr xp_cells = xmlXPathEvalExpression((const xmlChar*)(query.c_str()), xpathCtx_);


	//Make sure we got something
	if(xp_cells != NULL && (xp_cells->nodesetval))
	{
		xmlNodePtr cellContents = xp_cells->nodesetval->nodeTab[0];
		return (char*)(xmlNodeGetContent(cellContents));
	}
	else
	{
		// The cell is not defined - return empty string
		return "";
	}
}

void spreadsheet::set_version(std::string new_ver)
{
	std::string query =  "/server_ss/spreadsheet";
	xmlXPathObjectPtr xp_ss = xmlXPathEvalExpression((const xmlChar*)(query.c_str()), xpathCtx_);

	xmlNodePtr ss_node = xp_ss->nodesetval->nodeTab[0];
	xmlSetProp(ss_node, (const xmlChar*)"version", (const xmlChar*)(new_ver.c_str()));
}

std::string spreadsheet::get_version()
{
	std::string query =  "/server_ss/spreadsheet";
	xmlXPathObjectPtr xp_ss = xmlXPathEvalExpression((const xmlChar*)(query.c_str()), xpathCtx_);

	xmlNodePtr ss_node = xp_ss->nodesetval->nodeTab[0];
	return (const char*)(xmlGetProp(ss_node, (const xmlChar*)"version"));
}

std::string spreadsheet::get_password()
{
	return password_;
}

void spreadsheet::as_xml_string(std::string& xml_out)
{
	//Grab the spreadsheet node
	std::string query =  "/server_ss/spreadsheet";
	xmlXPathObjectPtr xp_ss = xmlXPathEvalExpression((const xmlChar*)(query.c_str()), xpathCtx_);

	xmlNodePtr ss_node = xp_ss->nodesetval->nodeTab[0];
	xmlChar *s;
	int size;
	xmlNodePtr ss_node_copy = xmlCopyNode(ss_node, 1);
	//Create a new doc, with the spreadsheet node at the root
	xmlDocPtr ss_doc = xmlNewDoc((const xmlChar*)"1.0");
	xmlDocSetRootElement(ss_doc, ss_node_copy);
	//Dump the spreadsheet node into the xmlChar*
	xmlDocDumpFormatMemoryEnc(ss_doc, &s, &size, (const char*)"UTF-8", 0);
	//And get rid of whitespace so it fits on a single line
	xml_out = (char *)(xmlSchemaCollapseString(s));
	xmlFree(s);
}

}

