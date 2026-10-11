// Join and admin passwords never cross the wire (Phase B step 10, ADR 0014): the server sends a random nonce with its
// version, and the client proves it knows a password with HMAC-SHA256(password, label + nonce).
#pragma once
#include <cstddef>
#include <cstdint>

namespace credentials
{
	const int NONCE_SIZE = 16, MAC_SIZE = 32;

	void hmac(const char * key, const void * data, size_t size, uint8_t out[MAC_SIZE]);
	void nonce(uint8_t out[NONCE_SIZE]);
	// The proof for one use of a password: "join", "admin user" or "admin password"
	void prove(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], uint8_t out[MAC_SIZE]);
	// Compares in constant time
	bool check(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], const uint8_t mac[MAC_SIZE]);
}
