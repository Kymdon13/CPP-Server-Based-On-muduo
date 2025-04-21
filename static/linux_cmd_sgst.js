const sortArray = function (a, b) {
    return a.nIdx - b.nIdx
};
const markdownit = window.markdownit;
sg = new class suggest {
    query = "";
    query_size = 10;
    constructor() {
        function f(elmid) { return document.getElementById(elmid) }
        this.commands = linux_commands,
            this.elm_query = document.getElementById("query"),
            this.elm_result = document.getElementById("result"),
            this.md = document.getElementById("markdown-content"),
            this.cur_cmd = document.getElementById("cur_cmd"),
            this.init()
    }
    isSreachIndexOF(cmdNameOrDesc, query) {//如果e和t存在，返回t在e中的位置
        return cmdNameOrDesc && query ? cmdNameOrDesc.toLowerCase().indexOf(query.toLowerCase()) : -1
    }
    simple(template, alter) {//匹配模板字符串template中的$xxx$，替换为alter[xxx]
        return template.replace(/\$\w+\$/gi, function (temp) {
            temp = alter[temp.replace(/\$/g, "")];//temp传入进来是$xxx$，temp.replace(/\$/g, "")是xxx
            return void 0 === temp ? "" : temp//如果alter中没有对应的属性，则返回空字符串
        })
    }
    createKeyworldsHTML(cmd, query) {//cmd是命令，query是搜索框中的内容，isResult代表是否是搜索结果
        var r = '<b class="kwd">$1</b>';//$1是正则表达式匹配的第1个括号中的内容
        let cmdname = cmd.n
            , cmddesc = cmd.d;
        var re = new RegExp(`(${query})`, "ig"),
            query = (query && (cmdname = cmdname.replace(re, r),//将命令名称中的query加粗
                cmddesc = cmddesc.replace(re, r) || ""))//将命令描述中的query加粗
        return {
            html: this.simple(`<span>$name$</span> - <i>$des$</i>`, {
                name: cmdname,
                des: cmddesc
            }),
            class: cmd.n
        };
    }
    searchResult(display = true) {//t为true时，将命令显示在搜索建议列表中
        let cmds = this.commands;
        let i = [], a = [], c = [];
        let e, l = this.query_size;
        Object.keys(cmds).forEach((key, index) => {//遍历所有命令，找到匹配的命令
            //cmds[key].n是命令名称，cmds[key].d是命令描述
            //this.query是搜索框中的内容
            //h,o分别是this.query在命令名称和命令描述中的位置
            let now = cmds[key];
            let h = this.isSreachIndexOF(key, this.query)
                , o = this.isSreachIndexOF(now.d, this.query)
                , u = now;
            h > -1 ? (u.nIdx = h,//新属性nIdx是r.query在命令名称中的位置，key中含有query就推入a数组
                a.push(u)) : o > -1 && (u.dIdx = o,//新属性dIdx是r.query在命令描述中的位置，描述中含有query就推入c数组
                    c.push(u))
        })
        a.sort(sortArray),//按照nIdx排序
            c.sort(sortArray),//按照dIdx排序
            a.concat(c).slice(0, l).forEach(e => {//取出前l个命令，key匹配的命令比描述匹配的命令优先
                i.push(this.createKeyworldsHTML(e, this.query))
            });
        let d = this.elm_result;
        d.innerHTML = "",
            i.forEach((e, index) => {
                let li = document.createElement("li");
                li.innerHTML = e.html;
                li.id = e.class;
                d.appendChild(li)
            }),
            i.length || (l = document.createElement("LI"),//如果没有搜索结果
                (e = document.createElement("strong")).innerText = this.query ? "查无此命令 ∨ ∧ ∨" : "请输入命令 ∧ ∨ ∧",
                l.appendChild(e),
                d.appendChild(l)
            )
    }
    selectedResult(e) {
        let t = this.elm_result.children;//搜索建议列表
        let n = 0;
        for (var r = 0; r < t.length; r++)//遍历搜索建议列表，根据上下键，将选中的结果上移或下移
            if ("selected" == t[r].className) {
                t[r].className = "";//将当前结果的class置为空
                if ("up" == e) {//按下键盘上下键时，选中的结果向上或向下移动
                    if (r == 0) { n = t.length - 1; } else { n = r - 1; }
                } else { n = (r + 1) % t.length; }
                break
            }
        t[n] && (t[n].className = "selected")//如果t[n]存在，则令其class为ok
    }
    isSelectedResult() {//返回选中的li元素
        var t = this.elm_result.children;
        for (let e = 0; e < t.length; e++)
            if ("selected" == t[e].className) {
                return t[e];
            }
        return false
    }
    init() {
        this.searchResult()
        const focus = (e) => {
            if ("/" == e.key) {
                this.elm_query.focus()
                e.preventDefault()//阻止默认行为，加了这个就不会在搜索框中输入"/"
            }
        }
        this.elm_query.addEventListener("focus", e => {
            document.removeEventListener("keydown", focus)
        })
        this.elm_query.addEventListener("input", e => {
            this.query = e.target.value,
                this.searchResult()
        })
        this.elm_query.addEventListener("blur", e => {
            document.addEventListener("keydown", focus)
            let t = this.elm_result.children;
            for (let e = 0; e < t.length; e++)
                t[e].className = ""
        })
        this.elm_query.addEventListener("keydown", (e) => {
            if (40 === e.keyCode && this.selectedResult("down"),
                38 === e.keyCode && this.selectedResult("up"),
                "Enter" == e.key) {//只有按下回车键时，才会触发if语句
                e = this.isSelectedResult();
                if (!e) {
                    e = this.elm_result.children[0];
                };
                e && e.click()//点击选中的结果
            } else if ("Escape" == e.key) {
                this.elm_query.blur()
                this.searchResult()
            }
        })
        this.elm_result.addEventListener("click", (e) => {
            let t = e.target.closest("li");
            if (t) {
                let cmd = t.id;
                if (cmd && this.cur_cmd.className != cmd) {
                    console.log("fetching cmd: " + cmd + " ...")
                    // 获取Markdown文件内容并渲染
                    fetch("cmds/" + cmd + ".md")
                        .then((response) => {
                            return response.text();
                        })
                        .then((text) => {
                            this.md.innerHTML = markdownit.render(text);
                            this.cur_cmd.className = cmd;
                        })
                }
            }
        })
    }
}