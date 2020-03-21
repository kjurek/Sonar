#include "process_manager.h"

#include <vector>
#include <utility>
#include <fstream>
#include <tlhelp32.h>
#include <psapi.h>

ProcessManagerException::ProcessManagerException(std::string const& msg, std::string const& where)
{
    std::stringstream ss;
    ss << where << std::endl;
    ss << "\tmessage: " << msg << std::endl;

    char systemMessage[255];
    FormatMessage(
        FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        GetLastError(),
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        systemMessage,
        255,
        NULL
    );
    ss << "\tsystem message: " << systemMessage << std::endl;
    _message = ss.str();
}

const char* ProcessManagerException::what() noexcept
{
    return _message.c_str();
}

ProcessManager::ProcessManager()
    :	_window_handle(0), _pid(0), _process_handle(0)
{ }

ProcessManager::~ProcessManager()
{
    if (0 != _process_handle)
	{
        CloseHandle(_process_handle);
	}
}

bool ProcessManager::is_configured() const
{
	return
        (0 != _pid) &&
        (0 != _window_handle) &&
        (0 != _process_handle);
}

void ProcessManager::configure_from_window_title(std::string windowTitle)
{
    set_privilages();
    setup_from_window_title(windowTitle);
}

void ProcessManager::configure_from_process_name(std::string processName)
{
    set_privilages();
    setup_from_process_name(processName);
}

struct EnumData {
    DWORD pid;
    HWND window_handle;
};

namespace {
    BOOL CALLBACK enumProcFunc(_In_ HWND window_handle, _In_ LPARAM lParam)
    {
        EnumData& ed = *reinterpret_cast<EnumData*>(lParam);
        DWORD pid = 0;
        GetWindowThreadProcessId(window_handle, &pid);

        if (ed.pid == pid) {
            ed.window_handle = window_handle;
            SetLastError(ERROR_SUCCESS);
            return FALSE;
        }
        return TRUE;
    }
}

void ProcessManager::set_up_window_handle_from_pid(DWORD pid)
{
    EnumData ed = { pid, NULL };
    EnumWindows(enumProcFunc, reinterpret_cast<LPARAM>(&ed));
    DWORD error_code = GetLastError();

    if (error_code != ERROR_SUCCESS)
	{
        THROW_EXCEPTION("EnumWindows failed");
	}
    _window_handle = ed.window_handle;
}

void ProcessManager::setup_from_window_title(std::string window_title)
{
    HWND windowHandle = FindWindow(NULL, window_title.c_str());
	if (NULL == windowHandle)
	{
        THROW_EXCEPTION("FindWindow failed");
	}
    _window_handle = windowHandle;

    setup_process_handle_from_window_handle(_window_handle);
}

void ProcessManager::setup_process_handle_from_window_handle(HWND window_handle)
{
	DWORD dwProcessId = 0;
    DWORD dwThreadId = GetWindowThreadProcessId(window_handle, &dwProcessId);
    if (0 == dwThreadId)
	{
        THROW_EXCEPTION("GetWindowThreadProcessId failed");
	}

	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessId);
	if (NULL == hProcess)
	{
        THROW_EXCEPTION("OpenProcess failed");
	}
    _pid = dwProcessId;
    _process_handle = hProcess;
}

void ProcessManager::setup_from_process_name(std::string process_name)
{
    set_privilages();
	PROCESSENTRY32 entry;
	entry.dwSize = sizeof(PROCESSENTRY32);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (NULL == snapshot)
	{
        THROW_EXCEPTION("CreateToolhelp32Snapshot failed");
	}

	if (TRUE == Process32First(snapshot, &entry))
	{
		while (TRUE == Process32Next(snapshot, &entry))
		{
            if (process_name == std::string(entry.szExeFile))
			{
				HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, entry.th32ProcessID);
				if (NULL == hProcess)
				{
                    THROW_EXCEPTION("OpenProcess failed");
				}
                _process_handle = hProcess;
                _pid = entry.th32ProcessID;
                set_up_window_handle_from_pid(_pid);
                break;
			}			
		}
	}

	CloseHandle(snapshot);
}

void ProcessManager::set_privilages()
{
    HANDLE token;
	LUID luid;
	TOKEN_PRIVILEGES tkp;

    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token);

	LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid);

	tkp.PrivilegeCount = 1;
	tkp.Privileges[0].Luid = luid;
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(token, false, &tkp, sizeof(tkp), NULL, NULL);

    CloseHandle(token);
}

std::vector<std::pair<std::string, HMODULE> > ProcessManager::get_modules() const
{
    if (!is_configured())
	{
        THROW_EXCEPTION("ProcessManager is not configured, configure first!");
	}

    HMODULE modules[1024];
    DWORD cb_needed;
    std::vector<std::pair<std::string, HMODULE> > result;
	
    BOOL operation_result = EnumProcessModules(_process_handle, modules, sizeof(modules), &cb_needed);
    if (FALSE == operation_result)
	{
        THROW_EXCEPTION("EnumProcessModules failed");
	}

    for (unsigned int i = 0; i < (cb_needed / sizeof(HMODULE)); i++)
	{
        TCHAR module_name_tchar[MAX_PATH];

        operation_result = GetModuleFileNameEx(_process_handle, modules[i], module_name_tchar, sizeof(module_name_tchar) / sizeof(TCHAR));
        if (FALSE == operation_result)
		{
            THROW_EXCEPTION("GetModuleFileNameEx failed");
		}
        std::string module_name = module_name_tchar;
        module_name.assign(module_name.begin() + module_name.rfind("\\") + 1, module_name.end());
        result.push_back(std::make_pair(module_name, modules[i]));
	}
	return result;
}

HMODULE ProcessManager::get_module_address(std::string module_name) const
{
    auto modules = get_modules();
	
	for (auto& module : modules)
	{
        if (module.first == module_name)
		{
			return module.second;
		}
	}
    THROW_EXCEPTION("Module " + module_name + " not found");
}

void ProcessManager::validate_address(DWORD address, size_t nbytes) const
{
	MEMORY_BASIC_INFORMATION mbi;
	
    BOOL operation_result = VirtualQueryEx(_process_handle, (void*)address, &mbi, sizeof(MEMORY_BASIC_INFORMATION));
    if (FALSE == operation_result)
	{
        THROW_EXCEPTION("VirtualQueryEx failed");
	}

	if (mbi.State != MEM_COMMIT)
	{
        THROW_EXCEPTION("Invalid state");
	}

	if (mbi.Protect == PAGE_NOACCESS || mbi.Protect == PAGE_EXECUTE)
	{
        THROW_EXCEPTION("Invalid page");
	}

    size_t block_offset = (size_t)((char *)address - (char *)mbi.AllocationBase);
    size_t block_bytes_post_ptr = mbi.RegionSize - block_offset;

    if (block_bytes_post_ptr < nbytes)
	{
        validate_address(address + block_bytes_post_ptr, nbytes - block_bytes_post_ptr);
    }
}

HWND ProcessManager::get_window_handle() const
{
    if (!is_configured())
	{
        THROW_EXCEPTION("ProcessManager is not configured, configure first!");
	}
    return _window_handle;
}

DWORD ProcessManager::get_process_id() const
{
    if (!is_configured())
	{
        THROW_EXCEPTION("ProcessManager is not configured, configure first!");
	}
    return _pid;
}

HANDLE ProcessManager::get_process_handle() const
{
    if (!is_configured())
	{
        THROW_EXCEPTION("ProcessManager is not configured, configure first!");
	}
    return _process_handle;
}
