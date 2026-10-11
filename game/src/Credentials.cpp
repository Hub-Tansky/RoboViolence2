#include "Credentials.h"

#include <sodium.h>

#include <cstdlib>
#include <cstring>
#include <vector>

namespace credentials
{
	void hmac(const char * key, const void * data, size_t size, uint8_t out[MAC_SIZE])
	{
		crypto_auth_hmacsha256_state state;
		crypto_auth_hmacsha256_init(&state, reinterpret_cast<const unsigned char *>(key), strlen(key));
		crypto_auth_hmacsha256_update(&state, static_cast<const unsigned char *>(data), size);
		crypto_auth_hmacsha256_final(&state, out);
	}

	void nonce(uint8_t out[NONCE_SIZE])
	{
		if (sodium_init() < 0) abort(); // no random source
		randombytes_buf(out, NONCE_SIZE);
	}

	void prove(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], uint8_t out[MAC_SIZE])
	{
		std::vector<uint8_t> message(label, label + strlen(label) + 1); // the label's '\0' separates it from the nonce
		message.insert(message.end(), nonce, nonce + NONCE_SIZE);
		hmac(password, message.data(), message.size(), out);
	}

	bool check(const char * password, const char * label, const uint8_t nonce[NONCE_SIZE], const uint8_t mac[MAC_SIZE])
	{
		uint8_t expected[MAC_SIZE];
		prove(password, label, nonce, expected);
		return sodium_memcmp(expected, mac, MAC_SIZE) == 0;
	}
}
