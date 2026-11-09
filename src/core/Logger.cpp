#include "core/Logger.h"

#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QThread>

#include <atomic>
#include <cstdio>
#include <cstring>

namespace ergo {

namespace {

std::atomic<Logger *> g_qtLogger{nullptr};

const QRegularExpression &logFilePattern()
{
    static const QRegularExpression re(QStringLiteral("^ergopro2-(\\d{4}-\\d{2}-\\d{2})(?:-(\\d+))?\\.log$"));
    return re;
}

void printToStderr(const QString &line)
{
    const QByteArray bytes = line.toLocal8Bit();
    std::fprintf(stderr, "%s\n", bytes.constData());
    std::fflush(stderr);
}

LogLevel levelFromQtType(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return LogLevel::Debug;
    case QtInfoMsg:
        return LogLevel::Info;
    case QtWarningMsg:
        return LogLevel::Warning;
    case QtCriticalMsg:
        return LogLevel::Error; // qCritical() is used for errors in this project
    case QtFatalMsg:
        return LogLevel::Critical;
    }
    return LogLevel::Info;
}

void qtMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    QString module = QStringLiteral("app");
    if (context.category && std::strcmp(context.category, "default") != 0)
        module = QString::fromLatin1(context.category);

    if (Logger *logger = g_qtLogger.load()) {
        logger->write(levelFromQtType(type), module, message);
    } else {
        printToStderr(QStringLiteral("[%1] [%2] %3").arg(logLevelName(levelFromQtType(type)), module, message));
    }
    // For QtFatalMsg Qt aborts the process right after this handler returns.
}

QString threadName()
{
    QThread *thread = QThread::currentThread();
    if (thread && !thread->objectName().isEmpty())
        return thread->objectName();
    return QStringLiteral("0x%1").arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16);
}

} // namespace

Logger::Logger(const IClock &clock)
    : m_clock(clock)
{
}

Logger::~Logger()
{
    if (g_qtLogger.load() == this)
        installQtMessageHandler(nullptr);
    close();
}

bool Logger::open(const Config &config)
{
    QMutexLocker lock(&m_mutex);
    m_config = config;
    m_config.retentionDays = qBound(kMinRetentionDays, m_config.retentionDays, kMaxRetentionDays);
    m_opened = true;

    if (!QDir().mkpath(m_config.directory)) {
        m_lastError = QStringLiteral("Cannot create log directory '%1'").arg(m_config.directory);
        printToStderr(m_lastError);
        return false;
    }

    const QDate today = m_clock.now().date();
    return openFileLocked(today, findWritableIndexLocked(today));
}

void Logger::close()
{
    QMutexLocker lock(&m_mutex);
    if (m_file.isOpen()) {
        m_file.flush();
        m_file.close();
    }
    m_opened = false;
}

void Logger::setSelection(LogLevelSelection selection)
{
    QMutexLocker lock(&m_mutex);
    m_config.selection = selection;
}

LogLevelSelection Logger::selection() const
{
    QMutexLocker lock(&m_mutex);
    return m_config.selection;
}

void Logger::setRetentionDays(int days)
{
    QMutexLocker lock(&m_mutex);
    m_config.retentionDays = qBound(kMinRetentionDays, days, kMaxRetentionDays);
}

int Logger::retentionDays() const
{
    QMutexLocker lock(&m_mutex);
    return m_config.retentionDays;
}

QString Logger::directory() const
{
    QMutexLocker lock(&m_mutex);
    return m_config.directory;
}

QString Logger::currentFilePath() const
{
    QMutexLocker lock(&m_mutex);
    return m_file.fileName();
}

QString Logger::lastError() const
{
    QMutexLocker lock(&m_mutex);
    return m_lastError;
}

QString Logger::fileNameFor(const QDate &date, int index)
{
    const QString day = date.toString(QStringLiteral("yyyy-MM-dd"));
    if (index <= 0)
        return QStringLiteral("ergopro2-%1.log").arg(day);
    return QStringLiteral("ergopro2-%1-%2.log").arg(day).arg(index);
}

void Logger::write(LogLevel level, QStringView module, QStringView message)
{
    const QDateTime now = m_clock.now();
    QMutexLocker lock(&m_mutex);
    if (!isLevelEnabled(m_config.selection, level))
        return;

    const QString line = formatLine(now, level, module, message);

    if (m_config.consoleOutput || !m_opened)
        printToStderr(line);
    if (!m_opened)
        return;

    ensureFileLocked(now.date(), line.size() + 1);
    writeLineLocked(line);
}

int Logger::cleanupOldFiles()
{
    QMutexLocker lock(&m_mutex);
    return cleanupLocked(m_clock.now().date());
}

void Logger::ensureFileLocked(const QDate &today, qint64 incomingBytes)
{
    if (today != m_fileDate) {
        // New day: switch file and apply the retention policy.
        openFileLocked(today, findWritableIndexLocked(today));
        cleanupLocked(today);
        return;
    }
    if (!m_file.isOpen()) {
        openFileLocked(today, findWritableIndexLocked(today));
        return;
    }
    if (m_file.size() + incomingBytes > m_config.maxFileSizeBytes && m_file.size() > 0)
        openFileLocked(today, m_fileIndex + 1);
}

bool Logger::openFileLocked(const QDate &date, int index)
{
    if (m_file.isOpen())
        m_file.close();

    m_fileDate = date;
    m_fileIndex = index;
    m_file.setFileName(QDir(m_config.directory).filePath(fileNameFor(date, index)));
    if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        m_lastError = QStringLiteral("Cannot open log file '%1': %2").arg(m_file.fileName(), m_file.errorString());
        printToStderr(m_lastError);
        return false;
    }
    m_lastError.clear();
    return true;
}

int Logger::findWritableIndexLocked(const QDate &date) const
{
    const QDir dir(m_config.directory);
    int index = 0;
    while (true) {
        const QFileInfo info(dir.filePath(fileNameFor(date, index)));
        if (!info.exists() || info.size() < m_config.maxFileSizeBytes)
            return index;
        ++index;
    }
}

int Logger::cleanupLocked(const QDate &today)
{
    // Keep today plus the previous (retentionDays - 1) days.
    const QDate oldestKept = today.addDays(-(m_config.retentionDays - 1));
    const QDir dir(m_config.directory);
    int removed = 0;

    const QStringList files = dir.entryList({QStringLiteral("ergopro2-*.log")}, QDir::Files);
    for (const QString &name : files) {
        const auto match = logFilePattern().match(name);
        if (!match.hasMatch())
            continue;
        const QDate date = QDate::fromString(match.captured(1), QStringLiteral("yyyy-MM-dd"));
        if (!date.isValid() || date >= oldestKept)
            continue;
        if (QFile::remove(dir.filePath(name))) {
            ++removed;
        } else {
            writeLineLocked(formatLine(m_clock.now(), LogLevel::Error, u"logging",
                                       QStringLiteral("Cannot delete old log file '%1'").arg(name)));
        }
    }
    if (removed > 0) {
        writeLineLocked(formatLine(m_clock.now(), LogLevel::Info, u"logging",
                                   QStringLiteral("Removed %1 log file(s) older than %2 day(s)")
                                       .arg(removed)
                                       .arg(m_config.retentionDays)));
    }
    return removed;
}

void Logger::writeLineLocked(const QString &line)
{
    if (!m_file.isOpen())
        return;
    const QByteArray bytes = line.toUtf8() + '\n';
    if (m_file.write(bytes) != bytes.size()) {
        m_lastError = QStringLiteral("Cannot write to log file '%1': %2").arg(m_file.fileName(), m_file.errorString());
        printToStderr(m_lastError);
        return;
    }
    m_file.flush(); // never lose the last lines before a crash
}

QString Logger::formatLine(const QDateTime &time, LogLevel level, QStringView module, QStringView message) const
{
    return QStringLiteral("%1 [%2] [%3] [%4] %5")
        .arg(time.toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz")), logLevelName(level), threadName(),
             module.toString(), message.toString());
}

void Logger::installQtMessageHandler(Logger *logger)
{
    g_qtLogger.store(logger);
    qInstallMessageHandler(logger ? qtMessageHandler : nullptr);
}

void logCritical(QStringView module, const QString &message)
{
    if (Logger *logger = g_qtLogger.load())
        logger->write(LogLevel::Critical, module, message);
    else
        printToStderr(QStringLiteral("[CRITICAL] [%1] %2").arg(module.toString(), message));
}

} // namespace ergo
