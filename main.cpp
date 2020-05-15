#include <X11/Xlib.h>
#include <iostream>
// #include <iostream>
#include "webview.h"

#define WEBVIEW_GTK

int main(int argc, char *argv[])
{
    std::string url;
    if (argc == 2) {
        url = argv[1];
    } else {
        url = "http://127.0.0.1:1337";
    }

    Display *d = XOpenDisplay(NULL);
    Screen *s = DefaultScreenOfDisplay(d);

    webview::webview w(true, nullptr);
    w.set_title("jpanel");
    w.set_size(s->width, 20, WEBVIEW_HINT_FIXED);
    w.set_window_type(GdkWindowTypeHint::GDK_WINDOW_TYPE_HINT_DOCK);
    w.set_decorated(false);
    w.move(0, 0);
    w.stick();
    w.navigate(url);
    w.run();

    return 0;
}