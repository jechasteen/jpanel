// main.js -- browser side script

function httpGet(url)
{
    var xmlHttp = new XMLHttpRequest();
    xmlHttp.open( "GET", url, false ); // false for synchronous request
    xmlHttp.send( null );
    return xmlHttp.responseText;
}

const mods = {};
/**
 * Workspace Pager Mod
 * @class pager-workspace
 * @class pager-workspace-active
 */
mods.pager = {}
mods.pager.enabled = false;
mods.pager.activeWorkspace = 0;
mods.pager.insert = (w, el) => {
    mods.pager.enabled = true;

    const switcher = (e) => {
        const name = e.target.innerText;
        for (let i = 0; i < w.length; i++) {
            if (w[i].name == name) {
                httpGet("/pager/switch/" + w[i].index);
                return;
            }
        }
    }

    el.innerHTML = "";
    for (let i = 0; i < w.length; i++) {
        workspace = document.createElement("div");
        workspace.classList.add("pager-workspace");
        if (w[i].active) {
            workspace.classList.add("pager-workspace-active");
            mods.pager.activeWorkspace = i;
        }
        workspace.innerText = w[i].name;
        el.appendChild(workspace);
    }
    for (let i = 0; i < el.childNodes.length; i++) {
        el.childNodes[i].addEventListener("click", switcher);
    }
}

/**
 * Tasklist Mod
 * @class tasklist
 * @class tasklist-task
 */
mods.tasklist = (t, el) => {
    const focusTask = (e) => {
        const task = e.target.id;
        console.log(task);
        for (let i = 0; i < t.length; i++) {
            if (t[i].name == task) {
                httpGet('/tasklist/focus/' + t[i].name);
            }
        }
    }

    const truncate = (name, maxLength) => {
        return name.slice(0, maxLength - 3) + "...";
    }

    const createTask = (name) => {
        let task = document.createElement("div");
        task.classList.add("tasklist-task");
        task.innerText = truncate(name, 32);
        task.id = name;
        el.appendChild(task);
    }

    el.innerHTML = "";
    el.classList.add("tasklist");
    for (let i = 0; i < t.length; i++) {
        if (t[i].name != "jpanel") {
            createTask(t[i].name);
        }
    }
    for (let i = 0; i < el.childNodes.length; i++) {
        el.childNodes[i].addEventListener("click", focusTask);
    }
};

/**
 * Clock Mod
 * @param el the element in which to insert the clock
 */
mods.clock = (el) => {
    el.innerText = new Date();
};

window.onload = () => {
    setInterval(() => {
        const info = JSON.parse(httpGet('/info'));
        mods.pager.insert(info.workspaces, document.getElementById("pager"));
        mods.tasklist(info.tasklist, document.getElementById("tasklist"));
        mods.clock(document.getElementById("time"));
    }, 1000);
}