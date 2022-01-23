#include <Windows.h>
#include <string>
#include <regex>
#include <unordered_map>
#include <memory>
#include <thread>
#include <natupnp.h>
#include <objbase.h>
#include <oleauto.h>
#include <winsock.h>
#include <algorithm>
#include <random>
#include <deque>
#include <ctime>
#pragma comment(lib, "wsock32.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"oleaut32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib,"winmm")
#pragma comment(lib, "winhttp.lib")
#include <cpprest/http_client.h>
namespace h {

    template <class BT, class CT>
    class ObjectManager {
    protected:
        BT base;
        CT created;
        virtual void cr(BT base) = 0;
        virtual void dr() = 0;
    public:
        ObjectManager() {}
        ObjectManager(BT base) :base(base) {}
        virtual ~ObjectManager() {}
        virtual BT& getBase() = 0;
        virtual CT& getCreated() = 0;
        virtual void reset(BT base) = 0;

    };
    class colorManager :private ObjectManager<COLORREF, HBRUSH> {
    private:
        void cr(COLORREF base) override {
            this->base = base;
            created = CreateSolidBrush(base);
        }
        void dr() {
            if (created == NULL)return;
            DeleteObject(created);
        }
    public:
        colorManager() {}
        colorManager(COLORREF base) :ObjectManager(base) {
            cr(base);
        }
        COLORREF& getBase()override {
            return base;
        }
        HBRUSH& getCreated()override {
            return created;
        }
        ~colorManager() override {
            dr();
        }
        void reset(COLORREF base)override {
            dr();
            cr(base);
        }
    };
    class fontManager :private ObjectManager<std::wstring, HFONT> {
    private:
        int height;
        void cr(std::wstring base) override {
            this->base = base;
            created = CreateFont(height, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, base.c_str());
        }
        void dr() {
            if (created == NULL)return;
            DeleteObject(created);
        }
    public:
        fontManager() {}
        fontManager(std::wstring base) :ObjectManager(base) {
            cr(base);
        }
        std::wstring& getBase()override {
            return base;
        }
        HFONT& getCreated()override {
            return created;
        }
        ~fontManager() override {
            dr();
        }
        void reset(std::wstring base)override {
            dr();
            cr(base);
        }
        auto& setHeight(decltype(height) height) {
            this->height = height;
            reset(base);
            return *this;
        }
    };
    namespace global {
        fontManager font(L"游明朝");
        colorManager bk(RGB(0, 0, 0)),
            base(RGB(0, 255, 0)),
            red(RGB(255,0,0));
    };
    namespace constGlobal {
        enum BTN {
            BTN_STR_SET = 100,
            BTN_MSG_SET
        };
    };
    inline auto split(std::wstring str, const std::wstring cut) noexcept(false) {
        std::vector<std::wstring> data;
        for (auto pos = str.find(cut); pos != std::string::npos; pos = str.find(cut)) {
            data.push_back(str.substr(0, pos));
            str = str.substr(pos + cut.size());
        }
        if (!str.empty())data.push_back(str);
        return data;
    }
    inline auto split(std::string str, const std::string cut) noexcept(false) {
        std::vector<std::string> data;
        for (auto pos = str.find(cut); pos != std::string::npos; pos = str.find(cut)) {
            data.push_back(str.substr(0, pos));
            str = str.substr(pos + cut.size());
        }
        if (!str.empty())data.push_back(str);
        return data;
    }
    inline auto getListStr(const HWND list, const int id)noexcept(false) {
        const int BUFSIZE = SendMessage(list, LB_GETTEXTLEN, id, 0) + 1;
        std::unique_ptr<TCHAR> text(new TCHAR[BUFSIZE]);
        SendMessage(list, LB_GETTEXT, id, (LPARAM)text.get());
        return std::wstring(text.get(), text.get() + BUFSIZE - 1);
    }
    inline auto getWindowStr(const HWND hwnd)noexcept(false) {
        const int BUFSIZE = GetWindowTextLength(hwnd) + 1;
        std::unique_ptr<TCHAR> text(new TCHAR[BUFSIZE]);
        GetWindowText(hwnd, text.get(), BUFSIZE);
        return std::string(text.get(), text.get() + BUFSIZE - 1);
    }
    inline auto getWindow(const HWND hwnd)noexcept(false) {
        const int BUFSIZE = GetWindowTextLength(hwnd) + 1;
        std::unique_ptr<TCHAR> text(new TCHAR[BUFSIZE]);
        GetWindowText(hwnd, text.get(), BUFSIZE);
        return std::wstring(text.get(), text.get() + BUFSIZE - 1);
    }
    inline std::wstring stringToWstring(const std::string str)noexcept(true) {
        const int BUFSIZE = MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, (wchar_t*)nullptr, 0);
        std::unique_ptr<wchar_t> wtext(new wchar_t[BUFSIZE]);
        MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, wtext.get(), BUFSIZE);
        return std::wstring(wtext.get(), wtext.get() + BUFSIZE - 1);
    }
    inline std::string wstringToString(const std::wstring str)noexcept(true) {
        const int BUFSIZE = WideCharToMultiByte(CP_OEMCP, 0, str.c_str(), -1, (char*)nullptr, 0, nullptr, nullptr);
        std::unique_ptr<char> wtext(new char[BUFSIZE]);
        WideCharToMultiByte(CP_OEMCP, 0, str.c_str(), -1, wtext.get(), BUFSIZE, nullptr, nullptr);
        return std::string(wtext.get(), wtext.get() + BUFSIZE - 1);
    }
    class File {
    private:
        std::string name,
            content;
    public:
        inline File(const std::string name)noexcept(true) :name(name) {
            read();
        }
        inline auto& setName(const std::string name)noexcept(true) {
            this->name = name;
            return *this;
        }
        inline auto& getContent()const noexcept(true) {
            return content;
        }
        inline File& read() noexcept(false) {
            std::fstream file(name);
            if (file.fail())return *this;
            content = std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
            return *this;
        }
        inline auto write(const std::string str, const bool reset = false) const noexcept(false) {
            std::ofstream file(name, reset? std::ios_base::trunc: std::ios_base::app);
            file << str;
            file.close();
            return *this;
        }
    };
    class ResizeManager {
    private:
        HWND hwnd;
        double ratioX, ratioY;
        std::vector<std::pair<HWND, RECT> > children;
    public:
        ResizeManager(HWND hwnd, std::vector<HWND> children) :hwnd(hwnd) {
            RECT rect;
            GetClientRect(hwnd, &rect);
            ratioX = rect.right;
            ratioY = rect.bottom;
            WINDOWPLACEMENT pos;
            pos.length = sizeof(WINDOWPLACEMENT);
            for (auto& hw : children) {
                GetWindowPlacement(hw, &pos);
                this->children.push_back({ hw,pos.rcNormalPosition });
            }
        }
        ResizeManager() {

        }
        void resize() {
            RECT rect;
            GetClientRect(hwnd, &rect);
            double ratioX = rect.right / this->ratioX;
            double ratioY = rect.bottom / this->ratioY;
            for (auto& [hw, re] : children) {
                MoveWindow(hw, re.left * ratioX, re.top * ratioY, ratioX * (re.right - re.left), ratioY * (re.bottom - re.top), TRUE);
            }
        }
    };
    inline auto baseStyle(WNDPROC wndproc, LPCWSTR name) {
        WNDCLASS winc;
        winc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
        winc.cbClsExtra = winc.cbWndExtra = 0;
        winc.hInstance = (HINSTANCE)GetModuleHandle(0);
        winc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
        winc.hCursor = LoadCursor(NULL, IDC_ARROW);
        winc.lpszMenuName = NULL;
        winc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
        winc.lpfnWndProc = wndproc;
        winc.lpszClassName = name;
        return RegisterClass(&winc);
    }
    template <class T>
    concept haveBeginEnd = requires(T & data) {
        data.begin();
        data.end();
    };
    template <haveBeginEnd T>
    void shuffle(T* data) {
        static auto engine = std::mt19937(std::random_device()());
        std::shuffle(data->begin(), data->end(), engine);
    }
    class Timer {
    private:
        clock_t startTime,
            finishTime;
    public:
        auto& start() {
            startTime = clock();
            return *this;
        }
        auto& finish() {
            finishTime = clock();
            return *this;
        }
        auto now() {
            return clock() - startTime;
        }
        auto get() {
            return finishTime - startTime;
        }
    }
    ;
    class Typing {
    private:
        std::deque<std::pair<std::string, std::string> > data,
            question;
        int checkPos;
        bool miss;
        static constexpr int BEGIN = 0;
        std::function<void()> falseDo,reset;
    public:
        Typing(decltype(falseDo) falseDo, decltype(reset) reset) :falseDo(falseDo), reset(reset) {}
        auto& set(decltype(data) data) {
            this->question=this->data = data;
            h::shuffle(&question);
            return *this;
        }
        ~Typing() {
            data.clear();
            data.shrink_to_fit();
            question.clear();
            question.shrink_to_fit();
        }
        auto& restart() {
            checkPos = BEGIN;
            h::shuffle(&(question=data));
            reset();
            return *this;
        }
        auto& update(char answer) {
            if (answer != question.front().second[checkPos]) {
                miss = true;
                falseDo();
                return *this;
            }
            else if (++checkPos < question.front().second.size()) {
                
            //if (answer != question.front().second[checkPos] || ++checkPos < question.front().second.size()) {
                
                return *this;
            }
            checkPos = BEGIN;
            if (miss) {
                miss = false;
                return *this;
            }
            question.pop_front();
            if (!question.size()) {
                restart();
                return *this;
            }
            h::shuffle(&question);
            return *this;
        }
        auto& back() {
            if (checkPos > 0)
                --checkPos;
            //falseDO
            return *this;
        }
        auto& getPos() {
            return checkPos;
        }
        auto& getQuestion() {
            return question.front();
        }
    };

};
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    static h::Timer timer;
    static double count;
    static int back;
    static h::ResizeManager rm;
    static  web::http::client::http_client client(L"https://script.google.com/macros/s/AKfycbx4R3ki7xY6bTxgtCt-k5Gb1LXz5GNW9dKdWclabS-fz7zwazA/exec");
    enum MSG {
        showList
    };
    static h::Typing typing(
        [&] {
            --count;
            ++back;
            PlaySound(TEXT("WAV_FALSE"), nullptr, SND_ASYNC | SND_RESOURCE);
        },
        [&] {
            auto score = count / timer.finish().get();
            MessageBox(nullptr, std::to_wstring(score).c_str(), TEXT(""), 0);
            if (score > std::stod("0" + h::File("score.txt").read().getContent())) {
                MessageBox(nullptr, TEXT("Best Score!"), TEXT("wow"), MB_OK);
                h::File("score.txt").write(std::to_string(score), true);
                auto user = h::split(h::File("user.txt").read().getContent(), "\n");
                    client.request(web::http::methods::POST, L"", web::uri::encode_uri((h::stringToWstring("name=" + user[0] + "&&password=" + user[1]) + L"&&score=" + std::to_wstring(score)).c_str()), L"application/x-www-form-urlencoded");
            }
            timer.start();
            back=count = 0;
            
        });
    PAINTSTRUCT ps;
    HDC hdc;
    RECT rect;
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_CREATE:
    {
        int i = 1;
        std::deque <std::pair< std::string, std::string> > data;
        auto content = h::split(h::File("list.txt").read().getContent(), "\n");
        if (content.empty() || content.size() % 2 == 1) {
            PostQuitMessage(0);
            break;
        }
        for (auto raw = h::split(h::File("list.txt").read().getContent(), "\n"); i < raw.size(); i += 2) {
            data.emplace_back(raw[i - 1], raw[i]);
        }
        typing.set(data);
        h::global::font.setHeight(50);
        GetClientRect(hwnd,&rect);
        rm = std::move(h::ResizeManager(hwnd,{
            CreateWindow(TEXT("BUTTON"),TEXT("Show ranking"),WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|WS_CHILD,0,rect.bottom/10*9,rect.right,rect.bottom/10,hwnd,(HMENU)MSG::showList,LPCREATESTRUCT(lp)->hInstance,nullptr)
            }));
        timer.start();
    }
    break;
    case WM_SIZE:
        rm.resize();//サブクラスか
        break;
    case WM_CHAR:
        if (wp == VK_ESCAPE) {
            typing.restart();
        }
        else if (!back&&wp == VK_BACK) {//==0
            typing.back();
        }
        else if (!(back and 0 <= (back += (wp == VK_BACK) * -2 + 1))) {
            ++count;
            typing.update(wp);
        }
        InvalidateRect(hwnd, nullptr, TRUE);
        UpdateWindow(hwnd);
        break;
    case WM_PAINT:
        GetClientRect(hwnd, &rect);
        hdc = BeginPaint(hwnd, &ps);
        SetTextColor(hdc, back?h::global::red.getBase() : h::global::base.getBase());
        SetBkColor(hdc, h::global::bk.getBase());
        SelectObject(hdc, h::global::font.getCreated());
        DrawText(hdc, 
            h::stringToWstring(
            typing.getQuestion().first+"\n"+
            typing.getQuestion().second.substr(0, typing.getPos()) + (back ? std::to_string(back) : "")+"\n"+
            std::to_string(count / timer.now())
            ).c_str(), -1, &rect, DT_WORDBREAK | DT_CENTER);
        EndPaint(hwnd, &ps);
        break;
    case WM_COMMAND:
        switch (LOWORD(wp)) {
        case MSG::showList:
            auto list=CreateWindow(TEXT("LISTBOX"), TEXT(""), WS_OVERLAPPEDWINDOW|LBS_NOTIFY | LBS_DISABLENOSCROLL | WS_HSCROLL | WS_VSCROLL | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, (HINSTANCE)GetModuleHandle(0), nullptr);
            auto res=client.request(web::http::methods::GET).get();
            if (res.status_code() != web::http::status_codes::OK)break;
            for (auto& item : h::split(std::regex_replace(res.extract_string().get(), std::wregex(LR"(\[END_NAME\])"), L":"), L"[END_SCORE]")) {
                SendMessage(list, LB_ADDSTRING, SendMessage(list, LB_GETCOUNT, 0, 0), (LPARAM)item.c_str());//i++
            }
            break;
        }
        break;
    }
    return DefWindowProc(hwnd, msg, wp, lp);
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow) {
    MSG msg;
    h::baseStyle(WndProc, TEXT("main"));
    CreateWindow(TEXT("main"), TEXT("Typing"), WS_VISIBLE | WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, nullptr);
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    } 
    return 0;
}
