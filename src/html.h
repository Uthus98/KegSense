#pragma once

const char MAIN_page[] PROGMEM = R"=====(

<!DOCTYPE html>

<html>

<head>

<meta charset="UTF-8">

<meta name="viewport"
content="width=device-width, initial-scale=1">

<meta name="apple-mobile-web-app-capable" content="yes">
<meta name="apple-mobile-web-app-status-bar-style" content="black">
<meta name="apple-mobile-web-app-title" content="KegSense">
<meta name="mobile-web-app-capable" content="yes">

<link rel="apple-touch-icon" sizes="180x180"
      href="/apple-touch-icon.png">
<link rel="apple-touch-icon" sizes="152x152"
      href="/apple-touch-icon-152.png">
<link rel="apple-touch-icon" sizes="120x120"
      href="/apple-touch-icon-120.png">

<title>KegSense</title>


<style>

*{
    box-sizing:border-box;
}

body{
    margin:0;
    background:#181818;
    color:white;
    font-family:Arial,sans-serif;
}

.header{
    text-align:center;
    padding:25px 10px 10px 10px;
}

.header h1{
    margin:0;
    font-size:42px;
}

.system{
    margin-top:8px;
    color:#999;
    font-size:14px;
}

.container{
    width:92%;
    max-width:900px;
    margin:auto;

    display:grid;
    grid-template-columns:
        repeat(auto-fit,minmax(300px,1fr));

    gap:20px;

    padding:20px 0 40px 0;
}

.card{
    background:#252525;

    border-radius:22px;

    padding:25px;

    text-align:center;

    box-shadow:
        0 5px 20px rgba(0,0,0,.35);
}

.card.disabled{
    opacity:.45;
}

.name{
    font-size:27px;
    font-weight:bold;
    margin-bottom:12px;
}

.percent{
    font-size:70px;
    font-weight:bold;

    color:#00d26a;

    transition:.3s;
}

.bar{
    width:100%;
    height:24px;

    background:#444;

    border-radius:20px;

    overflow:hidden;

    margin:20px 0;
}

.fill{
    width:0%;
    height:100%;

    background:#00d26a;

    transition:
        width .5s,
        background .3s;
}

.liter{
    font-size:30px;

    margin-bottom:20px;
}

.info{
    background:#303030;

    border-radius:14px;

    padding:10px 15px;

    text-align:left;
}

.row{
    display:flex;

    justify-content:space-between;

    padding:9px 0;

    border-bottom:
        1px solid #444;
}

.row:last-child{
    border-bottom:none;
}

.label{
    color:#aaa;
}

.value{
    font-weight:bold;
}

.status{
    margin-top:18px;
    font-size:15px;
}

.online{
    color:#5cff88;
}

.offline{
    color:#ff5c5c;
}

.disabledText{
    color:#aaa;
}

.button{
    display:block;

    width:260px;

    margin:
        0 auto 35px auto;

    text-align:center;

    background:#00d26a;

    color:white;

    padding:14px 20px;

    border-radius:12px;

    text-decoration:none;

    font-size:20px;

    font-weight:bold;
}

</style>

</head>


<body>


<div class="header">

<h1>🍺 KegSense</h1>

<div class="system">

<span id="version">...</span>

&nbsp; • &nbsp;

📶 <span id="wifi">...</span> dBm

&nbsp; • &nbsp;

⏱ <span id="uptime">...</span>

</div>

</div>


<div
id="kegs"
class="container">
</div>


<a
class="button"
href="/settings">

⚙ Innstillinger

</a>


<script>


function formatTime(sec)
{
    let days =
        Math.floor(sec / 86400);

    sec %= 86400;

    let hours =
        Math.floor(sec / 3600);

    sec %= 3600;

    let minutes =
        Math.floor(sec / 60);


    if(days > 0)
        return days + "d " + hours + "t";

    if(hours > 0)
        return hours + "t " + minutes + "m";

    return minutes + "m";
}


function getColor(percent)
{
    if(percent < 5)
        return "#ff3b30";

    if(percent < 10)
        return "#ff9500";

    if(percent < 20)
        return "#ffd60a";

    return "#00d26a";
}


function buildKegCard(keg)
{
    const card =
        document.createElement("div");

    const color =
        getColor(keg.percent);

    card.className = "card";

    if(!keg.enabled)
        card.classList.add("disabled");


    let statusText = "";
    let statusClass = "";


    if(!keg.enabled)
    {
        statusText =
            "⚫ Deaktivert";

        statusClass =
            "disabledText";
    }
    else if(keg.online)
    {
        statusText =
            "🟢 Online";

        statusClass =
            "online";
    }
    else
    {
        statusText =
            "🔴 Offline";

        statusClass =
            "offline";
    }


    card.innerHTML = `

        <div class="name">
            🍺 ${keg.name}
        </div>

        <div
            class="percent"
            id="percent-${keg.index}"
            style="color:${color}">

            ${keg.percent.toFixed(1)}%

        </div>


        <div class="bar">

            <div
                class="fill"
                id="fill-${keg.index}"
                style="width:${keg.percent}%;background:${color}">
            </div>

        </div>


        <div class="liter">

            ${keg.liter.toFixed(2)}
            Liter igjen

        </div>


        <div class="info">

            <div class="row">

                <div class="label">
                    ⚖ Total vekt
                </div>

                <div class="value">
                    ${keg.weight.toFixed(2)} kg
                </div>

            </div>


            <div class="row">

                <div class="label">
                    🍺 Innhold
                </div>

                <div class="value">
                    ${keg.beerWeight.toFixed(2)} kg
                </div>

            </div>


            <div class="row">

                <div class="label">
                    Forbruk i dag
                </div>

                <div class="value">
                    ${keg.consumptionToday.toFixed(2)} L
                </div>

            </div>


            <div class="row">

                <div class="label">
                    Halvlitere igjen
                </div>

                <div class="value">
                    ${keg.halfLiters}
                </div>

            </div>


            <div class="row">

                <div class="label">
                    Fatvolum
                </div>

                <div class="value">
                    ${keg.volume.toFixed(1)} L
                </div>

            </div>

        </div>


        <div
            class="status ${statusClass}">

            ${statusText}

        </div>

        ${keg.enabled ? `
        <a href="/new-keg?index=${keg.index}"
           style="display:block;margin-top:18px;padding:11px;border-radius:10px;background:#3b3b3b;color:white;text-decoration:none;font-weight:bold">
            Nytt fat
        </a>` : ''}
    `;


    return card;
}


async function update()
{
    try
    {
        const response =
            await fetch("/api");

        const data =
            await response.json();


        document.getElementById(
            "version"
        ).innerHTML =
            data.version;


        document.getElementById(
            "wifi"
        ).innerHTML =
            data.wifiRSSI;


        document.getElementById(
            "uptime"
        ).innerHTML =
            formatTime(data.uptime);


        const container =
            document.getElementById(
                "kegs"
            );


        container.innerHTML = "";


        data.kegs.forEach(keg =>
        {
            container.appendChild(
                buildKegCard(keg)
            );
        });
    }
    catch(error)
    {
        console.log(
            "API error:",
            error
        );
    }
}


update();

setInterval(
    update,
    1000
);


</script>


</body>

</html>

)=====";
