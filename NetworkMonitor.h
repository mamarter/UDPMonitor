#ifndef SOCKETS_MONITOR_CLASS_H
#define SOCKETS_MONITOR_CLASS_H

#include <winsock2.h>
#include <vector>
#include <unordered_map>
#include "Machine.h"

class NetworkMonitor
{
public:

	NetworkMonitor(){}
	~NetworkMonitor();

	/* Adds a socket to the sockets poll list */
	void AddSocketToPoll(WSAPOLLFD& poll) { mPollVector.push_back(poll); }
	
	/* Polls network information from sockets blocking
	during a period of time established by the user (default value
	is UPDATE_RATE in case it's not specified) */
	bool Poll(const int timeout);

	/* Turns off machines that have timed out (machines that haven't sent any
	heartbeat for TIMEOUT_MACHINES seconds) */
	void UpdateStatus();

	/* Shows machines status:
	"machines in session" are connected machines
	"machines waiting to join session" are slave machines waiting for the
	master to connect in order to join the session
	"just joined" machines are machines that have just connected but haven't sent
	any heartbeat messages and therefore there is no state information	*/
	void PrintStatus();
	
private:

	/*Tokenizes the udp message and updates sessions and machines maps
	performing the corresponding action*/
	void ProcessMessage(const string& message);

	/*Parses string into a vector of words*/
	int Tokenize(const string& input, vector<string>& word);

	/* Adds/updates a machine (master/slave) to the machines map */
	void AddMachine(const string& machineId, bool isMaster, const string& sessionName);

	/* Updates the state of a machine (version, fps, last time received) */
	void UpdateMachine(const string& machineId, float version, u_int fps);

	/* Adds/Updates a session */
	void AddSession(const string& session, const vector<string>& machines);

	/* Removes a session from the sessions map */
	void RemoveSession(const string& sessionName);

	vector<WSAPOLLFD> mPollVector;

	typedef unordered_map<string, Machine*> machinesMap;
	typedef unordered_map<string, vector<Machine*>> sessionsMap;

	sessionsMap mSessions;
	machinesMap mMachines;

};

#endif //SOCKETS_MONITOR_CLASS_H
