#include "xhelpers.h"
#include "x11-window-capture.hpp"

using namespace godot;

static Display *disp = NULL;
static xcb_connection_t *conn = NULL;
// Atoms used throughout our plugin
static xcb_atom_t ATOM_UTF8_STRING;
static xcb_atom_t ATOM_STRING;
static xcb_atom_t ATOM_TEXT;
static xcb_atom_t ATOM_COMPOUND_TEXT;
static xcb_atom_t ATOM_WM_NAME;
static xcb_atom_t ATOM_WM_CLASS;
static xcb_atom_t ATOM__NET_WM_NAME;
static xcb_atom_t ATOM__NET_SUPPORTING_WM_CHECK;
static xcb_atom_t ATOM__NET_CLIENT_LIST;


