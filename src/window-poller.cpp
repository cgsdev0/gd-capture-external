#include "window-poller.h"
#include <godot_cpp/core/class_db.hpp>

#include <godot_cpp/classes/rd_texture_format.hpp>
#include <godot_cpp/classes/rd_texture_view.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "ext-foreign-toplevel-list-v1-client-protocol.h"
#include "ext-image-capture-source-v1-client-protocol.h"
#include "ext-image-copy-capture-v1-client-protocol.h"

extern "C" {
#include "sway-ipc-client.h"
}

#include "sway-structs.h"

#define WL_OUTPUT_VERSION 4

enum event_loop_fd {
  EVENT_LOOP_WAYLAND,
  EVENT_LOOP_SWAYIPC,
};
static struct wl_display *wl_display;
static struct ext_foreign_toplevel_image_capture_source_manager_v1 *manager;
static struct ext_image_copy_capture_manager_v1 *manager2;
static const struct ext_foreign_toplevel_handle_v1 *handle;

static void
foreign_toplevel_handle_closed(void *data,
                               struct ext_foreign_toplevel_handle_v1 *handle) {}

static void
foreign_toplevel_handle_done(void *data,
                             struct ext_foreign_toplevel_handle_v1 *handle) {}

static void
foreign_toplevel_handle_title(void *data,
                              struct ext_foreign_toplevel_handle_v1 *handle,
                              const char *title) {
  printf("title: %s\n", title);
}

static void
foreign_toplevel_handle_app_id(void *data,
                               struct ext_foreign_toplevel_handle_v1 *handle,
                               const char *app_id) {
  printf("app_id: %s\n", app_id);
}

static void foreign_toplevel_handle_identifier(
    void *data, struct ext_foreign_toplevel_handle_v1 *handle,
    const char *identifier) {
  printf("id: %s\n", identifier);
}

static const struct ext_foreign_toplevel_handle_v1_listener
    foreign_toplevel_handle_listener = {
        .closed = foreign_toplevel_handle_closed,
        .done = foreign_toplevel_handle_done,
        .title = foreign_toplevel_handle_title,
        .app_id = foreign_toplevel_handle_app_id,
        .identifier = foreign_toplevel_handle_identifier,
};

static void ext_session_buffer_size(
    void *data,
    struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
    uint32_t width, uint32_t height) {
  printf("got buffer size: %u x %u\n", width, height);
}

static void ext_session_shm_format(
    void *data,
    struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
    uint32_t format) {
  printf("got shm format: %u\n", format);
}

static void ext_session_dmabuf_device(
    void *data,
    struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
    struct wl_array *device_arr) {
  printf("got dmabuf device\n");
}

static void ext_session_dmabuf_format(
    void *data,
    struct ext_image_copy_capture_session_v1 *ext_image_copy_capture_session_v1,
    uint32_t format, struct wl_array *modifiers) {
  printf("got dmabuf format: %u\n", format);
}

static void ext_session_done(void *data,
                             struct ext_image_copy_capture_session_v1
                                 *ext_image_copy_capture_session_v1) {
  printf("session done\n");
}

static void ext_session_stopped(void *data,
                                struct ext_image_copy_capture_session_v1
                                    *ext_image_copy_capture_session_v1) {
  printf("session stopped\n");
}

static const struct ext_image_copy_capture_session_v1_listener
    ext_session_listener = {
        .buffer_size = ext_session_buffer_size,
        .shm_format = ext_session_shm_format,
        .dmabuf_device = ext_session_dmabuf_device,
        .dmabuf_format = ext_session_dmabuf_format,
        .done = ext_session_done,
        .stopped = ext_session_stopped,
};

static void foreign_toplevel_list_handle_toplevel(
    void *data, struct ext_foreign_toplevel_list_v1 *list,
    struct ext_foreign_toplevel_handle_v1 *handle) {
  // if (!demo) {
  //   printf("toplevel_handle: %x\n", handle);
  //   demo = true;

  //   struct ext_image_capture_source_v1 *source = NULL;
  //   source =
  //   ext_foreign_toplevel_image_capture_source_manager_v1_create_source(
  //       manager, handle);
  //   if (source != NULL) {
  //     printf("we got a source: %x\n", source);
  //     auto *session = ext_image_copy_capture_manager_v1_create_session(
  //         manager2, source,
  //         EXT_IMAGE_COPY_CAPTURE_MANAGER_V1_OPTIONS_PAINT_CURSORS);
  //     ext_image_copy_capture_session_v1_add_listener(
  //         session, &ext_session_listener, NULL);

  //     printf("we got a session: %x\n", session);
  //     wl_display_roundtrip(wl_display);
  //   }
  // }
  ext_foreign_toplevel_handle_v1_add_listener(
      handle, &foreign_toplevel_handle_listener, NULL);
}

static void foreign_toplevel_list_handle_finished(
    void *data, struct ext_foreign_toplevel_list_v1 *list) {
  // printf("finished\n");
  // wl_list_for_each_safe(toplevel, toplevel_tmp, &ctx->toplevels, link) {
  // 	foreign_toplevel_destroy(toplevel);
  // }

  // ext_foreign_toplevel_list_v1_destroy(ctx->ext_foreign_toplevel_list);
  // ctx->ext_foreign_toplevel_list = NULL;
}

static const struct ext_foreign_toplevel_list_v1_listener
    foreign_toplevel_list_listener = {
        .toplevel = foreign_toplevel_list_handle_toplevel,
        .finished = foreign_toplevel_list_handle_finished,
};

struct ext_foreign_toplevel_list_v1 *ext_foreign_toplevel_list;

static void wlr_registry_handle_add(void *data, struct wl_registry *reg,
                                    uint32_t id, const char *interface,
                                    uint32_t ver) {

  if (!strcmp(interface,
              ext_foreign_toplevel_image_capture_source_manager_v1_interface
                  .name)) {
    // printf("got manager: %s\n", interface);
    manager = reinterpret_cast<
        ext_foreign_toplevel_image_capture_source_manager_v1 *>(
        wl_registry_bind(
            reg, id,
            &ext_foreign_toplevel_image_capture_source_manager_v1_interface,
            1));
  }
  if (!strcmp(interface, ext_image_copy_capture_manager_v1_interface.name)) {
    // printf("got manager: %s\n", interface);
    manager2 =
        reinterpret_cast<ext_image_copy_capture_manager_v1 *>(wl_registry_bind(
            reg, id, &ext_image_copy_capture_manager_v1_interface, 1));
  }

  if (!strcmp(interface, ext_foreign_toplevel_list_v1_interface.name)) {
    // printf("got manager: %s\n", interface);
    ext_foreign_toplevel_list =
        reinterpret_cast<ext_foreign_toplevel_list_v1 *>(wl_registry_bind(
            reg, id, &ext_foreign_toplevel_list_v1_interface, 1));
    ext_foreign_toplevel_list_v1_add_listener(
        ext_foreign_toplevel_list, &foreign_toplevel_list_listener, NULL);
  }
}

static void wlr_registry_handle_remove(void *data, struct wl_registry *reg,
                                       uint32_t id) {
  // struct xdpw_screencast_context *ctx = data;
  // struct xdpw_wlr_output *output = xdpw_wlr_output_find(ctx, NULL, id);
  // if (output) {
  // 	logprint(DEBUG, "wlroots: output removed (%s)", output->name);
  // 	struct xdpw_screencast_instance *cast, *tmp;
  // 	wl_list_for_each_safe(cast, tmp, &ctx->screencast_instances, link) {
  // 		if (cast->target->output == output) {
  // 			// screencopy might be in process for this instance
  // 			xdpw_screencast_instance_destroy(cast);
  // 		}
  // 	}
  // 	wlr_remove_output(output);
  // }
}

static const struct wl_registry_listener wlr_registry_listener = {
    .global = wlr_registry_handle_add,
    .global_remove = wlr_registry_handle_remove,
};

void print_node(sway::Node &n, std::string indent = "") {
  // if (n.type == "output" && n.name != "DP-2") return;
  // if (n.type == "con") {
  std::cout << indent << n.type << std::endl;
  // }
  indent += "  ";
  for (auto &child : n.nodes) {
    print_node(child, indent);
  }
}
void godot::WindowPoller::_bind_methods() {
  godot::ClassDB::bind_method(
      godot::D_METHOD("set_monitor_index", "should_capture"),
      &WindowPoller::set_monitor_index);
  godot::ClassDB::bind_method(godot::D_METHOD("get_monitor_index"),
                              &WindowPoller::get_monitor_index);

  ADD_PROPERTY(PropertyInfo(Variant::INT, "monitor_index"), "set_monitor_index",
               "get_monitor_index");

  ADD_SIGNAL(MethodInfo("window_opened", PropertyInfo(Variant::INT, "hwnd"),
                        PropertyInfo(Variant::STRING, "title"),
                        PropertyInfo(Variant::STRING, "class_type")));
  ADD_SIGNAL(MethodInfo("window_closed", PropertyInfo(Variant::INT, "hwnd")));
  ADD_SIGNAL(MethodInfo("focus_change", PropertyInfo(Variant::INT, "hwnd")));
}

void godot::WindowPoller::set_monitor_index(int monitor_index) {
  if (monitor_index < 0)
    return;
  this->monitor_index = monitor_index;
}

int godot::WindowPoller::get_monitor_index() { return this->monitor_index; }

void godot::WindowPoller::_physics_process(double delta) {}

godot::WindowPoller::WindowPoller() {

  wl_display = wl_display_connect(NULL);
  struct wl_registry *registry = wl_display_get_registry(wl_display);
  wl_registry_add_listener(registry, &wlr_registry_listener, NULL);
  int wl_fd = wl_display_get_fd(wl_display);

  wl_display_dispatch(wl_display);
  wl_display_roundtrip(wl_display);

  int sway_fd = ipc_open_socket(get_socketpath());

  struct pollfd pollfds[] = {
      [EVENT_LOOP_WAYLAND] =
          {
              .fd = wl_fd,
              .events = POLLIN,
          },
      [EVENT_LOOP_SWAYIPC] =
          {
              .fd = sway_fd,
              .events = POLLIN,
          },
  };

  uint32_t len = 0;
  char *tree = ipc_single_command(sway_fd, IPC_GET_TREE, "", &len);
  auto thing = sway::parse_node(tree);
  print_node(thing);
  free(tree, this);
  std::string command = "[\"window\"]";
  len = command.length();
  char *subscribe =
      ipc_single_command(sway_fd, IPC_SUBSCRIBE, command.c_str(), &len);
  printf("SUBSCRIBE: %s\n", subscribe);
  free(subscribe, this);
  int ret;
  while ("true") {
    ret = poll(pollfds, sizeof(pollfds) / sizeof(pollfds[0]), -1);

    if (pollfds[EVENT_LOOP_WAYLAND].revents & POLLHUP) {
      printf("event-loop: disconnected from wayland\n");
      break;
    }

    if (pollfds[EVENT_LOOP_SWAYIPC].revents & POLLHUP) {
      printf("event-loop: disconnected from sway IPC\n");
      break;
    }

    if (pollfds[EVENT_LOOP_WAYLAND].revents & POLLIN) {
      ret = wl_display_dispatch(wl_display);
      if (ret < 0) {
        printf("wl_display_dispatch failed: %s\n", strerror(errno));
        break;
      }
    }
    if (pollfds[EVENT_LOOP_SWAYIPC].revents & POLLIN) {
      auto *resp = ipc_recv_response(sway_fd);
      auto change = sway::parse_change(resp->payload);
      std::cout << change.change << std::endl;
      print_node(change.container);
      free(resp, this);
    }
    do {
      ret = wl_display_dispatch_pending(wl_display);
      wl_display_flush(wl_display);
    } while (ret > 0);
  }

  // wl_display_roundtrip(wl_display);
  // wl_display_flush(wl_display);
  // char buf[512];
  // ssize_t bytes;
  // while(-1 != (bytes = read(fd, &buf, sizeof(buf)))) {
  // 	if (bytes) {
  // 		printf("got %ld bytes\n", bytes);
  // 		wl_display_dispatch(wl_display);
  // 	}
  // }
  printf("goodbye\n");
}

godot::WindowPoller::~WindowPoller() {}
