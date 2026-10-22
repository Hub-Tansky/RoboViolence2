// dkwSetWindowIcon with the Linux client's compiled-in icon (AppIcon.h, ADR 0030): the built-in PNG decodes, so the
// call succeeds even with no window to take it (best effort); bytes that aren't a PNG make it fail.
#include "AppIcon.h"
#include "dkw.h"

#include <stdio.h>

int main()
{
	if (!dkwSetWindowIcon(g_appIconPng, sizeof(g_appIconPng)))
	{
		printf("FAIL: the built-in window icon doesn't decode: %s\n", dkwGetLastError());
		return 1;
	}
	static const unsigned char notPng[] = {0x89, 'P', 'N', 'G', 0, 1, 2, 3};
	if (dkwSetWindowIcon(notPng, sizeof(notPng)))
	{
		printf("FAIL: dkwSetWindowIcon accepted bytes that are not a PNG\n");
		return 1;
	}
	printf("ok: built-in window icon decodes; garbage is refused\n");
	return 0;
}
