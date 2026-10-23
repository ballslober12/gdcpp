// Port of main.js: create the "game" and start with the boot scene.
#include "boot_scene.h"
#include "engine/scene.h"

#ifdef main
#undef main
#endif

int main(int, char**) {
  px::App app;
  if (!app.init()) return 1;
  app.startScene<BootScene>();
  app.run();
  app.shutdown();
  return 0;
}
