#ifndef WINDOW_POLLER_TEXTURE_H
#define WINDOW_POLLER_TEXTURE_H

#include <godot_cpp/classes/node.hpp>
#include <unordered_map>
#include "pch.h"

const int TTL_FRAMES = 5;

struct ActiveCapture
{
    int ttl = TTL_FRAMES;
};

namespace godot
{

    class WindowPoller : public Node
    {
        GDCLASS(WindowPoller, Node);

    private:
        std::unordered_map<HWND, ::ActiveCapture> activeCaptures;
        uint64_t monitor_index = 0;
        HMONITOR monitor;
        HWND lastActive;

    protected:
        static void
        _bind_methods();

    public:
        WindowPoller();
        ~WindowPoller();

        // Custom methods
        void _physics_process(double delta) override;

        int get_monitor_index();
        void set_monitor_index(int);
    };
}

#endif // WINDOW_POLLER_TEXTURE_H