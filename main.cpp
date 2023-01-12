#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <tchar.h>
#include <cmath>
#include <corecrt_math_defines.h>



/* for ImPlot */
#include "implot.h"

#include "K6dijkstra.hpp"
#include "lexikalischeAnalyse.hpp"

/* for ImNotify */
#include "imgui_notify.h"
#include "tahoma.h" // <-- Required font!

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

#ifdef DX12_ENABLE_DEBUG_LAYER
#include <dxgidebug.h>
#pragma comment(lib, "dxguid.lib")
#endif


struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

// Data
static int const                    NUM_FRAMES_IN_FLIGHT = 3;
static FrameContext                 g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
static UINT                         g_frameIndex = 0;

static int const                    NUM_BACK_BUFFERS = 3;
static ID3D12Device*                g_pd3dDevice = NULL;
static ID3D12DescriptorHeap*        g_pd3dRtvDescHeap = NULL;
static ID3D12DescriptorHeap*        g_pd3dSrvDescHeap = NULL;
static ID3D12CommandQueue*          g_pd3dCommandQueue = NULL;
static ID3D12GraphicsCommandList*   g_pd3dCommandList = NULL;
static ID3D12Fence*                 g_fence = NULL;
static HANDLE                       g_fenceEvent = NULL;
static UINT64                       g_fenceLastSignaledValue = 0;
static IDXGISwapChain3*             g_pSwapChain = NULL;
static HANDLE                       g_hSwapChainWaitableObject = NULL;
static ID3D12Resource*              g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
static D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};

static char inputBuffer[255]{};
static char outputBuffer[1024]{};
static dijkstra graf(STATIONA);

// Forward declarations of helper functions
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void WaitForLastSubmittedFrame();
FrameContext* WaitForNextFrameResources();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void arpPacketRecieved(arp_paket arp) {
    //memset(&inputBuffer[0], 0, sizeof(inputBuffer));
    ImGui::InsertNotification({ ImGuiToastType_Success,1000,"Arp Packet Empfangen!" });
    std::stringstream ss;
    if (arp.macAddress == mac("FF:FF:FF:FF:FF:FF"))
        graf.print_all(ss);
    else {
        if (graf.knoten_vorhanden(arp.macAddress)) {
            Knoten gefundenerKnoten = graf.macToKnoten(arp.macAddress);
            if (arp.displayoption > gefundenerKnoten.maxSupportedDisplayOption) {
                ss << "Dieser Knoten unterstuetzt diese Displayoption nicht!\n";
                goto startParsing;
            }
            ss << gefundenerKnoten.name.c_str() << " found\n";
            graf.print_path(ss, gefundenerKnoten.macAddress, arp.displayoption);
        }
        else {
            ss << "Knoten " << arp.macAddress.macString.c_str() << " not found";
        }
    }
startParsing:
    strcpy(outputBuffer, ss.str().c_str());
}
static void errorCallback(std::string errorMessage) {
    printf("%s\n", errorMessage.c_str());
    ImGui::InsertNotification({ ImGuiToastType_Error,3000,errorMessage.c_str()});
}

float distanze(float x1,float y1,float x2,float y2) {
    return sqrt((x2 - x1) * (x2 - x1) + (y2-y1)* (y2 - y1));
}

static void HelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

// Main code
int main(int, char**)
{
    // Create application window
    //ImGui_ImplWin32_EnableDpiAwareness();
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Testing Text 2", NULL };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Pseudo ARP mit Graph", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

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
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    io.Fonts->AddFontFromMemoryTTF((void*)tahoma,sizeof(tahoma),17.f,&font_cfg);

    // Initialize notify
    ImGui::MergeIconsWithLatestFont(16.f, false);

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX12_Init(g_pd3dDevice, NUM_FRAMES_IN_FLIGHT,
        DXGI_FORMAT_R8G8B8A8_UNORM, g_pd3dSrvDescHeap,
        g_pd3dSrvDescHeap->GetCPUDescriptorHandleForHeapStart(),
        g_pd3dSrvDescHeap->GetGPUDescriptorHandleForHeapStart());

    // Our state
    bool show_demo_window = false;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    
    /* Created new graph for plotting! */
    
    ImVec2 nodePositions[ANZAHL] = { {0, 0} };
    ImVec2 nodePositionsBackup[ANZAHL] = { {0, 0} };
    ImVec2 nodeForces[ANZAHL][2] = { {{0, 0}} };
    static double now = (double)time(0);
    srand(now);
    {
        /* callculate distance from center manually */
        int distances[ANZAHL] = { 0 };
        for (int i = 0; i < ANZAHL; i++) {
            int j = i;
            while (graf.n_info[j].predecessor != -1) {
                distances[i]++;
                j = graf.n_info[j].predecessor;
            }
        }
        int alreadyPlotted = 1;
        int currentLayer = 1;
        float angles[ANZAHL] = {};
        while (alreadyPlotted != ANZAHL) {
            int nodeid = 0;
            /* fill up vector with all nodes on current layer */
            std::vector<int> nodesOnThis;
            for (auto& ah : distances) {
                if (ah == currentLayer)
                    nodesOnThis.push_back(nodeid);
                nodeid++;
            }
            int sizer = nodesOnThis.size();
            /* sort nodes in vector*/
            std::vector<float> usedAngles;
            int lastNode = 0;
            int lastlastNode;
            for (int iRot = 0; iRot < sizer; iRot++) {
                float ohaX, ohaY;
                if(currentLayer == 1){
                    if (iRot == 0) {
                        lastNode = nodesOnThis.at(iRot);
                        angles[lastNode] = 0;
                        printf("Node with id %i\n",lastNode);
                        usedAngles.push_back(0);
                    }
                    else if(iRot == 1) {
                        int closestNode;
                        int closestDistance = xxx;
                        for (int yy = 1; yy < sizer; ++yy) {
                            int disti = graf.adistance[lastNode][nodesOnThis.at(yy)];
                            if ((disti < closestDistance) && (lastNode != nodesOnThis.at(yy))) {
                                closestNode = nodesOnThis.at(yy);
                                closestDistance = disti;
                            }
                        }
                        lastlastNode = lastNode;
                        lastNode = closestNode;
                        printf("Node with id %i\n", lastNode);
                        angles[lastNode] = 2 * M_PI * iRot / sizer;
                    }
                    else {
                        int closestNode;
                        int closestDistance = xxx;
                        for (int yy = 1; yy < sizer; ++yy) {
                            int disti = graf.adistance[lastNode][nodesOnThis.at(yy)];
                            if ((disti < closestDistance) && (lastNode != nodesOnThis.at(yy)) && (lastlastNode != nodesOnThis.at(yy))) {
                                closestNode = nodesOnThis.at(yy);
                                closestDistance = disti;
                            }
                        }
                        lastlastNode = lastNode;
                        lastNode = closestNode;
                        printf("Node with id %i\n", lastNode);
                        angles[lastNode] = 2 * M_PI * iRot / sizer;
                    }
                    
                    }
                    else {
                    lastNode = nodesOnThis.at(iRot);
                        float newAngle = angles[graf.n_info[lastNode].predecessor];
                        int nmbNotFound = 0;
                        while (std::find(usedAngles.begin(), usedAngles.end(), newAngle) != usedAngles.end()) {
                            newAngle = angles[graf.n_info[lastNode].predecessor] +
                                (2 * (nmbNotFound % 2 - 1) * 2 * M_PI *
                                    ((int)((nmbNotFound + 1) / 2)) / 32);
                            nmbNotFound++;
                        }
                        usedAngles.push_back(newAngle);
                        angles[lastNode] = newAngle;
                    }

                ohaX = 1.0f * currentLayer * sin(angles[lastNode]);
                ohaY = 1.0f * currentLayer * cos(angles[lastNode]);

                nodePositions[lastNode] = {
                    ohaX + (float)(0.1 * rand() / RAND_MAX),
                    ohaY + (float)(0.1 * rand() / RAND_MAX) };
            }
            alreadyPlotted += sizer;
            currentLayer++;
        }
    }

    //memcpy(nodePositionsBackup, nodePositions,sizeof(nodePositions));
    std::copy(std::begin(nodePositions), std::end(nodePositions), std::begin(nodePositionsBackup));
    CParser obj;
    obj.InitParse(nullptr, stderr, stdout);
    // Main loop
    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        
        // Start the Dear ImGui frame
        ImGui_ImplDX12_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
        if (show_demo_window)
            ImGui::ShowDemoWindow(&show_demo_window);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(io.DisplaySize);
        // 2. Show a simple window that we create ourselves. We use a Begin/End pair
        // to created a named window.
        {
            static float fg = 0.0f;
            static float fp = 0.0f;
            static bool doForces = false;

            ImGui::Begin("Hello, world!", NULL,
                ImGuiWindowFlags_NoDecoration |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoBackground); // Create a window called
            // "Hello, world!" and
            // append into it.
            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            // ImGui::Text("This is some useful text."); // Display some text (you can
            //                                           // use a format strings too)
            //ImGui::Checkbox( "UI Options (will be removed)",&show_demo_window);
            ImGui::Checkbox("Auflistung aller Knoten", &show_another_window);
            ImGui::NewLine();
            ImGui::SliderFloat("Gravitational Force", &fg, 0.0f, 100.0f,"%.4f", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("Node Connection Force", &fp, 0.0f, 10.0f, "%.4f", ImGuiSliderFlags_Logarithmic);
            ImGui::Checkbox("Apply Forces", &doForces);
            ImGui::SameLine();
            if (ImGui::Button("Reset Graph")) {
                doForces = false;
                std::copy(std::begin(nodePositionsBackup), std::end(nodePositionsBackup), std::begin(nodePositions));
            }
            ImGui::NewLine();
            // ImGui::ColorEdit3(
            //     "clear color",
            //     (float *)&clear_color); // Edit 3 floats representing a color

            {
                ImGui::Columns(2, nullptr, false); // You set 2 columns
                //ImGui::SetColumnOffset(1, 200);
                
                {
                    if (ImPlot::BeginPlot("Graphen Plotter", ImVec2(-1, -1), ImPlotFlags_CanvasOnly | ImPlotFlags_NoFrame)) {
                        ImPlot::SetupAxes("", "", ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                        int id = 100;
                        /*calc gravity - hoch 3, weil raum ist qubisch*/
                        for (int i = 1; i < ANZAHL; i++) {
                            nodeForces[i][0] = { 0, 0 };
                            for (int j = 0; j < ANZAHL; j++) {
                                if (i != j) {
                                    nodeForces[i][0].x +=
                                        0.00000001 / ((nodePositions[i].x - nodePositions[j].x) *
                                            (nodePositions[i].x - nodePositions[j].x) *
                                            (nodePositions[i].x - nodePositions[j].x));
                                    nodeForces[i][0].y +=
                                        0.00000001 / ((nodePositions[i].y - nodePositions[j].y) *
                                            (nodePositions[i].y - nodePositions[j].y) *
                                            (nodePositions[i].y - nodePositions[j].y));
                                }
                            }
                        }
                        // calc pull
                        for (int i = 1; i < ANZAHL; i++) {
                            std::vector<int> connectedNodes;
                            for (int ii = 0; ii < ANZAHL; ii++) {
                                if ((i != ii) && (graf.adistance[i][ii] != xxx))
                                    connectedNodes.push_back(ii);
                            }
                            nodeForces[i][1] = {};
                            for (auto& node : connectedNodes) {
                                float dist = ((nodePositions[i].x - nodePositions[node].x) *
                                    (nodePositions[i].x - nodePositions[node].x)) +
                                    ((nodePositions[i].y - nodePositions[node].y) *
                                        (nodePositions[i].y - nodePositions[node].y));
                                dist *=
                                    dist * dist; // increase quadratic with distance (for testing)

                                if (dist > graf.adistance[i][node]) {
                                    nodeForces[i][1].x += (nodePositions[node].x - nodePositions[i].x) /
                                        dist;
                                    nodeForces[i][1].y += (nodePositions[node].y - nodePositions[i].y) /
                                        dist;
                                }
                            }
                        }
                        // appling forces
                        if (doForces) {
                            for (int iNodes = 0; iNodes < ANZAHL; iNodes++) {
                                nodePositions[iNodes].x +=
                                    (fg * nodeForces[iNodes][0].x + fp * nodeForces[iNodes][1].x);
                                nodePositions[iNodes].y +=
                                    (fg * nodeForces[iNodes][0].y + fp * nodeForces[iNodes][1].y);
                            }
                        }

                        /* Line Drawing */
                        ImPlot::PlotText(graf.knoten[0].name.c_str(), nodePositions[0].x,
                            nodePositions[0].y);
                        for (int i = 1; i < ANZAHL; i++) {
                            float lines[2][2] = { {0}, {0} };

                            //int j = i;
                            //int linlen = 1;
                            for (int kk = i - 1; kk >= 0; --kk) {
                                if (graf.adistance[i][kk] != xxx) {
                                    lines[0][0] = nodePositions[i].x;
                                    lines[1][0] = nodePositions[i].y;

                                    lines[0][1] = nodePositions[kk].x;
                                    lines[1][1] = nodePositions[kk].y;

                                    ImPlot::PlotLine("##Line", lines[0], lines[1], 2);
                                }
                            }
                            ImPlot::PlotText(graf.knoten[i].name.c_str(), nodePositions[i].x,
                                nodePositions[i].y);
                        }
                        /* Plot distances */
                        for (int i = 1; i < ANZAHL; i++) {
                            for (int uh = 0; uh < i; uh++) {
                                if (graf.adistance[i][uh] != xxx) {
                                    std::string distanceString = std::to_string(graf.adistance[i][uh]);
                                    ImPlot::PlotText(distanceString.c_str(),
                                        (nodePositions[i].x + nodePositions[uh].x) / 2,
                                        (nodePositions[i].y + nodePositions[uh].y) / 2);
                                }
                            }
                        }

                        ImPlot::EndPlot();
                    }

                  


                    ImGui::NextColumn(); // You go into 2nd column
                   
                    //printf("Wdt:%f\n", ImGui::GetColumnWidth(-1));


                    //ImGui::InputTextMultiline("Ausgabe", outputBuffer, sizeof(outputBuffer), ImVec2(0, 0), ImGuiInputTextFlags_ReadOnly);

                    ImGui::BeginChild("Outut",ImVec2(0, 0),true);
                    ImGui::BeginListBox("Ausgabe",ImVec2(-1, 300));
                    ImGui::TextWrapped(outputBuffer);
                    ImGui::EndListBox();

                    if (ImGui::InputTextWithHint("Eingabe", "XX:XX:XX:XX:XX:XX / 2", inputBuffer, sizeof(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll))
                        obj.yyparse(arpPacketRecieved,errorCallback, inputBuffer);
                    ImGui::EndChild();



                    ImGui::NextColumn(); // You put yourself back in the first column

                }
            }

            
            ImGui::End();
        }

        // 3. Show another simple window.
        if (show_another_window) {
            ImGui::Begin(
                "Alle Stationen",
                &show_another_window); // Pass a pointer to our bool variable (the
            // window will have a closing button that will
            // clear the bool when clicked)
            ImGui::BeginChild("Scrolling");
            for (int n = 0; n < ANZAHL; n++)
                ImGui::Text("%s,(%f,%f) with forces:grav(%f,%f),pull(%f,%f)\n",
                    graf.knoten[n].name.c_str(), nodePositions[n].x,
                    nodePositions[n].y, nodeForces[n][0].x, nodeForces[n][0].y,
                    nodeForces[n][1].x, nodeForces[n][1].y);
            ImGui::EndChild();

            

            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();

        }

        // Render toasts on top of everything, at the end of your code!
        // You should push style vars here
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 5.f); // Round borders
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(43.f / 255.f, 43.f / 255.f, 43.f / 255.f, 100.f / 255.f)); // Background color
        ImGui::RenderNotifications(); // <-- Here we render all notifications
        ImGui::PopStyleVar(1); // Don't forget to Pop()
        ImGui::PopStyleColor(1);

        // Rendering
        ImGui::Render();


        FrameContext* frameCtx = WaitForNextFrameResources();
        UINT backBufferIdx = g_pSwapChain->GetCurrentBackBufferIndex();
        frameCtx->CommandAllocator->Reset();

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags                  = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource   = g_mainRenderTargetResource[backBufferIdx];
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
        g_pd3dCommandList->Reset(frameCtx->CommandAllocator, NULL);
        g_pd3dCommandList->ResourceBarrier(1, &barrier);

        // Render Dear ImGui graphics
        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dCommandList->ClearRenderTargetView(g_mainRenderTargetDescriptor[backBufferIdx], clear_color_with_alpha, 0, NULL);
        g_pd3dCommandList->OMSetRenderTargets(1, &g_mainRenderTargetDescriptor[backBufferIdx], FALSE, NULL);
        g_pd3dCommandList->SetDescriptorHeaps(1, &g_pd3dSrvDescHeap);
        ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), g_pd3dCommandList);
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
        g_pd3dCommandList->ResourceBarrier(1, &barrier);
        g_pd3dCommandList->Close();

        g_pd3dCommandQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&g_pd3dCommandList);

        g_pSwapChain->Present(1, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync

        UINT64 fenceValue = g_fenceLastSignaledValue + 1;
        g_pd3dCommandQueue->Signal(g_fence, fenceValue);
        g_fenceLastSignaledValue = fenceValue;
        frameCtx->FenceValue = fenceValue;
    }

    WaitForLastSubmittedFrame();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC1 sd;
    {
        ZeroMemory(&sd, sizeof(sd));
        sd.BufferCount = NUM_BACK_BUFFERS;
        sd.Width = 0;
        sd.Height = 0;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.Flags = DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        sd.Scaling = DXGI_SCALING_STRETCH;
        sd.Stereo = FALSE;
    }

    // [DEBUG] Enable debug interface
#ifdef DX12_ENABLE_DEBUG_LAYER
    ID3D12Debug* pdx12Debug = NULL;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pdx12Debug))))
        pdx12Debug->EnableDebugLayer();
#endif

    // Create device
    D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
    if (D3D12CreateDevice(NULL, featureLevel, IID_PPV_ARGS(&g_pd3dDevice)) != S_OK)
        return false;

    // [DEBUG] Setup debug interface to break on any warnings/errors
#ifdef DX12_ENABLE_DEBUG_LAYER
    if (pdx12Debug != NULL)
    {
        ID3D12InfoQueue* pInfoQueue = NULL;
        g_pd3dDevice->QueryInterface(IID_PPV_ARGS(&pInfoQueue));
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        pInfoQueue->Release();
        pdx12Debug->Release();
    }
#endif

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = NUM_BACK_BUFFERS;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dRtvDescHeap)) != S_OK)
            return false;

        SIZE_T rtvDescriptorSize = g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = g_pd3dRtvDescHeap->GetCPUDescriptorHandleForHeapStart();
        for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        {
            g_mainRenderTargetDescriptor[i] = rtvHandle;
            rtvHandle.ptr += rtvDescriptorSize;
        }
    }

    {
        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = 1;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        if (g_pd3dDevice->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&g_pd3dSrvDescHeap)) != S_OK)
            return false;
    }

    {
        D3D12_COMMAND_QUEUE_DESC desc = {};
        desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        desc.NodeMask = 1;
        if (g_pd3dDevice->CreateCommandQueue(&desc, IID_PPV_ARGS(&g_pd3dCommandQueue)) != S_OK)
            return false;
    }

    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_frameContext[i].CommandAllocator)) != S_OK)
            return false;

    if (g_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_frameContext[0].CommandAllocator, NULL, IID_PPV_ARGS(&g_pd3dCommandList)) != S_OK ||
        g_pd3dCommandList->Close() != S_OK)
        return false;

    if (g_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_fence)) != S_OK)
        return false;

    g_fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (g_fenceEvent == NULL)
        return false;

    {
        IDXGIFactory4* dxgiFactory = NULL;
        IDXGISwapChain1* swapChain1 = NULL;
        if (CreateDXGIFactory1(IID_PPV_ARGS(&dxgiFactory)) != S_OK)
            return false;
        if (dxgiFactory->CreateSwapChainForHwnd(g_pd3dCommandQueue, hWnd, &sd, NULL, NULL, &swapChain1) != S_OK)
            return false;
        if (swapChain1->QueryInterface(IID_PPV_ARGS(&g_pSwapChain)) != S_OK)
            return false;
        swapChain1->Release();
        dxgiFactory->Release();
        g_pSwapChain->SetMaximumFrameLatency(NUM_BACK_BUFFERS);
        g_hSwapChainWaitableObject = g_pSwapChain->GetFrameLatencyWaitableObject();
    }

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->SetFullscreenState(false, NULL); g_pSwapChain->Release(); g_pSwapChain = NULL; }
    if (g_hSwapChainWaitableObject != NULL) { CloseHandle(g_hSwapChainWaitableObject); }
    for (UINT i = 0; i < NUM_FRAMES_IN_FLIGHT; i++)
        if (g_frameContext[i].CommandAllocator) { g_frameContext[i].CommandAllocator->Release(); g_frameContext[i].CommandAllocator = NULL; }
    if (g_pd3dCommandQueue) { g_pd3dCommandQueue->Release(); g_pd3dCommandQueue = NULL; }
    if (g_pd3dCommandList) { g_pd3dCommandList->Release(); g_pd3dCommandList = NULL; }
    if (g_pd3dRtvDescHeap) { g_pd3dRtvDescHeap->Release(); g_pd3dRtvDescHeap = NULL; }
    if (g_pd3dSrvDescHeap) { g_pd3dSrvDescHeap->Release(); g_pd3dSrvDescHeap = NULL; }
    if (g_fence) { g_fence->Release(); g_fence = NULL; }
    if (g_fenceEvent) { CloseHandle(g_fenceEvent); g_fenceEvent = NULL; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }

#ifdef DX12_ENABLE_DEBUG_LAYER
    IDXGIDebug1* pDebug = NULL;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&pDebug))))
    {
        pDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_SUMMARY);
        pDebug->Release();
    }
#endif
}

void CreateRenderTarget()
{
    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
    {
        ID3D12Resource* pBackBuffer = NULL;
        g_pSwapChain->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer));
        g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, g_mainRenderTargetDescriptor[i]);
        g_mainRenderTargetResource[i] = pBackBuffer;
    }
}

void CleanupRenderTarget()
{
    WaitForLastSubmittedFrame();

    for (UINT i = 0; i < NUM_BACK_BUFFERS; i++)
        if (g_mainRenderTargetResource[i]) { g_mainRenderTargetResource[i]->Release(); g_mainRenderTargetResource[i] = NULL; }
}

void WaitForLastSubmittedFrame()
{
    FrameContext* frameCtx = &g_frameContext[g_frameIndex % NUM_FRAMES_IN_FLIGHT];

    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue == 0)
        return; // No fence was signaled

    frameCtx->FenceValue = 0;
    if (g_fence->GetCompletedValue() >= fenceValue)
        return;

    g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
    WaitForSingleObject(g_fenceEvent, INFINITE);
}

FrameContext* WaitForNextFrameResources()
{
    UINT nextFrameIndex = g_frameIndex + 1;
    g_frameIndex = nextFrameIndex;

    HANDLE waitableObjects[] = { g_hSwapChainWaitableObject, NULL };
    DWORD numWaitableObjects = 1;

    FrameContext* frameCtx = &g_frameContext[nextFrameIndex % NUM_FRAMES_IN_FLIGHT];
    UINT64 fenceValue = frameCtx->FenceValue;
    if (fenceValue != 0) // means no fence was signaled
    {
        frameCtx->FenceValue = 0;
        g_fence->SetEventOnCompletion(fenceValue, g_fenceEvent);
        waitableObjects[1] = g_fenceEvent;
        numWaitableObjects = 2;
    }

    WaitForMultipleObjects(numWaitableObjects, waitableObjects, TRUE, INFINITE);

    return frameCtx;
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
        if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            WaitForLastSubmittedFrame();
            CleanupRenderTarget();
            HRESULT result = g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT);
            assert(SUCCEEDED(result) && "Failed to resize swapchain.");
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
