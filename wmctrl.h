// Functions adapted from the very useful wmctrl source
#pragma once
#pragma GCC diagnostic ignored "-Wwrite-strings"

#include "json.hpp"
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <glib.h>
#include <iostream>
#include <string>

using json = nlohmann::json;

#define MAX_PROPERTY_LEN 4096

static int client_msg(Display* disp, Window win, char* msg, /* {{{ */
    unsigned long data0, unsigned long data1,
    unsigned long data2, unsigned long data3,
    unsigned long data4)
{
    XEvent event;
    long mask = SubstructureRedirectMask | SubstructureNotifyMask;

    event.xclient.type = ClientMessage;
    event.xclient.serial = 0;
    event.xclient.send_event = True;
    event.xclient.message_type = XInternAtom(disp, msg, False);
    event.xclient.window = win;
    event.xclient.format = 32;
    event.xclient.data.l[0] = data0;
    event.xclient.data.l[1] = data1;
    event.xclient.data.l[2] = data2;
    event.xclient.data.l[3] = data3;
    event.xclient.data.l[4] = data4;

    if (XSendEvent(disp, DefaultRootWindow(disp), False, mask, &event)) {
        return EXIT_SUCCESS;
    } else {
        std::cerr << "Cannot send " << msg << " event." << std::endl;
        return EXIT_FAILURE;
    }
}

static char* get_property(Display* display, Window window,
    Atom xa_prop_type, char* prop_name, ulong* size)
{
    Atom xa_ret_type;
    int ret_format;
    ulong ret_nitems;
    ulong ret_bytes_after;
    ulong tmp_size;
    u_char* ret_prop;
    char* ret;

    Atom xa_prop_name = XInternAtom(display, prop_name, False);

    if (XGetWindowProperty(display, window, xa_prop_name, 0, MAX_PROPERTY_LEN / 4,
            False, xa_prop_type, &xa_ret_type, &ret_format, &ret_nitems,
            &ret_bytes_after, &ret_prop)
        != Success) {
        std::cerr << "Cannot get " << prop_name << " property." << std::endl;
        return nullptr;
    }

    if (xa_ret_type != xa_prop_type) {
        std::cerr << "Invalid type of " << prop_name << " property." << std::endl;
        XFree(ret_prop);
    }

    // null terminate the result to make string handling easier
    tmp_size = (ret_format / (32 / sizeof(long))) * ret_nitems;
    ret = (char*)malloc(tmp_size + 1);
    memcpy(ret, ret_prop, tmp_size);
    ret[tmp_size] = '\0';

    if (size) {
        *size = tmp_size;
    }

    XFree(ret_prop);
    return ret;
}

static gchar* get_output_str(gchar* str, gboolean is_utf8)
{
    gchar* out;

    if (str == NULL) {
        return NULL;
    }

    if (g_get_charset(nullptr)) {
        if (is_utf8) {
            out = g_strdup(str);
        } else {
            if (!(out = g_locale_to_utf8(str, -1, NULL, NULL, NULL))) {
                std::cerr << "Cannot convert string from locale charset to UTF-8." << std::endl;
                out = g_strdup(str);
            }
        }
    } else {
        if (is_utf8) {
            if (!(out = g_locale_from_utf8(str, -1, NULL, NULL, NULL))) {
                std::cerr << "Cannot convert string from UTF-8 to locale charset." << std::endl;
                out = g_strdup(str);
            }
        } else {
            out = g_strdup(str);
        }
    }

    return out;
}

static int longest_str(gchar** strv)
{
    int max = 0;
    int i = 0;

    while (strv && strv[i]) {
        if (strlen(strv[i]) > max) {
            max = strlen(strv[i]);
        }
        i++;
    }

    return max;
}

static Screen* get_screen()
{
    Display* m_display = XOpenDisplay(NULL);
    Screen* screen = DefaultScreenOfDisplay(m_display);
    XCloseDisplay(m_display);
    return screen;
}

static ulong* get_num_desktops(Display* display)
{
    ulong* num_desktops;
    Window root = DefaultRootWindow(display);
    if (!(num_desktops = (ulong*)get_property(display, root,
              XA_CARDINAL, "_NET_NUMBER_OF_DESKTOPS", NULL))) {
        if (!(num_desktops = (ulong*)get_property(display, root,
                  XA_CARDINAL, "WIN_WORKSPACE_COUNT", NULL))) {
            std::cerr << "Failed to get number of desktops! "
                      << "(_NET_NUMBER_OF_DESKTOPS or _WIN_WORKSPACE_COUNT)" << std::endl;
        }
    }
    return num_desktops;
}

static json get_desktop_info()
{
    Display* m_display = XOpenDisplay(NULL);
    Screen* m_screen = DefaultScreenOfDisplay(m_display);
    Window m_root = DefaultRootWindow(m_display);

    ulong* cur_desktop = NULL;
    ulong desktop_list_size = 0;

    ulong* desktop_geometry = NULL;
    ulong desktop_geometry_size = 0;
    gchar** desktop_geometry_str = NULL;

    ulong* desktop_viewport = NULL;
    ulong desktop_viewport_size = 0;
    gchar** desktop_viewport_str = NULL;

    ulong* desktop_workarea = NULL;
    ulong desktop_workarea_size = 0;
    gchar** desktop_workarea_str = NULL;

    gchar* list = NULL;
    int i;
    int id;
    json ret;
    gchar** names = NULL;
    gboolean names_are_utf8 = TRUE;

    ulong* num_desktops = get_num_desktops(m_display);

    if (!(cur_desktop = (ulong*)get_property(m_display, m_root,
              XA_CARDINAL, "_NET_CURRENT_DESKTOP", NULL))) {
        if (!(cur_desktop = (ulong*)get_property(m_display, m_root,
                  XA_CARDINAL, "_WIN_WORKSPACE", NULL))) {
            std::cerr << "Failed to get current desktop properties. "
                      << "(_NET_CURRENT_DESKTOP or _WIN_WORKSPACE)" << std::endl;
            goto cleanup;
        }
    }

    if ((list = get_property(m_display, m_root, XInternAtom(m_display, "UTF8_STRING", False), "_NET_DESKTOP_NAMES", &desktop_list_size)) == NULL) {
        names_are_utf8 = FALSE;
        if ((list = get_property(m_display, m_root, XA_STRING, "_WIN_WORKSPACE_NAMES", &desktop_list_size)) == NULL) {
            std::cerr << "Failed to get desktop names properties. "
                      << "(_NET_DESKTOP_NAMES or _WIN_WORKSPACE_NAMES)" << std::endl;
            // Ignore the error and give no desktop names
        }
    }

    // Common size of all desktops
    if (!(desktop_geometry = (ulong*)get_property(m_display, m_root, XA_CARDINAL, "_NET_DESKTOP_GEOMETRY", &desktop_geometry_size))) {
        std::cerr << "Failed to get common size of all desktops (_NET_DESKTOP_GEOMETRY)." << std::endl;
    }

    // desktop viewport
    if (!(desktop_viewport = (ulong*)get_property(m_display, m_root, XA_CARDINAL, "_NET_DESKTOP_VIEWPORT", &desktop_viewport_size))) {
        std::cerr << "Failed to get common size of all desktops (_NET_DESKTOP_VIEWPORT)." << std::endl;
    }

    // desktop workarea
    if (!(desktop_workarea = (ulong*)get_property(m_display, m_root, XA_CARDINAL, "_NET_WORKAREA", &desktop_workarea_size))) {
        if (!(desktop_workarea = (ulong*)get_property(m_display, m_root, XA_CARDINAL, "_WIN_WORKAREA", &desktop_workarea_size))) {
            std::cerr << "Failed to get workarea size (_NET_WORKAREA or _WIN_WORKAREA) property." << std::endl;
        }
    }

    // prepare the array of desktop names
    names = (gchar**)g_malloc0(*num_desktops * sizeof(char*));
    if (list) {
        id = 0;
        names[id++] = list;
        for (i = 0; i < desktop_list_size; i++) {
            if (list[i] == '\0') {
                if (id >= *num_desktops) {
                    break;
                }
                names[id++] = list + i + 1;
            }
        }
    }

    // prepare desktop geometry strings
    desktop_geometry_str = (char**)g_malloc0((*num_desktops + 1) * sizeof(char*));
    if (desktop_geometry && desktop_geometry_size > 0) {
        if (desktop_geometry_size == 2 * sizeof(*desktop_geometry)) {
            // only one value for all desktops
            for (i = 0; i < *num_desktops; i++) {
                desktop_geometry_str[i] = g_strdup_printf("%lux%lu", desktop_geometry[0], desktop_geometry[1]);
            }
        } else {
            // separate values for desktops of different size
            for (i = 0; i < *num_desktops; i++) {
                if (i < desktop_geometry_size / sizeof(*desktop_geometry) / 2) {
                    desktop_geometry_str[i] = g_strdup_printf("%lux%lu", desktop_geometry[i * 2], desktop_geometry[i * 2 + 1]);
                } else {
                    desktop_geometry_str[i] = g_strdup("N/A");
                }
            }
        }
    } else {
        for (i = 0; i < *num_desktops; i++) {
            desktop_geometry_str[i] = g_strdup("N/A");
        }
    }

    // prepare desktop viewport strings
    desktop_viewport_str = (char**)g_malloc0((*num_desktops + 1) * sizeof(char*));
    if (desktop_viewport && desktop_viewport_size > 0) {
        if (desktop_viewport_size == 2 * sizeof(*desktop_viewport)) {
            // only one value - use it for current desktop
            for (i = 0; i < *num_desktops; i++) {
                if (i == *cur_desktop) {
                    desktop_viewport_str[i] = g_strdup_printf("%lux%lu", desktop_viewport[0], desktop_viewport[1]);
                } else {
                    desktop_viewport_str[i] = g_strdup("N/A");
                }
            }
        } else {
            // separate values for each of desktops
            for (i = 0; i < *num_desktops; i++) {
                if (i < desktop_viewport_size / sizeof(*desktop_viewport) / 2) {
                    desktop_viewport_str[i] = g_strdup_printf("%lu,%lu", desktop_viewport[i * 2], desktop_viewport[i * 2 + 1]);
                } else {
                    desktop_viewport_str[i] = g_strdup("N/A");
                }
            }
        }
    } else {
        for (i = 0; i < *num_desktops; i++) {
            desktop_viewport_str[i] = g_strdup("N/A");
        }
    }

    // prepare desktop workarea strings
    desktop_workarea_str = (gchar**)g_malloc0((*num_desktops + 1) * sizeof(char*));
    if (desktop_workarea && desktop_workarea_size > 0) {
        if (desktop_workarea_size == 4 * sizeof(*desktop_workarea)) {
            // only one value - use it for current desktop
            for (i = 0; i < *num_desktops; i++) {
                if (i == *cur_desktop) {
                    desktop_workarea_str[i] = g_strdup_printf("%lu,%lu %lux%lu",
                        desktop_workarea[0], desktop_workarea[1],
                        desktop_workarea[2], desktop_workarea[3]);
                } else {
                    desktop_workarea_str[i] = g_strdup("N/A");
                }
            }
        } else {
            // separate values for each desktop
            for (i = 0; i < *num_desktops; i++) {
                if (i < desktop_workarea_size / sizeof(*desktop_workarea) / 4) {
                    desktop_workarea_str[i] = g_strdup_printf("%lu,%lu %lux%lu",
                        desktop_workarea[i * 4], desktop_workarea[i * 4 + 1],
                        desktop_workarea[i * 4 + 2], desktop_workarea[i * 4 + 3]);
                } else {
                    desktop_workarea_str[i] = g_strdup("N/A");
                }
            }
        }
    } else {
        for (i = 0; i < *num_desktops; i++) {
            desktop_workarea_str[i] = g_strdup("N/A");
        }
    }

    // build json
    for (i = 0; i < *num_desktops; i++) {
        gchar* name = get_output_str(names[i], names_are_utf8);
        if (i == *cur_desktop)
            ret["current"] = i;
        ret["desktops"][i] = {
            { "name", name ? name : "N/A" },
            { "geometry", desktop_geometry_str[i] },
            { "viewport", desktop_viewport_str[i] },
            { "workarea", desktop_workarea_str[i] }
        };
    }

    goto cleanup;
cleanup:
    g_free(names);
    g_free(num_desktops);
    g_free(cur_desktop);
    g_free(desktop_geometry);
    g_strfreev(desktop_geometry_str);
    g_free(desktop_viewport);
    g_strfreev(desktop_viewport_str);
    g_free(desktop_workarea);
    g_strfreev(desktop_workarea_str);
    g_free(list);
    XCloseDisplay(m_display);

    return ret;
}

static int switch_desktop(int index)
{
    if (index < 0) {
        std::cerr << "Invalid desktop ID: " << index << std::endl;
        return EXIT_FAILURE;
    }
    Display* display = XOpenDisplay(NULL);
    int result = client_msg(display, DefaultRootWindow(display), "_NET_CURRENT_DESKTOP",
        (ulong)index, 0, 0, 0, 0);
    XCloseDisplay(display);
    return result;
}

static Window* get_client_list(Display* disp, unsigned long* size)
{
    Window* client_list;

    if ((client_list = (Window*)get_property(disp, DefaultRootWindow(disp),
             XA_WINDOW, "_NET_CLIENT_LIST", size))
        == NULL) {
        if ((client_list = (Window*)get_property(disp, DefaultRootWindow(disp),
                 XA_CARDINAL, "_WIN_CLIENT_LIST", size))
            == NULL) {
            std::cerr << "Failed to get client list." << std::endl
                      << "(_NET_CLIENT_LIST or _WIN_CLIENT_LIST)" << std::endl;
            return NULL;
        }
    }

    return client_list;
}

static gchar* get_window_title(Display* disp, Window win)
{
    gchar* title_utf8;
    gchar* wm_name;
    gchar* net_wm_name;

    wm_name = get_property(disp, win, XA_STRING, "WM_NAME", NULL);
    net_wm_name = get_property(disp, win,
        XInternAtom(disp, "UTF8_STRING", False), "_NET_WM_NAME", NULL);

    if (net_wm_name) {
        title_utf8 = g_strdup(net_wm_name);
    } else {
        if (wm_name) {
            title_utf8 = g_locale_to_utf8(wm_name, -1, NULL, NULL, NULL);
        } else {
            title_utf8 = NULL;
        }
    }

    g_free(wm_name);
    g_free(net_wm_name);

    return title_utf8;
}

static gchar* get_window_class(Display* disp, Window win)
{
    gchar* class_utf8;
    gchar* wm_class;
    unsigned long size;

    wm_class = get_property(disp, win, XA_STRING, "WM_CLASS", &size);
    if (wm_class) {
        gchar* p_0 = strchr(wm_class, '\0');
        if (wm_class + size - 1 > p_0) {
            *(p_0) = '.';
        }
        class_utf8 = g_locale_to_utf8(wm_class, -1, NULL, NULL, NULL);
    } else {
        class_utf8 = NULL;
    }

    g_free(wm_class);

    return class_utf8;
}

static int list_windows()
{
    Display* display = XOpenDisplay(NULL);
    Window* client_list;
    ulong client_list_size;
    int i;
    int max_client_machine_len = 0;

    if ((client_list = get_client_list(display, &client_list_size)) == NULL) {
        return EXIT_FAILURE;
    }

    for (i = 0; i < client_list_size / sizeof(Window); i++) {
        gchar* client_machine;
        if ((client_machine = get_property(display, client_list[i],
                 XA_STRING, "WM_CLIENT_MACHINE", NULL))) {
            max_client_machine_len = strlen(client_machine);
        }
        g_free(client_machine);
    }

    // print the list
    for (i = 0; i < client_list_size / sizeof(Window); i++) {
        std::cout << *client_list[i] << std::endl;
        gchar* title_utf8 = get_window_title(display, client_list[i]);
        gchar* title_out = get_output_str(title_utf8, TRUE);
        gchar* client_machine;
        gchar* class_out = get_window_class(display, client_list[i]);
        ulong* pid;
        ulong* desktop;
        int x, y, junkx, junky;
        uint wwidth, wheight, bw, depth;
        Window junkroot;

        // Desktop ID
        if ((desktop = (ulong*)get_property(display, client_list[i],
                 XA_CARDINAL, "_NET_WM_DESKTOP", NULL))
            == NULL) {
            desktop = (ulong*)get_property(display, client_list[i],
                XA_CARDINAL, "_WIN_WORKSPACE", NULL);
        }

        // Client Machine
        client_machine = get_property(display, client_list[i],
                XA_STRING, "WM_CLIENT_MACHINE", NULL);

        // pid
        pid = (ulong*)get_property(display, client_list[i],
                XA_CARDINAL, "_NET_WM_PID", NULL);

        // geometry
        XGetGeometry(display, client_list[i], &junkroot, &junkx, &junky,
                &wwidth, &wheight, &bw, &depth);
        XTranslateCoordinates(display, client_list[i], junkroot, junkx, junky,
                &x, &y, &junkroot);

        // Special desktop id -1 means "all desktops," so we have to convert
        // the desktop value to signed long
        printf("0x%.8lx %2ld", client_list[i], desktop ? (long)*desktop : 0);
        printf(" %-6lu", pid ? *pid : 0);
        printf(" %-4d %-4d %-4d %-4d", x, y, wwidth, wheight);
        printf(" %-20s ", class_out ? class_out : "N/A");
        printf(" %*s %s\n",
            max_client_machine_len,
            client_machine ? client_machine : "N/A",
            title_out ? title_out : "N/A");
        
        g_free(title_utf8);
        g_free(title_out);
        g_free(desktop);
        g_free(client_machine);
        g_free(class_out);
        g_free(pid);
    }
    g_free(client_list);

    XCloseDisplay(display);
    return EXIT_SUCCESS;
}