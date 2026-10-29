// Phase B step 10: the credential HMAC is HMAC-SHA256 (RFC 4231 test case 2), and a proof checks only against its
// own password, label and nonce. A signed packet (ADR 0038) is HMAC over the packet with its Proof zeroed.
#include "Credentials.h"

#include <cstdio>
#include <cstring>

int main()
{
	int failures = 0;
	auto check = [&](bool ok, const char * what) { if (!ok) std::printf("FAIL %s\n", what), ++failures; };

	const char data[] = "what do ya want for nothing?";
	const uint8_t rfc4231[credentials::MAC_SIZE] = {0x5b, 0xdc, 0xc1, 0x46, 0xbf, 0x60, 0x75, 0x4e, 0x6a, 0x04, 0x24,
		0x26, 0x08, 0x95, 0x75, 0xc7, 0x5a, 0x00, 0x3f, 0x08, 0x9d, 0x27, 0x39, 0x83, 0x9d, 0xec, 0x58, 0xb9, 0x64, 0xec,
		0x38, 0x43};
	uint8_t mac[credentials::MAC_SIZE];
	credentials::hmac("Jefe", data, strlen(data), mac);
	check(memcmp(mac, rfc4231, sizeof(mac)) == 0, "RFC 4231 test case 2");

	uint8_t nonce[credentials::NONCE_SIZE], other[credentials::NONCE_SIZE];
	credentials::nonce(nonce);
	credentials::nonce(other);
	check(memcmp(nonce, other, sizeof(nonce)) != 0, "two nonces differ");
	credentials::prove("secret", "join", nonce, mac);
	check(credentials::check("secret", "join", nonce, mac), "the right password checks");
	check(!credentials::check("Secret", "join", nonce, mac), "another password");
	check(!credentials::check("secret", "admin password", nonce, mac), "another label");
	check(!credentials::check("secret", "join", other, mac), "another nonce (a replay)");

	// Known answer: HMAC-SHA256("ban-password-long", "abcd" + 32 zero bytes)
	struct Packet { char Text[4]; uint8_t Proof[credentials::MAC_SIZE]; } packet = {{'a', 'b', 'c', 'd'}, {}};
	const uint8_t signedAbcd[credentials::MAC_SIZE] = {0x4b, 0xaf, 0xf8, 0x26, 0xa5, 0xab, 0xd7, 0x34, 0x03, 0x06, 0x67,
		0x15, 0xad, 0x04, 0x14, 0xec, 0xb5, 0x0c, 0x63, 0xce, 0xde, 0x5e, 0xd5, 0x0d, 0xc2, 0x15, 0x58, 0xfb, 0x02, 0x1b,
		0x24, 0x76};
	credentials::signPacket("ban-password-long", packet);
	check(memcmp(packet.Proof, signedAbcd, sizeof(signedAbcd)) == 0, "signed packet known answer");
	check(credentials::checkPacket("ban-password-long", packet), "a signed packet checks");
	check(!credentials::checkPacket("ban-password-lonG", packet), "another password");
	packet.Text[0] = 'A';
	check(!credentials::checkPacket("ban-password-long", packet), "a changed packet");

	if (!failures) std::printf("ok: credentials\n");
	return failures ? 1 : 0;
}
