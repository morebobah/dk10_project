const WORD_REGEX = /^[^//][^.]*$/;
const VALID_URI_REGEX = /^[-:.&#+()[\]$'*;@~!,?%=\/\w]+$/; // Will check that only RFC3986 allowed characters are included
const SCHEMED_URI_REGEX = /^\w+:.+$/;

let settings = {
    scriptsEnabled: true,
    blockPopups: true
};

const messageHandler = event => {
    var message = event.data.message;
    var args = event.data.args;

    switch (message) {
        case commands.MG_UPDATE_URI:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);
                let previousURI = tab.uri;

                // Update the tab state
                tab.uri = args.uri;
                tab.uriToShow = args.uriToShow;
                tab.canGoBack = args.canGoBack;
                tab.canGoForward = args.canGoForward;

                // Don't add history entry if URI has not changed
                if (tab.uri == previousURI) {
                    break;
                }

                // Filter URIs that should not appear in history
                if (!tab.uri || tab.uri == 'about:blank') {
                    tab.historyItemId = INVALID_HISTORY_ID;
                    break;
                }

                if (tab.uriToShow && tab.uriToShow.substring(0, 10) == 'browser://') {
                    tab.historyItemId = INVALID_HISTORY_ID;
                    break;
                }

                addHistoryItem(historyItemFromTab(args.tabId), (id) => {
                    tab.historyItemId = id;
                });
            }
            break;
        case commands.MG_NAV_STARTING:
            if (isValidTabId(args.tabId)) {
                // Update the tab state
                tabs.get(args.tabId).isLoading = true;
            }
            break;
        case commands.MG_NAV_COMPLETED:
            if (isValidTabId(args.tabId)) {
                // Update tab state
                tabs.get(args.tabId).isLoading = false;
            }
            break;
        case commands.MG_UPDATE_TAB:
            if (isValidTabId(args.tabId)) {
                const tab = tabs.get(args.tabId);
                const tabElement = document.getElementById(`tab-${args.tabId}`);

                if (!tabElement) {
                    refreshTabs();
                    return;
                }

                // Update tab label
                // Use given title or fall back to a generic tab title
                tab.title = args.title || 'Tab';
                const tabLabel = tabElement.firstChild;
                const tabLabelSpan = tabLabel.firstChild;
                tabLabelSpan.textContent = tab.title;

                // Update title in history item
                // Browser pages will keep an invalid history ID
                if (tab.historyItemId != INVALID_HISTORY_ID) {
                    updateHistoryItem(tab.historyItemId, historyItemFromTab(args.tabId));
                }
            }
            break;
        case commands.MG_CLOSE_WINDOW:
            closeWindow();
            break;
        case commands.MG_SOCKET_ESTABLISHED:
            document.getElementById('ip-field').setAttribute("disabled", true);
            document.getElementById('port-field').setAttribute("disabled", true);
            break;
        default:
            console.log(`Received unexpected message: ${JSON.stringify(event.data)}`);
    }
};

function navigateActiveTab(uri, isSearch) {
    var message = {
        message: commands.MG_NAVIGATE,
        args: {
            uri: uri,
            encodedSearchURI: isSearch ? uri : getSearchURI(uri)
        }
    };

    window.chrome.webview.postMessage(message);
}

function reloadActiveTabContent() {
    var message = {
        message: commands.MG_RELOAD,
        args: {}
    };
    window.chrome.webview.postMessage(message);
}

function containsIlegalCharacters(query) {
    return !VALID_URI_REGEX.test(query);
}

function isSingleWord(query) {
    return WORD_REGEX.test(query);
}

function hasScheme(query) {
    return SCHEMED_URI_REGEX.test(query);
}

function closeWindow() {
    var message = {
        message: commands.MG_CLOSE_WINDOW,
        args: {}
    };

    window.chrome.webview.postMessage(message);
}

function loadTabUI(tabId) {
    if (isValidTabId(tabId)) {
        let tab = tabs.get(tabId);

        let tabElement = document.createElement('div');
        tabElement.className = tabId == activeTabId ? 'tab-active' : 'tab';
        tabElement.id = `tab-${tabId}`;

        let tabLabel = document.createElement('div');
        tabLabel.className = 'tab-label';

        let labelText = document.createElement('span');
        labelText.textContent = tab.title;
        tabLabel.appendChild(labelText);

        tabElement.appendChild(tabLabel);

        var createTabButton = document.getElementById('btn-new-tab');
        document.getElementById('tabs-strip').insertBefore(tabElement, createTabButton);
    }
}

function refreshControls() {
    let controlsElement = document.getElementById('controls-bar');
    if (controlsElement) {
        controlsElement.remove();
    }

    controlsElement = document.createElement('div');
    controlsElement.id = 'controls-bar';

    // Navigation controls
    let navControls = document.createElement('div');
    navControls.className = 'controls-group';
    navControls.id = 'nav-controls-container';

    let connectButton = document.createElement('div');
    connectButton.className = 'btn';
    connectButton.id = 'btn-connect';
    connectButton.title = 'Connect to controller';
    navControls.append(connectButton);

    let sendprgButton = document.createElement('div');
    sendprgButton.className = 'btn';
    sendprgButton.id = 'btn-sendprg';
    sendprgButton.title = 'Send program to controller';
    navControls.append(sendprgButton);

    let delButton = document.createElement('div');
    delButton.className = 'btn';
    delButton.id = 'btn-del';
    delButton.title = 'Clear program list';
    navControls.append(delButton);

    /*
        let saveprgButton = document.createElement('div');
        saveprgButton.className = 'btn';
        saveprgButton.id = 'btn-saveprg';
        saveprgButton.title = 'Save program';
        navControls.append(saveprgButton);

        let confButton = document.createElement('div');
        confButton.className = 'btn';
        confButton.id = 'btn-saveconf';
        confButton.title = 'Save configuration';
        navControls.append(confButton);
    */



    controlsElement.append(navControls);

    let addressBar = document.createElement('div');
    addressBar.id = 'address-bar-container';

    let ipInput = document.createElement('input');
    ipInput.id = 'ip-field';
    ipInput.placeholder = 'IP address';
    ipInput.type = 'text';
    ipInput.value = '192.168.0.22';
    ipInput.spellcheck = false;
    addressBar.append(ipInput);

    let portInput = document.createElement('input');
    portInput.id = 'port-field';
    portInput.placeholder = 'Port number';
    portInput.type = 'text';
    portInput.value = '81';
    portInput.spellcheck = false;
    addressBar.append(portInput);

    let perSwitch = document.createElement('div');
    perSwitch.className = 'btn';
    perSwitch.id = 'btn-switcher';
    perSwitch.title = 'Send command immidiately';
    perSwitch.innerHTML = '<label class="switch"><input type="checkbox"><span class="slider round"></span></label>';
    addressBar.append(perSwitch);
    controlsElement.append(addressBar);


    // Insert controls bar into document
    let tabsElement = document.getElementById('tabs-strip');
    if (tabsElement) {
        tabsElement.parentElement.insertBefore(controlsElement, tabsElement);
    } else {
        let bodyElement = document.getElementsByTagName('body')[0];
        bodyElement.append(controlsElement);
    }

    addControlsListeners();
}

function refreshTabs() {
    let tabsStrip = document.getElementById('tabs-strip');
    if (tabsStrip) {
        tabsStrip.remove();
    }

    tabsStrip = document.createElement('div');
    tabsStrip.id = 'tabs-strip';

    let newTabButton = document.createElement('div');
    newTabButton.id = 'btn-new-tab';

    let buttonSpan = document.createElement('span');
    buttonSpan.textContent = '+';
    buttonSpan.id = 'plus-label';
    tabsStrip.append(newTabButton);

    let bodyElement = document.getElementsByTagName('body')[0];
    bodyElement.append(tabsStrip);

    Array.from(tabs).map((tabEntry) => {
        loadTabUI(tabEntry[0]);
    });
}

function addControlsListeners() {
    /*
    let inputField = document.querySelector('#address-field');
    let clearButton = document.querySelector('#btn-clear');

    inputField.addEventListener('keypress', function(e) {
        var key = e.which || e.keyCode;
        if (key === 13) { // 13 is enter
            e.preventDefault();
            processAddressBarInput();
        }
    });
    */


    document.querySelector('#btn-connect').addEventListener('click', function(e) {
        if (document.getElementById('btn-connect').className === 'btn') {
            var message = {
                message: commands.MG_INIT_SOCKET,
                args: { ipAddr: "ws://" + document.getElementById('ip-field').value + ":" + document.getElementById('port-field').value }
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-sendprg').addEventListener('click', function(e) {
        if (document.getElementById('btn-sendprg').className === 'btn') {
            var message = {
                message: commands.MG_SEND_GCODE,
                args: { ipAddr: "ws://" + document.getElementById('ip-field').value + ":" + document.getElementById('port-field').value }
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-del').addEventListener('click', function(e) {
        if (document.getElementById('btn-sendprg').className === 'btn') {
            var message = {
                message: commands.MG_CLEAR_LIST,
                args: { ipAddr: "ws://" + document.getElementById('ip-field').value + ":" + document.getElementById('port-field').value }
            };
            window.chrome.webview.postMessage(message);
        }
    });

    document.querySelector('#btn-switcher').addEventListener('click', function(e) {
        if (e.pointerId == 1) {
            if (document.getElementById('btn-switcher').className === 'btn') {
                var message = {
                    message: commands.MG_DIRECT_SWITCHER,
                    args: { ipAddr: "ws://" + document.getElementById('ip-field').value + ":" + document.getElementById('port-field').value }
                };
                window.chrome.webview.postMessage(message);
            }
        }
    });

    /*
    window.onkeydown = function(event) {
        if (event.ctrlKey) {
            switch (event.key) {
                case 'r':
                case 'R':
                    reloadActiveTabContent();
                    break;
                case 'd':
                case 'D':
                    toggleFavorite();
                    break;
                case 't':
                case 'T':
                    createNewTab(true);
                    break;
                case 'p':
                case 'P':
                case '+':
                case '-':
                case '_':
                case '=':
                    break;
                default:
                    return;
            }

            event.preventDefault();
        }
    };
    */
}

function init() {
    window.chrome.webview.addEventListener('message', messageHandler);
    refreshControls();
    refreshTabs();

    createNewTab(true);
}

init();