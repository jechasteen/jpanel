#include "webview.h"
#include "wmctrl.h"
#include <X11/Xlib.h>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>

#define WEBVIEW_GTK

std::string load_file(std::string path)
{
    std::ifstream index(path);
    index.seekg(0, std::ios::end);
    size_t size = index.tellg();
    std::string buffer(size, ' ');
    index.seekg(0);
    index.read(&buffer[0], size);
    return buffer;
}

// JS lambdas
auto js_print = [](std::string s) -> std::string {
    std::cout << s << std::endl;
    return "";
};

std::string desktop_info = get_desktop_info().dump();
auto json_desktop_info = [](std::string s) -> std::string {
    return desktop_info;
};

auto js_switch_desktop = [](std::string s) -> std::string {
    std::regex e("[\\[,\\],\",\"]");
    int index = stoi(std::regex_replace(s, e, ""));
    switch_desktop(index);
};

int main(int argc, char* argv[])
{
    webview::webview w(true, nullptr);
    w.set_title("jpanel");
    w.set_size(get_screen()->width, 32, WEBVIEW_HINT_FIXED);
    w.set_window_type(GdkWindowTypeHint::GDK_WINDOW_TYPE_HINT_DOCK);
    w.set_decorated(false);
    w.move(0, 0);
    w.stick();

    // bind js functions
    w.bind("print", js_print);
    w.bind("getDesktopInfo", json_desktop_info);
    w.bind("switchDesktop", js_switch_desktop);

    w.navigate(load_file("index.html"));
    w.init(load_file("main.js"));
    list_windows();
    w.run();
    return 0;
}