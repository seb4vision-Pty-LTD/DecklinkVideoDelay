#include "MainWindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDebug>
#include <cmath>
#include <iostream>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    setWindowTitle("DeckLink Dual-Channel Delay");
    resize(1280, 800);

    // --- Create Video Rendering Widgets ---
    auto createVideoTile = [](const QString &title, QWidget *&widget) -> QGroupBox * {
        auto *box = new QGroupBox(title);
        auto *layout = new QVBoxLayout(box);
        widget = new QWidget();
        widget->setStyleSheet("background-color: black;");
        widget->setMinimumSize(320, 240);
        layout->addWidget(widget);
        return box;
    };

    QGroupBox *boxL0 = createVideoTile("Input 0: Live Feed (DeckLink 0)", m_videoWidgetLive0);
    QGroupBox *boxD0 = createVideoTile("Input 0: Delayed Output (DeckLink 2)", m_videoWidgetDelay0);
    QGroupBox *boxL1 = createVideoTile("Input 1: Live Feed (DeckLink 1)", m_videoWidgetLive1);
    QGroupBox *boxD1 = createVideoTile("Input 1: Delayed Output (DeckLink 3)", m_videoWidgetDelay1);

    // Grid Layout for Side-by-Side Visual Inspection
    auto *videoGrid = new QGridLayout();
    videoGrid->addWidget(boxL0, 0, 0);
    videoGrid->addWidget(boxD0, 0, 1);
    videoGrid->addWidget(boxL1, 1, 0);
    videoGrid->addWidget(boxD1, 1, 1);

    // --- Create Audio VU Bars ---
    auto setupProgressBar = [](QProgressBar *bar) {
        bar->setRange(0, 100);
        bar->setTextVisible(false);
        bar->setOrientation(Qt::Vertical);
        bar->setFixedHeight(120);
        bar->setStyleSheet(
            "QProgressBar { border: 1px solid grey; background-color: #222; }"
            "QProgressBar::chunk { background-color: qlineargradient(x1:0, y1:1, x2:0, y2:0, "
            "stop: 0 green, stop: 0.7 yellow, stop: 0.9 red); }"
        );
    };

    m_audioBarCh0_L = new QProgressBar(); setupProgressBar(m_audioBarCh0_L);
    m_audioBarCh0_R = new QProgressBar(); setupProgressBar(m_audioBarCh0_R);
    m_audioBarCh1_L = new QProgressBar(); setupProgressBar(m_audioBarCh1_L);
    m_audioBarCh1_R = new QProgressBar(); setupProgressBar(m_audioBarCh1_R);

    auto *audioLayout0 = new QHBoxLayout();
    audioLayout0->addWidget(new QLabel("Ch0 L:")); audioLayout0->addWidget(m_audioBarCh0_L);
    audioLayout0->addWidget(new QLabel("R:")); audioLayout0->addWidget(m_audioBarCh0_R);

    auto *audioLayout1 = new QHBoxLayout();
    audioLayout1->addWidget(new QLabel("Ch1 L:")); audioLayout1->addWidget(m_audioBarCh1_L);
    audioLayout1->addWidget(new QLabel("R:")); audioLayout1->addWidget(m_audioBarCh1_R);

   // --- Delay Control Panels ---
    m_delaySpinBox0 = new QDoubleSpinBox();
    m_delaySpinBox0->setRange(0.00, 10.00);
    m_delaySpinBox0->setSingleStep(0.05);
    m_delaySpinBox0->setDecimals(3);
    m_delaySpinBox0->setValue(0.000);
    m_delaySpinBox0->setSuffix(" sec");

    m_delaySpinBox1 = new QDoubleSpinBox();
    m_delaySpinBox1->setRange(0.00, 10.00);
    m_delaySpinBox1->setSingleStep(0.05);
    m_delaySpinBox1->setDecimals(3);
    m_delaySpinBox1->setValue(0.000);
    m_delaySpinBox1->setSuffix(" sec");

    m_startBtn = new QPushButton("Start");
    m_stopBtn = new QPushButton("Stop");
    m_stopBtn->setEnabled(false);

    // Group controls for Channel 0
    auto *controlsLayout0 = new QHBoxLayout();
    controlsLayout0->addWidget(new QLabel("<b>Ch 0 Delay:</b>"));
    controlsLayout0->addWidget(m_delaySpinBox0);
    controlsLayout0->addSpacing(10);
    controlsLayout0->addLayout(audioLayout0);

    // Group controls for Channel 1
    auto *controlsLayout1 = new QHBoxLayout();
    controlsLayout1->addWidget(new QLabel("<b>Ch 1 Delay:</b>"));
    controlsLayout1->addWidget(m_delaySpinBox1);
    controlsLayout1->addSpacing(10);
    controlsLayout1->addLayout(audioLayout1);

    // Main bottom control bar
    auto *controlsLayout = new QHBoxLayout();
    controlsLayout->addLayout(controlsLayout0);
    controlsLayout->addSpacing(40); // Visual separation between channels
    controlsLayout->addLayout(controlsLayout1);
    controlsLayout->addStretch();
    controlsLayout->addWidget(m_startBtn);
    controlsLayout->addWidget(m_stopBtn);

    auto *mainLayout = new QVBoxLayout();
    mainLayout->addLayout(videoGrid, 1);
    mainLayout->addLayout(controlsLayout);

    auto *centralWidget = new QWidget();
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    connect(m_delaySpinBox0, &QDoubleSpinBox::valueChanged, this, &MainWindow::onDelay0ValueChanged);
    connect(m_delaySpinBox1, &QDoubleSpinBox::valueChanged, this, &MainWindow::onDelay1ValueChanged);
    connect(m_startBtn, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(m_stopBtn, &QPushButton::clicked, this, &MainWindow::onStopClicked);
}


MainWindow::~MainWindow() {
    onStopClicked();
}

void MainWindow::initPipeline() {
    // Window handles for VideoOverlay mapping
    m_winIdLive0 = m_videoWidgetLive0->winId();
    m_winIdDelay0 = m_videoWidgetDelay0->winId();
    m_winIdLive1 = m_videoWidgetLive1->winId();
    m_winIdDelay1 = m_videoWidgetDelay1->winId();

    // High-performance low-latency GStreamer pipeline string stringifying hardware & software branches
    QString pipelineStr = QString(
        "pipeline name=decklink_delay_pipe "

        // --- CHANNEL 0 PIPELINE ---
        " decklinkvideosrc device-number=0 mode=1080i50 ! videoconvert ! tee name=tv0 "
        "tv0. ! queue ! videoconvert ! d3d11videosink name=sink_live0 sync=false qos=false "
        "tv0. ! queue name=q_delay_v0 max-size-buffers=0 max-size-bytes=0 ! videoconvert ! tee name=tv0_out "
        "tv0_out. ! queue ! decklinkvideosink device-number=2 mode=1080i50 "
        "tv0_out. ! queue ! videoconvert ! d3d11videosink name=sink_delay0 sync=false qos=false "

        "decklinkaudiosrc device-number=0 ! audioconvert ! audioresample ! level name=lvl0 message=true interval=50000000 ! "
        "queue name=q_delay_a0 max-size-buffers=0 max-size-bytes=0 ! audioconvert ! audioresample ! decklinkaudiosink device-number=2 "

        // --- CHANNEL 1 PIPELINE ---
        " decklinkvideosrc device-number=1 mode=1080i50 ! videoconvert ! tee name=tv1 "
        "tv1. ! queue ! videoconvert ! d3d11videosink name=sink_live1 sync=false qos=false "
        "tv1. ! queue name=q_delay_v1 max-size-buffers=0 max-size-bytes=0 ! videoconvert ! tee name=tv1_out "
        "tv1_out. ! queue ! decklinkvideosink device-number=3 mode=1080i50 "
        "tv1_out. ! queue ! videoconvert ! d3d11videosink name=sink_delay1 sync=false qos=false "

        "decklinkaudiosrc device-number=1 ! audioconvert ! audioresample ! level name=lvl1 message=true interval=50000000 ! "
        "queue name=q_delay_a1 max-size-buffers=0 max-size-bytes=0 ! audioconvert ! audioresample ! decklinkaudiosink device-number=3 "
    );

    GError *error = nullptr;
    m_pipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), &error);

    if (error) {
        qCritical() << "Pipeline creation failed:" << error->message;
        g_error_free(error);
        return;
    }

    // Retrieve Queue Elements for Dynamic Time Delay Adjustments
    m_qDelayV0 = gst_bin_get_by_name(GST_BIN(m_pipeline), "q_delay_v0");
    m_qDelayA0 = gst_bin_get_by_name(GST_BIN(m_pipeline), "q_delay_a0");
    m_qDelayV1 = gst_bin_get_by_name(GST_BIN(m_pipeline), "q_delay_v1");
    m_qDelayA1 = gst_bin_get_by_name(GST_BIN(m_pipeline), "q_delay_a1");

    // Set Initial Delay
    onDelay0ValueChanged(m_delaySpinBox0->value());
    onDelay1ValueChanged(m_delaySpinBox1->value());

    // Intercept GstBus Messages synchronously for Overlay handles and Level updates
    GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(m_pipeline));
    gst_bus_set_sync_handler(bus, MainWindow::busSyncHandler, this, nullptr);
    gst_object_unref(bus);
}
void MainWindow::onDelay0ValueChanged(double value) {
    if (!m_qDelayV0 || !m_qDelayA0) return;

    guint64 delayNs = static_cast<guint64>(value * GST_SECOND);

    // Apply only to Channel 0 (Video and Audio)
    g_object_set(G_OBJECT(m_qDelayV0),
        "max-size-time", delayNs,
        "min-threshold-time", delayNs,
        NULL);
        
    g_object_set(G_OBJECT(m_qDelayA0),
        "max-size-time", delayNs,
        "min-threshold-time", delayNs,
        NULL);
}

void MainWindow::onDelay1ValueChanged(double value) {
    if (!m_qDelayV1 || !m_qDelayA1) return;

    guint64 delayNs = static_cast<guint64>(value * GST_SECOND);

    // Apply only to Channel 1 (Video and Audio)
    g_object_set(G_OBJECT(m_qDelayV1),
        "max-size-time", delayNs,
        "min-threshold-time", delayNs,
        NULL);
        
    g_object_set(G_OBJECT(m_qDelayA1),
        "max-size-time", delayNs,
        "min-threshold-time", delayNs,
        NULL);
}



GstBusSyncReply MainWindow::busSyncHandler(GstBus *bus, GstMessage *message, gpointer user_data) {
    auto *self = static_cast<MainWindow *>(user_data);

    // 1. Handle Overlay Window Handles for Video Rendering on HWND
    if (gst_is_video_overlay_prepare_window_handle_message(message)) {
        GstVideoOverlay *overlay = GST_VIDEO_OVERLAY(GST_MESSAGE_SRC(message));
        const gchar *name = GST_OBJECT_NAME(GST_MESSAGE_SRC(message));

        if (g_strcmp0(name, "sink_live0") == 0) {
            gst_video_overlay_set_window_handle(overlay, self->m_winIdLive0);
        } else if (g_strcmp0(name, "sink_delay0") == 0) {
            gst_video_overlay_set_window_handle(overlay, self->m_winIdDelay0);
        } else if (g_strcmp0(name, "sink_live1") == 0) {
            gst_video_overlay_set_window_handle(overlay, self->m_winIdLive1);
        } else if (g_strcmp0(name, "sink_delay1") == 0) {
            gst_video_overlay_set_window_handle(overlay, self->m_winIdDelay1);
        }
        
        gst_message_unref(message);
        return GST_BUS_DROP;
    }

    // 2. Process Level Element Messages for Audio Bar Updates
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ELEMENT) {
        
        const GstStructure *s = gst_message_get_structure(message);
        if (s && gst_structure_has_name(s, "level")) {
            
            const gchar *srcName = GST_OBJECT_NAME(GST_MESSAGE_SRC(message));
            const GValue *list_rms = gst_structure_get_value(s, "rms");

            if (list_rms) {
                double rms_l = -60.0;
                double rms_r = -60.0;
                bool parsedRms = false;

                // Different GStreamer builds expose "rms" using different container types.
#if defined(G_TYPE_VALUE_ARRAY)
                if (G_VALUE_TYPE(list_rms) == G_TYPE_VALUE_ARRAY) {
                    const auto *legacyArray = static_cast<const GValueArray *>(g_value_get_boxed(list_rms));
                    if (legacyArray && legacyArray->n_values > 0) {
                        const GValue *v0 = g_value_array_get_nth(const_cast<GValueArray *>(legacyArray), 0);
                        const GValue *v1 = (legacyArray->n_values > 1)
                            ? g_value_array_get_nth(const_cast<GValueArray *>(legacyArray), 1)
                            : v0;

                        if (v0 && G_VALUE_HOLDS_DOUBLE(v0)) {
                            rms_l = g_value_get_double(v0);
                            rms_r = (v1 && G_VALUE_HOLDS_DOUBLE(v1)) ? g_value_get_double(v1) : rms_l;
                            parsedRms = true;
                        }
                    }
                } else
#endif
                if (GST_VALUE_HOLDS_LIST(list_rms) && gst_value_list_get_size(list_rms) > 0) {
                    rms_l = g_value_get_double(gst_value_list_get_value(list_rms, 0));
                    rms_r = (gst_value_list_get_size(list_rms) > 1) ? 
                             g_value_get_double(gst_value_list_get_value(list_rms, 1)) : rms_l;
                    parsedRms = true;
                } 
                else if (GST_VALUE_HOLDS_ARRAY(list_rms) && gst_value_array_get_size(list_rms) > 0) {
                    rms_l = g_value_get_double(gst_value_array_get_value(list_rms, 0));
                    rms_r = (gst_value_array_get_size(list_rms) > 1) ? 
                             g_value_get_double(gst_value_array_get_value(list_rms, 1)) : rms_l;
                    parsedRms = true;
                } else if (G_VALUE_HOLDS_DOUBLE(list_rms)) {
                    rms_l = g_value_get_double(list_rms);
                    rms_r = rms_l;
                    parsedRms = true;
                }

                if (!parsedRms) {
                    std::cout << "Unhandled RMS type from " << srcName
                              << ": " << G_VALUE_TYPE_NAME(list_rms) << std::endl;
                    gst_message_unref(message);
                    return GST_BUS_DROP;
                }

                // std::cout << "Level Message from " << srcName << ": RMS L="
                //           << rms_l << " dB, RMS R=" << rms_r << " dB"
                //           << " (type=" << G_VALUE_TYPE_NAME(list_rms) << ")" << std::endl;

                int ch = -1;
                if (g_strcmp0(srcName, "lvl0") == 0) {
                    ch = 0;
                } else if (g_strcmp0(srcName, "lvl1") == 0) {
                    ch = 1;
                }

                if (ch < 0) {
                    std::cout << "Ignoring level message from unexpected element: "
                              << srcName << std::endl;
                    return GST_BUS_PASS;
                }

                QMetaObject::invokeMethod(self, [self, ch, rms_l, rms_r]() {
                    auto convertDbToPercent = [](double db) -> int {
                        if (db < -60.0) return 0;
                        if (db >= 0.0) return 100;
                        return static_cast<int>(((db + 60.0) / 60.0) * 100.0);
                    };

                    if (ch == 0) {
                        self->m_audioBarCh0_L->setValue(convertDbToPercent(rms_l));
                        self->m_audioBarCh0_R->setValue(convertDbToPercent(rms_r));
                    } else {
                        self->m_audioBarCh1_L->setValue(convertDbToPercent(rms_l));
                        self->m_audioBarCh1_R->setValue(convertDbToPercent(rms_r));
                    }
                }, Qt::QueuedConnection);
            }
        }
    }
    
    // 3. Catch and print actual errors before dropping them
    if (GST_MESSAGE_TYPE(message) == GST_MESSAGE_ERROR) {
        GError *err = nullptr;
        gchar *debug_info = nullptr;
        gst_message_parse_error(message, &err, &debug_info);
        qCritical() << "Pipeline Error from" << GST_OBJECT_NAME(message->src) << ":" << err->message;
        if (debug_info) qCritical() << "Debug info:" << debug_info;
        g_clear_error(&err);
        g_free(debug_info);
    }

    // 4. Drop the message to prevent bus overflow
    gst_message_unref(message);
    return GST_BUS_DROP;
}

void MainWindow::onStartClicked() {
    if (!m_pipeline) {
        initPipeline();
    }
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
        m_startBtn->setEnabled(false);
        m_stopBtn->setEnabled(true);
    }
}

void MainWindow::onStopClicked() {
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
        m_qDelayV0 = m_qDelayA0 = m_qDelayV1 = m_qDelayA1 = nullptr;

        m_audioBarCh0_L->setValue(0);
        m_audioBarCh0_R->setValue(0);
        m_audioBarCh1_L->setValue(0);
        m_audioBarCh1_R->setValue(0);
    }
    m_startBtn->setEnabled(true);
    m_stopBtn->setEnabled(false);
}