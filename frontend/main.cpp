#include <cstdio>
#include <cstring>
#include <string>

#include <Windows.h>

#include <QApplication>
#include <QIcon>

#include <obs.h>

#include "main-window.hpp"
#include "obs-app.hpp"

/* CLI handling:
 *   obs-lite.exe --version     -> print version and exit
 *   obs-lite.exe --smoke-test  -> headless plugin/module verification (CI)
 *   obs-lite.exe -minimized    -> start minimized to tray (shell startup) */

static bool HasArg(int argc, char *argv[], const char *arg)
{
	for (int i = 1; i < argc; i++) {
		if (argv[i] && strcmp(argv[i], arg) == 0)
			return true;
	}
	return false;
}

/* libobs resolves its default module paths (../../obs-plugins/64bit) and
 * plugin data paths relative to the process working directory.  Anchor the
 * working directory to the executable's folder so obs-lite finds its
 * plugins no matter how it is launched (Explorer, shortcuts, shell, CI). */
static void SetWorkingDirectoryToExe()
{
	wchar_t path[MAX_PATH];
	if (!GetModuleFileNameW(NULL, path, MAX_PATH))
		return;

	wchar_t *slash = wcsrchr(path, L'\\');
	if (!slash)
		return;
	*slash = 0;

	SetCurrentDirectoryW(path);
}

int main(int argc, char *argv[])
{
	SetWorkingDirectoryToExe();

	if (HasArg(argc, argv, "--version")) {
		AttachConsole(ATTACH_PARENT_PROCESS);
		printf("obs-lite %s\n", obs_get_version_string());
		return 0;
	}

	if (HasArg(argc, argv, "--smoke-test")) {
		AttachConsole(ATTACH_PARENT_PROCESS);
		OBSApp smokeApp;
		return smokeApp.RunSmokeTest();
	}

	QApplication app(argc, argv);
	app.setApplicationName("obs-lite");
	app.setApplicationDisplayName("obs-lite");
	app.setOrganizationName("obs-lite");
	app.setQuitOnLastWindowClosed(false);

	QIcon appIcon(":/obs-lite.png");
	app.setWindowIcon(appIcon);

	OBSApp obsApp;
	if (!obsApp.Initialize()) {
		fprintf(stderr, "obs-lite failed to initialize. See the log file "
				"in %%APPDATA%%\\obs-lite\\logs\\\n");
		return 1;
	}

	MainWindow window;
	Q_UNUSED(window);

	return app.exec();
}
