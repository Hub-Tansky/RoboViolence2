// dkt for the headless server (CONSOLE): there is no GL context, so textures are never uploaded.
// The server still runs game code that loads and deletes textures; these calls do nothing.
#include "dkt.h"

void			 dktInit() {}
void			 dktShutDown() {}
unsigned int	 dktCreateTextureFromFile(char *filename, int filter) { (void)filename; (void)filter; return 0; }
void			 dktDeleteTexture(unsigned int *textureID) { if (textureID) *textureID = 0; }
