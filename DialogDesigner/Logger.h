#pragma once
#define LOGGER_H
#include <windows.h>
#include <stdarg.h>

#define LOGGER_THREAD_NAME_MAX 64
#define LOGGER_THREAD_STR_BUF 32

typedef struct
{
    DWORD tid;
    char name[LOGGER_THREAD_STR_BUF];
} ThreadNameEntry;

class LoggerV1 {
private:
    char minlevel[8];
    WORD default_attr;
    bool toConsole;

    ThreadNameEntry m_threadNames[LOGGER_THREAD_NAME_MAX];
    DWORD m_threadNameCount;

    static bool StrEq(const char* a, const char* b) {
        while (*a && *b) { if (*a != *b) return false; ++a; ++b; }
        return *a == *b;
    }
    static void StrCopy(char* dst, DWORD cap, const char* src) {
        if (cap == 0) return;
        DWORD i = 0;
        while (i + 1 < cap && src[i]) { dst[i] = src[i]; ++i; }
        dst[i] = '\0';
    }

    const char* LookupThreadName(DWORD tid) {
        for (DWORD i = 0; i < m_threadNameCount; i++)
            if (m_threadNames[i].tid == tid) return m_threadNames[i].name;
        return nullptr;
    }

    int LevelWeight(const char* lv) const {
        if (StrEq(lv, "TRACE")) return 0;
        if (StrEq(lv, "DEBUG")) return 1;
        if (StrEq(lv, "INFO"))  return 2;
        if (StrEq(lv, "WARN"))  return 3;
        if (StrEq(lv, "ERROR")) return 4;
        return 6;
    }
    WORD LevelColor(const char* lv) const {
        if (StrEq(lv, "ERROR")) return FOREGROUND_RED | FOREGROUND_INTENSITY;
        if (StrEq(lv, "INFO"))  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        if (StrEq(lv, "WARN"))  return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        if (StrEq(lv, "DEBUG")) return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        if (StrEq(lv, "TRACE")) return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
        return default_attr;
    }

    static char* Write2(char* p, WORD v) {
        WORD val = v % 100;
        *p++ = char('0' + (val / 10) % 10);
        *p++ = char('0' + val % 10);
        return p;
    }
    static char* WriteU(char* p, DWORD v) {
        char tmp[12]; int n = 0;
        if (v == 0) tmp[n++] = '0';
        else { while (v) { tmp[n++] = char('0' + v % 10); v /= 10; } }
        while (n) *p++ = tmp[--n];
        return p;
    }

    /* ---------- ANSI 输出 ---------- */
    void Output(const char* level, const char* body) {
        SYSTEMTIME st; GetLocalTime(&st);
        char line[2200];
        char* p = line;
        *p++ = '[';
        p = Write2(p, st.wHour);   *p++ = ':';
        p = Write2(p, st.wMinute); *p++ = ':';
        p = Write2(p, st.wSecond);
        *p++ = ']'; *p++ = ' ';

        DWORD currentTid = GetCurrentThreadId();
        const char* thrName = LookupThreadName(currentTid);

        *p++ = '[';
        if (thrName != nullptr && !StrEq(thrName, "")) {
            const char* ns = thrName;
            while (*ns) *p++ = *ns++;
        }
        else {
            *p++ = 'T'; *p++ = 'I'; *p++ = 'D'; *p++ = '-';
            p = WriteU(p, currentTid);
        }
        *p++ = '/';
        { const char* t = level; while (*t) *p++ = *t++; }
        *p++ = ']'; *p++ = ' ';
        {
            const char* b = body;
            while (*b && ((DWORD)(p - line)) < (sizeof(line) - 2)) *p++ = *b++;
        }
        *p++ = '\n'; *p = '\0';
        DWORD lineLen = (DWORD)(p - line);

        if (toConsole) {
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            if (h != INVALID_HANDLE_VALUE && h != NULL) {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                WORD savedAttr = default_attr;
                bool haveSaved = GetConsoleScreenBufferInfo(h, &csbi) != FALSE;
                if (haveSaved) savedAttr = csbi.wAttributes;
                SetConsoleTextAttribute(h, LevelColor(level));
                DWORD written = 0;
                WriteConsoleA(h, line, lineLen, &written, NULL);
                if (haveSaved) SetConsoleTextAttribute(h, savedAttr);
            }
        }
        OutputDebugStringA(line);
    }

    void LogV(const char* level, const char* fmt, va_list args) {
        if (LevelWeight(level) < LevelWeight(minlevel)) return;
        char body[2048]{};
        body[0] = '\0';
        wvsprintfA(body, fmt, args);
        Output(level, body);
    }

    /* ---------- 宽字符路径 ---------- */
    static bool WStrEq(const WCHAR* a, const WCHAR* b) {
        while (*a && *b) { if (*a != *b) return false; ++a; ++b; }
        return *a == *b;
    }
    static WCHAR* Write2W(WCHAR* p, WORD v) {
        WORD val = v % 100;
        *p++ = (WCHAR)(L'0' + (val / 10) % 10);
        *p++ = (WCHAR)(L'0' + val % 10);
        return p;
    }
    static WCHAR* WriteUW(WCHAR* p, DWORD v) {
        WCHAR tmp[12]; int n = 0;
        if (v == 0) tmp[n++] = L'0';
        else { while (v) { tmp[n++] = (WCHAR)(L'0' + v % 10); v /= 10; } }
        while (n) *p++ = tmp[--n];
        return p;
    }

    void OutputW(const WCHAR* level, const WCHAR* body) {
        SYSTEMTIME st; GetLocalTime(&st);
        WCHAR line[2200];
        WCHAR* p = line;
        *p++ = L'[';
        p = Write2W(p, st.wHour);   *p++ = L':';
        p = Write2W(p, st.wMinute); *p++ = L':';
        p = Write2W(p, st.wSecond);
        *p++ = L']'; *p++ = L' ';

        DWORD currentTid = GetCurrentThreadId();
        const char* thrName = LookupThreadName(currentTid);

        *p++ = L'[';
        if (thrName != nullptr && !StrEq(thrName, "")) {
            const char* ns = thrName;
            while (*ns) *p++ = (WCHAR)(unsigned char)*ns++;
        }
        else {
            *p++ = L'T'; *p++ = L'I'; *p++ = L'D'; *p++ = L'-';
            p = WriteUW(p, currentTid);
        }
        *p++ = L'/';
        { const WCHAR* t = level; while (*t) *p++ = *t++; }
        *p++ = L']'; *p++ = L' ';
        {
            const WCHAR* b = body;
            while (*b && ((DWORD)(p - line)) < (sizeof(line) / sizeof(WCHAR) - 2)) *p++ = *b++;
        }
        *p++ = L'\n'; *p = L'\0';
        DWORD lineLen = (DWORD)(p - line);

        if (toConsole) {
            HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
            if (h != INVALID_HANDLE_VALUE && h != NULL) {
                CONSOLE_SCREEN_BUFFER_INFO csbi;
                WORD savedAttr = default_attr;
                bool haveSaved = GetConsoleScreenBufferInfo(h, &csbi) != FALSE;
                if (haveSaved) savedAttr = csbi.wAttributes;

                char lvA[8] = { 0 };
                for (int i = 0; i < 7 && level[i]; ++i) lvA[i] = (char)level[i];
                SetConsoleTextAttribute(h, LevelColor(lvA));

                DWORD written = 0;
                WriteConsoleW(h, line, lineLen, &written, NULL);

                if (haveSaved) SetConsoleTextAttribute(h, savedAttr);
            }
        }

        // 调试器：宽转 ANSI 再输出，兼容性最好
        {
            char ansi[2200 * 4];
            int r = WideCharToMultiByte(CP_ACP, 0, line, (int)lineLen, ansi, (int)sizeof(ansi) - 2, NULL, NULL);
            if (r < 0) r = 0;
            ansi[r] = '\0';
            OutputDebugStringA(ansi);
        }
    }

    void LogVW(const WCHAR* level, const WCHAR* fmt, va_list args) {
        char lvA[8] = { 0 };
        for (int i = 0; i < 7 && level[i]; ++i) lvA[i] = (char)level[i];
        if (LevelWeight(lvA) < LevelWeight(minlevel)) return;

        WCHAR body[2048];
        body[0] = L'\0';
        wvsprintfW(body, fmt, args);   // user32，无 CRT
        OutputW(level, body);
    }

    void SetConsole(bool on) { toConsole = on; }

public:
    LoggerV1()
        : default_attr(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE),
        toConsole(true),
        m_threadNameCount(0U)
    {
        StrCopy(minlevel, sizeof(minlevel), "TRACE");
        ZeroMemory(m_threadNames, sizeof(m_threadNames));
    }

    void SetLevel(const char* lv) {
        if (!lv) return;
        StrCopy(minlevel, sizeof(minlevel), lv);
    }

    void SetName(const char* name) {
        if (!name) return;
        DWORD tid = GetCurrentThreadId();
        for (DWORD i = 0; i < m_threadNameCount; i++) {
            if (m_threadNames[i].tid == tid) {
                StrCopy(m_threadNames[i].name, LOGGER_THREAD_STR_BUF, name);
                return;
            }
        }
        if (m_threadNameCount >= LOGGER_THREAD_NAME_MAX) return;
        m_threadNames[m_threadNameCount].tid = tid;
        StrCopy(m_threadNames[m_threadNameCount].name, LOGGER_THREAD_STR_BUF, name);
        m_threadNameCount++;
    }

    /* ---------- ANSI 接口 ---------- */
    void Log(const char* level, const char* fmt, ...) {
        va_list ap; va_start(ap, fmt); LogV(level, fmt, ap); va_end(ap);
    }
    void trace(const char* fmt, ...) { va_list ap; va_start(ap, fmt); LogV("TRACE", fmt, ap); va_end(ap); }
    void debug(const char* fmt, ...) { va_list ap; va_start(ap, fmt); LogV("DEBUG", fmt, ap); va_end(ap); }
    void info(const char* fmt, ...) { va_list ap; va_start(ap, fmt); LogV("INFO", fmt, ap); va_end(ap); }
    void warn(const char* fmt, ...) { va_list ap; va_start(ap, fmt); LogV("WARN", fmt, ap); va_end(ap); }
    void error(const char* fmt, ...) { va_list ap; va_start(ap, fmt); LogV("ERROR", fmt, ap); va_end(ap); }

    /* ---------- 宽字符接口（新增） ---------- */
    void Log(const WCHAR* level, const WCHAR* fmt, ...) {
        va_list ap; va_start(ap, fmt); LogVW(level, fmt, ap); va_end(ap);
    }
    void trace(const WCHAR* fmt, ...) { va_list ap; va_start(ap, fmt); LogVW(L"TRACE", fmt, ap); va_end(ap); }
    void debug(const WCHAR* fmt, ...) { va_list ap; va_start(ap, fmt); LogVW(L"DEBUG", fmt, ap); va_end(ap); }
    void info(const WCHAR* fmt, ...) { va_list ap; va_start(ap, fmt); LogVW(L"INFO", fmt, ap); va_end(ap); }
    void warn(const WCHAR* fmt, ...) { va_list ap; va_start(ap, fmt); LogVW(L"WARN", fmt, ap); va_end(ap); }
    void error(const WCHAR* fmt, ...) { va_list ap; va_start(ap, fmt); LogVW(L"ERROR", fmt, ap); va_end(ap); }
};
LoggerV1 LOGGER;