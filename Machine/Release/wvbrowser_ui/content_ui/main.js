const DEFAULT_HISTORY_ITEM_COUNT = 20;
const EMPTY_HISTORY_MESSAGE = `You haven't visited any sites yet.`;
let requestedTop = 0;
let lastRequestSize = 0;
let itemHeight = 48;
var socket = null;
var ipAddr = null;

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
    ipAddr = event.data.args.ipAddr;

    switch (message) {
        case commands.MG_INIT_SOCKET:
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
                console.log('Код: ' + event.code + ' причина: ' + event.reason);
            };
            socket.onmessage = function(event) {
                refreshPanel(event.data);
            };
            break;
        case commands.MG_SOCKET_ESTABLISHED:
            if (socket) {
                if (socket.readyState == 1) {
                    socket.send("getms");
                } else {
                    socket = new WebSocket(ipAddr);
                }
            }
            break;
        default:
            console.log(`Unexpected message: ${JSON.stringify(event.data)}`);
            break;
    }
}

function refreshAuto() {
    let ctrlBar = document.createElement('div');
    ctrlBar.id = 'apanel';
    ctrlBar.className = 'barsControls';

    let startBtn = document.createElement('input');
    startBtn.id = 'startButton';
    startBtn.type = 'button';
    startBtn.value = 'Старт';
    startBtn.className = 'ctrlBTN';
    ctrlBar.append(startBtn);

    let stopBtn = document.createElement('input');
    stopBtn.id = 'stopButton';
    stopBtn.type = 'button';
    stopBtn.value = 'Стоп';
    stopBtn.className = 'ctrlBTN';
    ctrlBar.append(stopBtn);

    let pauseBtn = document.createElement('input');
    pauseBtn.id = 'pauseButton';
    pauseBtn.type = 'button';
    pauseBtn.value = 'Пауза';
    pauseBtn.className = 'ctrlBTN';
    ctrlBar.append(pauseBtn);

    let AutoPrg = document.createElement('div');
    AutoPrg.id = 'prgSelDiv';
    AutoPrg.className = 'styled-select';
    let AutoPrgSelector = document.createElement('select');
    AutoPrgSelector.id = 'prgSel';
    let AutoPrgOpts = document.createElement('option');
    AutoPrgOpts.id = 'prgOpt';
    AutoPrgOpts.innerHTML = 'sjkjhnkdj';
    AutoPrgSelector.append(AutoPrgOpts);
    AutoPrg.append(AutoPrgSelector);

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
            var gcode = 'G' + machine + objJSON[key]["PINTYPE"];
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
            } else {
                gcode += '1';
            }
            gRec.innerHTML = gcode;
            recordBar.append(gRec);
            /*
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
            timerCtrl.innerHTML = '<input class="M" type="checkbox"><span class="M"></span>';
            pinControl.append(timerCtrl);
            let shildData = document.createElement('div');
            shildData.className = objJSON[key]["PINTYPE"] + "Data";
            let inputData = document.createElement('input');
            inputData.className = objJSON[key]["PINTYPE"] + "input";
            inputData.id = 'M' + machine + objJSON[key]["PINTYPE"];
            inputData.value = 0;
            shildData.append(inputData);
            pinControl.append(shildData);
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
            inputData.value = 0;
            shildData.append(inputData);
            pinControl.append(icoUpd);
            pinControl.append(shildData);
        }
        ctrlBar.append(pinControl);
    });

}

function initSocket() {
    alert('a');
    socket = new WebSocket("ws://127.0.0.1:27015");
    socket.onopen = function() {
        socket.send("Загрузочка");
        var message = {
            message: commands.MG_SOCKET_ESTABLISHED,
            args: {}
        };


        window.chrome.webview.postMessage(message);

    };
    socket.onclose = function(event) {
        if (event.wasClean) {
            alert('Соединение закрыто чисто');
        } else {
            alert('Обрыв соединения'); // например, "убит" процесс сервера
        }
        alert('Код: ' + event.code + ' причина: ' + event.reason);
    };
    socket.onmessage = function(event) {
        var obj = $.parseJSON(event.data);
        alert(Object.keys(obj)[0]);
        refreshPanel(event.data);
        /*
        alert(JSON.stringify(obj));
        for (i = 0; i < obj.length; i++) {
            var numbers = obj[i];
            alert(obj[i]);
            for (j = 0; j < numbers.length; j++) {
                alert(numbers[j]);
            }

        }
        $.each(obj["MACHINE"], function(key, val) {
            $.each(val["PINS"], function(sk, sv) {
                alert('sub ' + sk + ' ' + sv);
            })
            alert(key + ' NUMBER' + val["NUMBER"] + ' PINS' + val["PINS"]);
        });
        */
    };

    if (socket.OPEN) {
        alert('a');
    }
}


function sendText() {
    if (socket.readyState == 1) {
        socket.send("Текстовочка");
    } else {
        alert(socket.readyState);
    }
}

function addControlsListeners() {}


function init() {
    refreshAuto();
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