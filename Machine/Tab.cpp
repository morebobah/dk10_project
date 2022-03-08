// Copyright (C) Microsoft Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "MachineWindow.h"
#include "Tab.h"
#include <commdlg.h>

using namespace Microsoft::WRL;

DWORD g_BytesTransferred = 0;

VOID CALLBACK FileIOCompletionRoutine(
    __in  DWORD dwErrorCode,
    __in  DWORD dwNumberOfBytesTransfered,
    __in  LPOVERLAPPED lpOverlapped)
{
    _tprintf(TEXT("Error code:\t%x\n"), dwErrorCode);
    _tprintf(TEXT("Number of bytes:\t%x\n"), dwNumberOfBytesTransfered);
    g_BytesTransferred = dwNumberOfBytesTransfered;
}

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
        case MG_LOAD_PROGRAM:
        {
            OPENFILENAME ofn;       // common dialog box structure
            TCHAR szFile[260] = { 0 };       // if using TCHAR macros
            char   ReadBuffer[BUFFERSIZE] = { 0 };
            OVERLAPPED ol = { 0 };
            DWORD  dwBytesRead = 0;

            // Initialize OPENFILENAME
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = this->m_parentHWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = _T("JSON\0*.json\0All\0*.*\0");
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
            web::json::value jsonForSend = web::json::value::parse(L"{}");
            jsonForSend[L"message"] = web::json::value(MG_LOAD_PROGRAM);
            //jsonForSend[L"name"] = web::json::value(L"Программа сложной автоматики");
            //jsonForSend[L"gcode"] = web::json::value(L"G1M00V1G1M01V1G1M02V1G1S03V1+M02V1G1S04V1+M02V1G1M05V1G1M02V1G1M00V1G1M01V1G1M02V1G1W06V3+S03V1+M05V1G2M02V1G2S03V1+M02V1G2T1+M01V1");

            if (GetOpenFileName(&ofn) == TRUE)
            {
                HANDLE hFile = ::CreateFile(ofn.lpstrFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile == INVALID_HANDLE_VALUE) {
                    break;
                }
                if (FALSE == ReadFileEx(hFile, ReadBuffer, BUFFERSIZE - 1, &ol, FileIOCompletionRoutine))
                {
                    CloseHandle(hFile);
                    break;
                }
                SleepEx(5000, TRUE);
                dwBytesRead = g_BytesTransferred;
                if (dwBytesRead > 0 && dwBytesRead <= BUFFERSIZE - 1)
                {
                    ReadBuffer[dwBytesRead] = '\0'; // NULL character
                    //PostJsonToWebView(jsonForSend, m_contentWebView.Get());
                    DWORD dwPos = 0;
                    for (DWORD dwI = 0; dwI < dwBytesRead; dwI++) {
                        if(ReadBuffer[dwI]=='\r')ReadBuffer[dwI] = '\0';
                        if (ReadBuffer[dwI] == '\n') {
                            ReadBuffer[dwI] = '\0';
                            dwPos = dwI + 1;
                        }
                    }
                    jsonForSend[L"name"] = web::json::value(CA2CT(ReadBuffer));
                    jsonForSend[L"gcode"] = web::json::value(CA2CT(ReadBuffer + dwPos));
                    PostJsonToWebView(jsonForSend, m_contentWebView.Get());
                }
                ::CloseHandle(hFile);
            }
            //m_contentWebView->ExecuteScript(L"initSocket()", m_uiScriptExecutor.Get());
        }
        break;
        case MG_SAVE_PROGRAM:
        {
            OPENFILENAME ofn;       // common dialog box structure
            TCHAR szFile[260] = { 0 };       // if using TCHAR macros

            // Initialize OPENFILENAME
            ZeroMemory(&ofn, sizeof(ofn));
            ofn.lStructSize = sizeof(ofn);
            ofn.hwndOwner = this->m_parentHWnd;
            ofn.lpstrFile = szFile;
            ofn.nMaxFile = sizeof(szFile);
            ofn.lpstrFilter = _T("JSON\0*.json\0All\0*.*\0");
            ofn.nFilterIndex = 1;
            ofn.lpstrFileTitle = NULL;
            ofn.nMaxFileTitle = 0;
            ofn.lpstrInitialDir = NULL;
            ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

            if (GetSaveFileName(&ofn) == TRUE)
            {
                HANDLE hFile = ::CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile == INVALID_HANDLE_VALUE) {
                    break;
                }
                DWORD dwBytesWritten = 0;
                try {
                    if (!WriteFile(hFile, CT2CA(args[L"prg"].as_string().c_str()), args[L"prg"].as_string().length(), &dwBytesWritten, NULL)) throw 1;
                    if (!WriteFile(hFile, "\r\n", 2, &dwBytesWritten, NULL))throw 1;
                    if (!WriteFile(hFile, CT2CA(args[L"gcode"].as_string().c_str()), args[L"gcode"].as_string().length(), &dwBytesWritten, NULL)) throw 1;
                    MessageBox(this->m_parentHWnd, L"Файл успешно сохранен", L"Сохранение", MB_OK | MB_ICONINFORMATION);
                }
                catch (...) {
                    MessageBox(this->m_parentHWnd, L"Ошибка записи.", L"Error", MB_OK | MB_ICONEXCLAMATION);
                }
                
                ::CloseHandle(hFile);
            }
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

HRESULT Tab::PostJsonToWebView(web::json::value jsonObj, ICoreWebView2* webview)
{
    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    return webview->PostWebMessageAsJson(stream.str().c_str());
}