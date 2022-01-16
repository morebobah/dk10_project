const DEFAULT_HISTORY_ITEM_COUNT = 20;
const EMPTY_HISTORY_MESSAGE = `You haven't visited any sites yet.`;
let requestedTop = 0;
let lastRequestSize = 0;
let itemHeight = 48;
var socket = null;
var ipAddr = null;
var gcode = '';
var bDirect = true;
var sended = ' sended';
var brancher = 0;

/*

const dateStringFormat = new Intl.DateTimeFormat('default', {
    weekday: 'long',
    year: 'numeric',
    month: 'long',
    day: 'numeric'
});

const timeStringFormat = new Intl.DateTimeFormat('default', {
    hour: '2-digit',
    minute: '2-digit'
});

const messageHandler = event => {
    var message = event.data.message;
    var args = event.data.args;

    switch (message) {
        case commands.MG_GET_HISTORY:
            let entriesContainer = document.getElementById('entries-container');
            if (args.from == 0 && args.items.length) {
                entriesContainer.textContent = '';

                let clearButton = document.getElementById('btn-clear');
                clearButton.classList.remove('hidden');
            }

            loadItems(args.items);
            if (args.items.length == lastRequestSize) {
                document.addEventListener('scroll', requestTrigger);
            } else if (entriesContainer.childElementCount == 0) {
                loadUIForEmptyHistory();
            }
            break;
        default:
            console.log(`Unexpected message: ${JSON.stringify(event.data)}`);
            break;
    }
};

const requestTrigger = function(event) {
    let triggerRange = 50;
    let element = document.body;

    if (element.scrollTop + element.clientHeight >= element.scrollHeight - triggerRange) {
        getMoreHistoryItems();
        event.target.removeEventListener('scroll', requestTrigger);
    }
};

function requestHistoryItems(from, count) {
    let message = {
        message: commands.MG_GET_HISTORY,
        args: {
            from: from,
            count: count || DEFAULT_HISTORY_ITEM_COUNT
        }
    };

    window.chrome.webview.postMessage(message);
}

function removeItem(id) {
    let message = {
        message: commands.MG_REMOVE_HISTORY_ITEM,
        args: {
            id: id
        }
    };

    window.chrome.webview.postMessage(message);
}

function createItemElement(item, id, date) {
    let itemContainer = document.createElement('div');
    itemContainer.id = id;
    itemContainer.className = 'item-container';

    let itemElement = document.createElement('div');
    itemElement.className = 'item';

    // Favicon
    let faviconElement = document.createElement('div');
    faviconElement.className = 'favicon';
    let faviconImage = document.createElement('img');
    faviconImage.src = item.favicon;
    faviconElement.append(faviconImage);
    itemElement.append(faviconElement);

    // Title
    let titleLabel = document.createElement('div');
    titleLabel.className = 'label-title';
    let linkElement = document.createElement('a');
    linkElement.href = item.uri;
    linkElement.title = item.title;
    linkElement.textContent = item.title;
    titleLabel.append(linkElement);
    itemElement.append(titleLabel);

    // URI
    let uriLabel = document.createElement('div');
    uriLabel.className = 'label-uri';
    let textElement = document.createElement('p');
    textElement.title = item.uri;
    textElement.textContent = item.uri;
    uriLabel.append(textElement);
    itemElement.append(uriLabel);

    // Time
    let timeLabel = document.createElement('div');
    timeLabel.className = 'label-time';
    let timeText = document.createElement('p');
    timeText.textContent = timeStringFormat.format(date);
    timeLabel.append(timeText);
    itemElement.append(timeLabel);

    // Close button
    let closeButton = document.createElement('div');
    closeButton.className = 'btn-close';
    closeButton.addEventListener('click', function(e) {
        if (itemContainer.parentNode.children.length <= 2) {
            itemContainer.parentNode.remove();
        } else {
            itemContainer.remove();
        }

        let entriesContainer = document.getElementById('entries-container');
        if (entriesContainer.childElementCount == 0) {
            loadUIForEmptyHistory();
        }
        removeItem(parseInt(id.split('-')[1]));
    });
    itemElement.append(closeButton);
    itemContainer.append(itemElement);

    return itemContainer;
}

function createDateContainer(id, date) {
    let dateContainer = document.createElement('div');
    dateContainer.id = id;

    let dateLabel = document.createElement('h3');
    dateLabel.className = 'header-date';
    dateLabel.textContent = dateStringFormat.format(date);
    dateContainer.append(dateLabel);

    return dateContainer;
}

function loadItems(items) {
    let dateContainer;
    let fragment;

    items.map((entry) => {
        let id = entry.id;
        let item = entry.item;
        let itemContainerId = `item-${id}`;

        // Skip the item if already loaded. This could happen if the user
        // visits an item for the current date again before requesting more
        // history items.
        let itemContainer = document.getElementById(itemContainerId);
        if (itemContainer) {
            return;
        }

        let date = new Date(item.timestamp);
        let day = date.getDate();
        let month = date.getMonth();
        let year = date.getFullYear();
        let dateContainerId = `entries-${month}-${day}-${year}`;

        // If entry belongs to a new date, append buffered items for previous
        // date.
        if (dateContainer && dateContainer.id != dateContainerId) {
            dateContainer.append(fragment);
        }

        dateContainer = document.getElementById(dateContainerId);
        if (!dateContainer) {
            dateContainer = createDateContainer(dateContainerId, date);
            fragment = document.createDocumentFragment();

            let entriesContainer = document.getElementById('entries-container');
            entriesContainer.append(dateContainer);
        } else if (!fragment) {
            fragment = document.createDocumentFragment();
        }

        itemContainer = createItemElement(item, itemContainerId, date);
        fragment.append(itemContainer);
    });

    // Append remaining items in buffer
    if (fragment) {
        dateContainer.append(fragment);
    }
}

function getMoreHistoryItems(n) {
    n = n ? n : DEFAULT_HISTORY_ITEM_COUNT;

    requestHistoryItems(requestedTop, n);
    requestedTop += n;
    lastRequestSize = n;
    document.removeEventListener('scroll', requestTrigger);
}

function addUIListeners() {
    let confirmButton = document.getElementById('prompt-true');
    confirmButton.addEventListener('click', function(event) {
        clearHistory();
        event.stopPropagation();
    });

    let cancelButton = document.getElementById('prompt-false');
    cancelButton.addEventListener('click', function(event) {
        toggleClearPrompt();
        event.stopPropagation();
    });

    let promptBox = document.getElementById('prompt-box');
    promptBox.addEventListener('click', function(event) {
        event.stopPropagation();
    });

    let promptOverlay = document.getElementById('overlay');
    promptOverlay.addEventListener('click', toggleClearPrompt);

    let clearButton = document.getElementById('btn-clear');
    clearButton.addEventListener('click', toggleClearPrompt);
}

function toggleClearPrompt() {
    let promptOverlay = document.getElementById('overlay');
    promptOverlay.classList.toggle('hidden');
}

function loadUIForEmptyHistory() {
    let entriesContainer = document.getElementById('entries-container');
    entriesContainer.textContent = EMPTY_HISTORY_MESSAGE;

    let clearButton = document.getElementById('btn-clear');
    clearButton.classList.add('hidden');
}

function clearHistory() {
    toggleClearPrompt();
    loadUIForEmptyHistory();

    let message = {
        message: commands.MG_CLEAR_HISTORY,
        args: {}
    };

    window.chrome.webview.postMessage(message);
}
*/

const messageHandler = event => {
    var message = event.data.message;

    switch (message) {
        case commands.MG_INIT_SOCKET:
            ipAddr = event.data.args.ipAddr;
            console.log(ipAddr);
            socket = new WebSocket(ipAddr);
            socket.onopen = function() {
                var message = {
                    message: commands.MG_SOCKET_ESTABLISHED,
                    args: {}
                };
                window.chrome.webview.postMessage(message);
            };
            socket.onclose = function(event) {
                if (event.wasClean) {
                    console.log('Соединение закрыто чисто');
                } else {
                    console.log('Обрыв соединения'); // например, "убит" процесс сервера
                }
                brancher = 0;
                console.log('Код: ' + event.code + ' причина: ' + event.reason);
            };
            socket.onmessage = function(event) {
                switch (brancher) {
                    case 1:
                        brancher = 2;
                        refreshAuto(event.data);
                        break;
                    case 2:
                        console.log('Обрыв соединения');
                        break;
                    default:
                        refreshPanel(event.data);
                        brancher = 1;
                        sendText("get_list_prg");
                }
            };
            break;
        case commands.MG_SOCKET_ESTABLISHED:
            sendText("config_download");
            brancher = 0;
            break;
        case commands.MG_DIRECT_SWITCHER:
            bDirect = !bDirect;
            break;
        case commands.MG_CLEAR_LIST:
            clearlist();
            break;
        default:
            console.log(`Unexpected message: ${JSON.stringify(event.data)}`);
            break;
    }
}

function refreshAuto(data) {
    var objJSON = $.parseJSON(data);
    var keys = Object.keys(objJSON);
    let ctrlBar = document.getElementById('apanel');
    if (ctrlBar) {
        ctrlBar.remove();
    }

    ctrlBar = document.createElement('div');
    ctrlBar.id = 'apanel';
    ctrlBar.className = 'barsControls';

    let mainpanel = document.createElement('div');
    mainpanel.className = 'main_auto_panel';
    let titlepanel = document.createElement('div');
    titlepanel.className = 'title_auto_panel';
    titlepanel.innerHTML = 'Запуск автоматических программ';
    mainpanel.append(titlepanel);
    let bodypanel = document.createElement('div');
    bodypanel.className = 'body_auto_panel';
    mainpanel.append(bodypanel);
    let keyspanel = document.createElement('div');
    keyspanel.className = 'keys_auto_panel';
    bodypanel.append(keyspanel);
    let start_bnt = document.createElement('input');
    start_bnt.type = 'button';
    start_bnt.id = 'start_btn';
    start_bnt.className = 'btn_auto';
    start_bnt.value = 'Старт';
    start_bnt.disabled = true;
    keyspanel.append(start_bnt);
    let pause_bnt = document.createElement('input');
    pause_bnt.type = 'button';
    pause_bnt.id = 'pause_btn';
    pause_bnt.className = 'btn_auto';
    pause_bnt.value = 'Пауза';
    pause_bnt.disabled = true;
    keyspanel.append(pause_bnt);
    let stop_bnt = document.createElement('input');
    stop_bnt.type = 'button';
    stop_bnt.id = 'stop_btn';
    stop_bnt.className = 'btn_auto';
    stop_bnt.value = 'Стоп';
    stop_bnt.disabled = true;
    keyspanel.append(stop_bnt);
    let ctrlpanel = document.createElement('div');
    ctrlpanel.className = 'ctrl_auto';
    bodypanel.append(ctrlpanel);
    let runstr = document.createElement('div');
    runstr.className = 'run_string';
    runstr.id = 'runstring';
    ctrlpanel.append(runstr);
    let itemslist = document.createElement('div');
    itemslist.className = 'items_list';
    ctrlpanel.append(itemslist);


    $.each(objJSON[keys], function(key, val) {
        var iNumOption = 0;
        let optname = '';
        let optid = '';
        Object.keys(val).forEach(function(k) {
            if (k == "name") {
                optname = val[k];
                iNumOption += 1;
            }
            if (k == "ID") {
                optid = 'start_prg' + val[k].toString();
                iNumOption += 2;
            }
            if (iNumOption == 3) {
                let item_div = document.createElement('div');
                item_div.className = 'item_auto_prg';
                item_div.id = optid;
                html = '<label class="switch">';
                html += '<input type="checkbox"><span class="slider round"></span></label><span>';
                html += '<span class="prg" data-key="' + optid + '">' + optname + '</span>';
                item_div.innerHTML = html;
                itemslist.append(item_div);
                item_div.addEventListener('click', function(e) {
                    if (e.target.className != 'prg') return;
                    let runstr = document.getElementById('runstring');
                    runstr.innerHTML = e.target.innerHTML;
                    runstr.setAttribute('data-key', e.target.getAttribute('data-key'));
                    let sbtn = document.getElementById('start_btn');
                    let pbtn = document.getElementById('pause_btn');
                    let stbtn = document.getElementById('stop_btn');
                    sbtn.setAttribute('data-key', e.target.getAttribute('data-key'));
                    sbtn.style.backgroundColor = 'lime';
                    sbtn.disabled = false;
                    pbtn.disabled = true;
                    stbtn.disabled = true;
                    pbtn.style.backgroundColor = '';
                    stbtn.style.backgroundColor = '';
                });
            }
        });
    });

    start_bnt.addEventListener('click', function(e) {
        gcode = e.target.getAttribute('data-key');
        if (gcode == '') return;
        document.getElementById(gcode).disabled = true;
        e.target.disabled = true;
        e.target.style.backgroundColor = '';
        let pbtn = document.getElementById('pause_btn');
        let stbtn = document.getElementById('stop_btn');
        pbtn.disabled = false;
        stbtn.disabled = false;
        pbtn.style.backgroundColor = 'lime';
        stbtn.style.backgroundColor = 'darkred';
        Array.from(document.getElementsByClassName('prg')).forEach(function(k) {
            k.className = 'disitem_auto_prg';
        });

        sendText(gcode);
    });

    ctrlBar.append(mainpanel);

    let mainBar = document.getElementById('autoBar');
    mainBar.append(ctrlBar);
    mainBar.append(AutoPrg);
}

function refreshRecord() {
    let mainBar = document.getElementById('handBar');
    let recordsBar = document.createElement('div');
    recordsBar.className = 'record';
    recordsBar.id = 'recordsBar';
    mainBar.append(recordsBar);

}

function refreshPanel(data) {
    var objJSON = $.parseJSON(data);
    var keys = Object.keys(objJSON);
    if (keys.length == 0) {
        console.log(`JSON wrong during MACHINE create!`);
        return false;
    }
    let ctrlBar = document.getElementById('panel');
    if (ctrlBar) {
        ctrlBar.remove();
    }

    ctrlBar = document.createElement('div');
    ctrlBar.id = 'panel';

    $.each(objJSON[keys[0]], function(key, val) {
        let machineControl = document.createElement('div');
        machineControl.className = 'machine';
        let labelControl = document.createElement('div');
        labelControl.className = 'machLabel';
        labelControl.innerHTML = 'Machine: ' + key;
        machineControl.append(labelControl);

        let icoControl = document.createElement('div');
        icoControl.className = 'machIco';
        icoControl.id = 'ico' + key;
        //machineControl.append(icoControl);
        Object.keys(val).forEach(function(k) {
            if (k == "PINS") {
                refreshPins(machineControl, val[k], key)
            }
        });
        ctrlBar.append(machineControl);
        //alert(Object.keys(val));
    });
    //let bodyElement = document.getElementsByTagName('body')[0];
    //bodyElement.append(ctrlBar);
    let mainBar = document.getElementById('handBar');
    mainBar.append(ctrlBar);
}

function refreshPins(ctrlBar, objJSON, machine = 0) {
    //create timer pin
    let pinControl = document.createElement('div');
    pinControl.className = 'T';
    pinControl.id = 'Timer' + machine;
    let icoPin = document.createElement('div');
    icoPin.className = "TIco";
    icoPin.id = 'TIco' + machine;
    let startCtrl = document.createElement('div');
    startCtrl.className = "TCtrl";
    startCtrl.addEventListener('click', function(e) {
        let ico = document.getElementById('TIco' + machine);
        let tinput = document.getElementById('T' + machine);
        if (tinput) {
            gcode = 'G' + machine + 'T' + tinput.value;
        }
    });
    let inputData = document.createElement('input');
    inputData.className = "Tinput";
    inputData.id = 'T' + machine;
    inputData.value = 1;
    let shildData = document.createElement('div');
    shildData.className = "TData";
    shildData.innerHTML = 'Время задержки в секундах<br>';
    shildData.append(inputData);
    pinControl.append(icoPin);
    pinControl.append(startCtrl);
    pinControl.append(shildData);
    ctrlBar.append(pinControl);
    //timer pin created

    var keys = Object.keys(objJSON);
    keys.forEach(function(key, pinum) {
        let pinControl = document.createElement('div');
        pinControl.className = objJSON[key]["PINTYPE"];
        pinControl.id = 'pin[' + objJSON[key]["ADR"] + ']';
        let icoPin = document.createElement('div');
        icoPin.className = objJSON[key]["PINTYPE"] + "Ico";
        let Label = document.createElement('div');
        Label.className = objJSON[key]["PINTYPE"] + "Label";
        Label.innerHTML = 'ADR:&nbsp;' + objJSON[key]["ADR"] + '<br>HWObj:&nbsp;' + objJSON[key]["HWObj"];
        let startCtrl = document.createElement('div');
        startCtrl.className = objJSON[key]["PINTYPE"] + "Ctrl";
        pinControl.append(icoPin);
        pinControl.append(Label);
        pinControl.append(startCtrl);

        startCtrl.addEventListener('click', function(e) {
            if (gcode.length > 0) {
                gcode += '+' + objJSON[key]["PINTYPE"];
            } else {
                gcode = 'G' + machine + objJSON[key]["PINTYPE"];
            }
            /*
            var checkbox = document.getElementById('T' + machine + objJSON[key]["ADR"]);
            var timebox = document.getElementById('Sec' + machine + objJSON[key]["ADR"]);
            */
            if (pinum < 10) {
                gcode += '0';
            }
            gcode += pinum + 'V';
            let recordBar = document.getElementById('recordsBar');
            let gRec = document.createElement('div');
            gRec.className = 'gcode';
            if (objJSON[key]["PINTYPE"] == 'W') {
                let wei = document.getElementById('W' + machine + objJSON[key]["PINTYPE"]);
                if (wei) {
                    gcode += wei.value;
                }
            } else if (objJSON[key]["PINTYPE"] == 'S') {
                gcode += '1';
            } else {
                gcode += '1';
                if (bDirect) {
                    gRec.innerHTML = sendgcode(gcode);
                } else {
                    gRec.innerHTML = gcode;
                }
                gcode = '';
                recordBar.append(gRec);
            }
            /*
            if (checkbox.checked) {
                gcode += '+T' + timebox.value;
            }
            var key = e.which || e.keyCode;
            if (key === 13) { // 13 is enter
                e.preventDefault();
                processAddressBarInput();
            }
            */
        });

        if (objJSON[key]["PINTYPE"] == 'M') {
            let stopCtrl = document.createElement('div');
            stopCtrl.className = objJSON[key]["PINTYPE"] + "SCtrl";
            pinControl.append(stopCtrl);
            let timerCtrl = document.createElement('label');
            timerCtrl.className = objJSON[key]["PINTYPE"] + "TCtrl";
            timerCtrl.innerHTML = '<input class="M" type="checkbox" id="T' + machine + objJSON[key]["ADR"] + '"><span class="M"></span>';
            let shildData = document.createElement('div');
            shildData.className = objJSON[key]["PINTYPE"] + "Data";
            let inputData = document.createElement('input');
            inputData.className = objJSON[key]["PINTYPE"] + "input";
            inputData.id = 'Sec' + machine + objJSON[key]["ADR"];
            inputData.value = 1;
            shildData.append(inputData);
            //pinControl.append(timerCtrl); //add if nessesary timer control
            //pinControl.append(shildData);//add if nessesary timer control

            stopCtrl.addEventListener('click', function(e) {
                if (gcode.length > 0) {
                    gcode += '+' + machine + objJSON[key]["PINTYPE"];
                } else {
                    gcode = 'G' + machine + objJSON[key]["PINTYPE"];
                }
                if (pinum < 10) {
                    gcode += '0';
                }
                gcode += pinum + 'V0';
                let recordBar = document.getElementById('recordsBar');
                let gRec = document.createElement('div');
                gRec.className = 'gcode';
                if (bDirect) {
                    gRec.innerHTML = sendgcode(gcode);
                } else {
                    gRec.innerHTML = gcode;
                }
                gcode = '';
                recordBar.append(gRec);
            });
        }


        if (objJSON[key]["PINTYPE"] == 'W') {
            let icoUpd = document.createElement('div');
            icoUpd.className = objJSON[key]["PINTYPE"] + "Upd";
            let shildData = document.createElement('div');
            shildData.className = objJSON[key]["PINTYPE"] + "Data";
            shildData.innerHTML = 'Вес: 0 кг<br>';
            let inputData = document.createElement('input');
            inputData.className = objJSON[key]["PINTYPE"] + "input";
            inputData.id = 'W' + machine + objJSON[key]["PINTYPE"];
            inputData.value = 3;
            shildData.append(inputData);
            pinControl.append(icoUpd);
            pinControl.append(shildData);
            icoUpd.addEventListener('click', function(e) {
                let gcode = 'G' + machine + objJSON[key]["PINTYPE"];
                if (pinum < 10) {
                    gcode += '0';
                }
                gcode += pinum + 'V0';
                let recordBar = document.getElementById('recordsBar');
                let gRec = document.createElement('div');
                gRec.className = 'gcode';
                gRec.innerHTML = sendgcode(gcode);
                recordBar.append(gRec);
            });
        }

        if (objJSON[key]["PINTYPE"] == 'S') {
            let icoUpd = document.createElement('div');
            icoUpd.className = objJSON[key]["PINTYPE"] + "Upd";
            pinControl.append(icoUpd);
            icoUpd.addEventListener('click', function(e) {
                let gcode = 'G' + machine + objJSON[key]["PINTYPE"];
                if (pinum < 10) {
                    gcode += '0';
                }
                gcode += pinum + 'V0';
                let recordBar = document.getElementById('recordsBar');
                let gRec = document.createElement('div');
                gRec.className = 'gcode';
                gRec.innerHTML = sendgcode(gcode);
                recordBar.append(gRec);
            });
        }

        ctrlBar.append(pinControl);
    });
}

function initSocket() {
    /*тестовая функция для вызова из C++*/
}

function sendgcode(code = '') {
    if (code.length == 0) {
        var gcodes = Array.from(document.getElementsByClassName("gcode"));
        gcodes.forEach(div => {
            divin = div.innerHTML.replace(sended, '');
            code += divin;
            div.innerHTML = divin + sended;
        });
    }
    sendText(code)
    return code;
}


function sendText(txt = "Текстовочка") {
    if (socket) {
        if (socket.readyState != 1) {
            socket = new WebSocket(ipAddr);
            socket.onopen = function() {
                socket.send(txt);
            };
        } else {
            socket.send(txt);
        }
    }
}

function clearlist() {
    document.getElementById('recordsBar').innerHTML = "";
}

function addControlsListeners() {}


function init() {
    bDirect = false;
    refreshRecord();
    window.chrome.webview.addEventListener('message', messageHandler);
    addControlsListeners();
    /*
    document.addEventListener("contextmenu", function(e) {
        e.preventDefault();
    });
    */

    //let viewportItemsCapacity = Math.round(window.innerHeight / itemHeight);
    //addUIListeners();
    //getMoreHistoryItems(viewportItemsCapacity);
}


init();