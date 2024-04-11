#if defined(_WIN32)
#    ifndef _CRT_SECURE_NO_WARNINGS
#        define _CRT_SECURE_NO_WARNINGS
#    endif

#    include "bee_utf8_crt.h"

#    include <Windows.h>
#    include <bee/platform/win/cwtf8.h>
#    include <bee/platform/win/wtf8.h>
#    include <io.h>

#    include <array>
#    include <atomic>
#    include <cassert>
#    include <string>

#    define U2W(s) bee::wtf8::u2w(s).c_str()

namespace {
    struct spinlock {
        struct guard {
            explicit guard(spinlock& lock) noexcept
                : plock(std::addressof(lock)) {
                plock->lock();
            }
            ~guard() noexcept {
                plock->unlock();
            }
            spinlock* plock;
        };
        void lock() noexcept {
            for (;;) {
                if (!l.exchange(true, std::memory_order_acquire)) {
                    return;
                }
                while (l.load(std::memory_order_relaxed)) {
                    YieldProcessor();
                }
            }
        }
        void unlock() noexcept {
            l.store(false, std::memory_order_release);
        }
        std::atomic<bool> l = { false };
    };
}

FILE* __cdecl utf8_fopen(const char* filename, const char* mode) {
    return _wfopen(U2W(filename), U2W(mode));
}

FILE* __cdecl utf8_freopen(const char* filename, const char* mode, FILE* stream) {
    return _wfreopen(U2W(filename), U2W(mode), stream);
}

FILE* __cdecl utf8_popen(const char* command, const char* type) {
    return _wpopen(U2W(command), U2W(type));
}

int __cdecl utf8_system(const char* command) {
    return _wsystem(U2W(command));
}

int __cdecl utf8_remove(const char* filename) {
    return _wremove(U2W(filename));
}

int __cdecl utf8_rename(const char* oldfilename, const char* newfilename) {
    return _wrename(U2W(oldfilename), U2W(newfilename));
}

namespace {
    template <size_t N>
    struct ringbuff {
        std::array<void*, N> data {};
        size_t n = 0;
        void push(void* v) {
            n = (n + 1) % N;
            free(data[n]);
            data[n] = v;
        }
    };
}
static spinlock getenv_lock;
static ringbuff<64> getenv_rets;

char* __cdecl utf8_getenv(const char* varname) {
    wchar_t* wstr;
    size_t wlen;
    errno_t err = _wdupenv_s(&wstr, &wlen, U2W(varname));
    if (err) {
        return NULL;
    }
    if (!wstr || !wlen) {
        return NULL;
    }
    size_t len = wtf8_from_utf16_length(wstr, wlen);
    char* str  = (char*)malloc(len + 1);
    if (!str) {
        free(wstr);
        return NULL;
    }
    wtf8_from_utf16(wstr, wlen, str, len);
    str[len] = '\0';
    free(wstr);

    spinlock::guard _(getenv_lock);
    getenv_rets.push(str);
    return str;
}

char* __cdecl utf8_tmpnam(char* buffer) {
    wchar_t tmp[L_tmpnam];
    if (!_wtmpnam(tmp)) {
        return NULL;
    }
    size_t wlen = wcslen(tmp);
    size_t len  = wtf8_from_utf16_length(tmp, wlen);
    if (len >= L_tmpnam) {
        return NULL;
    }
    wtf8_from_utf16(tmp, wlen, buffer, len);
    buffer[len] = '\0';
    return buffer;
}

void* __stdcall utf8_LoadLibraryExA(const char* filename, void* file, unsigned long flags) {
    return LoadLibraryExW(U2W(filename), file, flags);
}

unsigned long __stdcall utf8_GetModuleFileNameA(void* module, char* filename, unsigned long size) {
    wchar_t tmp[MAX_PATH + 1];
    DWORD tmplen = GetModuleFileNameW((HMODULE)module, tmp, MAX_PATH + 1);
    if (tmplen == 0) {
        return 0;
    }
    size_t len = wtf8_from_utf16_length(tmp, tmplen);
    if (len > size) {
        // TODO: impl ERROR_INSUFFICIENT_BUFFER
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    wtf8_from_utf16(tmp, tmplen, filename, len);
    filename[len] = '\0';
    return (unsigned long)len;
}

unsigned long __stdcall utf8_FormatMessageA(
    unsigned long dwFlags,
    const void* lpSource,
    unsigned long dwMessageId,
    unsigned long dwLanguageId,
    char* lpBuffer,
    unsigned long nSize,
    va_list* Arguments
) {
    wchar_t tmp[1024];
    DWORD tmplen = FormatMessageW(dwFlags, lpSource, dwMessageId, dwLanguageId, tmp, 1024, Arguments);
    if (tmplen == 0) {
        return 0;
    }
    size_t len = wtf8_from_utf16_length(tmp, tmplen);
    if (len > nSize) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }
    wtf8_from_utf16(tmp, tmplen, lpBuffer, len);
    lpBuffer[len] = '\0';
    return (unsigned long)len;
}

static void ConsoleWrite(FILE* stream, bee::zstring_view str) {
    HANDLE handle = (HANDLE)_get_osfhandle(_fileno(stream));
    if (FILE_TYPE_CHAR == GetFileType(handle)) {
        auto r = bee::wtf8::u2w(str);
        if (!r.empty()) {
            if (WriteConsoleW(handle, r.data(), (DWORD)r.size(), NULL, NULL)) {
                return;
            }
        }
    }
    fwrite(str.data(), sizeof(char), str.size(), stream);
    fflush(stream);
}

void utf8_ConsoleWrite(const char* str, size_t len) {
    ConsoleWrite(stdout, { str, len });
}

void utf8_ConsoleNewLine() {
    HANDLE handle = (HANDLE)_get_osfhandle(_fileno(stdout));
    if (FILE_TYPE_CHAR == GetFileType(handle)) {
        if (WriteConsoleW(handle, L"\n", 1, NULL, NULL)) {
            return;
        }
    }
    fwrite("\n", sizeof(char), 1, stdout);
    fflush(stdout);
}

void utf8_ConsoleError(const char* fmt, const char* param) {
    size_t len = (size_t)snprintf(NULL, 0, fmt, param);
    std::string str(len, '\0');
    snprintf(str.data(), str.size(), fmt, param);
    ConsoleWrite(stderr, str);
}

char** utf8_create_args(int argc, wchar_t** wargv) {
    char** argv = new (std::nothrow) char*[argc + 1];
    if (!argv) {
        return NULL;
    }
    for (int i = 0; i < argc; ++i) {
        size_t wlen = wcslen(wargv[i]);
        size_t len  = wtf8_from_utf16_length(wargv[i], wlen);
        argv[i]     = new (std::nothrow) char[len + 1];
        if (!argv[i]) {
            for (int j = 0; j < i; ++j) {
                delete[] argv[j];
            }
            delete[] argv;
            return NULL;
        }
        wtf8_from_utf16(wargv[i], wlen, argv[i], len);
        argv[i][len] = '\0';
    }
    argv[argc] = NULL;
    return argv;
}

void utf8_free_args(int argc, char** argv) {
    for (int i = 0; i < argc; ++i) {
        delete[] argv[i];
    }
    delete[] argv;
}

#endif
