#include <cef_app.h>
#include <cef_client.h>
#include <cef_life_span_handler.h>
#include <cef_load_handler.h>
#include <cef_render_handler.h>
#include <iostream>
#include <SDL3/SDL.h>
#include <sstream>
#include <stdio.h>
#include <wrapper/cef_helpers.h>
#include <chrono>
#include <string>
#include <mutex>

class RenderHandler:public CefRenderHandler {
public:
    RenderHandler(SDL_Renderer * renderer, int w, int h):
        width(w),
        height(h),
        renderer(renderer) {
            // Create an SDL texture to store the browser's pixel data
            texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
            if (!texture) {
                std::cerr << "Failed to create SDL texture: " << SDL_GetError() << std::endl;
            }
        }

    ~RenderHandler() {
        // Destroy the SDL texture when the handler is no longer needed
        if (texture) SDL_DestroyTexture(texture);
        renderer = nullptr; // Don't own the renderer, just nullify
    }

    // CefRenderHandler methods
    virtual void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override {
        // Provide the dimensions of the view to CEF
        rect = CefRect(0, 0, width, height);
    }

    virtual void OnPaint(CefRefPtr<CefBrowser> browser,
                         PaintElementType type,
                         const RectList &dirtyRects,
                         const void *buffer,
                         int w,
                         int h) override {
        // This method is called when the browser needs to redraw

        // Debug messages:
        // std::cout << "OnPaint called! Type: " << type << ", Buffer size: " << w * h * 4 << " bytes." << std::endl;

        // Copy the browser's pixel buffer to the SDL texture
        if (w == width && h == height) {
            unsigned char *texture_data = nullptr;
            int texture_pitch = 0;
            size_t bufferSize = static_cast<size_t>(w) * static_cast<size_t>(h) * 4;
            SDL_LockTexture(texture, nullptr, (void **) &texture_data, &texture_pitch);
            memcpy(texture_data, buffer, bufferSize); // 4 bytes per pixel (ARGB)
            SDL_UnlockTexture(texture);
        }
    }

    // Resize the internal texture when the window size changes
    void resize(int w, int h) {
        if (w == width && h == height) {
            return;
        }

        if (texture) SDL_DestroyTexture(texture);
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!texture) {
            std::cerr << "Failed to re-create SDL texture on resize: " << SDL_GetError() << std::endl;
        }
        SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);

        width = w;
        height = h;
    }

    // Render the CEF texture to the SDL renderer
    void render() {
        if (texture) {
            SDL_RenderTexture(renderer, texture, nullptr, nullptr);
        }
    }

private:
    int width;
    int height;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;

    // Implement CEF reference counting for proper memory management
    IMPLEMENT_REFCOUNTING(RenderHandler);
};

// BrowserClient class handles various browser events
class BrowserClient: public CefClient, public CefLifeSpanHandler, public CefLoadHandler {
public:
    BrowserClient(CefRefPtr<CefRenderHandler> ptr): handler(ptr) {}

    // CefClient methods to provide specific handlers
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override {
        return this;
    }

    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override {
        return this;
    }

    virtual CefRefPtr<CefRenderHandler> GetRenderHandler() override {
        return handler;
    }

    // CefLifeSpanHandler methods.
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override {
        // Must be executed on the UI thread.
        CEF_REQUIRE_UI_THREAD();
        browser_id = browser->GetIdentifier();
    }

    bool DoClose(CefRefPtr<CefBrowser> browser) override {
        // Must be executed on the UI thread.
        CEF_REQUIRE_UI_THREAD();
        if (browser->GetIdentifier() == browser_id) {
            closing = true; // Signal that the main window is closing
        }
        return false; // Allow the close event to proceed
    }

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {}

    // CefLoadHandler methods
    void OnLoadEnd(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int httpStatusCode) override {
        std::cout << "OnLoadEnd(" << httpStatusCode << ")" << std::endl;
        loaded = true; // Signal that the page has finished loading
    }

    virtual void OnLoadError(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             cef_errorcode_t errorCode,
                             const CefString& errorText,
                             const CefString& failedUrl) override
    {
        std::cerr << "OnLoadError(): Error Code: " << errorCode
                  << ", Error Text: " << errorText.ToString()
                  << ", Failed URL: " << failedUrl.ToString() << std::endl;
        loaded = true; // Consider it loaded even if there was an error
    }

    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack, bool canGoForward) override {
        std::cout << "OnLoadingStateChange(isLoading: " << isLoading << ")" << std::endl;
    }

    virtual void OnLoadStart(CefRefPtr<CefBrowser> browser,
                             CefRefPtr<CefFrame> frame,
                             cef_transition_type_t transition_type) override {
        std::cout << "OnLoadStart()" << std::endl;
    }

    bool closeAllowed() const {
        return closing;
    }

    bool isLoaded() const {
        return loaded;
    }

private:
    int browser_id = 0;
    bool closing = false;
    bool loaded = false;
    CefRefPtr<CefRenderHandler> handler;

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

// Custom CefApp implementation to handle command-line arguments
class SimpleCefApp : public CefApp {
public:
    SimpleCefApp() {}

    // CefApp methods
    virtual void OnBeforeCommandLineProcessing(const CefString& process_type,
                                               CefRefPtr<CefCommandLine> command_line) override {
        std::cout << "OnBeforeCommandLineProcessing for process type: " << process_type.ToString() << std::endl;

        // Hardware acceleration on x11
        command_line->AppendSwitchWithValue("use-gl", "angle");

        // Disable hardware acceleration
        // command_line->AppendSwitch("disable-gpu");

        // Optionally
        command_line->AppendSwitchWithValue("log-file", "cef_debug.log");
        command_line->AppendSwitchWithValue("log-severity", "verbose");
    }

private:
    IMPLEMENT_REFCOUNTING(SimpleCefApp);
};


// Helper function to translate SDL mouse button events to CEF mouse button types
CefBrowserHost::MouseButtonType translateMouseButton(const SDL_MouseButtonEvent& sdlButton) {
    CefBrowserHost::MouseButtonType result = MBT_LEFT;
    switch (sdlButton.button) {
        case SDL_BUTTON_LEFT:
            result = MBT_LEFT;
            break;
        case SDL_BUTTON_MIDDLE:
            result = MBT_MIDDLE;
            break;
        case SDL_BUTTON_RIGHT:
            result = MBT_RIGHT;
            break;
        default:
            result = MBT_LEFT; // Default to left for unknown buttons
            break;
    }
    return result;
}

int main(int argc, char * argv[]) {
    CefRefPtr<SimpleCefApp> app = new SimpleCefApp();

    CefMainArgs args(argc, argv);
    auto exitCode = CefExecuteProcess(args, app, nullptr);
    if (exitCode >= 0) {
        return exitCode;
    }

    CefSettings settings;

    std::string base_path_str = SDL_GetBasePath();
    if (base_path_str.empty()) {
        std::cerr << "SDL_GetBasePath() returned empty. Cannot set cache path." << std::endl;
    }
    std::string cache_dir = base_path_str + "cef_user_data/";
    CefString(&settings.cache_path) = cache_dir;

    // Set paths for CEF resources and locales.
    // SDL_GetBasePath() provides the directory where the executable is located.
    std::ostringstream ss_locales;
    ss_locales << SDL_GetBasePath() << "locales/";
    CefString(&settings.locales_dir_path) = ss_locales.str();
    CefString(&settings.resources_dir_path) = SDL_GetBasePath();

    settings.windowless_rendering_enabled = true;
    settings.multi_threaded_message_loop = false;
    settings.external_message_pump = true;

    // Initialize the CEF browser process.
    // Pass our custom CefApp instance to CefInitialize.
    bool cef_initialized = CefInitialize(args, settings, app.get(), nullptr);
    if (!cef_initialized) {
        std::cerr << "CEF initialization failed!" << std::endl;
        return 1; // Indicate an error
    }

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        CefShutdown(); // Ensure CEF is shut down even if SDL fails
        return SDL_APP_FAILURE;
    }

    int width = 950;
    int height = 750;

    // Create an SDL window
    auto window = SDL_CreateWindow("Render CEF with SDL3", width, height, SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window) {
        SDL_Log("Couldn't initialize window: %s", SDL_GetError());
        CefShutdown(); // Ensure CEF is shut down even if SDL fails
        return SDL_APP_FAILURE;
    }

    auto renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) {
        SDL_Log("Couldn't initialize renderer: %s", SDL_GetError());
        CefShutdown(); // Ensure CEF is shut down even if SDL fails
        return SDL_APP_FAILURE;
    }

    SDL_StartTextInput(window);
    SDL_Event e;
    CefRefPtr<RenderHandler> renderHandler = new RenderHandler(renderer, width, height);
    CefRefPtr<BrowserClient> browserClient = new BrowserClient(renderHandler);
    CefWindowInfo window_info;
    CefBrowserSettings browserSettings;
    window_info.SetAsWindowless(0); // 0 means no transparency (site background color)

    CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(
        window_info,
        browserClient.get(),
        "https://google.com", // Initial URL
        browserSettings,
        nullptr,
        nullptr
    );

    bool shutdown = false;

    // Main application loop
    while (!browserClient->closeAllowed()) {
        // Process SDL events
        while (!shutdown && SDL_PollEvent(&e) != 0) {
            switch (e.type) {
                case SDL_EVENT_QUIT:
                    shutdown = true;
                    browser->GetHost()->CloseBrowser(false);
                    break;

                case SDL_EVENT_TEXT_INPUT: {
                    CefKeyEvent event;
                    event.type = KEYEVENT_CHAR; // Event for a committed character
                    event.modifiers = 0;
                    for (int i = 0; e.text.text[i] != '\0'; ++i) {
                        CefKeyEvent char_event = event; // Copy the base event
                        char_event.character = e.text.text[i]; // Assign the character byte
                        browser->GetHost()->SendKeyEvent(char_event);
                    }
                    break;
                }

                case SDL_EVENT_KEY_DOWN: {
                    CefKeyEvent event;
                    event.modifiers = e.key.mod;
                    event.windows_key_code = e.key.key;

                    event.type = KEYEVENT_RAWKEYDOWN;
                    browser->GetHost()->SendKeyEvent(event);
                    break;
                }

                case SDL_EVENT_KEY_UP: {
                    CefKeyEvent event;
                    event.modifiers = e.key.mod;
                    event.windows_key_code = e.key.key;

                    event.type = KEYEVENT_KEYUP;

                    browser->GetHost()->SendKeyEvent(event);
                    break;
                }

                case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                    renderHandler->resize(e.window.data1, e.window.data2);
                    browser->GetHost()->WasResized();
                    break;

                case SDL_EVENT_WINDOW_FOCUS_GAINED:
                    browser->GetHost()->SetFocus(true);
                    break;

                case SDL_EVENT_WINDOW_FOCUS_LOST:
                    browser->GetHost()->SetFocus(false);
                    break;

                case SDL_EVENT_WINDOW_HIDDEN:
                case SDL_EVENT_WINDOW_MINIMIZED:
                    browser->GetHost()->WasHidden(true);
                    break;

                case SDL_EVENT_WINDOW_SHOWN:
                case SDL_EVENT_WINDOW_RESTORED:
                    browser->GetHost()->WasHidden(false);
                    break;

                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    // Push a QUIT event to cleanly exit the loop
                    e.type = SDL_EVENT_QUIT;
                    SDL_PushEvent(&e);
                    break;

                case SDL_EVENT_MOUSE_MOTION: {
                    CefMouseEvent event;
                    event.x = e.motion.x;
                    event.y = e.motion.y;
                    browser->GetHost()->SendMouseMoveEvent(event, false);
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_UP: {
                    CefMouseEvent event;
                    event.x = e.button.x;
                    event.y = e.button.y;
                    browser->GetHost()->SendMouseClickEvent(event, translateMouseButton(e.button), true, 1);
                    break;
                }

                case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                    CefMouseEvent event;
                    event.x = e.button.x;
                    event.y = e.button.y;
                    browser->GetHost()->SetFocus(true);
                    browser->GetHost()->SendMouseClickEvent(event, translateMouseButton(e.button), false, 1);
                    break;
                }

                case SDL_EVENT_MOUSE_WHEEL: {
                    int delta_x = e.wheel.x;
                    int delta_y = e.wheel.y;

                    // Adjust delta based on wheel direction for consistent behavior
                    if (SDL_MOUSEWHEEL_FLIPPED == e.wheel.direction) {
                        delta_y *= -1;
                    } else {
                        delta_x *= -1; // Assuming horizontal wheel is less common, but handle it
                    }

                    CefMouseEvent event;
                    browser->GetHost()->SendMouseWheelEvent(event, delta_x, delta_y);
                    break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        CefDoMessageLoopWork();
        SDL_RenderClear(renderer);
        renderHandler->render();
        SDL_RenderPresent(renderer);
    }

    browser = nullptr;
    browserClient = nullptr;
    renderHandler = nullptr;

    // Shut down CEF
    CefShutdown();
    SDL_DestroyRenderer(renderer);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
