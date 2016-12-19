#include "slideeffectplayer.h"
#include "application.h"
#include "controller/configsetter.h"
#include <QDebug>
#include <QFileInfo>
#include <QTimerEvent>
#include <QPainter>

namespace {

const QString DURATION_SETTING_GROUP = "SLIDESHOWDURATION";
const QString DURATION_SETTING_KEY = "Duration";
const int ANIMATION_DURATION  = 1000;

} // namespace

SlideEffectPlayer::SlideEffectPlayer(QObject *parent)
    : QObject(parent)
{

}

void SlideEffectPlayer::timerEvent(QTimerEvent *e)
{
    if (e->timerId() != m_tid)
        return;
    if (m_effect)
        m_effect->deleteLater();
    if (! startNext()) {
        stop();
    }
}

int SlideEffectPlayer::duration() const
{
    return dApp->setter->value(DURATION_SETTING_GROUP,
                               DURATION_SETTING_KEY).toInt() * 1000;
}

void SlideEffectPlayer::setFrameSize(int width, int height)
{
    m_w = width;
    m_h = height;
}

void SlideEffectPlayer::setImagePaths(const QStringList& paths)
{
    m_paths = paths;
    m_current = m_paths.constBegin();
}

void SlideEffectPlayer::setCurrentImage(const QString &path)
{
    m_current = std::find(m_paths.cbegin(), m_paths.cend(),  path);
    if (m_current == m_paths.constEnd())
        m_current = m_paths.constBegin();
}

QString SlideEffectPlayer::currentImagePath() const
{
    if (m_current == m_paths.constEnd())
        return *m_paths.constBegin();
    return *m_current;
}

bool SlideEffectPlayer::isRunning() const
{
    return m_running;
}

void SlideEffectPlayer::start()
{
    if (m_paths.isEmpty())
        return;
    m_running = true;
    m_tid = startTimer(duration());
}

bool SlideEffectPlayer::startNext()
{
    if (m_paths.isEmpty())
        return false;
    QSize fSize(m_w, m_h);
    if (! fSize.isValid()) {
        qWarning() << "Invalid frame size!";
        return false;
    }

    const QString oldPath = currentImagePath();

    if (m_paths.length() > 1) {

        m_current ++;
        if (m_current == m_paths.constEnd()) {
            m_current = m_paths.constBegin();
            if (!QFileInfo(*m_current).exists()) {
                m_current = m_paths.constBegin() + 1;
            }
        }
    }

    QString newPath = currentImagePath();
    m_effect = SlideEffect::create();
    m_effect->setDuration(ANIMATION_DURATION);
    m_effect->setSize(fSize);

    m_effect->setImages(oldPath, newPath);
    if (!m_thread.isRunning())
        m_thread.start();

    m_effect->moveToThread(&m_thread);
    connect(m_effect, &SlideEffect::frameReady, this, [=] (const QImage &img) {
        if (m_running) {
            Q_EMIT frameReady(img);
        }
    }, Qt::DirectConnection);
    QMetaObject::invokeMethod(m_effect, "start");
    return true;
}

void SlideEffectPlayer::stop()
{
    if (!isRunning())
        return;
    killTimer(m_tid);
    m_tid = 0;
    m_running = false;
    Q_EMIT finished();
}
