
#include <sstream>
#include <iomanip> 
#include "NetworkMonitor.h"

namespace
{
	const u_int MAX_LENGTH = 200;
	const u_int TIMEOUT_MACHINES = 1;
	const string MASTER_START = "SESSION2";
	const string SLAVE_START = "MACHINE";
	const string HEARTBEAT = "MACHINESTATUS";
	const char SEPARATOR = '|';

	enum{
		TOKENS_SESSION = 3,
		TOKENS_MACHINE = 3,
		TOKENS_HEARTBEAT = 4
	};
}

NetworkMonitor::~NetworkMonitor()
{
	mPollVector.clear();

	sessionsMap::iterator clearSessionsIt = mSessions.begin();
	for (clearSessionsIt = mSessions.begin(); clearSessionsIt != mSessions.end(); clearSessionsIt++)
	{
		vector<Machine*>& machinesVector = clearSessionsIt->second;
		machinesVector.clear();
	}
	mSessions.clear();

	machinesMap::iterator clearMachinesIt = mMachines.begin();
	for (clearMachinesIt = mMachines.begin(); clearMachinesIt != mMachines.end(); clearMachinesIt++)
	{
		Machine* current = clearMachinesIt->second;
		if (current)
		{
			delete current;
		}
	}
	mMachines.clear();
}

int NetworkMonitor::Tokenize(const string& input, vector<string>& tokens)
{
	stringstream ss;
	ss << input;

	string token;
	while (getline(ss, token, SEPARATOR))
	{
		tokens.push_back(token);
	}

	if (tokens.size() == 0 && input.size() > 0)
	{
		cout << "WARNING: Tokenize failed to read input string <" << input << ">" << endl;
	}

	return tokens.size();
}

void NetworkMonitor::ProcessMessage(const string& message)
{
	vector<string> tokens;
	if (Tokenize(message, tokens) > 0)
	{
		const string& protocol(tokens[0]);
		if (protocol == MASTER_START && tokens.size() >= TOKENS_SESSION)
		{
			// Use: SESSION2|sessionname|creator|machineid1|machineid2|machineid3|machinen
			const string& sessionName = tokens[1];
			const string& machineId = tokens[2];
			AddMachine(machineId, true, sessionName);

			// machines on the session
			vector<string> machineList(tokens.begin() + 2, tokens.end());
			AddSession(sessionName, machineList);
		}
		else if (protocol == SLAVE_START && tokens.size() == TOKENS_MACHINE)
		{
			// Use: MACHINE|machineid|sessionname
			const string& machineId = tokens[1];
			const string& sessionName = tokens[2];
			AddMachine(machineId, false, sessionName);
		}
		else if (protocol == HEARTBEAT && tokens.size() == TOKENS_HEARTBEAT)
		{
			// Use: MACHINESTATUS|machineid|version|fps
			const string& machineId = tokens[1];
			const float version = stof(tokens[2], 0);
			const int fps = stoi(tokens[3]);
			UpdateMachine(machineId, version, fps);
		}
		else
		{
			cout << "WARNING: '" << message << "' message format not recognised" << endl;
		}
	}
}

bool NetworkMonitor::Poll(const int timeout)
{
	const u_int pollSize = mPollVector.size();
	if (pollSize == 0)
	{
		cerr << "ERROR at NetworkMonitor::Poll - Socketsfd vector is empty " << endl;
		return false;
	}

	WSAPOLLFD* pollSet = &(mPollVector[0]);
	if (!pollSet)
	{
		cerr << "ERROR at NetworkMonitor::Poll - pollSet is null" << endl;
		return false;
	}

	int activeSockets;
	if ((activeSockets = WSAPoll(pollSet, pollSize, timeout*1000)) == SOCKET_ERROR)
	{
		cerr << "ERROR at NetworkMonitor::Poll - ";
		cerr << "WSAPoll error: " << WSAGetLastError() << endl;
		return false;
	}
	else if (activeSockets == 0)
	{
		cerr << "TIMEOUT: Not getting any data from sockets for " << timeout << " seconds" << endl;
		return false;
	}
	else
	{
		for (u_int s = 0; s < pollSize; s++)
		{
			// Read Event
			if (pollSet[s].revents & POLLIN) 
			{
				char buffer[MAX_LENGTH];
				int read;
				if ((read = recv(pollSet[s].fd, buffer, MAX_LENGTH, 0)) == SOCKET_ERROR)
				{
					const int wsaError = WSAGetLastError();
					if (wsaError == WSAEMSGSIZE)
					{
						cerr << "ERROR at NetworkMonitor::Poll - recv failed: ";
						cerr << "Message too long. Buffer size: " << strlen(buffer) << endl;
						return false;
					}
					else
					{
						cerr << "ERROR at NetworkMonitor::Poll - recv failed: " + wsaError << endl;
					}
				}
				else
				{
					ProcessMessage(string(buffer, read));
				}
			}
		}
	}
	return true;
}

void NetworkMonitor::UpdateStatus()
{
	// Identify timeout machines 
	machinesMap::iterator machinesIt;
	for (machinesIt = mMachines.begin(); machinesIt != mMachines.end(); machinesIt++)
	{
		Machine* machine = machinesIt->second;
		if (machine && machine->GetTimeSinceHeartbeat() > TIMEOUT_MACHINES)
		{
			machine->SetOnline(false);
			if (machine->GetIsMaster())
				RemoveSession(machine->GetSessionName());
		}
	}
}

void NetworkMonitor::PrintStatus()
{
	// Display status of the online machines
	cout << endl << string(4, '-') << "STATUS" << string(60, '-') << endl << endl;
	machinesMap::const_iterator machinesIt;
	for (machinesIt = mMachines.begin(); machinesIt != mMachines.end(); machinesIt++)
	{
		const Machine* current = machinesIt->second;
		if (current && current->GetOnline())
		{
			cout << setw(16) << left << "<" + machinesIt->first + ">";
			string printString = "session: ";
			if (current->GetInSession())
			{
				printString += current->GetIsMaster() ? "MASTER" : "SLAVE";
				printString += (current->HasValidStatus() ? " in " : " just joined ");
			}
			else
			{
				printString += "waiting to join ";
			}

			printString += "<" + current->GetSessionName() + ">";
			cout << setw(40) << left << printString;

			if (current->HasValidStatus())
			{
				cout << " v=" << current->GetVersion();
				cout << " FPS=" << current->GetFPS();
			}
			cout << endl;
		}
	}
	cout << endl << string(70, '-') << endl << endl;
}


void NetworkMonitor::AddMachine(const string& machineId, bool isMaster, const string& sessionName)
{
	//Adds/updates a machine in the machines map
	machinesMap::iterator itMachines = mMachines.find(machineId);
	Machine* machine;
	if (itMachines == mMachines.end()) //machine not found - add
	{
		machine = new Machine(sessionName, isMaster);
		mMachines.emplace(machineId, machine);
	}
	else // machine found - update 
	{
		machine = itMachines->second;
		if (machine)
		{
			machine->UpdateCurrentTimePoint();
			machine->SetIsMaster(isMaster);
			machine->SetSessionName(sessionName);
			machine->InvalidateStatus();
			machine->SetOnline(true);
		}
	}
}

void NetworkMonitor::UpdateMachine(const string& machineId, float version, u_int fps)
{
	machinesMap::const_iterator itMachines = mMachines.find(machineId);
	if (itMachines == mMachines.end()) // machine not found
	{
		cout << "WARNING: Machine <" << machineId << "> trying to be updated not found" << endl;
		return;
	}
	Machine* machine = itMachines->second;
	if (machine)
	{
		machine->UpdateCurrentTimePoint();
		machine->UpdateStatus(version, fps);
	}
}

void NetworkMonitor::RemoveSession(const string& sessionName)
{
	sessionsMap::iterator findSession = mSessions.find(sessionName);
	if (findSession != mSessions.end())
	{
		// Alert machines in session that its master left
		vector<Machine*>& machineList = findSession->second;
		vector<Machine*>::const_iterator MachinesIt;
		for (MachinesIt = machineList.begin(); MachinesIt != machineList.end(); MachinesIt++)
		{
			Machine* machine = *MachinesIt;
			if (machine)
			{
				machine->SetInSession(false);
			}
		}

		machineList.clear();
		// Remove session
		mSessions.erase(findSession);
	}
}
void NetworkMonitor::AddSession(const string& sessionName, const vector<string>& machines)
{
	vector<Machine*> machinesVector;
	vector<string>::const_iterator stringIt;
	// Add machines to the list of the session
	for (stringIt = machines.begin(); stringIt != machines.end(); stringIt++)
	{
		const string& machineId = *stringIt;
		machinesMap::iterator findIt = mMachines.find(machineId);
		Machine* machine;
		if (findIt == mMachines.end()) // not found - create offline machine
		{
			machine = new Machine(sessionName, false, false);
			mMachines.emplace(machineId, machine);
		}
		else // found - get address
		{
			machine = findIt->second;
		}
		if (machine)
		{
			machine->SetInSession(true);
			machinesVector.push_back(machine);
		}
	}

	sessionsMap::iterator sessionsIt = mSessions.find(sessionName);
	if (sessionsIt == mSessions.end())
	{

		mSessions.emplace(sessionName, machinesVector);
	}
	else
	{
		sessionsIt->second.clear();
		sessionsIt->second = machinesVector;
	}
}





