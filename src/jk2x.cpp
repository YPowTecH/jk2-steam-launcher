/*
jk2x - Steam launcher for Star Wars Jedi Knight II: Jedi Outcast that starts
a multiplayer client (EternalJK2MV, JK2MV, or any other client exe) instead
of the stock multiplayer exe.

Steam launch option:
    "<path>\jk2x.exe" %command%
    "<path>\jk2x.exe" -client jk2mv %command%

Steam replaces %command% with the stock exe it would have run:
    GameData\jk2mp.exe  ("Launch Multiplayer")   -> start the client
    GameData\jk2sp.exe  ("Launch Single Player") -> start the stock jk2sp.exe
Started without Steam (no stock exe on the command line) it starts the client.
Every other argument is passed on to the game.

jk2x waits until the game and everything it started have exited, so Steam
keeps tracking the session (playtime, overlay, friends list). If jk2x is
closed (Steam's "Stop" button) the game is closed with it.

Which client:
    -client <name>    a client from CLIENTS below by one of its short names
    -client "<path>"  any client exe
    (none)            the first client in CLIENTS that is found

A client is found, in order: through its installer's registry entry, as a
portable exe in the GameData folder, in its default Program Files folder.
It is started from its own folder, like its Start menu shortcut does.

Installed EternalJK2MV/JK2MV find the game's assets0-5.pk3 in the Steam
folder by themselves (through the registry entry Steam writes for JK2).
Portable builds can't (fs_assetspath is compiled out of them), so they either
live in GameData or have their own copies of the assets.
*/

#include <windows.h>
// windows.h must come first
#include <shellapi.h>

#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace {

constexpr const wchar_t *TITLE = L"jk2x";

// Windows paths can be longer than MAX_PATH; buffers grow up to this
constexpr DWORD MAX_LONG_PATH = 32768;

constexpr std::size_t MAX_CLIENT_NAMES = 4;

struct Client {
	// short names for -client, case-insensitive; unused slots are nullptr
	std::array<const wchar_t *, MAX_CLIENT_NAMES> names;
	const wchar_t *displayName;
	const wchar_t *exe;
	// Installers write SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\<key>
	// with InstallLocation; the key is also their default Program Files
	// folder. nullptr for clients without an installer.
	const wchar_t *installKey;
};

// Known clients. The order is the auto-detect priority when no -client is
// given. To support another client, add a line.
// clang-format off
constexpr std::array CLIENTS{
	//      short names                                                display name                   exe                    installer key
	Client{ { L"tommy", L"tommyternal", L"eternaljk2mv", L"eternal" }, L"EternalJK2MV (Tommyternal)", L"eternaljk2mvmp.exe", L"EternalJK2" },
	Client{ { L"jk2mv", L"mv" },                                       L"JK2MV",                      L"jk2mvmp.exe",        L"JK2MV" },
	Client{ { L"nwh" },                                                L"NWH",                        L"nwhmp.exe",          nullptr },
};
// clang-format on

struct HandleCloser {
	void operator()(HANDLE handle) const noexcept {
		CloseHandle(handle);
	}
};
using UniqueHandle = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleCloser>;

struct LocalFreer {
	void operator()(void *memory) const noexcept {
		LocalFree(memory);
	}
};

// Ordinal, so it doesn't depend on the user's locale (lstrcmpiW does, e.g.
// Turkish dotted/dotless i); the right comparison for file names and options
bool EqualsIgnoreCase(std::wstring_view lhs, std::wstring_view rhs) {
	return CompareStringOrdinal(lhs.data(), static_cast<int>(lhs.size()), rhs.data(), static_cast<int>(rhs.size()),
	                            TRUE) == CSTR_EQUAL;
}

std::wstring BaseName(const std::wstring &path) {
	const std::size_t sep = path.find_last_of(L"\\/");
	return sep == std::wstring::npos ? path : path.substr(sep + 1);
}

std::wstring DirName(const std::wstring &path) {
	const std::size_t sep = path.find_last_of(L"\\/");
	return sep == std::wstring::npos ? L"." : path.substr(0, sep);
}

bool FileExists(const std::wstring &path) {
	const DWORD attributes = GetFileAttributesW(path.c_str());
	return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
}

std::wstring ExeDir() {
	std::wstring path(MAX_PATH, L'\0');
	while (path.size() <= MAX_LONG_PATH) {
		const DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
		if (length == 0) {
			break;
		}
		if (length < path.size()) {
			path.resize(length);
			return DirName(path);
		}
		path.resize(path.size() * 2); // truncated, try a bigger buffer
	}
	return L".";
}

std::wstring EnvVar(const wchar_t *name) {
	const DWORD size = GetEnvironmentVariableW(name, nullptr, 0); // includes the terminator
	if (size == 0) {
		return {};
	}
	std::wstring value(size, L'\0');
	const DWORD length = GetEnvironmentVariableW(name, value.data(), size);
	if (length == 0 || length >= size) {
		return {};
	}
	value.resize(length);
	return value;
}

std::wstring RegistryString(HKEY root, const std::wstring &subKey, const wchar_t *valueName, DWORD viewFlag) {
	const DWORD flags = RRF_RT_REG_SZ | viewFlag;
	DWORD size = 0;
	if (RegGetValueW(root, subKey.c_str(), valueName, flags, nullptr, nullptr, &size) != ERROR_SUCCESS || size == 0) {
		return {};
	}
	std::wstring value(size / sizeof(wchar_t), L'\0');
	if (RegGetValueW(root, subKey.c_str(), valueName, flags, nullptr, value.data(), &size) != ERROR_SUCCESS) {
		return {};
	}
	value.resize(size / sizeof(wchar_t));
	while (!value.empty() && value.back() == L'\0') { // RegGetValueW includes the terminator
		value.pop_back();
	}
	return value;
}

// Appends arg to cmdLine, quoted so CommandLineToArgvW parses it back unchanged
void AppendArg(std::wstring &cmdLine, const std::wstring &arg) {
	if (!cmdLine.empty()) {
		cmdLine += L' ';
	}

	if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
		cmdLine += arg;
		return;
	}

	cmdLine += L'"';
	for (std::size_t i = 0;; ++i) {
		std::size_t backslashes = 0;
		while (i < arg.size() && arg[i] == L'\\') {
			++i;
			++backslashes;
		}
		if (i == arg.size()) {
			cmdLine.append(backslashes * 2, L'\\');
			break;
		}
		cmdLine.append(arg[i] == L'"' ? backslashes * 2 + 1 : backslashes, L'\\');
		cmdLine += arg[i];
	}
	cmdLine += L'"';
}

std::vector<std::wstring> CommandLineArgs() {
	int argc = 0;
	const std::unique_ptr<LPWSTR, LocalFreer> argv(CommandLineToArgvW(GetCommandLineW(), &argc));
	if (!argv) {
		return {};
	}
	// NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic): argv is an array of argc strings
	return { argv.get(), argv.get() + argc };
}

const Client *ClientByName(const std::wstring &name) {
	for (const Client &client : CLIENTS) {
		for (const wchar_t *clientName : client.names) {
			if (clientName != nullptr && EqualsIgnoreCase(name, clientName)) {
				return &client;
			}
		}
	}
	return nullptr;
}

std::wstring ClientNameList() {
	std::wstring list;
	for (const Client &client : CLIENTS) {
		list += L"  ";
		list += client.names[0];
		list += L"  -  ";
		list += client.displayName;
		list += L"\n";
	}
	return list;
}

// Installers record their folder for "Apps & features"; check both registry
// views so 32-bit and 64-bit installs are found
std::wstring InstalledExe(const Client &client) {
	if (client.installKey == nullptr) {
		return {};
	}

	const std::wstring subKey =
	    std::wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + client.installKey;

	for (HKEY root : { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER }) {
		for (const DWORD view : { RRF_SUBKEY_WOW6432KEY, RRF_SUBKEY_WOW6464KEY }) {
			std::wstring dir = RegistryString(root, subKey, L"InstallLocation", view);
			// stored quoted: "C:\Program Files (x86)\JK2MV"
			if (dir.size() >= 2 && dir.front() == L'"' && dir.back() == L'"') {
				dir = dir.substr(1, dir.size() - 2);
			}
			if (dir.empty()) {
				continue;
			}

			std::wstring exe = dir + L'\\' + client.exe;
			if (FileExists(exe)) {
				return exe;
			}
		}
	}

	return {};
}

// Finds client: installed, then portable in GameData, then the default
// Program Files folder. Returns an empty string if it isn't there.
std::wstring LocateClient(const Client &client, const std::vector<std::wstring> &gameDataDirs) {
	std::wstring exe = InstalledExe(client);
	if (!exe.empty()) {
		return exe;
	}

	std::vector<std::wstring> candidates;
	candidates.reserve(gameDataDirs.size() + 3);
	for (const std::wstring &dir : gameDataDirs) {
		candidates.push_back(dir + L'\\' + client.exe);
	}
	if (client.installKey != nullptr) {
		for (const wchar_t *var : { L"ProgramFiles(x86)", L"ProgramFiles", L"ProgramW6432" }) {
			const std::wstring dir = EnvVar(var);
			if (!dir.empty()) {
				candidates.push_back(dir + L'\\' + client.installKey + L'\\' + client.exe);
			}
		}
	}

	for (std::wstring &candidate : candidates) {
		if (FileExists(candidate)) {
			return std::move(candidate);
		}
	}

	return {};
}

void ShowError(const std::wstring &message) {
	MessageBoxW(nullptr, message.c_str(), TITLE, MB_OK | MB_ICONERROR);
}

// Works out which client exe to start; shows an error and returns an empty
// string if there is none
std::wstring ResolveClient(const std::wstring &requested, const std::wstring &gameDataDir) {
	std::vector<std::wstring> gameDataDirs;
	if (!gameDataDir.empty()) {
		gameDataDirs.push_back(gameDataDir);
	}
	gameDataDirs.push_back(DirName(ExeDir())); // jk2x\ inside GameData

	if (requested.empty()) {
		for (const Client &client : CLIENTS) {
			std::wstring exe = LocateClient(client, gameDataDirs);
			if (!exe.empty()) {
				return exe;
			}
		}
		ShowError(L"Could not find a multiplayer client.\n\n"
		          L"Install EternalJK2MV (github.com/TomArrow/jk2mv) or JK2MV, or choose\n"
		          L"a client by adding -client <name or path> before %command% in the\n"
		          L"Steam launch options. Names:\n\n" +
		          ClientNameList());
		return {};
	}

	if (const Client *client = ClientByName(requested)) {
		std::wstring exe = LocateClient(*client, gameDataDirs);
		if (exe.empty()) {
			const std::wstring where = client->installKey != nullptr ? L"Install it, or put " : L"Put ";
			ShowError(std::wstring(client->displayName) + L" was not found.\n\n" + where + client->exe +
			          L" in the GameData folder, or give its\nfull path: -client \"<path to " + client->exe + L">\"");
		}
		return exe;
	}

	if (FileExists(requested)) {
		return requested;
	}

	ShowError(L"-client " + requested +
	          L"\n\nis neither a known client nor an existing file. Use a path to the\n"
	          L"client exe, or one of these names:\n\n" +
	          ClientNameList());
	return {};
}

// Waits for the job's last process to exit; false if the port failed
bool WaitForJob(HANDLE completionPort) {
	DWORD message = 0;
	ULONG_PTR completionKey = 0;
	LPOVERLAPPED overlapped = nullptr;
	// the port is private to one job, so every message is about it
	while (GetQueuedCompletionStatus(completionPort, &message, &completionKey, &overlapped, INFINITE) != FALSE) {
		if (message == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO) {
			return true;
		}
	}
	return false;
}

// Runs exe with args from exe's folder and waits until it and every process
// it started have exited. Returns its exit code, or nothing if it could not
// be started (after showing an error).
std::optional<DWORD> RunAndWait(const std::wstring &exe, const std::vector<std::wstring> &args) {
	std::wstring cmdLine;
	AppendArg(cmdLine, exe);
	for (const std::wstring &arg : args) {
		AppendArg(cmdLine, arg);
	}

	// Track the whole process tree with a job object: the completion port
	// reports when its last process has exited, and closing the job (us
	// being killed) takes the game down with us. Both are best effort; without
	// them we wait for the first process only.
	const UniqueHandle job(CreateJobObjectW(nullptr, nullptr));
	const UniqueHandle port(CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1));
	bool portAttached = false;
	if (job) {
		JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
		limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
		SetInformationJobObject(job.get(), JobObjectExtendedLimitInformation, &limits,
		                        static_cast<DWORD>(sizeof(limits)));
		if (port) {
			JOBOBJECT_ASSOCIATE_COMPLETION_PORT association{};
			association.CompletionKey = job.get();
			association.CompletionPort = port.get();
			portAttached = SetInformationJobObject(job.get(), JobObjectAssociateCompletionPortInformation, &association,
			                                       static_cast<DWORD>(sizeof(association))) != FALSE;
		}
	}

	STARTUPINFOW startupInfo{};
	startupInfo.cb = static_cast<DWORD>(sizeof(startupInfo));
	PROCESS_INFORMATION processInfo{};
	const std::wstring workDir = DirName(exe); // like its Start menu shortcut

	if (CreateProcessW(exe.c_str(), cmdLine.data(), nullptr, nullptr, FALSE, CREATE_SUSPENDED, nullptr, workDir.c_str(),
	                   &startupInfo, &processInfo) == FALSE) {
		const DWORD error = GetLastError();
		ShowError(L"Could not start\n" + exe + L"\n\n(error " + std::to_wstring(error) + L")");
		return std::nullopt;
	}
	const UniqueHandle process(processInfo.hProcess);
	const UniqueHandle thread(processInfo.hThread);

	const bool inJob = job && AssignProcessToJobObject(job.get(), process.get()) != FALSE;
	if (ResumeThread(thread.get()) == static_cast<DWORD>(-1)) {
		const DWORD error = GetLastError();
		TerminateProcess(process.get(), 1);
		ShowError(L"Could not start\n" + exe + L"\n\n(error " + std::to_wstring(error) + L")");
		return std::nullopt;
	}

	if (!(inJob && portAttached && WaitForJob(port.get()))) {
		WaitForSingleObject(process.get(), INFINITE);
	}

	DWORD exitCode = 1;
	if (GetExitCodeProcess(process.get(), &exitCode) == FALSE) {
		exitCode = 1;
	}
	return exitCode;
}

} // namespace

int WINAPI wWinMain(_In_ HINSTANCE /*instance*/, _In_opt_ HINSTANCE /*prevInstance*/, _In_ LPWSTR /*cmdLine*/,
                    _In_ int /*showCmd*/) {
	const std::vector<std::wstring> args = CommandLineArgs();

	std::wstring stockSP;     // GameData\jk2sp.exe if Steam asked for singleplayer
	std::wstring gameDataDir; // folder of the stock exe Steam passed, if any
	std::wstring requestedClient;
	std::vector<std::wstring> gameArgs;

	for (std::size_t i = 1; i < args.size(); ++i) {
		const std::wstring &arg = args[i];
		const std::wstring name = BaseName(arg);

		if (EqualsIgnoreCase(name, L"jk2sp.exe")) {
			stockSP = arg;
			gameDataDir = DirName(arg);
		} else if (EqualsIgnoreCase(name, L"jk2mp.exe")) {
			gameDataDir = DirName(arg);
		} else if (EqualsIgnoreCase(arg, L"-client") && i + 1 < args.size()) {
			++i;
			requestedClient = args[i];
		} else {
			gameArgs.push_back(arg);
		}
	}

	const std::wstring exe = stockSP.empty() ? ResolveClient(requestedClient, gameDataDir) : stockSP;
	if (exe.empty()) {
		return 1;
	}

	const std::optional<DWORD> exitCode = RunAndWait(exe, gameArgs);
	return exitCode ? static_cast<int>(*exitCode) : 1;
}
