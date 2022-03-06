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
let timerId = 0;
var key_not_pressed = true;
var started_prg = 0;
var default_on = true;
var binsert = true;

let settings = {
    prg_options: null
};

function out(msg) {
    console.log(msg);
    let context = '<div class="logline">' + new Date().toLocaleString() + '&nbsp;';
    context += msg;
    context += '</div>' + document.getElementById('logBar').innerHTML;
    document.getElementById('logBar').innerHTML = context;
}

const messageHandler = event => {
    var message = event.data.message;

    switch (message) {
        case commands.MG_INIT_SOCKET:
            ipAddr = event.data.args.ipAddr;
            out(ipAddr);
            socket = new WebSocket(ipAddr);
            socket.onopen = function() {
                var message = {
                    message: commands.MG_SOCKET_ESTABLISHED,
                    args: {}
                };
                window.chrome.webview.postMessage(message);
                timerId = setInterval(() => {
                    sendText('c');
                }, 30000);
            };
            socket.onclose = function(event) {
                if (event.wasClean) {
                    out('Соединение закрыто чисто');
                } else {
                    out('Обрыв соединения'); // например, "убит" процесс сервера
                }
                out('Код: ' + event.code + ' причина: ' + event.reason);
                var message = {
                    message: commands.MG_SOCKET_DISCONNECT,
                    args: {}
                };
                window.chrome.webview.postMessage(message);
            };
            socket.onmessage = function(event) {
                //Will get always json
                var objJSON = $.parseJSON(event.data);
                var keys = Object.keys(objJSON);
                //Different keys depends befavior
                if (keys.includes("MACHINE")) {
                    refreshPanel(objJSON);
                    sendText("get_list_prg");
                }
                if (keys.includes("program")) refreshAuto(objJSON);
                if (keys.includes("status")) {
                    out(event.data);
                    refreshStatus(objJSON);
                }
                if (keys.includes("downloadprg")) {
                    let recBar = document.getElementById('recordsBar');
                    let bIrememebre = (recBar.firstChild.id == 'main_name');
                    if (bIrememebre) {
                        let crr = document.getElementById('current_cursor');
                        recBar.insertBefore(prgl(objJSON['downloadprg']['name']), crr);
                    }
                    parze(objJSON['downloadprg']['listing']);
                    if (!bIrememebre) {
                        recBar.firstChild.innerText = objJSON['downloadprg']['name'];
                    }
                }
            };
            break;
        case commands.MG_SOCKET_ESTABLISHED:
            sendText("config_download");
            break;
        case commands.MG_SOCKET_DISCONNECT:
            let ctrlBar = document.getElementById('apanel');
            if (ctrlBar) {
                ctrlBar.remove();
            }
            ctrlBar = document.getElementById('panel');
            if (ctrlBar) {
                ctrlBar.remove();
            }
            document.getElementById('tab0').checked = true;
            clearInterval(timerId);
            break;
        case commands.MG_DIRECT_SWITCHER:
            bDirect = !bDirect;
            break;
        case commands.MG_SWITCH_TAB:
            let input = document.getElementById(event.data.args.tabid);
            input.checked = true;
            localStorage.setItem('tabid', event.data.args.tabid);
            break;
        case commands.MG_CLEAR_LIST:
            clearlist();
            break;
        default:
            out(`Unexpected message: ${JSON.stringify(event.data)}`);
            break;
    }
}

function refreshStatus(objJSON) {
    key_not_pressed = true;
    out('prg_id=' + objJSON['prg_id']);
    started_prg = objJSON['prg_id'];
    default_on = objJSON['default_on'] == 1;
    setButtonState(objJSON['status']);
    var keys = Object.keys(objJSON['state']);
    keys.forEach(function(mach_num) {
        out(mach_num);
        let mC = objJSON['state'][mach_num]['machine'];
        out('machine_' + mC);
        Array.from(document.getElementById('machine_' + mC).getElementsByClassName('MIco')).forEach(function(pins) {
            if (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] !== undefined)
                pins.style.backgroundImage = (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] == default_on) ? 'url(img/small_smotor_run.gif)' : 'url(img/small_smotor.png)';
        });
        Array.from(document.getElementById('machine_' + mC).getElementsByClassName('SIco')).forEach(function(pins) {
            if (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] !== undefined)
                pins.style.backgroundImage = (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] == default_on) ? 'url(img/small_sensor_run.png)' : 'url(img/small_sensor.png)';
        });
        Array.from(document.getElementById('machine_' + mC).getElementsByClassName('WIco')).forEach(function(pins) {
            if (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] !== undefined)
                pins.style.backgroundImage = (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] >= 0) ? 'url(img/small_weigher_run.png)' : 'url(img/small_weigher.png)';
        });
        Array.from(document.getElementById('machine_' + mC).getElementsByClassName('WDat')).forEach(function(pins) {
            if (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] !== undefined)
                pins.innerHTML = (objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] >= 0) ? objJSON['state'][mach_num]['pins'][pins.getAttribute("widget_id")] : '0';
        });
    });
}

function setButtonState(status = '0') {
    let buttons = document.getElementsByClassName('btn_auto');
    Array.from(buttons).forEach(function(btn) {
        let prg = btn.getAttribute('data-prg_id');
        let type = btn.id.charCodeAt(2);
        out('Code=' + String(type));
        //Search for st'a'rt, codeof('a') = 97
        if (type == 97) {
            out('Status=' + String(status));
            if (status == '0') {
                out('data-prg_id=' + btn.getAttribute('data-prg_id'));
                if (btn.getAttribute('data-prg_id') == 'zero') {
                    btn.style.backgroundColor = '';
                    btn.disabled = true;
                } else {
                    btn.style.backgroundColor = 'lime';
                    btn.disabled = false;
                }
            } else {
                btn.style.backgroundColor = '';
                btn.disabled = true;
            }
        }

        //Search for st'o'p, codeof('o') = 111
        if (type == 111) {
            if (status == '0') {
                btn.style.backgroundColor = '';
                btn.disabled = true;
            }
            if (status == '1' || status == '2') {
                let prg = btn.getAttribute('data-prg_id');
                out(started_prg);
                out(prg);
                if (prg == started_prg) {
                    btn.style.backgroundColor = 'darkred';
                    btn.disabled = false;
                } else {
                    btn.style.backgroundColor = '';
                    btn.disabled = true;
                }
            }
        }

        //Search for pa'u'se, codeof('u') = 117
        if (type == 117) {
            if (status == '0') {
                btn.style.backgroundColor = '';
                btn.disabled = true;
            }
            if (status == '1') {
                if (prg == started_prg) {
                    btn.style.backgroundColor = 'lime';
                    btn.value = 'Далее';
                    btn.dataset.cmd = 'continue_prg';
                    btn.disabled = false;
                } else {
                    btn.style.backgroundColor = '';
                    btn.disabled = true;
                }
            }
            if (status == '2') {
                if (prg == started_prg) {
                    btn.style.backgroundColor = 'lime';
                    btn.value = 'Пауза';
                    btn.dataset.cmd = 'pause_prg';
                    btn.disabled = false;
                } else {
                    btn.style.backgroundColor = '';
                    btn.disabled = true;
                }
            }
        }

    });
}

function CreatePanel(prg_id, prg_name) {
    let panel = document.createElement('div');
    panel.className = 'main_auto_panel';
    panel.id = 'panel_' + prg_id;
    let titlepanel = document.createElement('div');
    titlepanel.className = 'title_auto_panel';
    titlepanel.id = 'panel_title_' + prg_id;
    titlepanel.innerHTML = prg_name;
    panel.append(titlepanel);
    let bodypanel = document.createElement('div');
    bodypanel.className = 'body_auto_panel';
    panel.append(bodypanel);
    let keyspanel = document.createElement('div');
    keyspanel.className = 'keys_auto_panel';
    bodypanel.append(keyspanel);
    let start_bnt = document.createElement('input');
    start_bnt.type = 'button';
    start_bnt.id = 'start_btn_' + prg_id;
    start_bnt.className = 'btn_auto';
    start_bnt.dataset.prg_id = prg_id;
    start_bnt.value = 'Старт';
    start_bnt.disabled = true;
    keyspanel.append(start_bnt);
    let pause_bnt = document.createElement('input');
    pause_bnt.type = 'button';
    pause_bnt.id = 'pause_btn_' + prg_id;
    pause_bnt.dataset.prg_id = prg_id;
    pause_bnt.dataset.cmd = 'pause_prg';
    pause_bnt.className = 'btn_auto';
    pause_bnt.value = 'Пауза';
    pause_bnt.disabled = true;
    keyspanel.append(pause_bnt);
    let stop_bnt = document.createElement('input');
    stop_bnt.type = 'button';
    stop_bnt.id = 'stop_btn_' + prg_id;
    stop_bnt.dataset.prg_id = prg_id;
    stop_bnt.className = 'btn_auto';
    stop_bnt.value = 'Стоп';
    stop_bnt.disabled = true;
    keyspanel.append(stop_bnt);
    let ctrlpanel = document.createElement('div');
    ctrlpanel.className = 'ctrl_auto';
    bodypanel.append(ctrlpanel);
    return panel;
}

function killPanel(prg_id) {
    let panel = document.getElementById('panel_' + prg_id);
    if (panel) panel.remove();
    setButtonState();
}

function setAutoPrgEvents() {
    Array.from(document.getElementsByClassName('btn_auto')).forEach((item) => {
        let type = item.id.charCodeAt(2);
        if (type == 97) {
            item.addEventListener('click', function(e) {
                let opt_id = e.target.getAttribute('data-prg_id');
                if (opt_id === 'zero') {
                    out('zero');
                } else {
                    if (key_not_pressed) {
                        out('start_prg' + opt_id);
                        started_prg = opt_id;
                        sendText('start_prg' + opt_id);
                        key_not_pressed = false;
                    }
                }
            });
        }
        if (type == 111) {
            item.addEventListener('click', function(e) {
                if (key_not_pressed) {
                    sendText('stop_prg');
                    started_prg = 0;
                    key_not_pressed = false;
                }
            });
        }
        if (type == 117) {
            item.addEventListener('click', function(e) {
                if (key_not_pressed) {
                    sendText(e.target.getAttribute('data-cmd'));
                    started_prg = 0;
                    key_not_pressed = false;
                }
            });
        }
    });
    setButtonState();
}

function refreshAuto(objJSON) {
    var keys = Object.keys(objJSON);
    var panels = new Array();
    let ctrlBar = document.getElementById('apanel');
    if (ctrlBar) {
        ctrlBar.remove();
    }

    ctrlBar = document.createElement('div');
    ctrlBar.id = 'apanel';
    ctrlBar.className = 'barsControls';
    let mainpanel = CreatePanel('zero', 'Запуск автоматических программ');

    let ctrlpanel = mainpanel.getElementsByClassName('ctrl_auto').item(0);
    if (!ctrlpanel) return;
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
        let optnum = '';
        Object.keys(val).forEach(function(k) {
            if (k == "name") {
                optname = val[k];
                iNumOption += 1;
            }
            if (k == "ID") {
                optid = 'start_prg' + val[k].toString();
                seloptid = val[k].toString();
                optnum = val[k].toString();
                iNumOption += 2;
            }
            if (iNumOption == 3) {
                //Start selector
                let prg_sel = document.getElementById('prg_list');
                if (prg_sel) {
                    let o = document.createElement('option');
                    o.value = seloptid;
                    o.text = optname;
                    prg_sel.appendChild(o);
                }

                //End selector
                let item_div = document.createElement('div');
                item_div.className = 'item_auto_prg';
                item_div.id = optid;
                let sw_label = document.createElement('label');
                sw_label.className = 'switch';
                sw_label.id = 'sw_' + optnum;
                let sw_input = document.createElement('input');
                sw_input.type = 'checkbox';
                sw_input.name = 'inp_' + optnum;
                let bP = (localStorage.getItem(sw_input.name) === 'true');
                sw_input.checked = bP;
                if (bP) panels.push(CreatePanel(optnum, optname));
                let sw_span = document.createElement('span');
                sw_span.className = 'slider round';
                sw_label.append(sw_input);
                sw_label.append(sw_span);
                let prg_span = document.createElement('span');
                prg_span.className = 'prg';
                prg_span.dataset.prg_id = optnum;
                prg_span.innerHTML = optname;
                item_div.append(sw_label);
                item_div.append(prg_span);
                itemslist.append(item_div);
                item_div.addEventListener('click', function(e) {
                    if (e.target.className != 'prg') return;
                    let runstr = document.getElementById('runstring');
                    runstr.innerHTML = e.target.innerHTML;
                    runstr.setAttribute('data-key', e.target.getAttribute('data-key'));
                    let trg = e.target.parentElement;
                    while (trg.className != 'body_auto_panel' && trg) {
                        trg = trg.parentElement;
                    }
                    Array.from(trg.getElementsByClassName('btn_auto')).forEach((item) => {
                        let type = item.id.charCodeAt(2);
                        item.setAttribute('data-prg_id', e.target.getAttribute('data-prg_id'));
                        if (type == 97) {
                            item.style.backgroundColor = 'lime';
                            item.disabled = false;
                        }
                        if (type == 111 || type == 117) {
                            item.style.backgroundColor = '';
                            item.disabled = true;
                        }
                    });
                });
                sw_input.addEventListener('click', function(e) {
                    localStorage.setItem(e.target.name, e.target.checked);
                    if (localStorage.getItem(e.target.name) === 'true') {
                        let ctrlBar = document.getElementById('apanel');
                        let panel = CreatePanel(optnum, optname);
                        ctrlBar.append(panel);
                        setButtonState();
                    } else {
                        killPanel(optnum);
                    }
                });
            }
        });
    });
    let new_prg = 1;
    let all_prg = Array.from(document.querySelector("#prg_list").options).map(o => o.value);
    while (all_prg.includes(new_prg.toString())) {
        new_prg++;
    }
    let opt_new = document.getElementById('opt_new');
    if (opt_new) {
        opt_new.value = new_prg.toString();
    }

    ctrlBar.append(mainpanel);
    panels.forEach((item) => { ctrlBar.append(item) });
    let mainBar = document.getElementById('autoBar');
    mainBar.append(ctrlBar);
    setAutoPrgEvents();
}

function cursorPos(cup = true) {
    Array.from(document.getElementsByClassName('editBox')).forEach(e => { e.remove() });
    let crr = document.getElementById('current_cursor');
    if (cup) {
        if (crr !== crr.parentElement.firstChild)
            crr.parentElement.insertBefore(crr, crr.previousElementSibling);
    } else {
        crr.parentElement.insertBefore(crr, crr.nextElementSibling.nextElementSibling);
    }
}

function refreshRecord() {
    let recControls = document.getElementById('record_ctrl');
    let recControls2 = document.getElementById('record_ctrl2');

    /* for future futures
    let insButton = document.createElement('div');
    insButton.className = 'btn';
    insButton.id = 'btn-ins';
    insButton.title = 'Insert mode';
    if (binsert) insButton.style.border = '1px solid black';
    recControls.append(insButton);
    */

    let upButton = document.createElement('div');
    upButton.className = 'btn';
    upButton.id = 'btn-up';
    upButton.title = 'Up cursor';
    recControls.append(upButton);

    let downButton = document.createElement('div');
    downButton.className = 'btn';
    downButton.id = 'btn-down';
    downButton.title = 'Down cursor';
    recControls.append(downButton);

    let clearButton = document.createElement('div');
    clearButton.className = 'btn';
    clearButton.id = 'btn-clear';
    clearButton.title = 'Reset program';
    recControls.append(clearButton);

    let separator = document.createElement('div');
    separator.className = 'btn-separator';
    recControls.append(separator);

    let openButton = document.createElement('div');
    openButton.className = 'btn';
    openButton.id = 'btn-open';
    openButton.title = 'Open json';
    recControls.append(openButton);

    let saveButton = document.createElement('div');
    saveButton.className = 'btn';
    saveButton.id = 'btn-save';
    saveButton.title = 'Save json';
    recControls.append(saveButton);

    let dloadButton = document.createElement('div');
    dloadButton.className = 'btn';
    dloadButton.id = 'btn-download';
    dloadButton.title = 'Download from ESP';
    recControls2.append(dloadButton);

    let loadButton = document.createElement('div');
    loadButton.className = 'btn';
    loadButton.id = 'btn-load';
    loadButton.title = 'Load to ESP';
    recControls2.append(loadButton);
    recControls2.append(separator);

    if (settings.prg_options === null) {
        settings.prg_options = document.createElement('select');
        settings.prg_options.className = 'prg_sel';
        settings.prg_options.id = 'prg_list';
        let o = document.createElement('option');
        o.id = 'opt_new';
        o.value = '0';
        o.text = 'New';
        settings.prg_options.appendChild(o);
        recControls2.append(settings.prg_options);
    }

    /*
     insButton.addEventListener('click', function(e) {
         binsert = !binsert;
         if (binsert) {
             e.target.style.border = '1px solid black';
         } else {
             e.target.style.border = 'none';
         }
     });
     */

    clearButton.addEventListener('click', function(e) {
        if (confirm("Удалить всю программу из редактора?")) clearlist();
    });

    upButton.addEventListener('click', e => cursorPos(true));

    downButton.addEventListener('click', e => cursorPos(false));

    openButton.addEventListener('click', function(e) {
        parze('G1M00V1G1M01V1G1M02V1G1S03V1+M02V1G1S04V1+M02V1G1M05V1G1M02V1G1M00V1G1M01V1G1M02V1G1W06V3+S03V1+M05V1G2M02V1G2S03V1+M02V1G2T1+M01V1');
        /*
        var message = {
            message: commands.MG_LOAD_PROGRAM,
            args: {}
        };
        window.chrome.webview.postMessage(message);
        */
    });

    saveButton.addEventListener('click', function(e) {
        let send_gcode = '';
        let prg_name = document.getElementById('main_name');
        let name_gcode = (Math.random() + 1).toString(36).substring(10);
        if (prg_name) name_gcode = prg_name.innerText || name_gcode;
        Array.from(document.getElementsByClassName('gcode')).forEach(item => { send_gcode += item.innerText });
        var message = {
            message: commands.MG_SAVE_PROGRAM,
            args: { prg: name_gcode, gcode: send_gcode }
        };
        window.chrome.webview.postMessage(message);
    });

    loadButton.addEventListener('click', function(e) {
        let send_gcode = '';
        let prg_name = document.getElementById('main_name');
        let name_gcode = (Math.random() + 1).toString(36).substring(10);
        if (prg_name) name_gcode = prg_name.innerText || name_gcode;
        let prg_sel = document.getElementById('prg_list');
        if (!prg_sel) return;
        Array.from(document.getElementsByClassName('gcode')).forEach(item => { send_gcode += item.innerText });
        if (send_gcode.length == 0) return;
        if (!confirm('Загрузить программу \"' + name_gcode + '\" с идентификатором ' + prg_sel.value + '?\n Существующая программа с таким идентификатором будет перезаписана.')) return;
        sendgcode('save_prg' + prg_sel.value + '\n' + name_gcode + '\n' + send_gcode);
    });

    dloadButton.addEventListener('click', function(e) {
        let prg_sel = document.getElementById('prg_list');
        if (!prg_sel) return;
        if (prg_sel.options[prg_sel.selectedIndex].text != 'New') sendgcode('load_prg' + prg_sel.value);
    });

    document.addEventListener("keydown", function(e) {
        if (localStorage.getItem('tabid') === 'tab2') {
            if (e.key === 'ArrowUp') cursorPos(true);
            if (e.key === 'ArrowDown') cursorPos(false);
        }
    });

    document.getElementById('recordsBar').addEventListener('click', function(e) {
        let crr = document.getElementById('current_cursor');
        if (e.target.className === 'gcode_item') {
            crr.parentElement.insertBefore(crr, e.target.nextElementSibling);
        }
    });

    saveButton.addEventListener('click', function(e) {
        result = '';
        let rBar = document.getElementById('recordsBar');
        Array.from(rBar.getElementsByClassName('gcode')).forEach(item => {
            result = result + item.innerText;
        });
    });
}

function edit_div(item) {
    Array.from(document.getElementsByClassName('editBox')).forEach(e => { e.remove() });
    var editbox = document.createElement('input');
    editbox.type = 'textbox';
    editbox.className = 'editBox';
    var rect = item.getBoundingClientRect();
    editbox.style.position = "absolute";
    editbox.style.zIndex = "999";
    editbox.style.top = rect.top + "px";
    editbox.style.left = rect.left + "px";
    editbox.style.width = (rect.right - rect.left) + "px";
    editbox.style.height = (rect.bottom - rect.top) + "px";
    editbox.value = item.innerText;
    item.parentElement.append(editbox);
    editbox.addEventListener("keydown", function(e) {
        if (e.key === 'Escape') {
            e.target.remove();
        }
        if (e.key === 'Enter') {
            if (String(e.target.value).replace(' ', '').length > 0) {
                item.innerText = e.target.value;
                e.target.remove();
            } else {
                e.target.style.border = '1px solid red';
            }
        }
    });
    editbox.addEventListener("focusout", function(e) { e.target.remove(); });
}


function parze(gcodes) {
    let gcds = String(gcodes);
    let crr = document.getElementById('current_cursor');
    from_pos = gcds.indexOf('G');
    to_pos = gcds.indexOf('G', from_pos + 1);
    while (to_pos >= 0) {
        insert_gcode_item(gcl(gcds.substring(from_pos, to_pos)));
        from_pos = to_pos;
        to_pos = gcds.indexOf('G', from_pos + 1);
    }
    insert_gcode_item(gcl(gcds.substring(from_pos)));
}

function insert_gcode_item(item) {
    let bMainName = true;
    let recBar = document.getElementById('recordsBar');
    let crr = document.getElementById('current_cursor');
    Array.from(recBar.getElementsByClassName('prg_label')).forEach(item => { if (item.id === 'main_name') bMainName = false; });
    if (bMainName) {
        let prg_label = prgl();
        recBar.insertBefore(prg_label, recBar.firstChild);
        prg_label.addEventListener('click', function(e) {
            edit_div(e.target);
        });
    }
    recBar.insertBefore(item, crr);
}

function gcl(gc_line) {
    let item = document.createElement('div');
    item.className = 'gcode_item';
    let item_close = document.createElement('div');
    item_close.className = 'gcode_close';
    item_close.innerText = ' ';
    item.append(item_close);
    item_close.addEventListener('click', function(e) { e.target.parentElement.remove(); });
    let item_cursor = document.createElement('div');
    item_cursor.className = 'gcode_label';
    item.append(item_cursor);
    let item_value = document.createElement('div');
    item_value.className = 'gcode';
    item_value.innerText = gc_line;
    item.append(item_value);
    item_value.addEventListener('click', function(e) { edit_div(e.target); });

    return item;
}

function prgl(lname = 'Без названия') {
    let item = document.createElement('div');
    item.className = 'prg_label';
    if (!document.getElementById('main_name'))
        item.id = 'main_name';
    item.innerText = lname;
    return item;
}

function refreshPanel(objJSON) {
    //var objJSON = $.parseJSON(data);
    var keys = Object.keys(objJSON);
    if (keys.length == 0) {
        out(`JSON wrong during MACHINE create!`);
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
        machineControl.id = 'machine_' + key;
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

function clearPins(node) {
    Array.from(node.parentElement.parentElement.children).forEach(e => e.style.border = '2px solid #cbcbcb');
}

function restrictPins(node) {
    node.parentElement.style.border = '2px solid red';
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
        restrictPins(e.target);
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
        //icoPin.id = 'ico' + objJSON[key]["PINTYPE"] + objJSON[key]["ADR"];
        icoPin.setAttribute("widget_id", objJSON[key]["PINTYPE"] + objJSON[key]["ADR"]);
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
            if (pinum < 10) {
                gcode += '0';
            }
            gcode += pinum + 'V';
            if (objJSON[key]["PINTYPE"] == 'W') {
                let wei = document.getElementById('W' + machine + objJSON[key]["PINTYPE"]);
                if (wei) {
                    gcode += wei.value;
                }
                restrictPins(e.target);
            } else if (objJSON[key]["PINTYPE"] == 'S') {
                gcode += '1';
                restrictPins(e.target);
            } else {
                gcode += '1';
                insert_gcode_item(gcl(gcode));
                if (bDirect) sendgcode(gcode);
                clearPins(e.target);
                gcode = '';

            }
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
                insert_gcode_item(gcl(gcode));
                if (bDirect) sendgcode(gcode);
                gcode = '';
                clearPins(e.target);
            });
        }


        if (objJSON[key]["PINTYPE"] == 'W') {
            let icoUpd = document.createElement('div');
            icoUpd.className = objJSON[key]["PINTYPE"] + "Upd";
            let shildData = document.createElement('div');
            shildData.className = objJSON[key]["PINTYPE"] + "Data";
            shildData.id = 'wei' + objJSON[key]["ADR"] + '_' + objJSON[key]["HWObj"];
            shildData.innerHTML = 'Вес: <span class="WDat" widget_id="' + objJSON[key]["PINTYPE"] + objJSON[key]["ADR"] + '">0</span> кг<br>';
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
            code += div.innerText;
            div.previousSibling.style.backgroundColor = '#39e929';
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
    Array.from(document.getElementById('recordsBar').getElementsByClassName('gcode_item')).forEach(item => { item.remove(); });
    Array.from(document.getElementById('recordsBar').getElementsByClassName('prg_label')).forEach(item => { if (item.id != 'main_name') item.remove(); });
}

function addControlsListeners() {}


function init() {
    bDirect = false;
    refreshRecord();
    window.chrome.webview.addEventListener('message', messageHandler);
    addControlsListeners();
    let tab = document.getElementById(localStorage.getItem('tabid') || 'tab0');
    if (!tab.style.display) {
        tab = document.getElementById('tab0');
    }
    tab.checked = true;

    /*
                            document.addEventListener("contextmenu", function(e) {
                                e.preventDefault();
                            });
    
                            document.addEventListener("keydown", function(e) {
                                if (e.key === 'Escape') sendgcode('alarm');
                                document.body.disabled = true;
                            });
                            */
    //let viewportItemsCapacity = Math.round(window.innerHeight / itemHeight);
    //addUIListeners();
    //getMoreHistoryItems(viewportItemsCapacity);
}


init();