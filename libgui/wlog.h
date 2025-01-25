#pragma once

#include <cstdarg>
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#ifndef STDOUT_FILENO
#define STDOUT_FILENO _fileno(stdout)
#endif
#define dup _dup
#define dup2 _dup2
#else
#include <unistd.h>
#include <fcntl.h> // Required for open() and file flags on POSIX
#endif

// WLOG_DISABLE_TRACE
// WLOG_DISABLE_DEBUG
// WLOG_DISABLE_INFO
// WLOG_DISABLE_WARN
// WLOG_DISABLE_ERROR
// WLOG_DISABLE_FATAL

// WLOG_MIRROR_CONSOLE

namespace wlog {
    enum wlogSeverity {
        WLOG_TRACE,
        WLOG_DEBUG,
        WLOG_INFO,
        WLOG_WARN,
        WLOG_ERROR,
        WLOG_FATAL
    };

    inline int wlog_stdout_restore;

    /**
     * @brief Redirects stdout to wlog
     * @attention Don't forget to restore_printf later!
     */
    inline void redirect_printf() {
        // Clear the logs
        std::ofstream ofs;
        ofs.open("wlog.txt", std::ofstream::out | std::ofstream::trunc);
        ofs.close();

        fflush(stdout);

        wlog_stdout_restore = dup(STDOUT_FILENO);
        const int redirect = open("wlog.txt", O_WRONLY | O_CREAT | O_TRUNC
#ifdef _WIN32
            , _S_IREAD | _S_IWRITE
#else
            , 0644
#endif
        );

        if (redirect != -1) {
            dup2(redirect, STDOUT_FILENO);
            close(redirect);
        }
    }

    /**
     * @brief Removes stdout from wlog and restores it
     */
    inline void restore_printf() {
        fflush(stdout);

        if (wlog_stdout_restore != -1) {
            dup2(wlog_stdout_restore, STDOUT_FILENO);
            close(wlog_stdout_restore);
        }
    }

    /**
     * @brief Mirrors incoming wlog chars to stdcerr
     */
    inline void mirror_console(const char* cout_log) {
    #ifdef WLOG_MIRROR_CONSOLE
        std::cerr << cout_log << std::endl;
    #endif
    }

    /**
     * @brief helper for disabled wlog severities
     */
    constexpr bool wlog_severity_disable(const wlogSeverity severity) {
    #ifdef WLOG_DISABLE_TRACE
        if (severity == WLOG_TRACE) return true;
    #endif

    #ifdef WLOG_DISABLE_DEBUG
        if (severity == WLOG_DEBUG) return true;
    #endif

    #ifdef WLOG_DISABLE_INFO
        if (severity == WLOG_INFO) return true;
    #endif

    #ifdef WLOG_DISABLE_WARN
        if (severity == WLOG_WARN) return true;
    #endif

    #ifdef WLOG_DISABLE_ERROR
        if (severity == WLOG_ERROR) return true;
    #endif

    #ifdef WLOG_DISABLE_FATAL
        if (severity == WLOG_FATAL) return true;
    #endif

        return false;
    }

    /**
     * @brief Converts wlog severity to string
     */
    constexpr const char* wlog_severity_str(const wlogSeverity severity) {
        switch (severity) {
            case WLOG_TRACE: return "TRACE";
            case WLOG_DEBUG: return "DEBUG";
            case WLOG_INFO:  return "INFO";
            case WLOG_WARN:  return "WARN";
            case WLOG_ERROR: return "ERROR";
            case WLOG_FATAL: return "FATAL";
        }

        return "UNKNOWN";
    }

    /// @param log string literal you want to log
    /// @param log_severity severity of the log
    /// @param ... printf args
    /// @attention Use WLOG_DISABLE_* to disable a severity from logging
    inline void logf(const wlogSeverity log_severity, const char* log, ...) {
        if (wlog_severity_disable(log_severity)) return;

        va_list args;
        va_start(args, log);

        char formatted[strlen(log) + 250];
        vsprintf(formatted, log, args);
        printf("[%s] %s\n", wlog_severity_str(log_severity), formatted);
        mirror_console(formatted);

        va_end(args);
    }

    /// @param log string literal you want to log
    /// @param log_severity severity of the log
    /// @param file use __FILE__
    /// @param line use __LINE__
    /// @param ... printf args
    /// @attention Use WLOG_DISABLE_* to disable a severity from logging
    inline void logfx(const wlogSeverity log_severity, const char* file, const int line, const char* log, ...) {
        if (wlog_severity_disable(log_severity)) return;

        va_list args;
        va_start(args, log);

        char formatted[strlen(log) + 250];
        vsprintf(formatted, log, args);
        printf("[%s : %d | %s] %s\n", file, line, wlog_severity_str(log_severity), formatted);
        mirror_console(formatted);

        va_end(args);
    }

    /// @param log string literal you want to log
    /// @param log_severity severity of the log
    /// @attention Use WLOG_DISABLE_* to disable a severity from logging
    inline void log(const wlogSeverity log_severity, const char* log) {
        if (wlog_severity_disable(log_severity)) return;
        printf("[%s] %s\n", wlog_severity_str(log_severity), log);
        mirror_console(log);
    }

    /// @param log string literal you want to log
    /// @param log_severity severity of the log
    /// @param file use __FILE__
    /// @param line use __LINE__
    /// @param ... printf args
    /// @attention Use WLOG_DISABLE_* to disable a severity from logging
    inline void logx(const wlogSeverity log_severity, const char* file, const int line, const char* log) {
        if (wlog_severity_disable(log_severity)) return;
        printf("[%s : %d | %s] %s\n", file, line, wlog_severity_str(log_severity), log);
        mirror_console(log);
    }

}
