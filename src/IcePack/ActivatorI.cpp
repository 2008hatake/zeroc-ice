// **********************************************************************
//
// Copyright (c) 2003
// ZeroC, Inc.
// Billerica, MA, USA
//
// All Rights Reserved.
//
// Ice is free software; you can redistribute it and/or modify it under
// the terms of the GNU General Public License version 2 as published by
// the Free Software Foundation.
//
// **********************************************************************

#ifdef _WIN32
#   error Sorry, the IcePack Activator is not yet supported on WIN32.
#endif

#include <Ice/Ice.h>
#include <IcePack/ActivatorI.h>
#include <IcePack/Admin.h>
#include <IcePack/Internal.h>
#include <IcePack/TraceLevels.h>

#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>

using namespace std;
using namespace Ice;
using namespace IcePack;

namespace IcePack
{

class TerminationListenerThread : public IceUtil::Thread
{
public:

    TerminationListenerThread(ActivatorI& activator) :
	_activator(activator)
    {
    }

    virtual 
    void run()
    {
	_activator.runTerminationListener();
    }

private:
    
    ActivatorI& _activator;
};

}

IcePack::ActivatorI::ActivatorI(const TraceLevelsPtr& traceLevels, const PropertiesPtr& properties) :
    _traceLevels(traceLevels),
    _properties(properties),
    _deactivating(false)
{
    int fds[2];
    if(pipe(fds) != 0)
    {
	SyscallException ex(__FILE__, __LINE__);
	ex.error = getSystemErrno();
	throw ex;
    }
    _fdIntrRead = fds[0];
    _fdIntrWrite = fds[1];
    int flags = fcntl(_fdIntrRead, F_GETFL);
    flags |= O_NONBLOCK;
    fcntl(_fdIntrRead, F_SETFL, flags);

    //
    // Parse the properties override property.
    //
    string props = _properties->getProperty("IcePack.Node.PropertiesOverride");
    if(!props.empty())
    {
	string::size_type end = 0;
	while(end != string::npos)
	{
	    const string delim = " \t\r\n";
		
	    string::size_type beg = props.find_first_not_of(delim, end);
	    if(beg == string::npos)
	    {
		break;
	    }
		
	    end = props.find_first_of(delim, beg);
	    string arg;
	    if(end == string::npos)
	    {
		arg = props.substr(beg);
	    }
	    else
	    {
		arg = props.substr(beg, end - beg);
	    }
	    if(arg.find("--") != 0)
	    {
		arg = "--" + arg;
	    }
	    _propertiesOverride.push_back(arg);
	}
    }    
}

IcePack::ActivatorI::~ActivatorI()
{
    assert(!_thread);
    
    close(_fdIntrRead);
    close(_fdIntrWrite);
}

bool
IcePack::ActivatorI::activate(const ServerPtr& server)
{
    IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);

    if(_deactivating)
    {
	return false;
    }

    string path = server->description.path;
    if(path.empty())
    {
	return false;
    }

    //
    // Normalize the pathname a bit.
    //
    {
	string::size_type pos;
	while((pos = path.find("//")) != string::npos)
	{
	    path.erase(pos, 1);
	}
	while((pos = path.find("/./")) != string::npos)
	{
	    path.erase(pos, 2);
	}
    }

    //
    // Normalize the path to the working directory.
    //
    string pwd = server->description.pwd;
    if(!pwd.empty())
    {
	string::size_type pos;
	while((pos = pwd.find("//")) != string::npos)
	{
	    pwd.erase(pos, 1);
	}
	while((pos = pwd.find("/./")) != string::npos)
	{
	    pwd.erase(pos, 2);
	}
    }

    //
    // Setup arguments.
    //
    StringSeq args;
    args.push_back(path);
    args.insert(args.end(), server->description.args.begin(), server->description.args.end());
    args.insert(args.end(), _propertiesOverride.begin(), _propertiesOverride.end());
    args.push_back("--Ice.Default.Locator=" + _properties->getProperty("Ice.Default.Locator"));

    if(_traceLevels->activator > 1)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->activatorCat);
	out << "activating server `" << server->description.name << "'";
	if(_traceLevels->activator > 2)
	{
	    out << "\n";
	    out << "path = " << path << "\n";
	    out << "pwd = " << pwd << "\n";
	    out << "args = ";

	    StringSeq::const_iterator p = args.begin();
            ++p;
	    for(StringSeq::const_iterator q = p; q != args.end(); ++q)
	    {
		out << " " << *q;
	    }
	}
    }

    //
    // Activate and create.
    //
    int fds[2];
    if(pipe(fds) != 0)
    {
	SyscallException ex(__FILE__, __LINE__);
	ex.error = getSystemErrno();
	throw ex;
    }
    pid_t pid = fork();
    if(pid == -1)
    {
	SyscallException ex(__FILE__, __LINE__);
	ex.error = getSystemErrno();
	throw ex;
    }
    if(pid == 0) // Child process.
    {

#ifdef __linux
	//
	// Create a process group for this child, to be able to send 
	// a signal to all the thread-processes with killpg
	//
	setpgrp();
#endif
	//
	// Close all file descriptors, except for standard input,
	// standard output, standard error output, and the write side
	// of the newly created pipe.
	//
	int maxFd = static_cast<int>(sysconf(_SC_OPEN_MAX));
	for(int fd = 3; fd < maxFd; ++fd)
	{
	    if(fd != fds[1])
	    {
		close(fd);
	    }
	}

	//
	// Convert to standard argc/argv.
	//
	int argc = static_cast<int>(args.size());
	char** argv = static_cast<char**>(malloc((argc + 1) * sizeof(char*)));
	int i = 0;
	for(StringSeq::const_iterator p = args.begin(); p != args.end(); ++p, ++i)
	{
	    assert(i < argc);
	    argv[i] = strdup(p->c_str());
	}
	assert(i == argc);
	argv[argc] = 0;

	for(StringSeq::const_iterator q = server->description.envs.begin(); q != server->description.envs.end(); ++q)
	{
	    if(putenv(strdup(q->c_str())) != 0)
	    {
		SyscallException ex(__FILE__, __LINE__);
		ex.error = getSystemErrno();
		ostringstream s;
		s << "can't set environment variable: " << *q << "':\n" << ex;
		write(fds[1], s.str().c_str(), s.str().length());
		close(fds[1]);
		exit(EXIT_FAILURE);
	    }
	}

	//
	// Redirect the standard error output to the write side of the
	// pipe.
	//
	// TODO: This doesn't work well if the server doesn't control
	// when and how the output is flushed. For example, we could
	// receive one character after the other over the pipe which
	// is problematic since we don't know when to log message. For
	// sure we don't want to log each character one after the
	// other. We want to log the message. One solution would be to
	// put the received information in a buffer and log it only
	// when we receive a '\n'.
	//

// 	if(fds[1] != STDERR_FILENO)
// 	{
// 	    if(dup2(fds[1], STDERR_FILENO) != STDERR_FILENO)
// 	    {
// 		//
// 		// Send any errors to the parent process, using the write
// 		// end of the pipe.
// 		//
// 		SyscallException ex(__FILE__, __LINE__);
// 		ex.error = getSystemErrno();
// 		ostringstream s;
// 		s << "can't redirect stderr to the pipe output";
// 		write(fds[1], s.str().c_str(), s.str().length());
// 		close(fds[1]);
// 		exit(EXIT_FAILURE);
// 	    }
// 	}

	//
	// Change working directory.
	//
	if(!pwd.empty())
	{
	    if(chdir(pwd.c_str()) == -1)
	    {
		//
		// Send any errors to the parent process, using the write
		// end of the pipe.
		//
		SyscallException ex(__FILE__, __LINE__);
		ex.error = getSystemErrno();
		ostringstream s;
		s << "can't change working directory to `" << pwd << "':\n" << ex;
		write(fds[1], s.str().c_str(), s.str().length());
		close(fds[1]);
		exit(EXIT_FAILURE);
	    }
	}	

	if(execvp(argv[0], argv) == -1)
	{
	    //
	    // Send any errors to the parent process, using the write
	    // end of the pipe.
	    //
	    SyscallException ex(__FILE__, __LINE__);
	    ex.error = getSystemErrno();
	    ostringstream s;
	    s << "can't execute `" << path << "':\n" << ex;
	    write(fds[1], s.str().c_str(), s.str().length());
	    close(fds[1]);
	    exit(EXIT_FAILURE);
	}
    }
    else // Parent process.
    {
	close(fds[1]);

	Process process;
	process.pid = pid;
	process.fd = fds[0];
	process.server = server;
	_processes.push_back(process);
	
	int flags = fcntl(process.fd, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(process.fd, F_SETFL, flags);

	setInterrupt(0);

	if(_traceLevels->activator > 0)
	{
	    Ice::Trace out(_traceLevels->logger, _traceLevels->activatorCat);
	    out << "activated server `" << server->description.name << "'(pid = " << pid << ")";
	}
    }

    return true;
}

void
IcePack::ActivatorI::deactivate(const ServerPtr& server)
{
    Ice::Int pid = server->getPid();

    if(pid == 0)
    {
	//
	// Server is already deactivated.
	//
	return;
    }

    //
    // Send a SIGTERM to the process.
    //

#ifdef __linux
    //
    // Use process groups on Linux instead of processes
    //
    int ret = ::killpg(static_cast<pid_t>(pid), SIGTERM);
#else
    int ret = ::kill(static_cast<pid_t>(pid), SIGTERM);
#endif
    if(ret != 0 && getSystemErrno() != ESRCH)
    {
	SyscallException ex(__FILE__, __LINE__);
	ex.error = getSystemErrno();
	throw ex;
    }
    
    if(_traceLevels->activator > 1)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->activatorCat);
	out << "sent SIGTERM to server `" << server->description.name << "' (pid = " << pid << ")";
    }
}

void
IcePack::ActivatorI::kill(const ServerPtr& server)
{
    Ice::Int pid = server->getPid();
    
    if(pid == 0)
    {
	//
	// Server is already deactivated.
	//
	return;
    }

    //
    // Send a SIGKILL to the process.
    //
#ifdef __linux
    // Use process groups on Linux instead of processes
    int ret = ::killpg(static_cast<pid_t>(pid), SIGKILL);
#else
    int ret = ::kill(static_cast<pid_t>(pid), SIGKILL);
#endif
    if(ret != 0 && getSystemErrno() != ESRCH)
    {
	SyscallException ex(__FILE__, __LINE__);
	ex.error = getSystemErrno();
	throw ex;
    }

    if(_traceLevels->activator > 1)
    {
	Ice::Trace out(_traceLevels->logger, _traceLevels->activatorCat);
	out << "sent SIGKILL to server `" << server->description.name << "' (pid = " << pid << ")";
    }
}

Ice::Int
IcePack::ActivatorI::getServerPid(const ServerPtr& server)
{
    IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);

    for(vector<Process>::iterator p = _processes.begin(); p != _processes.end(); ++p)
    {
	if(p->server == server)
	{
	    return static_cast<Ice::Int>(p->pid);
	}
    }

    return 0;
}

void
IcePack::ActivatorI::start()
{
    //
    // Create and start the termination listener thread.
    //
    _thread = new TerminationListenerThread(*this);
    _thread->start();
}

void
IcePack::ActivatorI::waitForShutdown()
{
    IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
    while(!_deactivating)
    {
	wait();
    }
}

void
IcePack::ActivatorI::shutdown()
{
    setInterrupt(1);
}

void
IcePack::ActivatorI::destroy()
{
    {
	IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
	assert(_deactivating);
    }

    //
    // Deactivate all the processes.
    //
    deactivateAll();

    //
    // Join the termination listener thread. This thread terminates
    // when there's no more processes and when _deactivating is set to
    // true.
    //
    _thread->getThreadControl().join();
    _thread = 0;
}

void
IcePack::ActivatorI::runTerminationListener()
{
    try
    {
	terminationListener();
    }
    catch(const Exception& ex)
    {
	Error out(_traceLevels->logger);
	out << "exception in process termination listener:\n" << ex;
    }
    catch(...)
    {
	Error out(_traceLevels->logger);
	out << "unknown exception in process termination listener";
    }
}

void
IcePack::ActivatorI::deactivateAll()
{
    //
    // Stop all activate processes.
    //
    std::vector<Process> processes;
    {
	IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
	processes = _processes;
    }

    for(std::vector<Process>::iterator p = processes.begin(); p != processes.end(); ++p)
    {
	//
	// Stop the server. The activator thread should detect the
	// process deactivation and remove it from the activator
	// active processes.
	//
	try
	{
	    p->server->stop();
	}
	catch(const Ice::ObjectNotExistException&)
	{
	    //
	    // Expected if the server was in the process of being
	    // destroyed.
	    //
	}
    }
}

void
IcePack::ActivatorI::terminationListener()
{
    while(true)
    {
	fd_set fdSet;
	int maxFd = _fdIntrRead;
	FD_ZERO(&fdSet);
	FD_SET(_fdIntrRead, &fdSet);
	
	{
	    IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);

	    for(vector<Process>::iterator p = _processes.begin(); p != _processes.end(); ++p)
	    {
		int fd = p->fd;
		FD_SET(fd, &fdSet);
		if(maxFd < fd)
		{
		    maxFd = fd;
		}
	    }
	}
	
    repeatSelect:
	int ret = ::select(maxFd + 1, &fdSet, 0, 0, 0);
	assert(ret != 0);
	
	if(ret == -1)
	{
#ifdef EPROTO
	    if(errno == EINTR || errno == EPROTO)
	    {
		goto repeatSelect;
	    }
#else
	    if(errno == EINTR)
	    {
		goto repeatSelect;
	    }
#endif
	    
	    SyscallException ex(__FILE__, __LINE__);
	    ex.error = getSystemErrno();
	    throw ex;
	}
	
	{
	    IceUtil::Monitor< IceUtil::Mutex>::Lock sync(*this);
	    
	    if(FD_ISSET(_fdIntrRead, &fdSet))
	    {
		bool deactivating = clearInterrupt();

		if(deactivating && !_deactivating)
		{
		    //
		    // Deactivation has been initiated. Set _deactivating to true to 
		    // prevent new processes to be activated. This will also cause the
		    // termination of this loop when there's no more active processes.
		    //
		    _deactivating = true;
		    notifyAll(); // For waitForShutdown

		    if(_processes.empty())
		    {
			return;
		    }
		}
	    }
	    
	    vector<Process>::iterator p = _processes.begin();
	    while(p != _processes.end())
	    {
		int fd = p->fd;
		if(!FD_ISSET(fd, &fdSet))   
		{
		    ++p;
		    continue;
		}

		char s[16];
		ssize_t rs;
		string message;

		//
		// Read the message over the pipe.
		//
		while((rs = read(fd, &s, 16)) > 0)
		{
		    message.append(s, rs);
		}

		if(rs == -1)
		{
		    if(errno != EAGAIN || message.empty())
		    {
			SyscallException ex(__FILE__, __LINE__);
			ex.error = getSystemErrno();
			throw ex;
		    }

		    ++p;
		}
		else if(rs == 0)
		{    
		    //
		    // If the pipe was closed, the process has terminated.
		    //

		    if(_traceLevels->activator > 0)
		    {
			Ice::Trace out(_traceLevels->logger, _traceLevels->activatorCat);
			out << "detected server `" << p->server->description.name << "' termination";
		    }

		    try
		    {
			p->server->terminated();
		    }
		    catch(const Ice::LocalException& ex)
		    {
			Ice::Warning out(_traceLevels->logger);
			out << "unexpected exception raised by server `" << p->server->description.name 
			    << "' termination:\n" << ex;
		    }
			    
		    p = _processes.erase(p);
		    close(fd);

		    //
		    // We are deactivating and there's no more active processes. We can now 
		    // end this loop
		    //
		    if(_deactivating && _processes.empty())
		    {
			return;
		    }
		}

		//
		// Log the received message.
		//
		if(!message.empty())
		{
		    Error out(_traceLevels->logger);
		    out << message;
		}
	    }
	}
    }
}

bool
IcePack::ActivatorI::clearInterrupt()
{
    bool shutdown = false;
    char c;

    while(read(_fdIntrRead, &c, 1) == 1)
    {
	shutdown = shutdown ? true : c == 1;
    }

    return shutdown;
}

void
IcePack::ActivatorI::setInterrupt(char c)
{
    write(_fdIntrWrite, &c, 1);
}
