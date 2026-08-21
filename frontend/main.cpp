#include <cstdio>
#include <cstring>
#include <string>

#include <QApplication>
#include <QIcon>

#include <obs.h>

#include "main-window.hpp"
#include "obs-app.hpp"

/* CLI handling:
 *   obs-light.exe --version     -> print version and exit
 *   obs-light.exe --smoke-test  -> headless plugin/module verification (CI)
 *   obs-light.exe -minimized    -> start minimized to tray (shell startup) */

static bool HasArg(int argc, char *argv[], const char *arg)
{
	for (int i = 1; i < argc; i++) {
		if (argv[i] && strcmp(argv[i], arg) == 0)
			return true;
	}
	return false;
}

int main(int argc, char *argv[])
{
	if (HasArg(argc, argv, "--version")) {
		printf("obs-light %s\n", obs_get_version_string());
		return 0;
	}

	if (HasArg(argc, argv, "--smoke-test")) {
		OBSApp smokeApp;
		return smokeApp.RunSmokeTest();
	}

	QApplication app(argc, argv);
	app.setApplicationName("obs-light");
	app.setApplicationDisplayName("obs-light");
	app.setOrganizationName("obs-light");
	app.setQuitOnLastWindowClosed(false);

	QIcon appIcon(":/obs-light.png");
	app.setWindowIcon(appIcon);

	OBSApp obsApp;
	if (!obsApp.Initialize()) {
		fprintf(stderr, "obs-light failed to initialize. See the log file "
				"in %%APPDATA%%\\obs-light\\logs\\\n");
		return 1;
	}

	MainWindow window;
	Q_UNUSED(window);

	return app.exec();
}
