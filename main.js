function Pager() {
    this.div = document.createElement("div")
}

Pager.prototype.update = function () {

}

Pager.prototype.get = function () {
    return this.div
}

window.onload = () => {
    const el_pager = document.getElementById("pager")
    const desktopInfo = await getDesktopInfo().catch((err) => { console.log(err) })
    console.log(desktopInfo)
    if (el_pager !== null) {
        let widget_pager = new Pager()
        widget_pager.update()
        el_pager.appendChild(widget_pager.get())
    }
    switchDesktop("1")
}