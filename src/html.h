#pragma once

const char MAIN_page[] PROGMEM = R"=====(

<!DOCTYPE html>
<html>

<head>

<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">

<title>KegSense</title>

<style>

body{
    margin:0;
    background:#181818;
    color:white;
    font-family:Arial;
    text-align:center;
}

.card{
    width:90%;
    max-width:420px;
    margin:auto;
    margin-top:30px;
    background:#252525;
    border-radius:20px;
    padding:25px;
    box-shadow:0px 0px 15px rgba(0,0,0,.4);
}

h1{
    margin-top:5px;
    font-size:42px;
}

.percent{
    font-size:84px;
    font-weight:bold;
    color:#00d26a;
}

.liter{
    font-size:38px;
}

.weight{
    font-size:26px;
    color:#cccccc;
}

.bar{
    width:100%;
    height:28px;
    background:#444;
    border-radius:20px;
    overflow:hidden;
    margin-top:25px;
}

.fill{
    height:100%;
    width:0%;
    background:#00d26a;
    transition:0.6s;
}

.button{
    display:inline-block;
    margin-top:30px;
    background:#00d26a;
    color:white;
    padding:14px 30px;
    border-radius:10px;
    text-decoration:none;
    font-size:22px;
}

.info{
    margin-top:25px;
    background:#303030;
    border-radius:15px;
    padding:15px;
    text-align:left;
}

.row{
    display:flex;
    justify-content:space-between;
    padding:10px 0;
    border-bottom:1px solid #444;
}

.row:last-child{
    border-bottom:none;
}

.label{
    color:#bbbbbb;
}

.value{
    font-weight:bold;
}

.online{
    margin-top:20px;
    color:#5cff88;
}

</style>

</head>

<body>

<div class="card">

<h1>🍺 KegSense</h1>

<div class="percent">
<span id="percent">0</span>%
</div>

<div class="bar">
<div id="fill" class="fill"></div>
</div>

<br>

<div class="liter">
<span id="liter">0</span> Liter igjen
</div>

<br>

<div class="weight">
⚖️ <span id="weight">0</span> kg
</div>

<div class="online">
🟢 Online
</div>

<div class="info">

<div class="row">
<div class="label">⚖️ Total vekt</div>
<div class="value"><span id="weight">0</span> kg</div>
</div>

<div class="row">
<div class="label">🍺 Ølvekt</div>
<div class="value"><span id="beerWeight">0</span> kg</div>
</div>

<div class="row">
<div class="label">📶 WiFi</div>
<div class="value"><span id="wifi">0</span> dBm</div>
</div>

<div class="row">
<div class="label">⏱ Oppetid</div>
<div class="value"><span id="uptime">0</span></div>
</div>

<a class="button" href="/settings">
⚙ Innstillinger
</a>

</div>

<script>

function update(){

fetch("/api")

.then(response => response.json())

.then(data=>{

document.getElementById("percent").innerHTML=data.percent.toFixed(1);

document.getElementById("liter").innerHTML=data.liter.toFixed(2);

document.getElementById("weight").innerHTML=data.weight.toFixed(2);

document.getElementById("beerWeight").innerHTML =
    data.beerWeight.toFixed(2);

document.getElementById("wifi").innerHTML =
    data.wifiRSSI;

document.getElementById("uptime").innerHTML =
    formatTime(data.uptime);

document.getElementById("fill").style.width=data.percent+"%";

let color = "#00d26a"; // Grønn

if (data.percent < 5)
    color = "#ff3b30";   // Rød
else if (data.percent < 10)
    color = "#ff9500";   // Oransje
else if (data.percent < 20)
    color = "#ffd60a";   // Gul

document.getElementById("percent").style.color = color;
document.getElementById("fill").style.background = color;

});

}

setInterval(update,1000);

function formatTime(sec)
{
    let d = Math.floor(sec/86400);
    sec %= 86400;

    let h = Math.floor(sec/3600);
    sec %= 3600;

    let m = Math.floor(sec/60);

    if(d>0)
        return d+"d "+h+"t";

    if(h>0)
        return h+"t "+m+"m";

    return m+"m";
}
update();

</script>

</body>

</html>

)=====";