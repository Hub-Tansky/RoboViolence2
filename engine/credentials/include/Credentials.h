// Join and admin passwords never cross the wire (Phase B step 10, ADR 0014): the server sends a random nonce with its
// version, and the client proves it knows a password with HMAC-SHA256(password, label + nonce). Master bans sign the
// whole packet instead (ADR 0038).
#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace credentials
{
	const int NONCE_SIZE = 16, MAC_SIZE = 32;

	// Plain HMAC-SHA256 (public for the RFC 4231 test)
	void hmac(const char * key, const void * data, size_t size, uint8_t out[MAC_SIZE]);
	void nonce(uint8_t out[NONCE_SIZE]);
	// The proof for one use of a password: "join", "admin user" or "admin password"
	void prove(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], uint8_t out[MAC_SIZE]);
	// Compares in constant time
	bool check(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], const uint8_t mac[MAC_SIZE]);
	// Constant-time MAC comparison
	bool equal(const uint8_t a[MAC_SIZE], const uint8_t b[MAC_SIZE]);

	// The shortest password used alone that NIST SP 800-63B-4 allows (ADR 0038)
	const size_t MIN_SECRET_LENGTH = 15;

	// Signs a packed wire struct with a uint8_t Proof[MAC_SIZE] field: HMAC(password, the struct with Proof zeroed)
	template <class Packet> void signPacket(const char * password, Packet & packet)
	{
		static_assert(sizeof(packet.Proof) == MAC_SIZE, "Proof holds one MAC");
		memset(packet.Proof, 0, sizeof(packet.Proof));
		hmac(password, &packet, sizeof(packet), packet.Proof);
	}

	template <class Packet> bool checkPacket(const char * password, Packet packet)
	{
		uint8_t sent[MAC_SIZE];
		memcpy(sent, packet.Proof, MAC_SIZE);
		signPacket(password, packet);
		return equal(sent, packet.Proof);
	}
}
