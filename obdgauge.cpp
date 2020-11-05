#include "obdgauge.h"
#include "ui_obdgauge.h"
#include "pid.h"

ObdGauge::ObdGauge(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ObdGauge)
{
    ui->setupUi(this);

    setWindowTitle("Elm327 Obd2");

    this->centralWidget()->setStyleSheet("background-image: url(:/img/carbon-fiber.png); border: none;");

    pushSim = new QPushButton;
    pushExit = new QPushButton;
    pushSim->setText("Start Sim");
    pushExit->setText("Exit");
    pushExit->setStyleSheet("font-size: 18pt; font-weight: bold; color: white;background-color: #055580;");
    pushSim->setStyleSheet("font-size: 18pt; font-weight: bold; color: white;background-color: #055580;");

    connect(pushSim, &QPushButton::clicked, this, &ObdGauge::on_pushSim_clicked);
    connect(pushExit, &QPushButton::clicked, this, &ObdGauge::on_pushExit_clicked);

    initGauges();

    if(osName() == "windows")
    {
        ui->gridLayout_Gauges->addWidget(mSpeedGauge, 0, 0);
        ui->gridLayout_Gauges->addWidget(mRpmGauge, 1, 0);
        ui->gridLayout_Gauges->addWidget(pushExit, 2, 0);
        //ui->gridLayout_Gauges->addWidget(pushSim, 3, 0);
    }
    else
    {
        foreach (QScreen *screen, QGuiApplication::screens())
        {
            if(screen->orientation() == Qt::LandscapeOrientation)
            {
                ui->gridLayout_Gauges->addWidget(mRpmGauge, 0, 0);
                ui->gridLayout_Gauges->addWidget(mSpeedGauge, 0, 1);
                ui->gridLayout_Gauges->addWidget(pushExit, 1, 0, 1, 2);
            }
            else if(screen->orientation() == Qt::PortraitOrientation)
            {
                ui->gridLayout_Gauges->addWidget(mRpmGauge, 0, 0);
                ui->gridLayout_Gauges->addWidget(mSpeedGauge, 1, 0);
                ui->gridLayout_Gauges->addWidget(pushExit, 2, 0);
            }

            screen->setOrientationUpdateMask(Qt::LandscapeOrientation |
                                             Qt::PortraitOrientation |
                                             Qt::InvertedLandscapeOrientation |
                                             Qt::InvertedPortraitOrientation);

            QObject::connect(screen, &QScreen::orientationChanged, this, &ObdGauge::orientationChanged);
        }
    }

    mRpmGauge->update();
    mSpeedGauge->update();

    runtimeCommands.clear();
    runtimeCommands.append(ENGINE_RPM);
    runtimeCommands.append(VEHICLE_SPEED);

    if(ConnectionManager::getInstance() && ConnectionManager::getInstance()->isConnected())
    {
        connect(ConnectionManager::getInstance(),&ConnectionManager::dataReceived,this, &ObdGauge::dataReceived);
        mRunning = true;
        send(VOLTAGE);
    }
}

ObdGauge::~ObdGauge()
{
    delete ui;
}

void ObdGauge::initGauges()
{
    //speed
    mSpeedGauge = new QcGaugeWidget;
    mSpeedGauge->addBackground(99);
    QcBackgroundItem *bkgSpeed1 = mSpeedGauge->addBackground(92);
    bkgSpeed1->clearrColors();
    bkgSpeed1->addColor(0.1,Qt::black);
    bkgSpeed1->addColor(1.0,Qt::white);

    QcBackgroundItem *bkgSpeed2 = mSpeedGauge->addBackground(88);
    bkgSpeed2->clearrColors();
    bkgSpeed2->addColor(0.1,Qt::black);
    bkgSpeed2->addColor(1.0,Qt::darkGray);

    mSpeedGauge->addArc(55);
    auto degrees = mSpeedGauge->addDegrees(65);
    degrees->setStep(20);
    degrees->setValueRange(0,220);

    auto colorBandSpeed = mSpeedGauge->addColorBand(50);
    QList<QPair<QColor, float> > colors;
    QColor tmpColor;
    tmpColor.setAlphaF(0.1);
    QPair<QColor,float> pair;

    pair.first = Qt::darkGreen;
    pair.second = 37;
    colors.append(pair);

    pair.first = Qt::yellow;
    pair.second = 55;
    colors.append(pair);

    pair.first = Qt::red;
    pair.second = 100;
    colors.append(pair);
    colorBandSpeed->setColors(colors);

    auto values = mSpeedGauge->addValues(74);
    values->setStep(20);
    values->setValueRange(0,220);

    mSpeedGauge->addLabel(60)->setText("Km/h");
    QcLabelItem *labSpeed = mSpeedGauge->addLabel(40);
    labSpeed->setText("0");
    mSpeedNeedle = mSpeedGauge->addNeedle(60);
    mSpeedNeedle->setNeedle(QcNeedleItem::DiamonNeedle);
    mSpeedNeedle->setLabel(labSpeed);
    mSpeedNeedle->setColor(Qt::white);
    mSpeedNeedle->setValueRange(0,220);
    mSpeedGauge->addBackground(7);
    mSpeedGauge->addGlass(88);

    //rpm
    mRpmGauge = new QcGaugeWidget;
    mRpmGauge->addBackground(99);
    QcBackgroundItem *bkgRpm1 = mRpmGauge->addBackground(92);
    bkgRpm1->clearrColors();
    bkgRpm1->addColor(0.1,Qt::black);
    bkgRpm1->addColor(1.0,Qt::white);

    QcBackgroundItem *bkgRpm2 = mRpmGauge->addBackground(88);
    bkgRpm2->clearrColors();
    bkgRpm2->addColor(0.1,Qt::black);
    bkgRpm2->addColor(1.0,Qt::darkGray);

    mRpmGauge->addArc(55);
    mRpmGauge->addDegrees(65)->setValueRange(0,60);
    auto colorBandRpm = mRpmGauge->addColorBand(50);
    colors.clear();

    pair.first = Qt::darkGreen;
    pair.second = 33;
    colors.append(pair);

    pair.first = Qt::yellow;
    pair.second = 66;
    colors.append(pair);

    pair.first = Qt::red;
    pair.second = 100;
    colors.append(pair);
    colorBandRpm->setColors(colors);

    mRpmGauge->addValues(74)->setValueRange(0,60);

    mRpmGauge->addLabel(60)->setText("X100");
    QcLabelItem *labRpm = mRpmGauge->addLabel(40);
    labRpm->setText("0");
    mRpmNeedle = mRpmGauge->addNeedle(80);
    mRpmNeedle->setNeedle(QcNeedleItem::DiamonNeedle);
    mRpmNeedle->setLabel(labRpm);
    mRpmNeedle->setColor(Qt::white);
    mRpmNeedle->setValueRange(0,60);
    mRpmGauge->addBackground(7);
    mRpmGauge->addGlass(88);

    /*engine = new QQmlApplicationEngine;
    engine->load(QUrl(QLatin1String("qrc:/GaugeScreen.qml")));
    QWindow *qmlWindow = qobject_cast<QWindow*>(engine->rootObjects().at(0));
    QWidget *container = QWidget::createWindowContainer(qmlWindow);
    ui->verticalLayout->addWidget(container);*/
}

void ObdGauge::startSim()
{
    m_realTime = 0;
    m_timerId  = startTimer(0);
    m_time.start();
}

void ObdGauge::stopSim()
{
    if ( m_timerId ) killTimer( m_timerId );
}

void ObdGauge::setSpeed(int speed)
{
    mSpeedNeedle->setCurrentValue(speed);
    /*QObject *rootObject = engine->rootObjects().first();
    if(rootObject != nullptr)
    {
        QMetaObject::invokeMethod(rootObject, "setSpeed", Q_ARG(QVariant, speed));
    }*/
}

void ObdGauge::setRpm(int rpm)
{
    mRpmNeedle->setCurrentValue(rpm);
    /*QObject *rootObject = engine->rootObjects().first();
    if(rootObject != nullptr)
    {
        QMetaObject::invokeMethod(rootObject, "setRpm", Q_ARG(QVariant, rpm));
    }*/
}

void ObdGauge::timerEvent( QTimerEvent *event )
{
    Q_UNUSED(event)

    auto timeStep = m_time.restart();
    m_realTime = m_realTime + timeStep / 1000.0f;
    valueGauge  =  111.0f * std::sin( m_realTime /  5.0f ) +  111.0f;
    setSpeed(static_cast<int>(valueGauge));
    setRpm(static_cast<int>(valueGauge/2.75));
}


QString ObdGauge::send(const QString &command)
{
    if(mRunning && ConnectionManager::getInstance())
    {
        ConnectionManager::getInstance()->send(command);
    }

    return QString();
}

void ObdGauge::analysData(const QString &dataReceived)
{
    unsigned A = 0;
    unsigned B = 0;
    unsigned PID = 0;
    double value = 0;

    std::vector<QString> vec;
    auto resp= elm->prepareResponseToDecode(dataReceived);

    if(resp.size()>2 && !resp[0].compare("41",Qt::CaseInsensitive))
    {
        QRegularExpression hexMatcher("^[0-9A-F]{2}$", QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatch match = hexMatcher.match(resp[1]);
        if (!match.hasMatch())
            return;

        PID =std::stoi(resp[1].toStdString(),nullptr,16);
        std::vector<QString> vec;

        vec.insert(vec.begin(),resp.begin()+2, resp.end());
        if(vec.size()>=2)
        {
            A = std::stoi(vec[0].toStdString(),nullptr,16);
            B = std::stoi(vec[1].toStdString(),nullptr,16);
        }
        else if(vec.size()>=1)
        {
            A = std::stoi(vec[0].toStdString(),nullptr,16);
            B = 0;
        }

        switch (PID)
        {
        case 12: //PID(0C): RPM
            //((A*256)+B)/4
            value = ((A * 256) + B) / 4;
            setRpm(static_cast<int>(value / 100));
            break;
        case 13://PID(0D): KM Speed
            // A
            value = A;
            setSpeed(static_cast<int>(value));
            break;
        default:
            //A
            value = A;
            break;
        }
    }
}

void ObdGauge::dataReceived(QString dataReceived)
{
    if(!mRunning)return;

    if(runtimeCommands.size() == commandOrder)
    {
        commandOrder = 0;
        send(runtimeCommands[commandOrder]);
    }

    if(commandOrder < runtimeCommands.size())
    {
        send(runtimeCommands[commandOrder]);
        commandOrder++;
    }

    if(dataReceived.isEmpty())return;

    if(dataReceived.toUpper().startsWith("UNABLETOCONNECT"))
        return;

    try
    {
        analysData(dataReceived);
    }
    catch (const std::exception& e)
    {
    }
    catch (...)
    {
    }
}

void ObdGauge::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    mRunning = false;
    emit on_close_gauge();
}

void ObdGauge::on_pushExit_clicked()
{
    mRunning = false;
    close();
}

void ObdGauge::on_pushSim_clicked()
{
    if(pushSim->text() == "Start Sim")
    {
        startSim();
        pushSim->setText("Stop Sim");
    }
    else
    {
        stopSim();
        setSpeed(static_cast<int>(0));
        setRpm(static_cast<int>(0));
        pushSim->setText("Start Sim");
    }
}

void ObdGauge::orientationChanged(Qt::ScreenOrientation orientation)
{  
    switch (orientation) {
    case Qt::ScreenOrientation::PortraitOrientation:
        ui->gridLayout_Gauges->addWidget(mSpeedGauge, 0, 0);
        ui->gridLayout_Gauges->addWidget(mRpmGauge, 1, 0);
        ui->gridLayout_Gauges->addWidget(pushExit, 2, 0);
        break;
    case Qt::ScreenOrientation::LandscapeOrientation:
        ui->gridLayout_Gauges->addWidget(mSpeedGauge, 0, 0);
        ui->gridLayout_Gauges->addWidget(mRpmGauge, 0, 1);
        ui->gridLayout_Gauges->addWidget(pushExit, 1, 0, 1, 2);
        break;
    default:
        break;
    }
}
