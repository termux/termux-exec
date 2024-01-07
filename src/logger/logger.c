#define _GNU_SOURCE
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "../os/safe_strerror.h"
#include "logger.h"

static char sDefaultLogTagBuffer[LOGGER_TAG_MAX_LENGTH] = "Logger";

/** The default log tag used if log methods that do not require a `tag` are called. */
static const char* sDefaultLogTag = sDefaultLogTagBuffer;

static char sLogTagPrefixBuffer[LOGGER_TAG_MAX_LENGTH] = "";

/** The log tag prefixed before the `tag` passed to log methods that require a `tag` by `getFullTag()`. */
static const char* sLogTagPrefix = sLogTagPrefixBuffer;

/** The current log level. */
static int sCurrentLogLevel = DEFAULT_LOG_LEVEL;

/**
 * The pid of current process.
 *
 * - https://man7.org/linux/man-pages/man2/getpid.2.html
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r54:bionic/libc/bionic/pthread_internal.h
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r54:bionic/libc/bionic/getpid.cpp
 */
static pid_t sLogPid = -1;



/**
 * Whether to log entries with the `pid priority tag: message` format
 * or only the `message`.
 */
static bool sFormattedLogging = true;



static const char* getFullTag(const char* tag, char *buffer, size_t buffer_len);
static pid_t getLogPid();





void setDefaultLogTagAndPrefix(const char* defaultLogTag) {
    if (defaultLogTag == NULL) return;
    setDefaultLogTag(defaultLogTag);

    char logTagPrefix[strlen(defaultLogTag) + 2];
    snprintf(logTagPrefix, sizeof(logTagPrefix), "%s.", defaultLogTag);
    setLogTagPrefix(logTagPrefix);
}



const char* getDefaultLogTag() {
    return sDefaultLogTag;
}

void setDefaultLogTag(const char* defaultLogTag) {
    if (defaultLogTag == NULL) return;
    strncpy(sDefaultLogTagBuffer, defaultLogTag, sizeof(sDefaultLogTagBuffer) - 1);
}


const char* getLogTagPrefix() {
    return sLogTagPrefix;
}

void setLogTagPrefix(const char* logTagPrefix) {
    if (logTagPrefix == NULL) return;
    strncpy(sLogTagPrefixBuffer, logTagPrefix, sizeof(sLogTagPrefixBuffer) - 1);
}



int getCurrentLogLevel() {
    return sCurrentLogLevel;
}

/**
 * Set `sCurrentLogLevel` if a valid `logLevel` is passed, otherwise to `DEFAULT_LOG_LEVEL`.
 *
 * If `context` passed is not `null`, then a toast will be shown for the actual new log level set.
 *
 * @return Returns the new log level set.
 */
int setCurrentLogLevel(int logLevel) {
    sCurrentLogLevel = getLogLevelIfValidOtherwiseDefault(logLevel);
    return sCurrentLogLevel;
}

/**
 * Returns `true` if `logLevel` passed is a valid log level, otherwise `DEFAULT_LOG_LEVEL`.
 */
int getLogLevelIfValidOtherwiseDefault(int logLevel) {
    return isLogLevelValid(logLevel) ? logLevel : DEFAULT_LOG_LEVEL;
}

/**
 * Returns `true` if `logLevel` passed is a valid log level, otherwise `false`.
 */
bool isLogLevelValid(int logLevel) {
    return (logLevel >= LOG_LEVEL_OFF && logLevel <= MAX_LOG_LEVEL);
}



/** Get logger `sLogPid`. */
static pid_t getLogPid() {
    if (sLogPid < 0)
        sLogPid = getpid();
    return sLogPid;
}



bool isFormattedLoggingEnabled() {
    return sFormattedLogging;
}


void setFormattedLogging(bool formattedLogging) {
    sFormattedLogging = formattedLogging;
}





void printMessageToStdout(char logPriorityChar, bool logOnStderr,
    const char* tag, const char* message) {
    int errnoCode = errno;
    pid_t pid = getLogPid();

    const char* finalTag = tag != NULL ? tag : "null";

    if (isFormattedLoggingEnabled()) {
        const char* fmt = "%5d %c %-8s: %s\n";
        if (logOnStderr) {
            fprintf(stderr, fmt, pid, logPriorityChar, finalTag, message);
        } else {
            fprintf(stdout, fmt, pid, logPriorityChar, finalTag, message);
            fflush(stdout);
        }
    } else {
        if (logOnStderr) {
            fprintf(stderr, "%s\n", message);
        } else {
            fprintf(stdout, "%s\n", message);
            fflush(stdout);
        }
    }
    errno = errnoCode;
}

void vprintMessageToStdout(char logPriorityChar, bool logOnStderr,
    const char* tag, const char* fmt, va_list args) {
    char message[LOGGER_ENTRY_MAX_PAYLOAD];
    vsnprintf(message, LOGGER_ENTRY_MAX_PAYLOAD, fmt, args);
    printMessageToStdout(logPriorityChar, logOnStderr, tag, message);
}

#define ILOGGER_IMPL(logPriorityName, logPriorityChar, logOnStderr)                                \
void logVPrint##logPriorityName(const char* tag, const char* fmt, va_list args) {                  \
    vprintMessageToStdout(logPriorityChar, logOnStderr, tag, fmt, args);                           \
}

ILOGGER_IMPL(Error, 'E', true)
ILOGGER_IMPL(Warn, 'W', true)
ILOGGER_IMPL(Info, 'I', false)
ILOGGER_IMPL(Debug, 'D', false)
ILOGGER_IMPL(Verbose, 'V', false)

#undef ILOGGER_IMPL





void logMessage(int logPriority, const char* tag, const char* fmt, va_list args) {
    char tag_buffer[LOGGER_TAG_MAX_LENGTH];

    if (logPriority == LOG_PRIORITY_ERROR && sCurrentLogLevel >= LOG_LEVEL_NORMAL) {
        logVPrintError(getFullTag(tag, tag_buffer, sizeof(tag_buffer)), fmt, args);
    } else if (logPriority == LOG_PRIORITY_WARN && sCurrentLogLevel >= LOG_LEVEL_NORMAL) {
        logVPrintWarn(getFullTag(tag, tag_buffer, sizeof(tag_buffer)), fmt, args);
    } else if (logPriority == LOG_PRIORITY_INFO && sCurrentLogLevel >= LOG_LEVEL_NORMAL) {
        logVPrintInfo(getFullTag(tag, tag_buffer, sizeof(tag_buffer)), fmt, args);
    } else if (logPriority == LOG_PRIORITY_DEBUG && sCurrentLogLevel >= LOG_LEVEL_DEBUG) {
        logVPrintDebug(getFullTag(tag, tag_buffer, sizeof(tag_buffer)), fmt, args);
    } else if (logPriority == LOG_PRIORITY_VERBOSE && sCurrentLogLevel >= LOG_LEVEL_VERBOSE) {
        logVPrintVerbose(getFullTag(tag, tag_buffer, sizeof(tag_buffer)), fmt, args);
    } else if (logPriority == LOG_PRIORITY_VVERBOSE && sCurrentLogLevel >= LOG_LEVEL_VVERBOSE) {
        logVPrintVerbose(getFullTag(tag, tag_buffer, sizeof(tag_buffer)), fmt, args);
    }
}



/**
 * Get full log tag to use for logging for a `tag`.
 *
 * If `tag` equals `sDefaultLogTag` or `sLogTagPrefix` is empty, then `tag` is
 * used as is, otherwise `sLogTagPrefix` is prefixed before `tag`.
 */
static const char* getFullTag(const char* tag, char *buffer, size_t buffer_len) {
    if (tag == NULL || strlen(tag) < 1) {
        return sLogTagPrefix;
    } else if (strcmp(sDefaultLogTag, tag) == 0 || strlen(sLogTagPrefix) < 1) {
        return tag;
    } else {
        snprintf(buffer, buffer_len, "%s%s", sLogTagPrefix, tag);
        return buffer;
    }
}





#define LOG_MESSAGE_IMPL(logPriorityName, logPriorityNum, logLevel)                                \
void log##logPriorityName(const char* tag, const char* fmt, ...) {                                 \
    if (sCurrentLogLevel >= logLevel) {                                                            \
        va_list args;                                                                              \
        va_start(args, fmt);                                                                       \
        logMessage(logPriorityNum, tag, fmt, args);                                                \
        va_end(args);                                                                              \
    }                                                                                              \
}

LOG_MESSAGE_IMPL(Error, LOG_PRIORITY_ERROR, LOG_LEVEL_NORMAL)
LOG_MESSAGE_IMPL(Warn, LOG_PRIORITY_WARN, LOG_LEVEL_NORMAL)
LOG_MESSAGE_IMPL(Info, LOG_PRIORITY_INFO, LOG_LEVEL_NORMAL)
LOG_MESSAGE_IMPL(Debug, LOG_PRIORITY_DEBUG, LOG_LEVEL_DEBUG)
LOG_MESSAGE_IMPL(Verbose, LOG_PRIORITY_VERBOSE, LOG_LEVEL_VERBOSE)
LOG_MESSAGE_IMPL(VVerbose, LOG_PRIORITY_VVERBOSE, LOG_LEVEL_VVERBOSE)

#undef LOG_MESSAGE_IMPL





static bool logErrorForLogLevel(int logLevel, const char* tag, const char* fmt, va_list args) {
    if (sCurrentLogLevel >= logLevel) {
        logMessage(LOG_PRIORITY_ERROR, tag, fmt, args);
        return true;
    }
    return false;
}



#define LOG_ERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(logPriorityName, logLevel)                            \
bool logError##logPriorityName(const char* tag, const char* fmt, ...) {                            \
    if (sCurrentLogLevel >= logLevel) {                                                            \
        va_list args;                                                                              \
        va_start(args, fmt);                                                                       \
        bool logged = logErrorForLogLevel(logLevel, tag, fmt, args);                               \
        va_end(args);                                                                              \
        return logged;                                                                             \
    }                                                                                              \
    return false;                                                                                  \
}

LOG_ERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(Debug, LOG_LEVEL_DEBUG)
LOG_ERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(Verbose, LOG_LEVEL_VERBOSE)
LOG_ERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(VVerbose, LOG_LEVEL_VVERBOSE)
LOG_ERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(Private, LOG_LEVEL_DEBUG)

#undef LOG_ERROR_MESSAGE_FOR_LOG_LEVEL_IMPL





void logStrerrorMessage(int errnoCode, const char* tag, const char* fmt, va_list args) {
    if (errnoCode == 0) {
        logMessage(LOG_PRIORITY_ERROR, tag, fmt, args);
    } else {
        char strerror_buffer[STRERROR_BUFFER_SIZE];
        safe_strerror_r(errnoCode, strerror_buffer, sizeof(strerror_buffer));

        char current_message[LOGGER_ENTRY_MAX_PAYLOAD - strlen(strerror_buffer) - 1];
        vsnprintf(current_message, LOGGER_ENTRY_MAX_PAYLOAD, fmt, args);

        char new_message[strlen(current_message) + strlen(strerror_buffer) + 3];
        snprintf(new_message, sizeof(new_message), "%s: %s", current_message, strerror_buffer);

        logError(tag, new_message);
    }
}

void logStrerror(const char* tag, const char* fmt, ...) {
    int errnoCode = errno;
    va_list args;
    va_start(args, fmt);
    logStrerrorMessage(errnoCode, tag, fmt, args);
    va_end(args);
    errno = errnoCode;
}



#define LOG_STRERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(logPriorityName, logLevel)                         \
bool logStrerror##logPriorityName(const char* tag, const char* fmt, ...) {                         \
    if (sCurrentLogLevel >= logLevel) {                                                            \
        int errnoCode = errno;                                                                     \
        va_list args;                                                                              \
        va_start(args, fmt);                                                                       \
        logStrerrorMessage(errnoCode, tag, fmt, args);                                                  \
        va_end(args);                                                                              \
        errno = errnoCode;                                                                         \
        return true;                                                                               \
    }                                                                                              \
    return false;                                                                                  \
}

LOG_STRERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(Debug, LOG_LEVEL_DEBUG)
LOG_STRERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(Verbose, LOG_LEVEL_VERBOSE)
LOG_STRERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(VVerbose, LOG_LEVEL_VVERBOSE)
LOG_STRERROR_MESSAGE_FOR_LOG_LEVEL_IMPL(Private, LOG_LEVEL_DEBUG)

#undef LOG_STRERROR_MESSAGE_FOR_LOG_LEVEL_IMPL
