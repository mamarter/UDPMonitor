#ifndef MACHINE_CLASS_H
#define MACHINE_CLASS_H

#include <chrono>
#include <string>
#include <iostream>

using namespace std;
namespace defaults
{
	const float INVALID_VERSION = 0.f;
	const int INVALID_FPS = -1;
}

class Machine
{
public:
	
	Machine(){}
	Machine(const string& sessionName, bool isMaster, bool active = true)
		: mLastTimePoint(chrono::system_clock::now()),
		mSessionName(sessionName),
		mVersion(defaults::INVALID_VERSION),
		mFps(defaults::INVALID_FPS),
		mIsMaster(isMaster),
		mInSession(isMaster),
		mOnline(active)
	{}

	~Machine(){}

	/*Updates the mLastTimePoint point with the current time point*/
	void UpdateCurrentTimePoint() { mLastTimePoint = chrono::system_clock::now(); }

	/*Returns the time spent since a message from this machine has been received*/
	double GetTimeSinceHeartbeat() const { 
		std::chrono::duration<double> elapsedTime = chrono::system_clock::now() - mLastTimePoint;
		return elapsedTime.count();
	}

	void SetSessionName(const string& sessionName) { mSessionName = sessionName; }
	const string& GetSessionName() const { return mSessionName; }

	void SetVersion(float version) { mVersion = version; }
	float GetVersion() const { return mVersion; }

	void SetFPS(int FPS) { mFps = FPS; }
	int GetFPS() const { return mFps; }

	void SetIsMaster(bool isMaster) { mIsMaster = isMaster;	}
	bool GetIsMaster() const { return mIsMaster; }

	void SetInSession(bool inSession) { mInSession = inSession; }
	bool GetInSession() const { return mInSession; }

	void SetOnline(bool online) { 
		if (!online && online != mOnline) InvalidateStatus();
		mOnline = online;
	}
	bool GetOnline() const { return mOnline; }

	bool HasValidStatus() const { return mFps != defaults::INVALID_FPS && 
										 mVersion != defaults::INVALID_VERSION;	}
	void UpdateStatus(float version, int FPS) {	mVersion = version;
												mFps = FPS;	}
	void InvalidateStatus() { UpdateStatus(defaults::INVALID_VERSION, defaults::INVALID_FPS); }

private:

	/* Stores the time point in which a machine has last sent a message*/
	chrono::time_point<chrono::system_clock> mLastTimePoint;

	/*Status details*/
	string mSessionName;
	float mVersion;
	int mFps;
	/* Identifies whether a machine is master or slave*/
	bool mIsMaster;
	/* Stores whether a machine is currently connected to a session*/
	bool mInSession;
	/* Stores whether a machine is active or inactive */
	bool mOnline;
};


#endif //MACHINE_CLASS_H