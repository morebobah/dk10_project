// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "MachineWindow.h"
#include "Tab.h"

using namespace Microsoft::WRL;

std::unique_ptr<Tab> Tab::CreateNewTab(HWND hWnd, ICoreWebView2Environment* env, size_t id, bool shouldBeActive)
{
    std::unique_ptr<Tab> tab = std::make_unique<Tab>();

    tab->m_parentHWnd = hWnd;
    tab->m_tabId = id;
    tab->SetMessageBroker();
    tab->Init(env, shouldBeActive);

    return tab;
}

HRESULT Tab::Init(ICoreWebView2Environment* env, bool shouldBeActive)
{
    return env->CreateCoreWebView2Controller(m_parentHWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this, shouldBeActive](HRESULT result, ICoreWebView2Controller* host) -> HRESULT {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Tab WebView creation failed\n");
            return result;
        }
        m_contentController = host;
        MachineWindow::CheckFailure(m_contentController->get_CoreWebView2(&m_contentWebView), L"");
        MachineWindow* machineWindow = reinterpret_cast<MachineWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
        RETURN_IF_FAILED(m_contentWebView->add_WebMessageReceived(m_messageBroker.Get(), &m_messageBrokerToken));

        // Register event handler for source change
        RETURN_IF_FAILED(m_contentWebView->add_SourceChanged(Callback<ICoreWebView2SourceChangedEventHandler>(
            [this, machineWindow](ICoreWebView2* webview, ICoreWebView2SourceChangedEventArgs* args) -> HRESULT
        {
            MachineWindow::CheckFailure(machineWindow->HandleTabURIUpdate(m_tabId, webview), L"Can't update address bar");

            return S_OK;
        }).Get(), &m_uriUpdateForwarderToken));

        // Enable listening for security events to update secure icon
        //RETURN_IF_FAILED(m_contentWebView->CallDevToolsProtocolMethod(L"Security.enable", L"{}", nullptr));

        //MachineWindow::CheckFailure(m_contentWebView->GetDevToolsProtocolEventReceiver(L"Security.securityStateChanged", &m_securityStateChangedReceiver), L"");

        std::wstring controlsPath = machineWindow->GetFullPathFor(L"wvbrowser_ui\\content_ui\\main.html");
        RETURN_IF_FAILED(m_contentWebView->Navigate(controlsPath.c_str()));
        machineWindow->HandleTabCreated(m_tabId, shouldBeActive);

        return S_OK;
    }).Get());
}

void Tab::SetMessageBroker()
{
    m_messageBroker = Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs) -> HRESULT
    {
        MachineWindow* machineWindow = reinterpret_cast<MachineWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
        MachineWindow::CheckFailure(machineWindow->HandleTabMessageReceived(m_tabId, webview, eventArgs), L"");
        wil::unique_cotaskmem_string jsonString;
        MachineWindow::CheckFailure(eventArgs->get_WebMessageAsJson(&jsonString), L"");  // Get the message from the UI WebView as JSON formatted string
        web::json::value jsonObj = web::json::value::parse(jsonString.get());

        if (!jsonObj.has_field(L"message"))
        {
            OutputDebugString(L"No message code provided\n");
            return S_OK;
        }

        if (!jsonObj.has_field(L"args"))
        {
            OutputDebugString(L"The message has no args field\n");
            return S_OK;
        }

        int message = jsonObj.at(L"message").as_integer();
        web::json::value args = jsonObj.at(L"args");

        switch (message) {
        case MG_SOCKET_ESTABLISHED:
        {
            machineWindow->ConnectionEstablished(m_contentWebView, jsonObj);
            //MessageBox(NULL, L"Connection established", L"Socket", MB_OK);
            //m_contentWebView->ExecuteScript(L"initSocket()", m_uiScriptExecutor.Get());
        }
        break;
        case MG_SOCKET_DISCONNECT:
        {
            machineWindow->SocketDisconnect(m_contentWebView, jsonObj);
            //MessageBox(NULL, L"Connection established", L"Socket", MB_OK);
            //m_contentWebView->ExecuteScript(L"initSocket()", m_uiScriptExecutor.Get());
        }
        break;
        default:
        {
            OutputDebugString(L"Unexpected message\n");
        }
        break;
        }

        return S_OK;
    });
}

HRESULT Tab::ResizeWebView()
{
    RECT bounds;
    GetClientRect(m_parentHWnd, &bounds);

    MachineWindow* machineWindow = reinterpret_cast<MachineWindow*>(GetWindowLongPtr(m_parentHWnd, GWLP_USERDATA));
    bounds.top += machineWindow->GetDPIAwareBound(MachineWindow::c_uiBarHeight);

    return m_contentController->put_Bounds(bounds);
}
