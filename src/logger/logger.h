#ifndef LOGGER_H
#define LOGGER_H

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>

/* Log Levels */

/**
 * The log level for `logMessage()` to log nothing.
 */
static const int LOG_LEVEL_OFF = 0;

/**
 * The log level for `logMessage()` to log error, warn and
 * info messages and stacktraces.
 */
static const int LOG_LEVEL_NORMAL = 1;

/**
 * The log level for `logMessage()` to log debug messages.
 */
static const int LOG_LEVEL_DEBUG = 2;

/**
 * The log level for `logMessage()` to log verbose messages.
 */
static const int LOG_LEVEL_VERBOSE = 3;

/**
 * The log level for `logMessage()` to log very verbose messages.
 */
static const int LOG_LEVEL_VVERBOSE = 4;


/**
 * The minimum log level.
 */
static const int MIN_LOG_LEVEL = LOG_LEVEL_OFF;

/**
 * The maximum log level.
 */
static const int MAX_LOG_LEVEL = LOG_LEVEL_VVERBOSE;

/**
 * The default log level.
 */
static const int DEFAULT_LOG_LEVEL = LOG_LEVEL_NORMAL;



/* Log Priorities */

/**
 * The log priority for `logMessage()` to use `logVPrintError()` if
 * `sCurrentLogLevel` is greater or equal to `LOG_LEVEL_NORMAL`.
 */
static const int LOG_PRIORITY_ERROR = 0;

/**
 * The log priority for `logMessage()` to use `logVPrintWarn()` if
 * `sCurrentLogLevel` is greater or equal to `LOG_LEVEL_NORMAL`.
 */
static const int LOG_PRIORITY_WARN = 1;

/**
 * The log priority for `logMessage()` to use `logVPrintInfo()` if
 * `sCurrentLogLevel` is greater or equal to `LOG_LEVEL_NORMAL`.
 */
static const int LOG_PRIORITY_INFO = 2;

/**
 * The log priority for `logMessage()` to use `logVPrintDebug()` if
 * `sCurrentLogLevel` is greater or equal to `LOG_LEVEL_DEBUG`.
 */
static const int LOG_PRIORITY_DEBUG = 3;

/**
 * The log priority for `logMessage()` to use `logVPrintVerbose()` if
 * `sCurrentLogLevel` is greater or equal to `LOG_LEVEL_VERBOSE`.
 */
static const int LOG_PRIORITY_VERBOSE = 4;

/**
 * The log priority for `logMessage()` to use `logVPrintVerbose()` if
 * `sCurrentLogLevel` is greater or equal to `LOG_LEVEL_VVERBOSE`.
 */
static const int LOG_PRIORITY_VVERBOSE = 5;



/*
 * The maximum size of the log entry tag.
 */
#define LOGGER_TAG_MAX_LENGTH 23 + 1

/*
 * The maximum size of the log entry payload that can be written to the logger. An attempt to write
 * more than this amount will result in a truncated log entry.
 *
 * The limit is 4068 but this includes log tag and log level prefix "D/" before log tag and ": "
 * suffix after it.
 *
 * #define LOGGER_ENTRY_MAX_PAYLOAD 4068
 * - https://cs.android.com/android/platform/superproject/+/android-13.0.0_r54:system/logging/liblog/include/log/log.h;l=66
 */
#define LOGGER_ENTRY_MAX_PAYLOAD 4068



/**
 * Set `sDefaultLogTag` by calling `setDefaultLogTag()` and set
 * `sLogTagPrefix` by calling `setLogTagPrefix()`.
 * The `sLogTagPrefix` will be set to `defaultLogTag` followed by a dot `.`.
 */
void setDefaultLogTagAndPrefix(const char* defaultLogTag);



/** Get logger `sDefaultLogTag`. */
const char* getDefaultLogTag();
/**
 * Set `sDefaultLogTag`.
 *
 * See also `getFullTag()`.
 */
void setDefaultLogTag(const char* defaultLogTag);



/** Get logger `sLogTagPrefix`. */
const char* getLogTagPrefix();

/**
 * Set `sLogTagPrefix`.
 *
 * See also `getFullTag()`.
 */
void setLogTagPrefix(const char* logTagPrefix);



/** Get logger `sCurrentLogLevel`. */
int getCurrentLogLevel();

/**
 * Set logger `sCurrentLogLevel` if a valid `logLevel` is passed,
 * otherwise to `DEFAULT_LOG_LEVEL`.
 *
 * @return Returns the new log level set.
 */
int setCurrentLogLevel(int logLevel);

/**
 * Returns `true` if `logLevel` passed is a valid log level, otherwise `DEFAULT_LOG_LEVEL`.
 */
int getLogLevelIfValidOtherwiseDefault(int logLevel);

/**
 * Returns `true` if `logLevel` passed is a valid log level, otherwise `false`.
 */
bool isLogLevelValid(int logLevel);



/** Get logger `sFormattedLogging`. */
bool isFormattedLoggingEnabled();

/**
 * Set `sFormattedLogging`.
 */
void setFormattedLogging(bool formattedLogging);





/**
 * Log a message with a specific priority.
 *
 * @param logPriority The priority to log the message with. Check `LOG_PRIORITY_*` constants
 *                    to know above which `sCurrentLogLevel` they will start logging.
 * @param tag The tag with which to log after passing it to `getFullTag()`.
 * @param fmt The format string for the message.
 * @param args The `va_list` of the arguments for the message.
 */
void logMessage(int logPriority, const char* tag, const char* fmt, va_list args);



/* Log a message with `LOG_PRIORITY_ERROR`. */
void logError(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_WARN`. */
void logWarn(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_INFO`. */
void logInfo(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_DEBUG`. */
void logDebug(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_VERBOSE`. */
void logVerbose(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_VVERBOSE`. */
void logVVerbose(const char* tag, const char* fmt, ...);



/* Log a message with `LOG_PRIORITY_ERROR` if current log level is `>=` `LOG_LEVEL_DEBUG`. */
bool logErrorDebug(const char* tag, const char* fmt, ...);


/* Log a message with `LOG_PRIORITY_ERROR` if current log level is `>=` `LOG_LEVEL_VERBOSE`. */
bool logErrorVerbose(const char* tag, const char* fmt, ...);


/* Log a message with `LOG_PRIORITY_ERROR` if current log level is `>=` `LOG_LEVEL_VERBOSE`. */
bool logErrorVVerbose(const char* tag, const char* fmt, ...);

/* Log a private message with `LOG_PRIORITY_ERROR`. This is equivalent to `logErrorDebug*()` methods. */
bool logErrorPrivate(const char* tag, const char* fmt, ...);





/* Log a message with `LOG_PRIORITY_ERROR` with `: strerror(errno)` appended to it.
 *
 * @param errnoCode The errno to use for `strerror(errno)`. If this
                    equals `0`, then `logMessage()` will be called
                    instead to log to message without any `strerror` appended.
 * @param tag The tag with which to log after passing it to `getFullTag()`.
 * @param fmt The format string for the message.
 * @param args The `va_list` of the arguments for the message.
 */
void logStrerrorMessage(int errnoCode, const char* tag, const char* fmt, va_list args);

/* Log a message with `LOG_PRIORITY_ERROR` with `: strerror(errno)` appended to it. */
void logStrerror(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_ERROR` with `: strerror(errno)` appended to it if current log level is `>=` `LOG_LEVEL_DEBUG`. */
bool logStrerrorDebug(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_ERROR` with `: strerror(errno)` appended to it if current log level is `>=` `LOG_LEVEL_VERBOSE`. */
bool logStrerrorVerbose(const char* tag, const char* fmt, ...);

/* Log a message with `LOG_PRIORITY_ERROR` with `: strerror(errno)` appended to it if current log level is `>=` `LOG_LEVEL_VERBOSE`. */
bool logStrerrorVVerbose(const char* tag, const char* fmt, ...);

/* Log a private message with `LOG_PRIORITY_ERROR` with `: strerror(errno)` appended to it. This is equivalent to `logStrerrorDebug*()` methods. */
bool logStrerrorPrivate(const char* tag, const char* fmt, ...);

#endif // LOGGER_H
