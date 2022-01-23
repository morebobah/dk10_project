#include "MachineWindow.h"
#include "shlobj.h"
#include <Urlmon.h>
#pragma comment (lib, "Urlmon.lib")

using namespace Microsoft::WRL;

WCHAR MachineWindow::s_windowClass[] = { 0 };
WCHAR MachineWindow::s_title[] = { 0 };

ATOM MachineWindow::RegisterClass(_In_ HINSTANCE hInstance)
{
    // Initialize window class string
    LoadStringW(hInstance, IDC_MACHINEAPP, s_windowClass, MAX_LOADSTRING);
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProcStatic;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MACHINE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_MACHINEAPP);
    wcex.lpszClassName = s_windowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL_MACHINE));

    return RegisterClassExW(&wcex);
}

LRESULT CALLBACK MachineWindow::WndProcStatic(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // Get the ptr to the BrowserWindow instance who created this hWnd.
    // The pointer was set when the hWnd was created during InitInstance.
    MachineWindow* browser_window = reinterpret_cast<MachineWindow*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    if (browser_window != nullptr)
    {
        return browser_window->WndProc(hWnd, message, wParam, lParam);  // Forward message to instance-aware WndProc
    }
    else
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

LRESULT CALLBACK MachineWindow::WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

    switch (message)
    {
    case WM_GETMINMAXINFO:
    {
        MINMAXINFO* minmax = reinterpret_cast<MINMAXINFO*>(lParam);
        minmax->ptMinTrackSize.x = m_minWindowWidth;
        minmax->ptMinTrackSize.y = m_minWindowHeight;
    }
    break;
    case WM_DPICHANGED:
    {
        UpdateMinWindowSize();
    }
    case WM_SIZE:
    {
        ResizeUIWebViews();
        if (m_tabs.find(m_activeTabId) != m_tabs.end())
        {
            m_tabs.at(m_activeTabId)->ResizeWebView();
        }
    }
    break;
    case WM_CLOSE:
    {
        web::json::value jsonObj = web::json::value::parse(L"{}");
        jsonObj[L"message"] = web::json::value(MG_CLOSE_WINDOW);
        jsonObj[L"args"] = web::json::value::parse(L"{}");

        CheckFailure(PostJsonToWebView(jsonObj, m_controlsWebView.Get()), L"Try again.");
    }
    break;
    case WM_NCDESTROY:
    {
        SetWindowLongPtr(hWnd, GWLP_USERDATA, NULL);
        delete this;
        PostQuitMessage(0);
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        EndPaint(hWnd, &ps);
    }
    break;
    default:
    {
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    break;
    }
    return 0;
}

BOOL MachineWindow::LaunchWindow(_In_ HINSTANCE hInstance, _In_ int nCmdShow)
{
    // BrowserWindow keeps a reference to itself in its host window and will
    // delete itself when the window is destroyed.
    MachineWindow* window = new MachineWindow();
    if (!window->InitInstance(hInstance, nCmdShow))
    {
        delete window;
        return FALSE;
    }
    return TRUE;
}

BOOL MachineWindow::InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    m_hInst = hInstance; // Store app instance handle
    LoadStringW(m_hInst, IDS_APP_TITLE, s_title, MAX_LOADSTRING);

    SetUIMessageBroker();
    SetUIScriptExecutor();

    m_hWnd = CreateWindowW(s_windowClass, s_title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, m_hInst, nullptr);
    DWORD dwErr = GetLastError();
    if (!m_hWnd)
    {
        return FALSE;
    }

    // Make the BrowserWindow instance ptr available through the hWnd
    SetWindowLongPtr(m_hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    UpdateMinWindowSize();
    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Get directory for user data. This will be kept separated from the
    // directory for the browser UI data.
    std::wstring userDataDirectory = GetAppDataDirectory();
    userDataDirectory.append(L"\\User Data");

    // Create WebView environment for web content requested by the user. All
    // tabs will be created from this environment and kept isolated from the
    // browser UI. This enviroment is created first so the UI can request new
    // tabs when it's ready.
    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, userDataDirectory.c_str(),
        nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
    {
        RETURN_IF_FAILED(result);

        m_contentEnv = env;
        HRESULT hr = InitUIWebViews();

        if (!SUCCEEDED(hr))
        {
            OutputDebugString(L"UI WebViews environment creation failed\n");
        }

        return hr;
    }).Get());

    if (!SUCCEEDED(hr))
    {
        OutputDebugString(L"Content WebViews environment creation failed\n");
        return FALSE;
    }

    return TRUE;
}

HRESULT MachineWindow::InitUIWebViews()
{
    // Get data directory for browser UI data
    std::wstring browserDataDirectory = GetAppDataDirectory();
    browserDataDirectory.append(L"\\Browser Data");

    // Create WebView environment for browser UI. A separate data directory is
    // used to isolate the browser UI from web content requested by the user.
    return CreateCoreWebView2EnvironmentWithOptions(nullptr, browserDataDirectory.c_str(),
        nullptr, Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
    {
        // Environment is ready, create the WebView
        m_uiEnv = env;

        RETURN_IF_FAILED(CreateBrowserControlsWebView());

        return S_OK;
    }).Get());
}

std::wstring MachineWindow::GetAppDataDirectory()
{
    TCHAR path[MAX_PATH];
    std::wstring dataDirectory;
    HRESULT hr = SHGetFolderPath(nullptr, CSIDL_APPDATA, NULL, 0, path);
    if (SUCCEEDED(hr))
    {
        dataDirectory = std::wstring(path);
        dataDirectory.append(L"\\Tomson\\");
    }
    else
    {
        dataDirectory = std::wstring(L".\\");
    }

    dataDirectory.append(s_title);
    return dataDirectory;
}

HRESULT MachineWindow::CreateBrowserControlsWebView()
{
    return m_uiEnv->CreateCoreWebView2Controller(m_hWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
        [this](HRESULT result, ICoreWebView2Controller* host) -> HRESULT
    {
        if (!SUCCEEDED(result))
        {
            OutputDebugString(L"Controls WebView creation failed\n");
            return result;
        }
        // WebView created
        m_controlsController = host;
        CheckFailure(m_controlsController->get_CoreWebView2(&m_controlsWebView), L"");

        wil::com_ptr<ICoreWebView2Settings> settings;
        RETURN_IF_FAILED(m_controlsWebView->get_Settings(&settings));
        RETURN_IF_FAILED(settings->put_AreDevToolsEnabled(FALSE));

        RETURN_IF_FAILED(m_controlsController->add_ZoomFactorChanged(Callback<ICoreWebView2ZoomFactorChangedEventHandler>(
            [](ICoreWebView2Controller* host, IUnknown* args) -> HRESULT
        {
            host->put_ZoomFactor(1.0);
            return S_OK;
        }
        ).Get(), &m_controlsZoomToken));

        RETURN_IF_FAILED(m_controlsWebView->add_WebMessageReceived(m_uiMessageBroker.Get(), &m_controlsUIMessageBrokerToken));
        RETURN_IF_FAILED(ResizeUIWebViews());

        std::wstring controlsPath = GetFullPathFor(L"wvbrowser_ui\\controls_ui\\default.html");
        RETURN_IF_FAILED(m_controlsWebView->Navigate(controlsPath.c_str()));

        return S_OK;
    }).Get());
}


void MachineWindow::UpdateMinWindowSize()
{
    RECT clientRect;
    RECT windowRect;

    GetClientRect(m_hWnd, &clientRect);
    GetWindowRect(m_hWnd, &windowRect);

    int bordersWidth = (windowRect.right - windowRect.left) - clientRect.right;
    int bordersHeight = (windowRect.bottom - windowRect.top) - clientRect.bottom;

    m_minWindowWidth = GetDPIAwareBound(MIN_WINDOW_WIDTH) + bordersWidth;
    m_minWindowHeight = GetDPIAwareBound(MIN_WINDOW_HEIGHT) + bordersHeight;
}

void MachineWindow::CheckFailure(HRESULT hr, LPCWSTR errorMessage)
{
    if (FAILED(hr))
    {
        std::wstring message;
        if (!errorMessage || !errorMessage[0])
        {
            message = std::wstring(L"Something went wrong.");
        }
        else
        {
            message = std::wstring(errorMessage);
        }

        MessageBoxW(nullptr, message.c_str(), nullptr, MB_OK);
    }
}

std::wstring MachineWindow::GetFullPathFor(LPCWSTR relativePath)
{
    WCHAR path[MAX_PATH];
    GetModuleFileNameW(m_hInst, path, MAX_PATH);
    std::wstring pathName(path);

    std::size_t index = pathName.find_last_of(L"\\") + 1;
    pathName.replace(index, pathName.length(), relativePath);

    return pathName;
}

HRESULT MachineWindow::ResizeUIWebViews()
{
    if (m_controlsWebView != nullptr)
    {
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        bounds.bottom = bounds.top + GetDPIAwareBound(c_uiBarHeight);
        bounds.bottom += 1;

        RETURN_IF_FAILED(m_controlsController->put_Bounds(bounds));
    }

    if (m_optionsWebView != nullptr)
    {
        RECT bounds;
        GetClientRect(m_hWnd, &bounds);
        bounds.top = GetDPIAwareBound(c_uiBarHeight);
        bounds.bottom = bounds.top + GetDPIAwareBound(c_optionsDropdownHeight);
        bounds.left = bounds.right - GetDPIAwareBound(c_optionsDropdownWidth);

        RETURN_IF_FAILED(m_optionsController->put_Bounds(bounds));
    }

    // Workaround for black controls WebView issue in Windows 7
    HWND wvWindow = GetWindow(m_hWnd, GW_CHILD);
    while (wvWindow != nullptr)
    {
        UpdateWindow(wvWindow);
        wvWindow = GetWindow(wvWindow, GW_HWNDNEXT);
    }

    return S_OK;
}


int MachineWindow::GetDPIAwareBound(int bound)
{
    // Remove the GetDpiForWindow call when using Windows 7 or any version
    // below 1607 (Windows 10). You will also have to make sure the build
    // directory is clean before building again.
    return (bound * GetDpiForWindow(m_hWnd) / DEFAULT_DPI);
}


void MachineWindow::SetUIMessageBroker()
{
    m_uiMessageBroker = Callback<ICoreWebView2WebMessageReceivedEventHandler>(
        [this](ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs) -> HRESULT
    {
        wil::unique_cotaskmem_string jsonString;
        CheckFailure(eventArgs->get_WebMessageAsJson(&jsonString), L"");  // Get the message from the UI WebView as JSON formatted string
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

        switch (message)
        {
        case MG_CREATE_TAB:
        {
            size_t id = args.at(L"tabId").as_number().to_uint32();
            bool shouldBeActive = args.at(L"active").as_bool();
            std::unique_ptr<Tab> newTab = Tab::CreateNewTab(m_hWnd, m_contentEnv.Get(), id, shouldBeActive);

            std::map<size_t, std::unique_ptr<Tab>>::iterator it = m_tabs.find(id);
            if (it == m_tabs.end())
            {
                m_tabs.insert(std::pair<size_t, std::unique_ptr<Tab>>(id, std::move(newTab)));
            }
            else
            {
                m_tabs.at(id)->m_contentController->Close();
                it->second = std::move(newTab);
            }
        }
        break;
        case MG_CLOSE_WINDOW:
        {
            DestroyWindow(m_hWnd);
        }
        break;
        case MG_INIT_SOCKET:
        {
            std::string s = utility::conversions::to_utf8string(args.at(L"ipAddr").as_string());
            //MessageBox(NULL, L"Init connection", L"Socket", MB_OK);
            web::json::value jsonSnd = web::json::value::parse(L"{}");
            jsonSnd[L"message"] = web::json::value(MG_INIT_SOCKET);
            jsonSnd[L"args"] = jsonObj.at(L"args");
            PostJsonToWebView(jsonSnd, m_tabs.at(m_activeTabId)->m_contentWebView.Get());
            PostJsonToWebView(jsonSnd, m_controlsWebView.Get());
        }
        break;
        case MG_SOCKET_ESTABLISHED:
        {
            m_tabs.at(m_activeTabId)->m_contentWebView->ExecuteScript(L"initSocket()", m_uiScriptExecutor.Get());
        }
        break;
        case MG_DIRECT_SWITCHER:
        {
            //MessageBox(NULL, L"Init connection", L"Socket", MB_OK);
            web::json::value jsonSnd = web::json::value::parse(L"{}");
            jsonSnd[L"message"] = web::json::value(MG_DIRECT_SWITCHER);
            jsonSnd[L"args"] = jsonObj.at(L"args");
            PostJsonToWebView(jsonSnd, m_tabs.at(m_activeTabId)->m_contentWebView.Get());
            //PostJsonToWebView(jsonSnd, m_controlsWebView.Get());
        }
        break;
        case MG_SEND_GCODE:
        {
            m_tabs.at(m_activeTabId)->m_contentWebView->ExecuteScript(L"sendgcode()", m_uiScriptExecutor.Get());
        }
        break;
        case MG_CLEAR_LIST:
        {
            if (MessageBox(m_hWnd, L"This operation will clear all of program commands.\nDo you want complete it?", L"Clearing program", MB_OKCANCEL) == IDOK) {
                web::json::value jsonSnd = web::json::value::parse(L"{}");
                jsonSnd[L"message"] = web::json::value(MG_CLEAR_LIST);
                jsonSnd[L"args"] = jsonObj.at(L"args");
                PostJsonToWebView(jsonSnd, m_tabs.at(m_activeTabId)->m_contentWebView.Get());
            }
            //web::json::value jsonSnd = web::json::value::parse(L"{}");
            //jsonSnd[L"message"] = web::json::value(MG_DIRECT_SWITCHER);
            //jsonSnd[L"args"] = jsonObj.at(L"args");
            //PostJsonToWebView(jsonSnd, m_tabs.at(m_activeTabId)->m_contentWebView.Get());
            //PostJsonToWebView(jsonSnd, m_controlsWebView.Get());
            //m_tabs.at(m_activeTabId)->m_contentWebView->ExecuteScript(L"sendgcode()", m_uiScriptExecutor.Get());
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

void MachineWindow::SetUIScriptExecutor() {
    m_uiScriptExecutor = Callback<ICoreWebView2ExecuteScriptCompletedHandler>(
        [this](HRESULT hr, LPCWSTR result) -> HRESULT
    {
        return S_OK;
    });
}


HRESULT MachineWindow::SwitchToTab(size_t tabId)
{
    
    size_t previousActiveTab = m_activeTabId;

    RETURN_IF_FAILED(m_tabs.at(tabId)->ResizeWebView());
    RETURN_IF_FAILED(m_tabs.at(tabId)->m_contentController->put_IsVisible(TRUE));
    m_activeTabId = tabId;

    if (previousActiveTab != INVALID_TAB_ID && previousActiveTab != m_activeTabId)
    {
        RETURN_IF_FAILED(m_tabs.at(previousActiveTab)->m_contentController->put_IsVisible(FALSE));
    }

    return S_OK;
}

void MachineWindow::HandleTabCreated(size_t tabId, bool shouldBeActive)
{
    if (shouldBeActive)
    {
        CheckFailure(SwitchToTab(tabId), L"");
    }
}

HRESULT MachineWindow::HandleTabURIUpdate(size_t tabId, ICoreWebView2* webview)
{
    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    web::json::value jsonObj = web::json::value::parse(L"{}");
    jsonObj[L"message"] = web::json::value(MG_UPDATE_URI);
    jsonObj[L"args"] = web::json::value::parse(L"{}");
    jsonObj[L"args"][L"tabId"] = web::json::value::number(tabId);
    jsonObj[L"args"][L"uri"] = web::json::value(source.get());

    std::wstring uri(source.get());

    RETURN_IF_FAILED(PostJsonToWebView(jsonObj, m_controlsWebView.Get()));

    return S_OK;
}


HRESULT MachineWindow::HandleTabMessageReceived(size_t tabId, ICoreWebView2* webview, ICoreWebView2WebMessageReceivedEventArgs* eventArgs)
{
    wil::unique_cotaskmem_string jsonString;
    RETURN_IF_FAILED(eventArgs->get_WebMessageAsJson(&jsonString));
    web::json::value jsonObj = web::json::value::parse(jsonString.get());

    wil::unique_cotaskmem_string uri;
    RETURN_IF_FAILED(webview->get_Source(&uri));

    int message = jsonObj.at(L"message").as_integer();
    web::json::value args = jsonObj.at(L"args");

    wil::unique_cotaskmem_string source;
    RETURN_IF_FAILED(webview->get_Source(&source));

    switch (message)
    {
    default:
    {
        OutputDebugString(L"Unexpected message\n");
    }
    break;
    }

    return S_OK;
}

std::wstring MachineWindow::GetFilePathAsURI(std::wstring fullPath)
{
    std::wstring fileURI;
    ComPtr<IUri> uri;
    DWORD uriFlags = Uri_CREATE_ALLOW_IMPLICIT_FILE_SCHEME;
    HRESULT hr = CreateUri(fullPath.c_str(), uriFlags, 0, &uri);

    if (SUCCEEDED(hr))
    {
        wil::unique_bstr absoluteUri;
        uri->GetAbsoluteUri(&absoluteUri);
        fileURI = std::wstring(absoluteUri.get());
    }

    return fileURI;
}

HRESULT MachineWindow::PostJsonToWebView(web::json::value jsonObj, ICoreWebView2* webview)
{
    utility::stringstream_t stream;
    jsonObj.serialize(stream);

    return webview->PostWebMessageAsJson(stream.str().c_str());
}

HRESULT MachineWindow::ConnectionEstablished(Microsoft::WRL::ComPtr<ICoreWebView2> m_contentWebView, web::json::value jsonObj)
{
    web::json::value jsonSnd = web::json::value::parse(L"{}");
    jsonSnd[L"message"] = web::json::value(MG_SOCKET_ESTABLISHED);
    jsonSnd[L"args"] = jsonObj.at(L"args");
    PostJsonToWebView(jsonSnd, m_contentWebView.Get());
    PostJsonToWebView(jsonSnd, m_controlsWebView.Get());
    return S_OK;
}

HRESULT MachineWindow::SocketDisconnect(Microsoft::WRL::ComPtr<ICoreWebView2> m_contentWebView, web::json::value jsonObj)
{
    web::json::value jsonSnd = web::json::value::parse(L"{}");
    jsonSnd[L"message"] = web::json::value(MG_SOCKET_DISCONNECT);
    jsonSnd[L"args"] = jsonObj.at(L"args");
    PostJsonToWebView(jsonSnd, m_contentWebView.Get());
    PostJsonToWebView(jsonSnd, m_controlsWebView.Get());
    return S_OK;
}
