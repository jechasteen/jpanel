# jpanel

A panel for your tiling window manager that configures like a web page. Configured with Node/Express for the info server and HTML/CSS/JS for ui. Leverages [zserge's webview](https://github.com/zserge/webview) -- with mods to allow sticky, no titlebar, etc -- to create a dock window. WebView uses gtk+3.0 and a nifty WebKit widget renders the page.

This project is in a 'pre-alpha' state, but I would love for people to give it a try. I'm not a great front-end designer, so my results are limited. I'd love to see some screenshots xD

### Prerequisites

* gtk+3.0
* X11 libs
* node/npm

### Install

```shell
make
./install
```

Then make sure that you add two commands to your WM's auto start apps:

```
/usr/local/bin/jpanel
nodemon ~/.config/jpanel/server.js
```

Note: you'll have to install nodemon via `npm i -g nodemon` to run it this way.