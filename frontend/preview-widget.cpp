#include "preview-widget.hpp"

#include <Windows.h>

#include <QWindow>

#include <graphics/graphics.h>

/* Mirror of the upstream OBSQTDisplay implementation for OBS 32's
 * obs_display_create(gs_init_data, background_color) API. */

static bool QtToGSWindow(QWindow *window, gs_window &gswindow)
{
#ifdef _WIN32
	gswindow.hwnd = (HWND)window->winId();
	return gswindow.hwnd != nullptr;
#else
	UNUSED_PARAMETER(window);
	UNUSED_PARAMETER(gswindow);
	return false;
#endif
}

PreviewWidget::PreviewWidget(QWidget *parent) : QWidget(parent)
{
	setAttribute(Qt::WA_PaintOnScreen);
	setAttribute(Qt::WA_StaticContents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_OpaquePaintEvent);
	setAttribute(Qt::WA_DontCreateNativeAncestors);
	setAttribute(Qt::WA_NativeWindow);

	auto windowVisible = [this](bool visible) {
		if (!visible) {
			return;
		}
		if (!display) {
			CreateDisplay();
		} else {
			QSize size = size();
			obs_display_resize(display, (uint32_t)size.width(),
					   (uint32_t)size.height());
		}
	};

	connect(windowHandle(), &QWindow::visibleChanged, this, windowVisible);
}

PreviewWidget::~PreviewWidget()
{
	destroying = true;
	DestroyDisplay();
}

void PreviewWidget::SetBackgroundColor(uint32_t rgba)
{
	backgroundColor = rgba;
	if (display)
		obs_display_set_background_color(display, backgroundColor);
}

void PreviewWidget::CreateDisplay()
{
	if (display || destroying)
		return;

	if (!windowHandle()->isExposed())
		return;

	QSize size = this->size();

	gs_init_data info = {};
	info.cx = (uint32_t)size.width();
	info.cy = (uint32_t)size.height();
	info.format = GS_BGRA;
	info.zsformat = GS_ZS_NONE;
	info.adapter = 0;

	if (!QtToGSWindow(windowHandle(), info.window))
		return;

	display = obs_display_create(&info, backgroundColor);
	if (display)
		emit DisplayCreated(display);
}

void PreviewWidget::DestroyDisplay()
{
	if (display) {
		obs_display_destroy(display);
		display = nullptr;
	}
}

void PreviewWidget::OnResize()
{
	CreateDisplay();
	if (isVisible() && display) {
		QSize size = this->size();
		obs_display_resize(display, (uint32_t)size.width(),
				   (uint32_t)size.height());
		emit DisplayResized();
	}
}

void PreviewWidget::paintEvent(QPaintEvent *event)
{
	CreateDisplay();
	QWidget::paintEvent(event);
}

QPaintEngine *PreviewWidget::paintEngine() const
{
	return nullptr;
}

void PreviewWidget::resizeEvent(QResizeEvent *event)
{
	QWidget::resizeEvent(event);
	OnResize();
}

void PreviewWidget::moveEvent(QMoveEvent *event)
{
	QWidget::moveEvent(event);
	if (display)
		obs_display_update_color_space(display);
}

bool PreviewWidget::nativeEvent(const QByteArray &, void *message, qintptr *)
{
#ifdef _WIN32
	const MSG &msg = *static_cast<MSG *>(message);
	switch (msg.message) {
	case WM_DISPLAYCHANGE:
		if (display)
			obs_display_update_color_space(display);
		break;
	case WM_HOTKEY:
		/* Forwarded to MainWindow's nativeEventFilter */
		break;
	}
#else
	UNUSED_PARAMETER(message);
#endif
	return false;
}
