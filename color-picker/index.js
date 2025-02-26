const fs = require("fs")

const modConfigFile = "..\\include\\ModConfig.hpp"

function update_color(dict){
    reg = new RegExp("^(.*CONFIG_VALUE\\( *([A-Za-z0-9]+) *,.*UnityEngine::Color\\()([0-9\\. ]+,[0-9\\. ]+,[0-9\\. ]+,[0-9\\. ]+)(\\).*)\r?$")
    only_comment = new RegExp("^\\s+//.*")


    ret = {__proto__:null}

    let txt = ""
    fs.readFileSync(modConfigFile, {
        encoding:"utf-8"
    }).split("\n").forEach((v)=>{
        m = v.match(reg)

        if(!m || only_comment.exec(v)){
            txt += v + "\n"
            return
        }

        let pre = m[1], name = m[2], value = m[3], post = m[4]
        ret[name]=value
        
        if(dict[name]){
            v = pre + dict[name] + post
            console.log(v)
        }
        // let m = v.match(reg)
        // console.log(m)
        txt += v + "\n"
    })

    if(txt.endsWith("\n")){
        //we inserted \n to the end of line, remove it
        txt = txt.substring(0, txt.length-1)
    }
    fs.writeFileSync(modConfigFile, txt, {
        encoding:"utf-8"
    })
    return ret
}

v = update_color({})
console.log(v)

const http = require('node:http');

const server = http.createServer((req, res) => {
    if(req.url == '/'){
        data = update_color({__proto__:null})
        res.write(`<!DOCTYPE html>
<html>
    <style>
        .picker{
            width: 400px;
            height: 100px;
            margin: 8px;
        }
    </style>
    <body>
    <button id='sub'>use F12 edit color and click me</button><br>
    <script>
    document.getElementById('sub').addEventListener('click',()=>{
        datas = document.getElementsByClassName('picker')
    
        out = {__proto__:null}
        for(let i=0;i<datas.length;i++){
        out[datas[i].innerText] = datas[i].style.backgroundColor
        }
        fetch('/submit.rnd=' + Math.random() + "?" + JSON.stringify(out))
    })
    </script>
`)

    for(var k in data){
        var v = data[k].split(',')

        var txt = "rgb("
        if(v.length == 4) txt = "rgba("
        for(let i=0;i<v.length;i++){
            if(i > 0) txt += ","
            if(i < 3){
                txt += Math.floor(+v[i] * 256)
            }else{
                txt += +v[i]
            }
        }
        txt += ")"
        res.write("<div class='picker' style='background-color:" + txt + "'>" + k + "</div>\n")
    }

    res.write(
    `
        </body>
    </html>`)

        
    }else if(req.url.startsWith('/submit.rnd')){
        let json = req.url
        json = json.substring(json.indexOf("?")+1)
        json = decodeURI(json)
        json = JSON.parse(json)
        let colorRe = new RegExp("^rgba?\\(\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*,\\s*([0-9]+)\\s*(,\\s*([0-9\\.]+))?\\s*\\)$")
        for(let k in json){
            let v = json[k]

            let m = colorRe.exec(v)
            if(m){
                console.log(m)
                let r = (+m[1])/255
                let g = (+m[2])/255
                let b = (+m[3])/255
                let a
                if(m[5] != undefined){
                    a = m[5]
                }else{
                    a = 1
                }
                v = r +"," + g + "," + b+"," + a
            }else{
                v = "0,0,0,1"
            }
            json[k] = v
        }
        console.log("update with dict:", json)
        update_color(json)
    }else{
        res.write("<html><body><a href='/'>go to page</a></body></html>")
    }
  res.end();
});
server.on('clientError', (err, socket) => {
  socket.end('HTTP/1.1 400 Bad Request\r\n\r\n');
});
server.listen(8000);