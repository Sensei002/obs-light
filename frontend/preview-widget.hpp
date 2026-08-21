#pragma once

#include <QWidget>

#include <obs.h>

/* Minimal preview widget backed by obs_display.  Mirrors the upstream
 * OBSQTDisplay class but without any of the OBSFrontend dependencies. */
class PreviewWidget : public QWidget {
	Q_OBJECT

public:
	PreviewWidget(QWidget *parent = nullptr);
	~PreviewWidget() override;

	obs_display_t *GetDisplay() const { return display; }
	void SetBackgroundColor(uint32_t rgba);
	uint32_t GetBackgroundColor() const { return backgroundColor; }

protected:
	void paintEvent(QPaintEvent *event) override;
	void resizeEvent(QResizeEvent *event) override;
	void moveEvent(QMoveEvent *event) override;
	bool nativeEvent(const QByteArray &eventType, void *message,
			 qintptr *result) override;
	QPaintEngine *paintEngine() const override;

private:
	void CreateDisplay();
	void DestroyDisplay();
	void OnResize();
	void OnDisplayChange();

	obs_display_t *display = nullptr;
	uint32_t backgroundColor = 0xFF222222;
	bool destroying = false;

signals:
	void DisplayCreated(obs_display_t *display);
	void DisplayResized();
};