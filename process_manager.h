#pragma once

#include <Windows.h>
#include <cassert>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#define THROW_EXCEPTION(msg) \
    throw ProcessManagerException(msg, \
                                  std::string(__FILE__) + std::string(":") \
                                      + std::to_string(__LINE__))

class ProcessManagerException : public std::exception
{
public:
    ProcessManagerException(std::string const &msg, std::string const &where);
    virtual const char *what() noexcept;

private:
    std::string _message;
};

class ProcessManager
{
public:
    ProcessManager();
    ~ProcessManager();

    bool is_configured() const;
    void configure_from_window_title(std::string windowTitle);
    void configure_from_process_name(std::string processName);

    std::vector<std::pair<std::string, HMODULE>> get_modules() const;
    HMODULE get_module_address(std::string module_name) const;
    HWND get_window_handle() const;
    DWORD get_process_id() const;
    HANDLE get_process_handle() const;

    void validate_address(DWORD address, size_t nbytes) const;

    template<typename T>
    void write_memory(DWORD address, T value) const
    {
        validate_address(address, sizeof(T));
        BOOL operation_result = WriteProcessMemory(_process_handle,
                                                   reinterpret_cast<LPVOID>(address),
                                                   &value,
                                                   sizeof(T),
                                                   NULL);
        if (FALSE == operation_result) {
            THROW_EXCEPTION("WriteProcessMemory failed");
        }
    }

    template<typename T>
    void write_memory(DWORD address, T *values, size_t count) const
    {
        validate_address(address, count * sizeof(T));
        BOOL operation_result = WriteProcessMemory(_process_handle,
                                                   address,
                                                   values,
                                                   count * sizeof(T),
                                                   NULL);
        if (FALSE == operation_result) {
            THROW_EXCEPTION("WriteProcessMemory failed");
        }
    }

    template<typename T>
    T read_memory(DWORD address) const
    {
        validate_address(address, sizeof(T));
        T result;
        BOOL operation_result = ReadProcessMemory(_process_handle,
                                                  reinterpret_cast<LPCVOID>(address),
                                                  &result,
                                                  sizeof(T),
                                                  NULL);
        if (FALSE == operation_result) {
            THROW_EXCEPTION("ReadProcessMemory failed");
        }
        return result;
    }

    template<typename T>
    std::vector<T> read_memory(DWORD address, size_t count) const
    {
        validate_address(address, count * sizeof(T));
        std::vector<T> result(count);
        BOOL operation_result = ReadProcessMemory(_process_handle,
                                                  reinterpret_cast<LPCVOID>(address),
                                                  &result[0],
                                                  count * sizeof(T),
                                                  NULL);
        if (FALSE == operation_result) {
            THROW_EXCEPTION("ReadProcessMemory failed");
        }
        return result;
    }

    std::string read_string(DWORD address, size_t maxLen) const
    {
        validate_address(address, maxLen);
        std::vector<char> result;
        char c;
        size_t currOffset = 0;

        do {
            c = read_memory<char>(address + currOffset++);
            result.push_back(c);
            if (result.size() > maxLen) {
                return "";
            }
        } while (c != '\0');

        return std::string(result.begin(), result.end());
    }

    std::wstring read_wstring(DWORD address, size_t maxLen) const
    {
        validate_address(address, maxLen);
        std::vector<wchar_t> result;
        wchar_t c;
        size_t current_offset = 0;

        do {
            c = read_memory<wchar_t>(address + current_offset);
            current_offset += sizeof(wchar_t);
            result.push_back(c);
            if (result.size() > maxLen) {
                return L"";
            }

        } while (c != L'\0');

        return std::wstring(result.begin(), result.end());
    }

private:
    HWND _window_handle;
    DWORD _pid;
    HANDLE _process_handle;

    void setup_from_window_title(std::string window_title);
    void set_up_window_handle_from_pid(DWORD pid);
    void setup_process_handle_from_window_handle(HWND window_handle);
    void setup_from_process_name(std::string process_name);
    void set_privilages();
};
