#include <cef_app.h>
#include <cef_client.h>
#include <cef_life_span_handler.h>
#include <cef_load_handler.h>
#include <cef_render_handler.h>
#include <iostream>
#include <SDL.h>
#include <sstream>
#include <stdio.h>
#include <wrapper/cef_helpers.h>
#include <chrono>
#include <string>

// RenderHandler class for off-screen rendering
class RenderHandler :
    public CefRenderHandler
{
public:
    RenderHandler(SDL_Renderer * renderer, int w, int h) :
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
        if (texture) {
            SDL_DestroyTexture(texture);
        }
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
                         const void * buffer,
                         int w,
                         int h) override {
        // This method is called when the browser needs to redraw
        std::cout << "OnPaint called! Type: " << type << ", Buffer size: " << w * h * 4 << " bytes." << std::endl;
        if (texture) {
            unsigned char * texture_data = NULL;
            int texture_pitch = 0;

            // Lock the texture to update its pixel data
            SDL_LockTexture(texture, 0, (void **)&texture_data, &texture_pitch);
            // Copy the browser's pixel buffer to the SDL texture
            memcpy(texture_data, buffer, w * h * 4); // 4 bytes per pixel (ARGB)
            SDL_UnlockTexture(texture);
        }
    }

    // Resize the internal texture when the window size changes
    void resize(int w, int h) {
        if (texture) {
            SDL_DestroyTexture(texture);
        }
        // Recreate the texture with new dimensions
        texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w, h);
        if (!texture) {
            std::cerr << "Failed to re-create SDL texture on resize: " << SDL_GetError() << std::endl;
        }
        width = w;
        height = h;
    }

    // Render the CEF texture to the SDL renderer
    void render() {
        if (texture) {
            SDL_RenderTexture(renderer, texture, NULL, NULL);
        }
    }

private:
    int width;
    int height;
    SDL_Renderer * renderer = nullptr;
    SDL_Texture * texture = nullptr;

    // Implement CEF reference counting for proper memory management
    IMPLEMENT_REFCOUNTING(RenderHandler);
};

// BrowserClient class handles various browser events
class BrowserClient :
    public CefClient,
    public CefLifeSpanHandler,
    public CefLoadHandler
{
public:
    BrowserClient(CefRefPtr<CefRenderHandler> ptr) :
        handler(ptr)
    {}

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

    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override {
        // Nothing specific to do here for this example
    }

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

    // Custom methods to check application state
    bool closeAllowed() const {
        return closing;
    }

    bool isLoaded() const {
        return loaded;
    }

private:
    int browser_id = 0; // Initialize to 0
    bool closing = false;
    bool loaded = false;
    CefRefPtr<CefRenderHandler> handler; // Reference to the render handler

    IMPLEMENT_REFCOUNTING(BrowserClient);
};

// Custom CefApp implementation to handle command-line arguments
class SimpleCefApp : public CefApp {
public:
    SimpleCefApp() {}

    // CefApp methods
    virtual void OnBeforeCommandLineProcessing(const CefString& process_type,
                                               CefRefPtr<CefCommandLine> command_line) override {
        // This method is called before the command line is processed.
        // We can add or modify command-line switches here.
        std::cout << "OnBeforeCommandLineProcessing for process type: " << process_type.ToString() << std::endl;

        // Add flags to disable GPU and software rasterizer for debugging hangs.
        // These are common flags used to troubleshoot rendering issues.
        command_line->AppendSwitch("disable-gpu");
        command_line->AppendSwitch("disable-software-rasterizer");

        // Optionally, you might want to enable logging to a file for more detailed debugging.
        // command_line->AppendSwitchWithValue("log-file", "cef_debug.log");
        // command_line->AppendSwitchWithValue("log-severity", "verbose");
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
    // Provide CEF with command-line arguments.
    CefMainArgs args(argc, argv);

    // Create an instance of our custom CefApp
    CefRefPtr<SimpleCefApp> app = new SimpleCefApp();

    // CEF applications have multiple sub-processes (render, GPU, etc) that share
    // the same executable. This function checks the command-line and, if this is
    // a sub-process, executes the appropriate logic.
    // The second parameter is the CefApp instance, which allows us to customize
    // subprocess behavior (like command-line processing).
    int result = CefExecuteProcess(args, app, nullptr);
    if (result >= 0) {
        // The sub-process has completed so return here.
        return result;
    }
    // If result == -1, we are in the main browser process.

    // Initialize CEF settings
    CefSettings settings;

    // Generate a unique cache path for each instance to avoid singleton errors
    // and conflicts when running multiple instances concurrently.
    std::ostringstream cache_path_ss;
    cache_path_ss << SDL_GetBasePath() << "cef_cache_"
                  << std::chrono::duration_cast<std::chrono::milliseconds>(
                         std::chrono::system_clock::now().time_since_epoch()).count()
                  << "/";
    CefString(&settings.cache_path) = cache_path_ss.str();

    // Set paths for CEF resources and locales.
    // SDL_GetBasePath() provides the directory where the executable is located.
    std::ostringstream ss_locales;
    ss_locales << SDL_GetBasePath() << "locales/";
    CefString(&settings.locales_dir_path) = ss_locales.str();
    CefString(&settings.resources_dir_path) = SDL_GetBasePath();

    // Enable windowless (off-screen) rendering.
    settings.windowless_rendering_enabled = true;

    // Initialize the CEF browser process.
    // Pass our custom CefApp instance to CefInitialize.
    bool cef_initialized = CefInitialize(args, settings, app.get(), nullptr);
    if (!cef_initialized) {
        std::cerr << "CEF initialization failed!" << std::endl;
        return 1; // Indicate an error
    }

    // Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL: %s", SDL_GetError());
        CefShutdown(); // Ensure CEF is shut down even if SDL fails
        return SDL_APP_FAILURE;
    }

    int width = 800;
    int height = 600;

    // Create an SDL window
    auto window = SDL_CreateWindow(
        "Render CEF with SDL",
        width,
        height,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (window) {
        // Create an SDL renderer
        auto renderer = SDL_CreateRenderer(window, nullptr);
        if (renderer) {
            SDL_Event e;

            // Create our RenderHandler instance
            CefRefPtr<RenderHandler> renderHandler = new RenderHandler(renderer, width, height);

            // Create our BrowserClient instance, passing the RenderHandler
            CefRefPtr<BrowserClient> browserClient = new BrowserClient(renderHandler);

            // Create CEF browser window
            CefWindowInfo window_info;
            CefBrowserSettings browserSettings;

            // Set browser to be windowless (off-screen)
            window_info.SetAsWindowless(0); // 0 means no transparency (site background color)

            // Create the browser synchronously
            CefRefPtr<CefBrowser> browser = CefBrowserHost::CreateBrowserSync(
                window_info,
                browserClient.get(),
                "https://www.google.com", // Initial URL
                browserSettings,
                nullptr,
                nullptr
            );

            bool shutdown = false;
            bool js_executed = false;

            // Main application loop
            while (!browserClient->closeAllowed()) {
                // Process SDL events
                while (!shutdown && SDL_PollEvent(&e) != 0) {
                    switch (e.type) {
                        case SDL_EVENT_QUIT:
                            shutdown = true;
                            // Request CEF browser to close gracefully
                            if (browser) {
                                browser->GetHost()->CloseBrowser(false);
                            }
                            break;

                        case SDL_EVENT_KEY_DOWN: {
                            CefKeyEvent event;
                            event.modifiers = e.key.mod;
                            event.windows_key_code = e.key.key;
                            event.type = KEYEVENT_RAWKEYDOWN;
                            if (browser) browser->GetHost()->SendKeyEvent(event);

                            event.type = KEYEVENT_CHAR;
                            if (browser) browser->GetHost()->SendKeyEvent(event);
                            break;
                        }

                        case SDL_EVENT_KEY_UP: {
                            CefKeyEvent event;
                            event.modifiers = e.key.mod;
                            event.windows_key_code = e.key.key;
                            event.type = KEYEVENT_KEYUP;
                            if (browser) browser->GetHost()->SendKeyEvent(event);
                            break;
                        }

                        case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                            // Update render handler and notify browser of resize
                            renderHandler->resize(e.window.data1, e.window.data2);
                            if (browser) browser->GetHost()->WasResized();
                            break;

                        case SDL_EVENT_WINDOW_FOCUS_GAINED:
                            if (browser) browser->GetHost()->SetFocus(true);
                            break;

                        case SDL_EVENT_WINDOW_FOCUS_LOST:
                            if (browser) browser->GetHost()->SetFocus(false);
                            break;

                        case SDL_EVENT_WINDOW_HIDDEN:
                        case SDL_EVENT_WINDOW_MINIMIZED:
                            if (browser) browser->GetHost()->WasHidden(true);
                            break;

                        case SDL_EVENT_WINDOW_SHOWN:
                        case SDL_EVENT_WINDOW_RESTORED:
                            if (browser) browser->GetHost()->WasHidden(false);
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
                            if (browser) browser->GetHost()->SendMouseMoveEvent(event, false);
                            break;
                        }

                        case SDL_EVENT_MOUSE_BUTTON_UP: {
                            CefMouseEvent event;
                            event.x = e.button.x;
                            event.y = e.button.y;
                            if (browser) browser->GetHost()->SendMouseClickEvent(event, translateMouseButton(e.button), true, 1);
                            break;
                        }

                        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
                            CefMouseEvent event;
                            event.x = e.button.x;
                            event.y = e.button.y;
                            if (browser) browser->GetHost()->SendMouseClickEvent(event, translateMouseButton(e.button), false, 1);
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
                            if (browser) browser->GetHost()->SendMouseWheelEvent(event, delta_x, delta_y);
                            break;
                        }
                    }
                }

                // Execute JavaScript example once the page is loaded
                if (!js_executed && browserClient->isLoaded()) {
                    js_executed = true;
                    if (browser) {
                        CefRefPtr<CefFrame> frame = browser->GetMainFrame();
                        if (frame) {
                            frame->ExecuteJavaScript("alert('ExecuteJavaScript works!');", frame->GetURL(), 0);
                        }
                    }
                }

                // Let CEF process its internal messages and events
                CefDoMessageLoopWork();

                // Clear the SDL renderer
                SDL_RenderClear(renderer);

                // Render the CEF browser's texture to the SDL renderer
                renderHandler->render();

                // Update the screen
                SDL_RenderPresent(renderer);
            }

            // Release CEF references before shutdown to ensure proper cleanup
            browser = nullptr;
            browserClient = nullptr;
            renderHandler = nullptr;

            // Shut down CEF
            CefShutdown();

            // Destroy SDL renderer
            SDL_DestroyRenderer(renderer);
        } else {
            std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        }
    } else {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
    }

    // Destroy SDL window and quit SDL subsystems
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
