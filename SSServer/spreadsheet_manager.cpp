/*
 * spreadsheet_manager.cpp
 *
 *  Created on: Apr 14, 2013
 *      Author: montgomc
 */

#include "spreadsheet_manager.h"

namespace ss {

spreadsheet_manager::spreadsheet_manager(std::string root_dir, std::string indexFile)
	: index_fullfile_(root_dir + indexFile),
	  root_dir_(root_dir),
	  next_file_id_(0)
{
	// Load the index file
	// Load the xml from disk (c library xml2 requires c strings)
	try
	{
		std::cout << "Root directory is: " << root_dir_ << std::endl;
		std::cout << "Index file is: " << indexFile << std::endl;
		std::cout << "Loading the spreadsheet index file...\n";
		index_xml_ = xmlReadFile(index_fullfile_.c_str(), NULL, 0);  //If no fileives "I/O Warning: failed to load external entity"
	}
	catch(...)
	{
		//Do nothing (just to suppress stdout output)
	}

	// We didn't get anything - let's create one
	if(index_xml_ == NULL)
	{
		// Create new document root
		index_xml_ = xmlNewDoc((const xmlChar*)"1.0");
		// Create a new node (will be the root node, called "index")
		index_ = xmlNewNode(NULL, (const xmlChar*)"index");
		// Set the new node as the root element of the doc
		xmlDocSetRootElement(index_xml_, index_);
		// Add a nextID attribute - this is the next spreadsheet serial number
		xmlNewProp(index_, (const xmlChar*)"nextID", (const xmlChar*)"1");
		// We now have an initialized index
		int res;
		res = xmlSaveFormatFileEnc((const char*)(index_fullfile_.c_str()), index_xml_, (const char*)"UTF-8", 1);
		if(res == -1)
		{
			throw SSFileIOException("SSServer Error: Root directory could not be found\n");//TODO how to throw up
		}
		else
		{
			std::cerr << "Info:\tSpecified spreadsheet index file not found - creating the requested file\n";
		}

	}

	std::cout << "Index file successfully loaded...\n";

	// Fill the xPath context
	xpathCtx_ = xmlXPathNewContext(index_xml_);

	// Grab the index node
	xmlXPathObjectPtr xp_index = xmlXPathEvalExpression((const xmlChar*)"/index", xpathCtx_);
	index_ = xp_index->nodesetval->nodeTab[0];

	// Grab the current version
    std::string strNextId = (char*)(xmlGetProp(index_, (const xmlChar*)"nextID"));
    next_file_id_ = boost::lexical_cast<int>(strNextId);
}

// Only valid create messages should be sent to this method
void spreadsheet_manager::handle_create_request(ss_client_ptr requester, ss_message create_request)
{
	if(create_request.command != ss_message::CREATE)
	{
		throw new std::exception();  // TODO put message in exception
	}
	std::string ss_name = create_request.get_val("Name");
	std::string password = create_request.get_val("Password");

	std::string* res = new_spreadsheet(ss_name, password);
	ss_message response;
	response.set("Name",ss_name);
	// NULL means sucess - non-null is reason why failed
	if(res == NULL)
	{
		response.command = ss_message::CREATE_OK;
		response.set("Password",password);
		requester->tell(response);
	}
	else
	{
		response.command = ss_message::CREATE_FAIL;
		response.set("message", *res);
		requester->tell(response);
		delete res;
	}
}

spreadsheet* spreadsheet_manager::get_spreadsheet(std::string ss_name)
{
	// Find the spreadsheet
	std::string* ss_file_ptr = find_spreadsheet(ss_name);
	if(ss_file_ptr == NULL)
	{
		return NULL;
	}

	spreadsheet* ss;

	try
	{
		std::string ss_file = *ss_file_ptr;
		delete ss_file_ptr;
		ss = new spreadsheet(ss_file, root_dir_);
	}
	catch(std::exception& e)
	{
		return NULL;
	}
	return ss;
}

// Returns the underlying filename in the root_dir, or null if not found
std::string* spreadsheet_manager::find_spreadsheet(std::string ss_name)
{
	// Check if the spreadsheet already exists
	std::string query =  "/index//*[contains(text(), '" + ss_name + "')]";
	xmlXPathObjectPtr xp_ss = xmlXPathEvalExpression((const xmlChar*)(query.c_str()), xpathCtx_);


	//Make sure we got something
	if(xp_ss != NULL && (xp_ss->nodesetval->nodeNr != 0))
	{
		xmlNodePtr ss = xp_ss->nodesetval->nodeTab[0];
		return new std::string((const char*)(xmlGetProp(ss, (const xmlChar*)"filename")));
	}
	else
	{
		return NULL;
	}
}

// This method is the approver/denier of CREATE requests.  Returns null for success.  Returns
//  a string with a reason for failure.
// LOCK BEFORE CALLING
std::string* spreadsheet_manager::new_spreadsheet(std::string ss_name, std::string password)
{
	// Check to see if the spreadsheet already exists
	std::string* res = (find_spreadsheet(ss_name));
	if(res != NULL)
	{
		delete res;
		return new std::string("The spreadsheet already exists");
	}
	delete res;
	// Create new document root
	xmlDocPtr server_ss_xml = xmlNewDoc((const xmlChar*)"1.0");
	// Create a new node (will be the root node, called "server_ss")
	xmlNodePtr server_ss_root = xmlNewNode(NULL, (const xmlChar*)"server_ss");
	// Set the new node as the root element of the doc
	xmlDocSetRootElement(server_ss_xml, server_ss_root);
	// Add a ss_name child node to root node, with the ss_name
	xmlNewChild(server_ss_root, NULL, (const xmlChar*)"ssName", (const xmlChar*)(ss_name.c_str()));
	// Add a password child node to root node, with content=password
	xmlNewChild(server_ss_root, NULL, (const xmlChar*)"password", (const xmlChar*)(password.c_str()));
	// Add the Spreadsheet node (this subnode contains the ss xml format specified in cs3500)
	xmlNewChild(server_ss_root, NULL, (const xmlChar*)"spreadsheet", NULL);
	// Set the ss version to 0 (separate from ss_session version...)
	//xmlNewProp(ss_root, (const xmlChar*)"version", (const xmlChar*)"0");


	// We now have an empty spreadsheet.
	std::string filename = boost::lexical_cast<std::string>(next_file_id_) + ".ss";
	int saveRes = xmlSaveFormatFileEnc((const char*)((root_dir_ + filename).c_str()), server_ss_xml, (const char*)"UTF-8", 1);
	if(saveRes != -1)
	{
		// The save was a success.  Write the info about the new spreadsheet to the index, and
		//  increment the next counters
		xmlNodePtr new_ss = xmlNewChild(index_, NULL, (const xmlChar*)"ss", (const xmlChar*)(ss_name.c_str()));
		xmlNewProp(new_ss, (const xmlChar*)"filename", (const xmlChar*)(filename.c_str()));
		next_file_id_++;
		std::string tmpId = boost::lexical_cast<std::string>(next_file_id_);
		xmlSetProp(index_, (const xmlChar*)"nextID", (const xmlChar*)(tmpId.c_str()));
		xmlSaveFormatFileEnc((const char*)((index_fullfile_).c_str()), index_xml_, (const char*)"UTF-8", 1);
		return NULL;
	}
	else
	{
		return new std::string("Could not write the file");
	}
}


} /* namespace ss */
