#pragma once

#include <QMainWindow>
#include <QWidget>
#include <QDoubleSpinBox>
#include <QProgressBar>
#include <QPushButton>
#include <QGridLayout>
#include <QLabel>

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onDelay0ValueChanged(double value);
    void onDelay1ValueChanged(double value);

    void onStartClicked();
    void onStopClicked();

private:
    void initPipeline();
    void updateAudioMeter(int channel, double rms_db);
    static GstBusSyncReply busSyncHandler(GstBus *bus, GstMessage *message, gpointer user_data);

    // UI Widgets
    QWidget *m_videoWidgetLive0;
    QWidget *m_videoWidgetDelay0;
    QWidget *m_videoWidgetLive1;
    QWidget *m_videoWidgetDelay1;

    QProgressBar *m_audioBarCh0_L;
    QProgressBar *m_audioBarCh0_R;
    QProgressBar *m_audioBarCh1_L;
    QProgressBar *m_audioBarCh1_R;

    QDoubleSpinBox *m_delaySpinBox0;
    QDoubleSpinBox *m_delaySpinBox1;
    
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;

    // GStreamer Pipeline Elements
    GstElement *m_pipeline = nullptr;
    GstElement *m_qDelayV0 = nullptr;
    GstElement *m_qDelayA0 = nullptr;
    GstElement *m_qDelayV1 = nullptr;
    GstElement *m_qDelayA1 = nullptr;

    // Window Handles for Overlay
    uintptr_t m_winIdLive0 = 0;
    uintptr_t m_winIdDelay0 = 0;
    uintptr_t m_winIdLive1 = 0;
    uintptr_t m_winIdDelay1 = 0;
};