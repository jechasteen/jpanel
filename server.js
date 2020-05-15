const express = require('express');
const app = express();
const exec = require('child_process').execSync;
const WebSocket = require("ws");
const wss = new WebSocket.Server({ port: 8080});

const getWorkspaceInfo = () => {
    const workspaceInfo = exec("wmctrl -d").toString().split('\n');
    workspaceInfo.pop();
    let workspaces = []
    for (let i = 0; i < workspaceInfo.length; i++) {
        let w = {};
        let current = workspaceInfo[i].split(' ');
        w.index = parseInt(current.shift());
        current.shift();
        w.active = current[0] === '*' ? true : false;
        current.shift();
        current.shift();
        w.desktopGeometry = current.shift();
        current.shift();
        current.shift();
        w.viewportPosition = current.shift();
        current.shift();
        current.shift();
        w.workArea = current.shift();
        w.name = current.join(' ').trim();
        workspaces.push(w);
    }
    return workspaces;
}

const getTasklist = () => {
    let rawClients = exec("wmctrl -l").toString().split('\n');
    let tasklist = [];
    rawClients.pop();
    for (let i = 0; i < rawClients.length; i++) {
        let c = rawClients[i].split(' ');
        let client = {};
        client.id = c.shift();
        c.shift();
        client.workspace = parseInt(c.shift());
        client.machine = c.shift();
        client.name = c.join(' ').trim();
        tasklist.push(client);
    }
    return tasklist;
};

app.use(express.static('public'))

app.get('/', (req, res) => {
    res.sendFile(__dirname + '/index.html');
});

app.get('/info', (req, res) => {
    let info = {
        time: Date.now(),
        workspaces: getWorkspaceInfo(),
        tasklist: getTasklist()
    }
    res.json(info);
});

app.get('/pager/switch/:id', (req, res) => {
    exec("wmctrl -s " + req.params.id);
    res.send("ok");
});

app.get('/tasklist/focus/:id', (req, res) => {
    exec("wmctrl -a " + req.params.id);
    res.send("ok");
})

app.listen(1337, '127.0.0.1');