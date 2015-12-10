
#include "NetworkMonitor.h"
#include "UDPSocket.h"

namespace
{
	const u_int UPDATE_RATE = 1;
	const int INFINIT_WAIT = -1;
	const u_int MAX_WAIT_TIME = 15;
}

void clearScreen()
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); // Handle to console screen buffer 
	COORD coordScreen = { 0, 0 };                      // Top left screen position (first cell)
	DWORD chars;                                       // Number of characters written 
	CONSOLE_SCREEN_BUFFER_INFO csbi;                   // Console screen buffer info
	DWORD consoleSize;                                 // Size of the console

	// Number of character cells in the current buffer
	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;

	// Console size: cells to write
	consoleSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Overwrite the screen buffer with whitespace
	if (!FillConsoleOutputCharacter(hConsole, TEXT(' '), consoleSize, 
									coordScreen, &chars))
		return;

	// Get the current text attribute.
	if (!GetConsoleScreenBufferInfo(hConsole, &csbi))
		return;

	// Set the buffer's attributes accordingly.
	if (!FillConsoleOutputAttribute(hConsole, csbi.wAttributes, 
									consoleSize, coordScreen, &chars))                       
		return;

	// Put the cursor at the top left position
	SetConsoleCursorPosition(hConsole, coordScreen);
}

int main(int argc, char *argv[])
{
	NetworkMonitor monitor;

	// Create udp sockets needed
	const u_short startupPort = 7106;
	UDPSocket startupSocket(startupPort);
	if (startupSocket.Initialise())
		monitor.AddSocketToPoll(startupSocket.GetPollFd());

	const u_short heartbeatPort = 7104;
	UDPSocket heartbeatSocket(heartbeatPort);
	if (heartbeatSocket.Initialise())
		monitor.AddSocketToPoll(heartbeatSocket.GetPollFd());

	// Read parameters
	// Screen updated every <updateScreenRate> seconds
	int updateScreenRate = UPDATE_RATE;
	// Poll function will wait for data from sockets for <pollTimeout> seconds
	int pollTimeout = MAX_WAIT_TIME;
	if (argc > 3)
	{
		cout << "Usage: " << argv[0] << " <updateScreenRate in secs> <sockets timeout in secs>" << endl;
	}
	else
	{
		if (argc > 2)
			pollTimeout = stoi(argv[2], 0);
		if (argc > 1)
			updateScreenRate = stoi(argv[1], 0);
	}

	chrono::time_point<chrono::system_clock> lastTimePrinted = chrono::system_clock::now();
	chrono::duration<double> elapsedTime;

	// listens to UDP messages
	// updates the monitor and prints status reports on screen
	while (monitor.Poll(pollTimeout))
	{
		monitor.UpdateStatus();

		elapsedTime = chrono::system_clock::now() - lastTimePrinted;
		if (elapsedTime.count() > updateScreenRate)
		{
			// Fill the screen with blanks
			clearScreen();
			monitor.PrintStatus();
			// Update timer
			lastTimePrinted = chrono::system_clock::now();
		}
	}

	return EXIT_SUCCESS;
}