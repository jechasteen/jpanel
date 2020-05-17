#include <X11/Xlib.h>
#include <iostream>
#include <fstream>
#include "webview.h"

#define WEBVIEW_GTK

std::string concatenateFiles(std::vector<std::string> files) {
    std::string output = "";

    for(auto i = files.begin(); i != files.end(); i++) {
        std::ifstream file;
        std::string line;
        file.open(*i);
        if (file.is_open()) {
            while (getline(file, line)) {
                output += line + "\n";
            }
            output += "\n";
        }
    }
    return output;
}

gboolean updateCallback(void *webview) {
    webview::webview *w = static_cast<webview::webview *>(webview);

    w->eval("window.info = { workspaces: [{ name: 'blah', active: true }] }; window.dispatchEvent(window.update);");
    return TRUE;
}

int main(int argc, char *argv[])
{
    Display *d = XOpenDisplay(NULL);
    Screen *s = DefaultScreenOfDisplay(d);
    
    std::vector<std::string> jsFiles = { "main.js" };
    std::string js = concatenateFiles(jsFiles);

    std::vector<std::string> cssFiles = { "main.css" };
    std::string css = concatenateFiles(cssFiles);

    webview::webview w(true, nullptr);
    w.init(js);
    w.style_sheet(css);
    w.set_title("jpanel");
    w.set_size(s->width, 20, WEBVIEW_HINT_FIXED);
    w.set_window_type(GdkWindowTypeHint::GDK_WINDOW_TYPE_HINT_DOCK);
    w.set_decorated(false);
    w.move(0, 0);
    w.stick();
    w.navigate("file:///home/jechasteen/Repos/panel/index.html");
    
    g_timeout_add(1000, updateCallback, &w);

    w.run();

    return 0;
}