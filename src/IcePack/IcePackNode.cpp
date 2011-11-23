// **********************************************************************
//
// Copyright (c) 2003-2004 ZeroC, Inc. All rights reserved.
//
// This copy of Ice is licensed to you under the terms described in the
// ICE_LICENSE file included in this distribution.
//
// **********************************************************************

#include <Ice/Ice.h>
#include <Ice/Service.h>
#include <IcePack/Activator.h>
#include <IcePack/WaitQueue.h>
#include <IcePack/Registry.h>
#include <IcePack/ActivatorI.h>
#include <IcePack/ServerFactory.h>
#include <IcePack/AdapterFactory.h>
#include <IcePack/ServerDeployerI.h>
#include <IcePack/AdapterI.h>
#include <IcePack/NodeI.h>
#include <IcePack/NodeInfo.h>
#include <IcePack/TraceLevels.h>
#include <IceUtil/UUID.h>

#ifdef _WIN32
#   include <direct.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   define S_ISDIR(mode) ((mode) & _S_IFDIR)
#   define S_ISREG(mode) ((mode) & _S_IFREG)
#else
#   include <csignal>
#   include <signal.h>
#   include <sys/wait.h>
#   include <sys/types.h>
#   include <sys/stat.h>
#   include <unistd.h>
#endif

using namespace std;
using namespace IcePack;

namespace IcePack
{

class NodeService : public Ice::Service
{
public:

    NodeService();

    virtual bool shutdown();

protected:

    virtual bool start(int, char*[]);
    virtual void waitForShutdown();
    virtual bool stop();

private:

    void usage(const std::string&);

    ActivatorPtr _activator;
    WaitQueuePtr _waitQueue;
    std::auto_ptr<Registry> _registry;
};

} // End of namespace IcePack

#ifndef _WIN32
extern "C"
{

static void
childHandler(int)
{
    //
    // Call wait to de-allocate any resources allocated for the child
    // process and avoid zombie processes. See man wait or waitpid for
    // more information.
    //
    int olderrno = errno;

    pid_t pid;
    do
    {
	pid = waitpid(-1, 0, WNOHANG);
    }
    while(pid > 0);

    assert(pid != -1 || errno == ECHILD);

    errno = olderrno;
}

}
#endif

IcePack::NodeService::NodeService()
{
}

bool
IcePack::NodeService::shutdown()
{
    assert(_activator);
    _activator->shutdown();
    return true;
}

bool
IcePack::NodeService::start(int argc, char* argv[])
{
#ifndef _WIN32
    //
    // This application forks, so we need a signal handler for child termination.
    //
    struct sigaction action;
    action.sa_handler = childHandler;
    sigemptyset(&action.sa_mask);
    sigaddset(&action.sa_mask, SIGCHLD);
    action.sa_flags = 0;
    sigaction(SIGCHLD, &action, 0);
#endif

    bool nowarn = false;
    string descriptor;
    vector<string> targets;
    for(int i = 1; i < argc; ++i)
    {
        if(strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0)
        {
            usage(argv[0]);
            return false;
        }
        else if(strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0)
        {
            trace(ICE_STRING_VERSION);
            return false;
        }
        else if(strcmp(argv[i], "--nowarn") == 0)
        {
            nowarn = true;
        }
        else if(strcmp(argv[i], "--deploy") == 0)
        {
            if(i + 1 >= argc)
            {
                error("missing descriptor argument for option `" + string(argv[i]) + "'");
                usage(argv[0]);
                return false;
            }

            descriptor = argv[++i];

            while(i + 1 < argc && argv[++i][0] != '-')
            {
                targets.push_back(argv[i]);
            }
        }
    }

    Ice::PropertiesPtr properties = communicator()->getProperties();

    //
    // Disable server idle time. Otherwise, the adapter would be
    // shutdown prematurely and the deactivation would fail.
    // Deactivation of the node relies on the object adapter
    // to be active since it needs to terminate servers.
    //
    // TODO: implement Ice.ServerIdleTime in the activator
    // termination listener instead?
    //
    properties->setProperty("Ice.ServerIdleTime", "0");

    if(properties->getPropertyAsIntWithDefault("Ice.ThreadPool.Server.Size", 5) <= 5)
    {
	properties->setProperty("Ice.ThreadPool.Server.Size", "5");
    }

    //
    // Collocate the IcePack registry if we need to.
    //
    if(properties->getPropertyAsInt("IcePack.Node.CollocateRegistry") > 0)
    {
        _registry = auto_ptr<Registry>(new Registry(communicator()));
        if(!_registry->start(nowarn, true))
        {
            return false;
        }

        //
        // The node needs a different thread pool to avoid
        // deadlocks in connection validation.
        //
        if(properties->getPropertyAsInt("IcePack.Node.ThreadPool.Size") == 0)
        {
            int size = properties->getPropertyAsInt("Ice.ThreadPool.Server.Size");

            ostringstream os1;
            os1 << static_cast<int>(size / 3);
            properties->setProperty("IcePack.Node.ThreadPool.Size", os1.str());

            ostringstream os2;
            os2 << size - static_cast<int>(size / 3);
            properties->setProperty("Ice.ThreadPool.Server.Size", os2.str());
        }

        //
        // Set the Ice.Default.Locator property to point to the
        // collocated locator (this property is passed by the
        // activator to each activated server).
        //
        string locatorPrx = "IcePack/Locator:" + properties->getProperty("IcePack.Registry.Client.Endpoints");
        properties->setProperty("Ice.Default.Locator", locatorPrx);
    }
    else if(properties->getProperty("Ice.Default.Locator").empty())
    {
        error("property `Ice.Default.Locator' is not set");
        return false;
    }

    //
    // Initialize the database environment (first setup the directory structure if needed).
    //
    string envName;
    string dataPath = properties->getProperty("IcePack.Node.Data");
    if(dataPath.empty())
    {
        error("property `IcePack.Node.Data' is not set");
        return false;
    }
    else
    {
#ifdef _WIN32
        struct _stat filestat;
        if(::_stat(dataPath.c_str(), &filestat) != 0 || !S_ISDIR(filestat.st_mode))
        {
            error("property `IcePack.Node.Data' is not set to a valid directory path");
            return false;
        }            
#else
        struct stat filestat;
        if(::stat(dataPath.c_str(), &filestat) != 0 || !S_ISDIR(filestat.st_mode))
        {
            error("property `IcePack.Node.Data' is not set to a valid directory path");
            return false;
        }            
#endif

        //
        // Creates subdirectories db and servers in the IcePack.Node.Data
        // directory if they don't already exist.
        //
        if(dataPath[dataPath.length() - 1] != '/')
        {
            dataPath += "/"; 
        }

        envName = dataPath + "db";
        string serversPath = dataPath + "servers";

#ifdef _WIN32
        if(::_stat(envName.c_str(), &filestat) != 0)
        {
            _mkdir(envName.c_str());
        }
        if(::_stat(serversPath.c_str(), &filestat) != 0)
        {
            _mkdir(serversPath.c_str());
        }
#else
        if(::stat(envName.c_str(), &filestat) != 0)
        {
            mkdir(envName.c_str(), 0755);
        }
        if(::stat(serversPath.c_str(), &filestat) != 0)
        {
            mkdir(serversPath.c_str(), 0755);
        }
#endif
    }

    //
    // Check that required properties are set and valid.
    //
    if(properties->getProperty("IcePack.Node.Endpoints").empty())
    {
        error("property `IcePack.Node.Endpoints' is not set");
        return false;
    }

    string name = properties->getProperty("IcePack.Node.Name");
    if(name.empty())
    {
        char host[1024 + 1];
        if(gethostname(host, 1024) != 0)
        {
            syserror("property `IcePack.Node.Name' is not set and couldn't get the hostname:");
            return false;
        }
        else if(!nowarn)
        {
            warning("property `IcePack.Node.Name' is not set, using hostname: " + string(host));
        }
    }

    //
    // Set the adapter id for this node and create the node object
    // adapter.
    //
    properties->setProperty("IcePack.Node.AdapterId", "IcePack.Node-" + name);

    Ice::ObjectAdapterPtr adapter = communicator()->createObjectAdapter("IcePack.Node");

    TraceLevelsPtr traceLevels = new TraceLevels(properties, communicator()->getLogger());

    //
    // Create the activator.
    //
    _activator = new ActivatorI(traceLevels, properties);

    //
    // Create the wait queue.
    //
    _waitQueue = new WaitQueue();
    _waitQueue->start();

    //
    // Create the server factory. The server factory creates persistent objects
    // for the server and server adapter. It also takes care of installing the
    // evictors and object factories necessary to store these objects.
    //
    ServerFactoryPtr serverFactory = new ServerFactory(adapter, traceLevels, envName, _activator, _waitQueue);

    //
    // Create the node object and the node info. Because of circular
    // dependencies on the node info we need to create the proxy for
    // the server registry and deployer now.
    //
    Ice::Identity deployerId;
    deployerId.name = IceUtil::generateUUID();

    NodePtr node = new NodeI(_activator, name, ServerDeployerPrx::uncheckedCast(adapter->createProxy(deployerId)));
    NodePrx nodeProxy = NodePrx::uncheckedCast(adapter->addWithUUID(node));

    NodeInfoPtr nodeInfo = new NodeInfo(communicator(), serverFactory, node, traceLevels);

    //
    // Create the server deployer.
    //
    ServerDeployerPtr deployer = new ServerDeployerI(nodeInfo);
    adapter->add(deployer, deployerId);

    //
    // Register this node with the node registry.
    //
    try
    {
        NodeRegistryPrx nodeRegistry = NodeRegistryPrx::checkedCast(
            communicator()->stringToProxy("IcePack/NodeRegistry@IcePack.Registry.Internal"));
        nodeRegistry->add(name, nodeProxy);
    }
    catch(const NodeActiveException&)
    {
        error("a node with the same name is already registered and active");
        return false;
    }
    catch(const Ice::LocalException&)
    {
        error("couldn't contact the IcePack registry");
        return false;
    }

    //
    // Start the activator.
    //
    _activator->start();

    //
    // We are ready to go! Activate the object adapter.
    //
    adapter->activate();

    //
    // Deploy application if a descriptor is passed as a command-line option.
    //
    if(!descriptor.empty())
    {
        AdminPrx admin;
        try
        {
            admin = AdminPrx::checkedCast(communicator()->stringToProxy("IcePack/Admin"));
        }
        catch(const Ice::LocalException& ex)
        {
            ostringstream ostr;
            ostr << "couldn't contact IcePack admin interface to deploy application `" << descriptor << "':" << endl
                 << ex;
            warning(ostr.str());
        }

        if(admin)
        {
            try
            {
                admin->addApplication(descriptor, targets);
            }
            catch(const ServerDeploymentException& ex)
            {
                ostringstream ostr;
                ostr << "failed to deploy application `" << descriptor << "':" << endl
                     << ex << ": " << ex.server << ": " << ex.reason;
                warning(ostr.str());
            }
            catch(const DeploymentException& ex)
            {
                ostringstream ostr;
                ostr << "failed to deploy application `" << descriptor << "':" << endl
                     << ex << ": " << ex.component << ": " << ex.reason;
                warning(ostr.str());
            }
            catch(const Ice::LocalException& ex)
            {
                ostringstream ostr;
                ostr << "failed to deploy application `" << descriptor << "':" << endl
                     << ex;
                warning(ostr.str());
            }
        }
    }

    string bundleName = properties->getProperty("IcePack.Node.PrintServersReady");
    if(!bundleName.empty())
    {
	cout << bundleName << " ready" << endl;
    }

    return true;
}

void
IcePack::NodeService::waitForShutdown()
{
    //
    // Wait for the activator shutdown. Once the run method returns
    // all the servers have been deactivated.
    //
    enableInterrupt();
    _activator->waitForShutdown();
    disableInterrupt();
}

bool
IcePack::NodeService::stop()
{
    try
    {
        _activator->destroy();
    }
    catch(...)
    {
    }

    //
    // The wait queue must be destroyed after the activator and before
    // the communicator is shutdown.
    //
    try
    {
        _waitQueue->destroy();
    }
    catch(...)
    {
    }

    //
    // We can now safely shutdown the communicator.
    //
    try
    {
        communicator()->shutdown();
        communicator()->waitForShutdown();
    }
    catch(...)
    {
    }

    _activator = 0;

    return true;
}

void
IcePack::NodeService::usage(const string& appName)
{
    string options =
	"Options:\n"
	"-h, --help           Show this message.\n"
	"-v, --version        Display the Ice version.\n"
	"--nowarn             Don't print any security warnings.\n"
	"\n"
	"--deploy DESCRIPTOR [TARGET1 [TARGET2 ...]]\n"
	"                     Deploy descriptor in file DESCRIPTOR, with\n"
	"                     optional targets.";
#ifdef _WIN32
    if(checkSystem())
    {
        options.append(
	"\n"
	"\n"
	"--service NAME       Run as the Windows service NAME.\n"
	"\n"
	"--install NAME [--display DISP] [--executable EXEC] [args]\n"
	"                     Install as Windows service NAME. If DISP is\n"
	"                     provided, use it as the display name,\n"
	"                     otherwise NAME is used. If EXEC is provided,\n"
	"                     use it as the service executable, otherwise\n"
	"                     this executable is used. Any additional\n"
	"                     arguments are passed unchanged to the\n"
	"                     service at startup.\n"
	"--uninstall NAME     Uninstall Windows service NAME.\n"
	"--start NAME [args]  Start Windows service NAME. Any additional\n"
	"                     arguments are passed unchanged to the\n"
	"                     service.\n"
	"--stop NAME          Stop Windows service NAME."
        );
    }
#else
    options.append(
        "\n"
        "\n"
        "--daemon             Run as a daemon.\n"
        "--noclose            Do not close open file descriptors.\n"
        "--nochdir            Do not change the current working directory."
    );
#endif
    cerr << "Usage: " << appName << " [options]" << endl;
    cerr << options << endl;
}

int
main(int argc, char* argv[])
{
    NodeService svc;
    return svc.main(argc, argv);
}
