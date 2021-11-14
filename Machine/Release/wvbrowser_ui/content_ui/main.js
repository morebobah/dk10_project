const DEFAULT_HISTORY_ITEM_COUNT = 20;
const EMPTY_HISTORY_MESSAGE = `You haven't visited any sites yet.`;
let requestedTop = 0;
let lastRequestSize = 0;
let itemHeight = 48;
var socket = null;

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
    var args = event.data.args;

    switch (message) {
        case commands.MG_INIT_SOCKET:
            console.log(args.ipAddr);
            socket = new WebSocket(args.ipAddr);
            socket.onopen = function() {
                socket.send("Загрузочка");
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
        default:
            console.log(`Unexpected message: ${JSON.stringify(event.data)}`);
            break;
    }
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
        machineControl.append(icoControl);
        Object.keys(val).forEach(function(k) {
            if (k == "PINS") {
                refreshPins(machineControl, val[k])
            }
        });
        ctrlBar.append(machineControl);
        //alert(Object.keys(val));
    });
    let bodyElement = document.getElementsByTagName('body')[0];
    bodyElement.append(ctrlBar);
}

function refreshPins(ctrlBar, objJSON) {
    var keys = Object.keys(objJSON);
    keys.forEach(function(key) {
        let pinControl = document.createElement('div');
        pinControl.className = objJSON[key]["PINTYPE"];
        pinControl.id = 'pin[' + objJSON[key]["ADR"] + ']';
        let icoPin = document.createElement('div');
        icoPin.className = objJSON[key]["PINTYPE"] + "Ico";
        pinControl.append(icoPin);
        ctrlBar.append(pinControl);
    });

}

function initSocket() {
    socket = new WebSocket("ws://127.0.0.1:27015");
    socket.onopen = function() {
        socket.send("Загрузочка");
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