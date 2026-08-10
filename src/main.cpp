#include "main.h"

// Data
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);


void PrintHistory(const std::multimap<float, TypeOperation>& history, bool max, int limit) {
    if (history.empty()) [[unlikely]] {
        return;
    }

    if (!(max)) [[likely]] {
        for (auto element_history : history) {
            if (element_history.second == TypeOperation::transfer_in) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.8f, 0.1f, 1.0f));
                ImGui::Text("+%0.2f", element_history.first);
                ImGui::PopStyleColor(1);
                ImGui::Spacing();
            }
            else if (element_history.second == TypeOperation::transfer_out) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
                ImGui::Text("-%0.2f", element_history.first);
                ImGui::PopStyleColor(1);
                ImGui::Spacing();
            }
            else if (element_history.second == TypeOperation::purchase) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
                ImGui::Text("-%0.2f", element_history.first);
                ImGui::PopStyleColor(1);
                ImGui::Spacing();
            }
            limit -= 1;
            if (limit <= 0) {
                return;
            }
        }
    }
    else {
        for (auto iter = history.rbegin(); iter != history.rend(); iter++) {
            if (iter->second == TypeOperation::transfer_in) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.8f, 0.1f, 1.0f));
                ImGui::Text("+%0.2f", iter->first);
                ImGui::PopStyleColor(1);
                ImGui::Spacing();
            }
            else if (iter->second == TypeOperation::transfer_out) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
                ImGui::Text("-%0.2f", iter->first);
                ImGui::PopStyleColor(1);
                ImGui::Spacing();
            }
            else if (iter->second == TypeOperation::purchase) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.25f, 0.25f, 1.0f));
                ImGui::Text("-%0.2f", iter->first);
                ImGui::PopStyleColor(1);
                ImGui::Spacing();
            }
            limit -= 1;
            if (limit <= 0) {
                return;
            }
        }
    }
}

bool FindSpecChar(const std::string& str) {
    if (str.find("@") != std::string::npos) { return true; }
    else if (str.find("#") != std::string::npos) { return true; }
    else if (str.find("!") != std::string::npos) { return true; }
    else if (str.find("&") != std::string::npos) { return true; }
    else if (str.find("*") != std::string::npos) { return true; }
    else if (str.find("%") != std::string::npos) { return true; }
    else if (str.find("^") != std::string::npos) { return true; }
    else if (str.find("_") != std::string::npos) { return true; }
    else if (str.find("(") != std::string::npos) { return true; }
    else if (str.find(")") != std::string::npos) { return true; }
    return false;
}

int CheckCoorectInput(const std::string& name, const std::string& surname, const std::string& phone_number, const std::string& login, const std::string& password) {
    if (name.empty() || surname.empty() || phone_number.empty() || login.empty() || password.empty()) {
        return EMPTY_INPUT;
    }
    if (FindSpecChar(name) || FindSpecChar(surname) || FindSpecChar(phone_number) || FindSpecChar(login) || FindSpecChar(password)) {
        return HAVE_SPECIAL_CHARACTERS;
    }
    return CORRECT_INPUT;
}

// Main code
int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Bank System", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(800 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(10);
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // Start the Dear ImGui frame
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);

        // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
        {
            static std::string login{""};
            static std::string password{ "" };
            static std::string name{ "" };
            static std::string surname{ "" };
            static std::string phone_number{ "+7" };
            static float balance{ 0.0f };
            static bool log_in_window_op = true;
            static bool sign_window_op = false;
            static bool account_cabinet_op = false;
            static bool history_op = false;
            static bool transfer_op = false;
            static bool error_account_not_found = false;
            static bool error_account_wrong_data = false;
            bool action = false;
            static bool empty_input = false;
            static bool spec_ch_input = false;
            static bool account_has_cr = false;
            static bool sign_sys_err = false;

            if (log_in_window_op) {
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);

                
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
                    | ImGuiWindowFlags_NoMove
                    | ImGuiWindowFlags_NoResize
                    | ImGuiWindowFlags_NoSavedSettings
                    | ImGuiWindowFlags_NoBringToFrontOnFocus;
                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.851f, 0.851f, 0.851f, 1.0f));
                
                ImGui::Begin("Entering", &log_in_window_op, window_flags);
                float element_width = 300.0f;
                float element_height = 20.0f; 
                ImVec2 window_size = ImGui::GetWindowSize();


                float center_x = (window_size.x - element_width) * 0.5f;
                float center_y = (window_size.y - element_height) * 0.25f;


                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::PushFont(NULL, 30.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("Welcome to our bank");
                ImGui::PopStyleColor(1);
                ImGui::PopFont();

                
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 10.0f));

                center_y += (window_size.y - element_height) * 0.25f * 0.25f;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));       
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));          
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));  
                ImGui::InputTextWithHint("##LoginField", "Enter login", &login);
                ImGui::PopStyleColor(3);

                center_y += (window_size.y - element_height) * 0.25f * 0.25f;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));       
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));         
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));  
                ImGui::InputTextWithHint("##PassField", "Enter password", &password, ImGuiInputTextFlags_Password);
                ImGui::PopStyleColor(3);

                center_y += (window_size.y - element_height) * 0.25f * 0.25f;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));       
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));          
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));  
                ImGui::InputTextWithHint("##PhoneField", "Enter phone number", &phone_number);
                ImGui::PopStyleColor(3);


                ImGui::PopStyleVar();

                if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    action = true;
                }

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));

                center_y += (window_size.y - element_height) * 0.25f * 0.25f;
                ImGui::SetCursorPos(ImVec2(center_x + 0.5f*150.0f, center_y));
                if (ImGui::Button("Log in", ImVec2(150.0f, 33.0f))) {
                    action = true;
                }
                if (action) {
                    int sucsec = Ego_bank.EnterAccount(phone_number, login, password);
                    if (sucsec == OPERATION_OK) {
                        Ego_bank.GetHistoryList(user_history, phone_number);
                        log_in_window_op = false;
                        account_cabinet_op = true;
                        error_account_wrong_data = false;
                        error_account_not_found = false;

                    }
                    else if (sucsec == ACCOUNT_NOT_FOUND) {
                        error_account_not_found = true;
                        error_account_wrong_data = false;
                        login = ""; password = ""; phone_number = "+7";
                    }
                    else if (sucsec == WRONG_DATA) {
                        error_account_wrong_data = true;
                        error_account_not_found = false;
                        login = ""; password = "";
                    }
                }
                if (error_account_not_found) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("There isn't account with this phone number. Please check phone number", &error_account_not_found);
                    ImGui::PopStyleColor(1);
                }
                if (error_account_wrong_data) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("Wrong login or password. Please try again", &error_account_wrong_data);
                    ImGui::PopStyleColor(1);
                }
                
                center_y += (window_size.y - element_height) * 0.1f;
                ImGui::SetCursorPos(ImVec2(center_x+30.0f, center_y));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("Don't have an account?");
                ImGui::PopStyleColor(1);


                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));

                center_y += (window_size.y - element_height) * 0.25f * 0.25f;
                ImGui::SetCursorPos(ImVec2(center_x + 0.5f*150.0f, center_y));
                if (ImGui::Button("Sign up", ImVec2(150.0f, 33.0f))) {
                    login = ""; password = ""; phone_number = "+7";
                    error_account_wrong_data = false;
                    error_account_not_found = false;
                    log_in_window_op = false;
                    sign_window_op = true;
                }

                ImGui::PopStyleColor(7);

                ImGui::End();
            }

            if (sign_window_op) {
               
                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);


                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
                    | ImGuiWindowFlags_NoMove
                    | ImGuiWindowFlags_NoResize
                    | ImGuiWindowFlags_NoSavedSettings
                    | ImGuiWindowFlags_NoBringToFrontOnFocus;



                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.851f, 0.851f, 0.851f, 1.0f));
                ImGui::Begin("Registration", &sign_window_op, window_flags);

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));
                if (ImGui::Button("<- Back")) {
                    login = ""; password = ""; phone_number = "+7"; name = ""; surname = ""; balance = 0.0f;
                    log_in_window_op = true;
                    sign_window_op = false;
                    sign_window_op = false;
                    spec_ch_input = false;
                    sign_sys_err = false;
                    account_has_cr = false;
                    empty_input = false;

                }
                ImGui::PopStyleColor(3);
                float element_width = 300.0f;
                float element_height = 20.0f;
                ImVec2 window_size = ImGui::GetWindowSize();

                float temp = (window_size.y - element_height) * 0.25f * 0.25f;
                float center_x = (window_size.x - element_width) * 0.5f;
                float center_y = (window_size.y - element_height) * 0.25f;
                





                ImGui::SetCursorPos(ImVec2(center_x + 45.0f, center_y));
                ImGui::PushFont(NULL, 30.0f);
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 0.0f, 0.0f, 1.0f));
                ImGui::Text("Registration");
                ImGui::PopStyleColor(1);
                ImGui::PopFont();


                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 10.0f));

                center_y += temp;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));
                (ImGui::InputTextWithHint("##NameField", "Enter name", &name));
                ImGui::PopStyleColor(3);

                center_y += temp;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));
                (ImGui::InputTextWithHint("##SurNameField", "Enter surname", &surname));
                ImGui::PopStyleColor(3);

                center_y += temp;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));
                (ImGui::InputTextWithHint("##PhoneField", "Enter phone number", &phone_number));
                ImGui::PopStyleColor(3);

                center_y += temp;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));
                (ImGui::InputTextWithHint("##LoginField", "Enter login", &login));
                ImGui::PopStyleColor(3);

                center_y += temp;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));
                (ImGui::InputTextWithHint("##PassField", "Enter password", &password, ImGuiInputTextFlags_Password));
                ImGui::PopStyleColor(3);

                center_y += temp;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::SetNextItemWidth(element_width);
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                (ImGui::InputFloat("##balance", &balance));
                ImGui::PopStyleColor(2);
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                ImGui::Text("Enter balance");
                ImGui::PopStyleColor(1);

                ImGui::PopStyleVar();

                if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    action = true;
                }

                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));

                center_y += temp;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                if (ImGui::Button("Sign Up", ImVec2(element_width, 48.0f))) {
                    action = true;
                }
                if (action) {
                    int rez = CheckCoorectInput(name, surname, phone_number, login, password);
                    if (rez == EMPTY_INPUT) {
                        empty_input = true;
                        spec_ch_input = false;
                        sign_sys_err = false;
                        account_has_cr = false;
                    }
                    else if (rez == HAVE_SPECIAL_CHARACTERS) {
                        spec_ch_input = true;
                        empty_input = false;
                        sign_sys_err = false;
                        account_has_cr = false;
                    }
                    else if (rez == CORRECT_INPUT) {
                        Account new_acc(login, password, phone_number, name, surname, balance);
                        int sucses = Ego_bank.CreateAccount(std::move(new_acc));
                        if (sucses == OPERATION_OK) {
                            sign_window_op = false;
                            spec_ch_input = false;
                            sign_sys_err = false;
                            account_has_cr = false;
                            empty_input = false;
                            account_cabinet_op = true;

                        }
                        else if (sucses == ACCOUNT_HAS_CREATED) {
                            account_has_cr = true;
                            spec_ch_input = false;
                            sign_sys_err = false;
                            empty_input = false;
                        }
                        else if (sucses == SYSTEM_ERROR) {
                            sign_sys_err = true;
                            account_has_cr = false;
                            spec_ch_input = false;
                            empty_input = false;
                        }
                    }                 
                }
                if (empty_input) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("Inputs must be not empty", &empty_input);
                    ImGui::PopStyleColor(1);
                }
                if (spec_ch_input) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("Inputs mustn't include special characters", &spec_ch_input);
                    ImGui::PopStyleColor(1);
                }
                if (account_has_cr) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("Account with this has created", &account_has_cr);
                    ImGui::PopStyleColor(1);
                }
                if (sign_sys_err) {
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                    ImGui::Text("System error. Please try again later", &sign_sys_err);
                    ImGui::PopStyleColor(1);
                }

                ImGui::PopStyleColor(4);


                ImGui::End();
            }

            if (account_cabinet_op) {
                //ImVec4(1.0f, 0.867f, 0.176f, 1.0f) yellow
                //ImVec4(0.115f, 0.115f, 0.115f, 1.0f) begie
                //ImVec4(0.086f, 0.616f, 0.867f, 1.0f) dark blue
                //ImVec4(0.392f, 0.753f, 0.863f, 1.0) light blue
                //ImVec4(1.0f, 1.0f, 1.0f, 1.0f) white
                //ImVec4(0.745f, 0.749f, 0.753f, 1.0f) light grey
                //ImVec4(0.718f, 0.718f, 0.718f, 1.0f) dark grey

                user = Ego_bank.FindAccountByPhone(phone_number);
                

                ImGuiViewport* viewport = ImGui::GetMainViewport();
                ImGui::SetNextWindowPos(viewport->WorkPos);
                ImGui::SetNextWindowSize(viewport->WorkSize);

                
                ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration
                    | ImGuiWindowFlags_NoMove
                    | ImGuiWindowFlags_NoResize
                    | ImGuiWindowFlags_NoSavedSettings
                    | ImGuiWindowFlags_NoBringToFrontOnFocus;


                ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.851f, 0.851f, 0.851f, 1.0f));
                
                ImGui::Begin("Cabinet", &account_cabinet_op, window_flags);
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));
                if (ImGui::Button("<- Log out")) {
                    account_cabinet_op = false;
                    log_in_window_op = true;

                }
                ImGui::PopStyleColor(3);

                float element_width = 300.0f;
                float element_height = 20.0f;
                ImVec2 window_size = ImGui::GetWindowSize();
                float center_y = (window_size.y - element_height) * 0.25f;
                float center_x = (window_size.x - element_width) * 0.5f;



                
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::PushFont(NULL, 30.0f);
                ImGui::Text("Hello, %s", user->GetName().c_str());
                center_y += 30.0f;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::Text("You have ");
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.1f, 0.8f, 0.1f, 1.0f));
                ImGui::Text("$%0.2f", user->GetBalance());
                ImGui::PopStyleColor(1);
                ImGui::PopFont();

                center_y += 30.0f;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));
                if (ImGui::Button("Transfer to", ImVec2(180.0f, 35.0f))) {
                    transfer_op = true;
                }
                ImGui::PopStyleColor(3);

                center_y += 40.0f;
                ImGui::SetCursorPos(ImVec2(center_x, center_y));
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));
                if (ImGui::Button("Look history", ImVec2(180.0f, 35.0f))) {
                    history_op = true;
                }
                ImGui::PopStyleColor(3);

                if (transfer_op) {
                    ImGui::OpenPopup("Transfer");
                }

                if (history_op) {
                    ImGui::OpenPopup("History");
                }


                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.718f, 0.718f, 0.718f, 1.0f));
                if (ImGui::BeginPopupModal("Transfer", &transfer_op, ImGuiWindowFlags_AlwaysAutoResize)) {

                    static std::string phone_to = "+7";
                    static float amount_transfer;


                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_TextDisabled, ImVec4(0.50f, 0.50f, 0.53f, 1.0f));
                    ImGui::InputTextWithHint("##PhoneField", "Enter phone number", &phone_to);
                    ImGui::PopStyleColor(3);

                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.95f, 1.0f));
                    ImGui::InputFloat("##balance", &amount_transfer);
                    ImGui::PopStyleColor(2);
                    ImGui::SameLine();
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    ImGui::Text("Enter amount of transfer");
                    ImGui::PopStyleColor(1);

                    ImGui::Separator();

                    if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                        action = true;
                    }

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));
                    if (ImGui::Button("OK", ImVec2(120, 0))) {
                        action = true;
                    }

                    if (action) {
                        /*if (phone_number == "") {
                            empty_input = true;
                            spec_ch_input = false;
                            ImGui::OpenPopup();
                        }*/
                        int rez = Ego_bank.TransferMoney(user->phone_number, phone_to, amount_transfer);

                        if (rez == OPERATION_OK) {
                            Ego_bank.GetHistoryList(user_history, user->phone_number);
                            transfer_op = false;
                            ImGui::CloseCurrentPopup();
                        }
                        else if (rez == ACCOUNT_NOT_FOUND) {
                            ImGui::OpenPopup("Error_msg_acc_not_found");
                        }
                        else if (rez == NOT_ENOUGH_BALANCE) {
                            ImGui::OpenPopup("Error_msg_not_balance");
                        }
                        else if (rez == SYSTEM_ERROR) {
                            ImGui::OpenPopup("Error_msg_sys_err");
                        }
                    }
                    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
                    if (ImGui::BeginPopup("Error_msg_acc_not_found", NULL)) {

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text("There isn't account with this phone number. Please check phone number", NULL);
                        ImGui::PopStyleColor(1);
                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginPopup("Error_msg_not_balance", NULL)) {

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text("Not enough balance for this operation", NULL);
                        ImGui::PopStyleColor(1);
                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginPopup("Error_msg_sys_err", NULL)) {

                        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
                        ImGui::Text("System error. Please try again later", NULL);
                        ImGui::PopStyleColor(1);
                        ImGui::EndPopup();
                    }
                    ImGui::PopStyleColor(1);

                    ImGui::SetItemDefaultFocus();
                    ImGui::SameLine();

                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.48f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.24f, 0.40f, 0.58f, 1.0f));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.12f, 0.24f, 0.38f, 1.0f));
                    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
                        transfer_op = false;
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::PopStyleColor(6);

                    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        transfer_op = false;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::EndPopup();
                }
                ImGui::PopStyleColor(1);

                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
                if (ImGui::BeginPopupModal("History", &history_op)) {
                    if (ImGui::Button("<- Back")) {
                        history_op = false;
                        ImGui::CloseCurrentPopup();

                    }
                    ImGui::SameLine();
                    if (ImGui::BeginTabBar("HistoryTabs")) {
                        if (ImGui::BeginTabItem("5 Min Op")) {
                            PrintHistory(user_history, false, 5);
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("20 op")) {
                            PrintHistory(user_history, false, 20);
                            ImGui::EndTabItem();
                        }
                        if (ImGui::BeginTabItem("5 Max Op")) {
                            PrintHistory(user_history, true, 5);
                            ImGui::EndTabItem();
                        }
                        ImGui::EndTabBar();
                    }
                        
                    
                    
                    
                    
                    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
                        history_op = false;
                        ImGui::CloseCurrentPopup();
                        
                    }


                    ImGui::EndPopup();
                
                }
                ImGui::PopStyleColor(1);

                ImGui::PopStyleColor(1);
                ImGui::End();
            }
                  
        }

        //// 3. Show another simple window.
        //if (show_another_window)
        //{
        //    ImGui::Begin("Another Window", &show_another_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        //    ImGui::Text("Hello from another window!");
        //    if (ImGui::Button("Close Me"))
        //        show_another_window = false;
        //    ImGui::End();
        //}

        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present
        HRESULT hr = g_pSwapChain->Present(1, 0);   // Present with vsync
        //HRESULT hr = g_pSwapChain->Present(0, 0); // Present without vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);





    //Code with BankSystem

    //Ego_bank.PrintUserList();

    

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}
