#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/syscall.h>

#include <termux/termux_core__nos__c/v1/android/shell/command/environment/AndroidShellEnvironment.h>
#include <termux/termux_core__nos__c/v1/data/DataUtils.h>
#include <termux/termux_core__nos__c/v1/logger/Logger.h>
#include <termux/termux_core__nos__c/v1/termux/file/TermuxFile.h>
#include <termux/termux_core__nos__c/v1/termux/shell/command/environment/TermuxShellEnvironment.h>
#include <termux/termux_core__nos__c/v1/unix/os/selinux/UnixSeLinuxUtils.h>

#include <termux/termux_exec__nos__c/v1/TermuxExecLibraryConfig.h>
#include <termux/termux_exec__nos__c/v1/termux/api/termux_exec/service/ld_preload/TermuxExecLDPreload.h>
#include <termux/termux_exec__nos__c/v1/termux/shell/command/environment/termux_exec/TermuxExecShellEnvironment.h>

static const char* LOG_TAG = "ld-preload";

static int sSystemLinkerExecShouldEnable = -1;



int shouldEnableSystemLinkerExec() {
     if (sSystemLinkerExecShouldEnable == 0 || sSystemLinkerExecShouldEnable == 1) {
        return sSystemLinkerExecShouldEnable;
    }

    bool isRunningTests = libtermux_exec__nos__c__getIsRunningTests();

    int systemLinkerExecMode = termuxExec_systemLinkerExec_mode_get();
    if (!isRunningTests) {
        logErrorVVerbose(LOG_TAG, "system_linker_exec_mode: '%d'", systemLinkerExecMode);
    }

    int systemLinkerExecShouldEnable = 1;
    if (systemLinkerExecMode == 0) { // disable
        systemLinkerExecShouldEnable = 1; // disable

    } else if (systemLinkerExecMode == 2 || systemLinkerExecMode == 3) { // force or force_all
        int androidBuildVersionSdk = android_buildVersionSdk_get();
        if (!isRunningTests) {
            logErrorVVerbose(LOG_TAG, "android_build_version_sdk: '%d'", androidBuildVersionSdk);
        }

        bool systemLinkerExecAvailable = false;
        // If running on Android `>= 10`.
        systemLinkerExecAvailable = androidBuildVersionSdk >= 29;
        if (!isRunningTests) {
            logErrorVVerbose(LOG_TAG, "system_linker_exec_available: '%d'", systemLinkerExecAvailable);
        }

        if (systemLinkerExecAvailable) {
            if (systemLinkerExecMode == 2) { // force
                uid_t uid = geteuid();
                if (uid == 0 || uid == 2000) {
                    logErrorVVerbose(LOG_TAG, "uid_to_exempt: '%d'", uid);
                } else {
                    systemLinkerExecShouldEnable = 0; // enable
                }
            } else if (systemLinkerExecMode == 3) { // force_all
                systemLinkerExecShouldEnable = 0; // enable
            }
        }

    } else { // enable
        if (systemLinkerExecMode != 1) {
            logErrorDebug(LOG_TAG, "Warning: Ignoring invalid system_linker_exec_mode value and using '1' instead");
        }

        bool appDataFileExecExempted = false;

        int androidBuildVersionSdk = android_buildVersionSdk_get();
        if (!isRunningTests) {
            logErrorVVerbose(LOG_TAG, "android_build_version_sdk: '%d'", androidBuildVersionSdk);
        }

        // If running on Android `>= 10`.
        if (androidBuildVersionSdk >= 29) {
            // If running as root or shell user, then the process will
            // be assigned a different process context like
            // `PROCESS_CONTEXT__AOSP_SU` (`u:r:su:s0`),
            // `PROCESS_CONTEXT__KERNEL_SU` (`u:r:ksu:s0`),
            // `PROCESS_CONTEXT__MAGISK_SU` (`u:r:magisk:s0`) or
            // `PROCESS_CONTEXT__SHELL` (`u:r:shell:s0`), which will
            // not be the same as the one that's exported in
            // `ENV__TERMUX__SE_PROCESS_CONTEXT`, so we need to check
            // effective uid equals `0` or `2000` instead. Moreover,
            // other su providers may have different contexts, so we
            // cannot just check AOSP, MAGISK or KERNEL SU contexts.
            // - https://man7.org/linux/man-pages/man2/getuid.2.html
            // However, if the su/shell user is used to drop
            // privileges/capabilities and uid to an unprivileged user,
            // like to Termux uid, only the uid may be changed to
            // Termux uid but the process context may not be switched
            // to Termux app's normal context, like one of the
            // `u:r:untrusted_app*` contexts, and so the process will
            // not be exempted from system linker exec unless those
            // different process contexts are also explicitly exempted,
            // as uid check will not be enough.
            // For example, for the `adb shell run-as com.termux`
            // command, the process context is switched to
            // `u:r:runas_app:*` and only uid is changed to Termux uid.
            // The `runas_app` context also allows execution of
            // `app_data_file`, even if app uses `targetSdkVersion` `>= 28`
            // and normally launches with `u:r:untrusted_app:*`
            // instead of `u:r:untrusted_app_25:*` and `u:r:untrusted_app_27:*`.
            // - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:system/core/run-as/run-as.cpp;l=241-244
            // - https://cs.android.com/android/platform/superproject/+/android-16.0.0_r1:system/sepolicy/private/runas_app.te;l=8
            // For `su` commands, the original root process context
            // may be preserved.
            uid_t uid = geteuid();
            if (uid == 0 || uid == 2000) {
                logErrorVVerbose(LOG_TAG, "uid_to_exempt: '%d'", uid);
                appDataFileExecExempted = true;
            } else {
                char seProcessContext[80];
                bool getSeProcessContextSuccess = false;

                if (getSeProcessContextFromEnv(LOG_TAG, ENV__TERMUX__SE_PROCESS_CONTEXT,
                        seProcessContext, sizeof(seProcessContext))) {
                    if (!isRunningTests) {
                        logErrorVVerbose(LOG_TAG, "se_process_context_from_env: '%s'", seProcessContext);
                    }
                    getSeProcessContextSuccess = true;
                } else if (getSeProcessContextFromFile(LOG_TAG,
                        seProcessContext, sizeof(seProcessContext))) {
                    if (!isRunningTests) {
                        logErrorVVerbose(LOG_TAG, "se_process_context_from_file: '%s'", seProcessContext);
                    }
                    getSeProcessContextSuccess = true;
                }

                if (getSeProcessContextSuccess) {
                    // Listed in order of likely higher use.
                    appDataFileExecExempted =
                        stringStartsWith(seProcessContext, PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_27) ||
                        stringStartsWith(seProcessContext, PROCESS_CONTEXT_PREFIX__UNTRUSTED_APP_25) ||
                        stringEquals(seProcessContext, PROCESS_CONTEXT__SHELL) ||
                        stringStartsWith(seProcessContext, PROCESS_CONTEXT_PREFIX__RUNAS_APP) ||
                        stringEquals(seProcessContext, PROCESS_CONTEXT__MAGISK_SU) ||
                        stringEquals(seProcessContext, PROCESS_CONTEXT__KERNEL_SU) ||
                        stringEquals(seProcessContext, PROCESS_CONTEXT__AOSP_SU);
                } else {
                    // If even '/proc/self/attr/current' is not accessible,
                    // then SeLinux may not be supported on the device.
                    logErrorVVerbose(LOG_TAG, "se_process_context_available: '0'");
                    appDataFileExecExempted = true;
                }
            }

            if (!isRunningTests) {
                logErrorVVerbose(LOG_TAG, "app_data_file_exec_exempted: '%d'", appDataFileExecExempted);
            }

            if (!appDataFileExecExempted) {
                systemLinkerExecShouldEnable = 0; // enable
            }
        }
    }

    sSystemLinkerExecShouldEnable = systemLinkerExecShouldEnable;

    if (!isRunningTests) {
        logErrorVVerbose(LOG_TAG, "system_linker_exec_should_enable: '%d'",
            sSystemLinkerExecShouldEnable == 0 ? true : false);
    }

    return sSystemLinkerExecShouldEnable;
}

int shouldEnableSystemLinkerExecForFile(const char *executablePath) {
    int systemLinkerExecResult = shouldEnableSystemLinkerExec();
    // If error or disabled, then just return.
    if (systemLinkerExecResult != 0) {
        return systemLinkerExecResult;
    }

    bool isRunningTests = libtermux_exec__nos__c__getIsRunningTests();

    int isExecutableUnderTermuxAppDataDir = termuxApp_dataDir_isPathUnder(LOG_TAG,
        executablePath, NULL, NULL);
    if (isExecutableUnderTermuxAppDataDir < 0) {
        return -1;
    }

    if (!isRunningTests) {
        logErrorVVerbose(LOG_TAG, "is_exe_under_termux_app_data_dir: '%d'",
            isExecutableUnderTermuxAppDataDir == 0 ? true : false);
    }

    bool shouldEnableSystemLinkerExec = isExecutableUnderTermuxAppDataDir == 0;

    if (!isRunningTests) {
        logErrorVVerbose(LOG_TAG, "system_linker_exec_should_enable_for_file: '%d'",
            shouldEnableSystemLinkerExec);
    }

    return shouldEnableSystemLinkerExec ? 0 : 1;
}
