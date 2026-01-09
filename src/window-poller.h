#ifndef WINDOW_POLLER_TEXTURE_H
#define WINDOW_POLLER_TEXTURE_H

#include <godot_cpp/classes/node.hpp>
#include <unordered_map>

#include <errno.h>
#include <iostream>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

namespace godot {

class WindowPoller : public Node {
  GDCLASS(WindowPoller, Node);

private:
  uint64_t monitor_index = 0;

protected:
  static void _bind_methods();

public:
  WindowPoller();
  ~WindowPoller();

  // Custom methods
  void _physics_process(double delta) override;

  int get_monitor_index();
  void set_monitor_index(int);
};
} // namespace godot

#endif // WINDOW_POLLER_TEXTURE_H
