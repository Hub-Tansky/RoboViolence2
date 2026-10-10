// The bb_* API for tests. This is an object file, so its bb_* always win over babonet's archive; babonet's baboNet.o
// is pulled in only if the server references a bb_* (or other symbol) defined there and not here, and the link then
// fails with duplicate bb_* symbols: add the missing function below. Covers every function in baboNet.h.
// Server side: connects, disconnects and messages come from the harness queues; sends are captured. Client and peer
// calls fail.
#include "baboNet.h"
#include "server_harness.h"

#include <deque>
#include <memory>

namespace
{
	struct Incoming
	{
		UINT4 from;
		int typeID;
		std::vector<char> data;
	};
	std::deque<INT4> pendingEvents; // > 0 new client, < 0 client gone (what bb_serverUpdate reports)
	std::deque<Incoming> pendingMessages;
	std::unique_ptr<char[]> current; // exact-size copy handed to the server; valid until the next bb_serverReceive
	std::vector<harness::SentPacket> sentPackets;
	char noError[] = "";
	char version[] = "4.0";
	char localIP[] = "127.0.0.1";
}

namespace harness
{
	void queueConnect(UINT4 id) { pendingEvents.push_back((INT4)id); }
	void disconnect(UINT4 id) { pendingEvents.push_back(-(INT4)id); }
	void queueMessage(UINT4 from, int typeID, const void * data, int size)
	{
		const char * p = static_cast<const char *>(data);
		pendingMessages.push_back({from, typeID, std::vector<char>(p, p + size)});
	}
	std::vector<SentPacket> & sent() { return sentPackets; }
}

int bb_init() { return 0; }
void bb_shutdown() {}

INT4 bb_serverUpdate(float, int, char * NewIP)
{
	if (pendingEvents.empty()) return 0;
	INT4 id = pendingEvents.front();
	pendingEvents.pop_front();
	if (NewIP && id > 0) strcpy(NewIP, localIP);
	return id;
}
int bb_serverCreate(bool, int, unsigned short) { return 0; }
int bb_serverSend(char * dataToSend, int dataSize, int typeID, INT4 destination, int)
{
	sentPackets.push_back({typeID, destination, std::vector<char>(dataToSend, dataToSend + dataSize)});
	return 0;
}
char * bb_serverReceive(UINT4 & babonetID, int & typeID, int * size)
{
	if (pendingMessages.empty()) return 0;
	Incoming m = pendingMessages.front();
	pendingMessages.pop_front();
	// A fresh allocation of exactly the delivered bytes, as on the wire: ASan sees any read past them
	current.reset(new char[m.data.empty() ? 1 : m.data.size()]);
	memcpy(current.get(), m.data.data(), m.data.size());
	babonetID = m.from;
	typeID = m.typeID;
	if (size) *size = (int)m.data.size();
	return current.get();
}
char * bb_serverGetLastError() { return noError; }
char * bb_serverGetLastMessage() { return noError; }
int bb_serverDisconnectClient(UINT4 baboNetID) { harness::disconnect(baboNetID); return 0; } // like babonet: reported next update
int bb_serverShutdown() { return 0; }
UINT4 bb_serverGetBytesSent() { return 0; }
UINT4 bb_serverGetBytesReceived() { return 0; }
int bb_serverGetQueueCount(int) { return 0; }
int bb_serverSetClientRate(int, UINT4) { return 0; }

int bb_clientUpdate(UINT4, float, int) { return 1; }
UINT4 bb_clientConnect(const char *, unsigned short) { return 0; }
int bb_clientSend(UINT4, char *, int, int, int) { return 1; }
char * bb_clientReceive(UINT4, int *, int *) { return 0; }
char * bb_clientGetLastError(UINT4) { return noError; }
char * bb_clientGetLastMessage(UINT4) { return noError; }
int bb_clientDisconnect(UINT4) { return 0; }
UINT4 bb_clientGetBytesSent(UINT4) { return 0; }
UINT4 bb_clientGetBytesReceived(UINT4) { return 0; }
int bb_clientSetRate(UINT4, int) { return 0; }

int bb_peerUpdate(float, bool & isNew) { isNew = false; return 0; }
int bb_peerBindPort(unsigned short) { return 0; }
int bb_peerSend(INT4, char *, int, int, bool) { return 1; }
int bb_peerSend(char *, unsigned short, char *, int, int, bool) { return 1; }
char * bb_peerReceive(INT4 *, int *) { return 0; }
int bb_peerGetIPport(UINT4, char *, unsigned short *) { return 1; }
int bb_peerDelete(UINT4, bool) { return 0; }
int bb_peerShutdown() { return 0; }
char * bb_peerGetLastError() { return noError; }
char * bb_peerGetLastMessage() { return noError; }
UINT4 bb_peerGetBytesSent() { return 0; }
UINT4 bb_peerGetBytesReceived() { return 0; }

char * bb_getVersion() { return version; }
char * bb_getMyIP() { return localIP; }
void bb_getMyMAC(unsigned char * AddrOut) { memset(AddrOut, 0, 6); }
void bb_enable(unsigned char) {}
void bb_disable(unsigned char) {}
