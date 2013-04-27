/*
 * SSServer.cpp
 *
 *  Created on: Apr 6, 2013
 *      Author: montgomc
 */


#include <iostream>
#include <string>
#include "ss_server.h"
#include "spreadsheet.h"
#include "ss_client.h"
#include "spreadsheet_manager.h"
#include "ss_message.h"
#include <boost/thread.hpp>
#include <pthread.h>
#include <signal.h>

bool test_spreadsheet_manager();

//srv = ss::ss_server;

int main(int argc, char **argv)
{

	std::string usage = "Usage: SSServer <port> <root directory> <index file>\n";
	usage += "\tport\t\tTCP port to listen on\n";
	usage += "\troot directory\tThe virtual directory root for spreadsheets\n";
	usage += "\tindex file\tThe file that indexes existing spreadsheets\n";

		int port;
		std::string root_dir;
		std::string index_file;
		//Check args
		if(argc != 4)
		{
			std::cerr << usage;
			return 1;

		}
		else
		{
			//Initialize the server with port number and root directory
			try
			{
				port = boost::lexical_cast<int>(argv[1]);
			}
			catch(boost::bad_lexical_cast& e)
			{
				std::cerr << "Invalid port number\n\n";
				std::cerr << usage;
				return -1;
			}
			root_dir = argv[2];  // TODO enforce root_dir ends with '/'
			if(root_dir[root_dir.length() - 1] != '/')
			{
				std::cerr << "Root directory must end with a '/'\n\n";
				std::cerr << usage;
				return -1;
			}
			index_file = argv[3];
		}

		//Start listening for Ctrl+C
		//signals()

		try
		{
			// Block all signals for background thread.
			sigset_t new_mask;
			sigfillset(&new_mask);
			sigset_t old_mask;
			pthread_sigmask(SIG_BLOCK, &new_mask, &old_mask);

			// Run server in background thread.
			ss::ss_server srv(port, root_dir, index_file);
			//Start the server
			std::cout << "Starting to listen for connections on port " << boost::lexical_cast<std::string>(port) << "...\n";
			std::cout << "Waiting for connections...\n";
			boost::thread srv_thread(boost::bind(&ss::ss_server::run, &srv));

			// Restore previous signals.
			pthread_sigmask(SIG_SETMASK, &old_mask, 0);



			// Wait for signal indicating time to shut down.
			sigset_t wait_mask;
			sigemptyset(&wait_mask);
			sigaddset(&wait_mask, SIGINT);
			sigaddset(&wait_mask, SIGQUIT);
			sigaddset(&wait_mask, SIGTERM);
			pthread_sigmask(SIG_BLOCK, &wait_mask, 0);
			int sig = 0;
			sigwait(&wait_mask, &sig);
			std::cerr << std::endl;

			//Once past the sigwait, the rest of this thread finishes.
			srv.stop();
			srv_thread.join();
		}
		catch(ss::SSFileIOException& e)
		{
			std::cerr << e.what() << std::endl << std::endl;
			std::cerr << usage;
			return -1;
		}


}


bool test_spreadsheet_manager()
{
	using namespace ss;
		ss_client_ptr dummy;
		spreadsheet_manager ssman("./test/", "ss_index.xml");
		ss_message createReq;
		createReq.command = ss_message::CREATE;
		createReq.set("Name", "ANewName");
		createReq.set("Password", "tryingThisPassword");
		ssman.handle_create_request(dummy, createReq);
		//std::cout << result << std::endl;
		spreadsheet* ssptr = ssman.get_spreadsheet(createReq.get_val("Name"));
		spreadsheet ss = *ssptr;
		ss.load();
		std::string asxml;
		ss.as_xml_string(asxml);
		std::cout << asxml << std::endl;
		std::string test;
		std::cin >> test;

		//ss.set_cell_contents("a2","with");
		//ss.set_cell_contents("a3", "new");
		ss.set_cell_contents("a4", "words");
		ss.save();
		ss.load();
		ss.set_cell_contents("a4", "even here");
		ss.set_version("woohoo");
		std::cout << ss.get_version() << std::endl;
		ss.set_cell_contents("a5", "new5");

		return true;

}
