/*
jk2x - Steam launcher for Star Wars Jedi Knight II: Jedi Outcast that starts
a modern client (EternalJK2MV, JK2MV, NWH, OpenJO, or any other exe) instead
of the stock game exes.

Steam launch option:
    "<path>\jk2x.exe" %command%
    "<path>\jk2x.exe" -client jk2mv %command%

Steam replaces %command% with the stock exe it would have run:
    GameData\jk2mp.exe  ("Launch Multiplayer")   -> a client from MP_CLIENTS
    GameData\jk2sp.exe  ("Launch Single Player") -> the first engine from
                                                    SP_CLIENTS that is found,
                                                    else the stock jk2sp.exe
Started without Steam (no stock exe on the command line) it does multiplayer.
Every other argument is passed on to the game.

jk2x waits until the game and everything it started have exited, so Steam
keeps tracking the session (playtime, overlay, friends list). If jk2x is
closed (Steam's "Stop" button) the game is closed with it.

Which multiplayer client:
    -client <name>    a client from MP_CLIENTS by one of its short names
    -client "<path>"  any client exe
    -client stock     the game's own jk2mp.exe
    (none)            the first client in MP_CLIENTS that is found
There is no option for singleplayer while SP_CLIENTS has a single entry.

A known client is found, in order: through its installer's registry entry,
as a portable exe in GameData (or GameData\<portable dir>), in its default
Program Files folder. It is started from its own folder, like its Start menu
shortcut does.

Installed EternalJK2MV/JK2MV find the game's assets0-5.pk3 in the Steam
folder by themselves (through the registry entry Steam writes for JK2).
Portable JK2MV builds can't (fs_assetspath is compiled out of them), nor can
NWH, so they live in GameData or keep their own copies of the assets.
OpenJO can: jk2x points its fs_cdpath at GameData when it lives elsewhere.
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
constexpr std::size_t MAX_CLIENT_EXES = 2;

struct Client {
	// short names for -client, case-insensitive; unused slots are nullptr
	std::array<const wchar_t *, MAX_CLIENT_NAMES> names;
	const wchar_t *displayName;
	// exe file names, preferred first (64-bit before 32-bit); unused slots are nullptr
	std::array<const wchar_t *, MAX_CLIENT_EXES> exes;
	// Installers write SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\<key>
	// with InstallLocation; the key is also their default Program Files
	// folder. nullptr for clients without an installer.
	const wchar_t *installKey;
	// Portable clients are looked for in GameData and, if set, GameData\<dir>
	const wchar_t *portableDir;
	// Cvar that points the client at the game files when it doesn't live in
	// GameData itself (jk2x passes +set <cvar> "<GameData>"). nullptr if the
	// client has no such setting.
	const wchar_t *gameDataCvar;
};

// Known clients per Steam menu entry. The order is the auto-detect priority
// (when no -client is given). To support another client, add a line.
// clang-format off
constexpr std::array MP_CLIENTS{
	//      short names                                                display name                   exes                                              installer key  portable dir  game files cvar
	Client{ { L"tommy", L"tommyternal", L"eternaljk2mv", L"eternal" }, L"EternalJK2MV (Tommyternal)", { L"eternaljk2mvmp.exe" },                         L"EternalJK2", nullptr,      nullptr },
	Client{ { L"jk2mv", L"mv" },                                       L"JK2MV",                      { L"jk2mvmp.exe" },                                L"JK2MV",      nullptr,      nullptr },
	Client{ { L"nwh" },                                                L"NWH",                        { L"nwhmp.exe" },                                  nullptr,       nullptr,      nullptr },
};
constexpr std::array SP_CLIENTS{
	Client{ { L"openjo" },                                             L"OpenJO",                     { L"openjo_sp.x86_64.exe", L"openjo_sp.x86.exe" }, nullptr,       L"OpenJO",    L"fs_cdpath" },
};
// clang-format on

// -client stock: the game's own exe that Steam would have run
constexpr const wchar_t *STOCK_NAME = L"stock";

struct Mode {
	const wchar_t *option;      // command line option choosing the program; nullptr if none
	const wchar_t *description; // for messages
	const wchar_t *stockExe;    // the game's own exe for this mode
	const wchar_t *installHint; // what to install when nothing is found
};
constexpr Mode MULTIPLAYER{ L"-client", L"multiplayer client", L"jk2mp.exe",
	                        L"Install EternalJK2MV (github.com/TomArrow/jk2mv) or JK2MV" };
constexpr Mode SINGLEPLAYER{ nullptr, L"singleplayer engine", L"jk2sp.exe",
	                         L"Put OpenJO (github.com/JACoders/OpenJK/releases) in GameData\\OpenJO" };

struct Launch {
	std::wstring exe;
	std::vector<std::wstring> args; // jk2x's own arguments, before the user's
};

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

bool HasGameFiles(const std::wstring &dir) {
	return FileExists(dir + L"\\base\\assets0.pk3");
}

template <typename Clients>
const Client *ClientByName(const Clients &clients, const std::wstring &name) {
	for (const Client &client : clients) {
		for (const wchar_t *clientName : client.names) {
			if (clientName != nullptr && EqualsIgnoreCase(name, clientName)) {
				return &client;
			}
		}
	}
	return nullptr;
}

// The known client an exe path belongs to, by its file name
template <typename Clients>
const Client *ClientByExe(const Clients &clients, const std::wstring &exePath) {
	const std::wstring fileName = BaseName(exePath);
	for (const Client &client : clients) {
		for (const wchar_t *exe : client.exes) {
			if (exe != nullptr && EqualsIgnoreCase(fileName, exe)) {
				return &client;
			}
		}
	}
	return nullptr;
}

template <typename Clients>
std::wstring ClientNameList(const Clients &clients, const Mode &mode) {
	std::wstring list;
	for (const Client &client : clients) {
		list += L"  ";
		list += client.names[0];
		list += L"  -  ";
		list += client.displayName;
		list += L"\n";
	}
	list += L"  ";
	list += STOCK_NAME;
	list += L"  -  the game's own ";
	list += mode.stockExe;
	list += L"\n";
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

			for (const wchar_t *exeName : client.exes) {
				if (exeName == nullptr) {
					continue;
				}
				std::wstring exe = dir + L'\\' + exeName;
				if (FileExists(exe)) {
					return exe;
				}
			}
		}
	}

	return {};
}

// Finds client: installed, then portable in GameData (or GameData\<portable
// dir>), then the default Program Files folder. Empty if it isn't there.
std::wstring LocateClient(const Client &client, const std::vector<std::wstring> &gameDataDirs) {
	std::wstring exe = InstalledExe(client);
	if (!exe.empty()) {
		return exe;
	}

	std::vector<std::wstring> dirs;
	for (const std::wstring &dir : gameDataDirs) {
		dirs.push_back(dir);
		if (client.portableDir != nullptr) {
			dirs.push_back(dir + L'\\' + client.portableDir);
		}
	}
	if (client.installKey != nullptr) {
		for (const wchar_t *var : { L"ProgramFiles(x86)", L"ProgramFiles", L"ProgramW6432" }) {
			const std::wstring dir = EnvVar(var);
			if (!dir.empty()) {
				dirs.push_back(dir + L'\\' + client.installKey);
			}
		}
	}

	for (const std::wstring &dir : dirs) {
		for (const wchar_t *exeName : client.exes) {
			if (exeName == nullptr) {
				continue;
			}
			std::wstring candidate = dir + L'\\' + exeName;
			if (FileExists(candidate)) {
				return candidate;
			}
		}
	}

	return {};
}

void ShowError(const std::wstring &message) {
	MessageBoxW(nullptr, message.c_str(), TITLE, MB_OK | MB_ICONERROR);
}

struct GameDataPaths {
	std::vector<std::wstring> searchDirs; // where portable clients may live
	std::wstring gameData;                // the one holding the game files; empty if unknown
};

// What to start for exe: a known client living outside GameData that can
// read the game files from elsewhere gets pointed at GameData
Launch LaunchFor(std::wstring exe, const Client *client, const GameDataPaths &paths) {
	Launch launch{ std::move(exe), {} };
	if (client != nullptr && client->gameDataCvar != nullptr && !paths.gameData.empty() &&
	    !EqualsIgnoreCase(DirName(launch.exe), paths.gameData)) {
		launch.args = { L"+set", client->gameDataCvar, paths.gameData };
	}
	return launch;
}

// Works out what to start for mode. stockExe is the game's own exe (empty if
// unknown). Shows an error and returns nothing if there is nothing to start.
template <typename Clients>
std::optional<Launch> ResolveLaunch(const Clients &clients, const Mode &mode, const std::wstring &requested,
                                    const GameDataPaths &paths, const std::wstring &stockExe) {
	const std::wstring names = ClientNameList(clients, mode);

	// a mode without an option (singleplayer) always auto-detects
	if (requested.empty() || mode.option == nullptr) {
		for (const Client &client : clients) {
			std::wstring exe = LocateClient(client, paths.searchDirs);
			if (!exe.empty()) {
				return LaunchFor(std::move(exe), &client, paths);
			}
		}
		if (&mode == &SINGLEPLAYER && !stockExe.empty()) {
			return Launch{ stockExe, {} }; // no replacement engine: the original game
		}
		std::wstring message = std::wstring(L"Could not find a ") + mode.description + L".\n\n" + mode.installHint;
		if (mode.option != nullptr) {
			message += std::wstring(L", or choose one by adding\n") + mode.option +
			           L" <name or path> before %command% in the Steam launch options. Names:\n\n" + names;
		}
		ShowError(message);
		return std::nullopt;
	}

	if (EqualsIgnoreCase(requested, STOCK_NAME)) {
		if (!stockExe.empty() && FileExists(stockExe)) {
			return Launch{ stockExe, {} };
		}
		ShowError(std::wstring(L"The game's own ") + mode.stockExe + L" was not found.");
		return std::nullopt;
	}

	if (const Client *client = ClientByName(clients, requested)) {
		std::wstring exe = LocateClient(*client, paths.searchDirs);
		if (exe.empty()) {
			const std::wstring where = client->installKey != nullptr ? L"Install it, or put " : L"Put ";
			const std::wstring exeName = client->exes[0];
			ShowError(std::wstring(client->displayName) + L" was not found.\n\n" + where + exeName +
			          L" in the GameData folder, or give its\nfull path: " + mode.option + L" \"<path to " + exeName +
			          L">\"");
			return std::nullopt;
		}
		return LaunchFor(std::move(exe), client, paths);
	}

	if (FileExists(requested)) {
		return LaunchFor(requested, ClientByExe(clients, requested), paths);
	}

	ShowError(std::wstring(mode.option) + L" " + requested +
	          L"\n\nis neither a known name nor an existing file. Use a path to an exe,\n"
	          L"or one of these names:\n\n" +
	          names);
	return std::nullopt;
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

	std::wstring stockSP;  // GameData\jk2sp.exe if Steam asked for singleplayer
	std::wstring stockMP;  // GameData\jk2mp.exe if Steam asked for multiplayer
	std::wstring steamDir; // folder of the stock exe Steam passed, if any
	std::wstring requestedClient;
	std::vector<std::wstring> gameArgs;

	for (std::size_t i = 1; i < args.size(); ++i) {
		const std::wstring &arg = args[i];
		const std::wstring name = BaseName(arg);

		if (EqualsIgnoreCase(name, SINGLEPLAYER.stockExe)) {
			stockSP = arg;
			steamDir = DirName(arg);
		} else if (EqualsIgnoreCase(name, MULTIPLAYER.stockExe)) {
			stockMP = arg;
			steamDir = DirName(arg);
		} else if (EqualsIgnoreCase(arg, MULTIPLAYER.option) && i + 1 < args.size()) {
			++i;
			requestedClient = args[i];
		} else {
			gameArgs.push_back(arg);
		}
	}

	// Where portable clients may live: the folder Steam passed, and the one
	// jk2x\ sits in. GameData is the first of them holding the game files.
	GameDataPaths paths;
	if (!steamDir.empty()) {
		paths.searchDirs.push_back(steamDir);
	}
	paths.searchDirs.push_back(DirName(ExeDir()));
	for (const std::wstring &dir : paths.searchDirs) {
		if (HasGameFiles(dir)) {
			paths.gameData = dir;
			break;
		}
	}
	if (stockMP.empty() && !paths.gameData.empty()) {
		stockMP = paths.gameData + L'\\' + MULTIPLAYER.stockExe; // for -client stock without Steam
	}

	const std::optional<Launch> launch = stockSP.empty()
	                                         ? ResolveLaunch(MP_CLIENTS, MULTIPLAYER, requestedClient, paths, stockMP)
	                                         : ResolveLaunch(SP_CLIENTS, SINGLEPLAYER, {}, paths, stockSP);
	if (!launch) {
		return 1;
	}

	// jk2x's own arguments first, so the user's can override them
	std::vector<std::wstring> launchArgs = launch->args;
	launchArgs.insert(launchArgs.end(), gameArgs.begin(), gameArgs.end());

	const std::optional<DWORD> exitCode = RunAndWait(launch->exe, launchArgs);
	return exitCode ? static_cast<int>(*exitCode) : 1;
}
