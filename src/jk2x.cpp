/*
jk2x - Steam launcher for Star Wars Jedi Knight II: Jedi Outcast that starts
a multiplayer client (EternalJK2MV, JK2MV, NWH, ...) instead of the stock
multiplayer exe.

Steam launch option:
	"<path>\jk2x.exe" %command%

Steam replaces %command% with the stock exe it would have run:
	GameData\jk2mp.exe  ("Launch Multiplayer")   -> start the client
	GameData\jk2sp.exe  ("Launch Single Player") -> start the stock jk2sp.exe
Started without Steam (no stock exe on the command line) it starts the client.
Every other argument is passed on to the game.

jk2x waits until the game and everything it started have exited, so Steam
keeps tracking the session (playtime, overlay, friends list). If jk2x is
closed (Steam's "Stop" button) the game is closed with it.

The client is looked up in this order:
	1. -client "<path to the client exe>" on the command line
	2. an installed client, by its installer's registry entry:
	   EternalJK2MV, then JK2MV
	3. a portable client in the GameData folder:
	   eternaljk2mvmp.exe, jk2mvmp.exe, nwhmp.exe
	4. Program Files (x86)\EternalJK2, \JK2MV (and Program Files)

The client is started from its own folder. Installed EternalJK2MV/JK2MV
find the game's assets0-5.pk3 in the Steam folder by themselves (through the
registry entry Steam writes for JK2). Portable builds can't (fs_assetspath is
compiled out of them), so they either live in GameData or have their own
copies of the assets.
*/

#include <windows.h>
#include <shellapi.h>
#include <string>
#include <vector>

static const wchar_t *TITLE = L"jk2x";

// Installers write SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\<key>
// with InstallLocation; the key is also their default Program Files folder
struct InstalledClient {
	const wchar_t *key;
	const wchar_t *exe;
};
static const InstalledClient INSTALLED_CLIENTS[] = {
	{ L"EternalJK2", L"eternaljk2mvmp.exe" },	// EternalJK2MV ("Tommyternal")
	{ L"JK2MV", L"jk2mvmp.exe" },
};

static const wchar_t *PORTABLE_EXES[] = {
	L"eternaljk2mvmp.exe",
	L"jk2mvmp.exe",
	L"nwhmp.exe",
};

static std::wstring BaseName(const std::wstring &path) {
	size_t sep = path.find_last_of(L"\\/");
	return sep == std::wstring::npos ? path : path.substr(sep + 1);
}

static std::wstring DirName(const std::wstring &path) {
	size_t sep = path.find_last_of(L"\\/");
	return sep == std::wstring::npos ? L"." : path.substr(0, sep);
}

static bool FileExists(const std::wstring &path) {
	DWORD attr = GetFileAttributesW(path.c_str());
	return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}

static std::wstring ExeDir() {
	wchar_t path[MAX_PATH];
	DWORD len = GetModuleFileNameW(NULL, path, MAX_PATH);
	if (len == 0 || len >= MAX_PATH)
		return L".";
	return DirName(path);
}

static std::wstring EnvVar(const wchar_t *name) {
	wchar_t value[MAX_PATH];
	DWORD len = GetEnvironmentVariableW(name, value, MAX_PATH);
	return (len == 0 || len >= MAX_PATH) ? std::wstring() : std::wstring(value);
}

// Appends arg to cmdLine, quoted so CommandLineToArgvW parses it back unchanged
static void AppendArg(std::wstring &cmdLine, const std::wstring &arg) {
	if (!cmdLine.empty())
		cmdLine += L' ';

	if (!arg.empty() && arg.find_first_of(L" \t\n\v\"") == std::wstring::npos) {
		cmdLine += arg;
		return;
	}

	cmdLine += L'"';
	for (size_t i = 0; ; i++) {
		size_t backslashes = 0;
		while (i < arg.size() && arg[i] == L'\\') {
			i++;
			backslashes++;
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

// Installers record their folder for "Apps & features"; check both registry
// views so 32-bit and 64-bit installs are found
static std::wstring InstalledClientExe(const InstalledClient &client) {
	const HKEY roots[] = { HKEY_LOCAL_MACHINE, HKEY_CURRENT_USER };
	const REGSAM views[] = { KEY_WOW64_32KEY, KEY_WOW64_64KEY };
	const std::wstring keyPath = std::wstring(L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\") + client.key;

	for (HKEY root : roots) {
		for (REGSAM view : views) {
			HKEY key;
			if (RegOpenKeyExW(root, keyPath.c_str(), 0, KEY_QUERY_VALUE | view, &key) != ERROR_SUCCESS) {
				continue;
			}

			wchar_t value[MAX_PATH] = {};
			DWORD size = sizeof(value) - sizeof(wchar_t);
			DWORD type;
			LONG result = RegQueryValueExW(key, L"InstallLocation", NULL, &type, (LPBYTE)value, &size);
			RegCloseKey(key);
			if (result != ERROR_SUCCESS || type != REG_SZ)
				continue;

			// stored quoted: "C:\Program Files (x86)\JK2MV"
			std::wstring dir = value;
			if (dir.size() >= 2 && dir.front() == L'"' && dir.back() == L'"')
				dir = dir.substr(1, dir.size() - 2);

			std::wstring exe = dir + L"\\" + client.exe;
			if (FileExists(exe))
				return exe;
		}
	}

	return std::wstring();
}

static std::wstring FindClient(const std::wstring &override, const std::wstring &gameDataDir) {
	if (!override.empty())
		return FileExists(override) ? override : std::wstring();

	for (const InstalledClient &client : INSTALLED_CLIENTS) {
		std::wstring exe = InstalledClientExe(client);
		if (!exe.empty())
			return exe;
	}

	std::vector<std::wstring> candidates;
	std::vector<std::wstring> gameDataDirs;
	if (!gameDataDir.empty())
		gameDataDirs.push_back(gameDataDir);
	gameDataDirs.push_back(DirName(ExeDir()));	// jk2x\ inside GameData
	for (const std::wstring &dir : gameDataDirs) {
		for (const wchar_t *exe : PORTABLE_EXES)
			candidates.push_back(dir + L"\\" + exe);
	}
	for (const wchar_t *var : { L"ProgramFiles(x86)", L"ProgramFiles", L"ProgramW6432" }) {
		std::wstring dir = EnvVar(var);
		if (dir.empty())
			continue;
		for (const InstalledClient &client : INSTALLED_CLIENTS)
			candidates.push_back(dir + L"\\" + client.key + L"\\" + client.exe);
	}

	for (const std::wstring &candidate : candidates) {
		if (FileExists(candidate))
			return candidate;
	}

	return std::wstring();
}

static void ShowError(const std::wstring &message) {
	MessageBoxW(NULL, message.c_str(), TITLE, MB_OK | MB_ICONERROR);
}

// Runs exe with cmdLine from workDir and waits until it and every process it
// started have exited. Returns false if it could not be started.
static bool RunAndWait(const std::wstring &exe, const std::wstring &args, const std::wstring &workDir, DWORD *exitCode) {
	std::wstring cmdLine;
	AppendArg(cmdLine, exe);
	if (!args.empty())
		cmdLine += L' ' + args;

	// Track the whole process tree with a job object: the completion port
	// reports when its last process has exited, and closing the job (us
	// being killed) takes the game down with us
	HANDLE job = CreateJobObjectW(NULL, NULL);
	HANDLE port = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
	JOBOBJECT_ASSOCIATE_COMPLETION_PORT jobPort = { job, port };
	SetInformationJobObject(job, JobObjectAssociateCompletionPortInformation, &jobPort, sizeof(jobPort));
	JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits = {};
	limits.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
	SetInformationJobObject(job, JobObjectExtendedLimitInformation, &limits, sizeof(limits));

	STARTUPINFOW si = { sizeof(si) };
	PROCESS_INFORMATION pi;
	std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
	cmdBuf.push_back(L'\0');

	if (!CreateProcessW(exe.c_str(), cmdBuf.data(), NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, workDir.c_str(), &si, &pi)) {
		DWORD error = GetLastError();
		CloseHandle(port);
		CloseHandle(job);
		ShowError(L"Could not start\n" + exe + L"\n\n(error " + std::to_wstring(error) + L")");
		return false;
	}

	const bool inJob = AssignProcessToJobObject(job, pi.hProcess) != FALSE;
	ResumeThread(pi.hThread);

	if (inJob) {
		DWORD msg;
		ULONG_PTR key;
		LPOVERLAPPED ov;
		while (GetQueuedCompletionStatus(port, &msg, &key, &ov, INFINITE)) {
			if (key == (ULONG_PTR)job && msg == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO)
				break;
		}
	} else {
		WaitForSingleObject(pi.hProcess, INFINITE);
	}

	GetExitCodeProcess(pi.hProcess, exitCode);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	CloseHandle(port);
	CloseHandle(job);
	return true;
}

int WINAPI wWinMain(HINSTANCE, HINSTANCE, LPWSTR, int) {
	int argc;
	LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (!argv)
		return 1;

	std::wstring stockSP;		// GameData\jk2sp.exe if Steam asked for singleplayer
	std::wstring gameDataDir;	// folder of the stock exe Steam passed, if any
	std::wstring clientOverride;
	std::wstring gameArgs;

	for (int i = 1; i < argc; i++) {
		std::wstring arg = argv[i];
		std::wstring name = BaseName(arg);

		if (!lstrcmpiW(name.c_str(), L"jk2sp.exe")) {
			stockSP = arg;
			gameDataDir = DirName(arg);
		} else if (!lstrcmpiW(name.c_str(), L"jk2mp.exe")) {
			gameDataDir = DirName(arg);
		} else if (!lstrcmpiW(arg.c_str(), L"-client") && i + 1 < argc) {
			clientOverride = argv[++i];
		} else {
			AppendArg(gameArgs, arg);
		}
	}
	LocalFree(argv);

	DWORD exitCode = 0;

	if (!stockSP.empty()) {
		if (!RunAndWait(stockSP, gameArgs, DirName(stockSP), &exitCode))
			return 1;
		return (int)exitCode;
	}

	std::wstring client = FindClient(clientOverride, gameDataDir);
	if (client.empty()) {
		if (!clientOverride.empty()) {
			ShowError(L"No client was found at\n" + clientOverride + L"\n\nCheck the -client path in the Steam launch options.");
		} else {
			ShowError(L"Could not find a multiplayer client.\n\n"
				L"Install EternalJK2MV (github.com/TomArrow/jk2mv) or JK2MV, or point\n"
				L"jk2x at a client by adding\n"
				L"-client \"<path to the client exe>\"\n"
				L"before %command% in the Steam launch options.");
		}
		return 1;
	}

	// start it from its own folder, like its Start menu shortcut does;
	// portable clients find their files relative to it
	if (!RunAndWait(client, gameArgs, DirName(client), &exitCode))
		return 1;
	return (int)exitCode;
}
