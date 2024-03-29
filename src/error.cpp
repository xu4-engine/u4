/*
 * $Id$
 */

#include <cstdio>
#include <cstdarg>
#include <cstdlib>

#include "error.h"

#if defined(_WIN32) || defined(__CYGWIN__)

/*
 * Windows: errors shown in message box
 */

#include <windows.h>

void errorFatal(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    MessageBox(NULL, buffer, "XU4 Error", MB_OK | MB_ICONERROR);

    exit(1);
}

void errorWarning(const char *fmt, ...) {
    char buffer[1000];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    MessageBox(NULL, buffer, "XU4 Warning", MB_OK | MB_ICONWARNING);
}

#elif defined(MACOSX)

/*
 * MacOS X: errors functios defined in objective-c code elsewhere.
 */

#elif GTK2_DIALOGS

/*
 * Linux/Unix with GTK2: errors shown in message box
 */

#include <gtk/gtk.h>

int need_gtk_init = 1;

void errorFatal(const char *fmt, ...) {
    char buffer[1000];
    va_list args;
    GtkWidget *dialog;

    if (need_gtk_init) {
        gtk_init(NULL, 0);
        need_gtk_init = 0;
    }

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "xu4: %s", buffer);
    va_end(args);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy (dialog);

    exit(1);
}

void errorWarning(const char *fmt, ...) {
    char buffer[1000];
    va_list args;
    GtkWidget *dialog;

    if (need_gtk_init) {
        gtk_init(NULL, 0);
        need_gtk_init = 0;
    }

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    dialog = gtk_message_dialog_new(NULL, 0, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK, "xu4: %s", buffer);
    va_end(args);

    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);

    while(gtk_events_pending())
        gtk_main_iteration();
}

#elif defined(ANDROID)

#include <setjmp.h>
#include <unistd.h>
#include "glv_activity.h"
extern struct android_app* gGlvApp;
extern jmp_buf gMainJump;

extern "C"
void errorFatal(const char *fmt, ...) {
    char message[512];
    va_list args;

    va_start(args, fmt);
    vsnprintf(message, sizeof(message), fmt, args);
    va_end(args);

    // Send to stderr for logging and show an AlertDialog.

    fprintf(stderr, "xu4 error: %s\n", message);

    ANativeActivity* activity = gGlvApp->activity;
    {
    JNIEnv* jni;
    jint result = activity->vm->AttachCurrentThread(&jni, NULL);
    if( result != JNI_ERR ) {
        jclass clazz = jni->GetObjectClass( activity->clazz );
        jmethodID method = jni->GetMethodID(clazz, "showAlert",
                                            "(Ljava/lang/String;)V");

        jstring jmessage = jni->NewStringUTF(message);
        jni->CallVoidMethod(activity->clazz, method, jmessage);
        jni->DeleteLocalRef(jmessage);

        activity->vm->DetachCurrentThread();
    }
    }

    // Must run event loop to display dialog.
    android_app_wait_destroy(gGlvApp);

    // Android apps restart on exit(), so use longjmp to go back to main.
    longjmp(gMainJump, 1);
}

void errorWarning(const char *fmt, ...) {
    va_list args;

    fputs("xu4 warning: ", stderr);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputs("\n", stderr);
}

#else

/*
 * no GUI error functions: errors go to standard error stream
 */

void errorFatal(const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "xu4: error: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");

    exit(1);
}

void errorWarning(const char *fmt, ...) {
    va_list args;

    fprintf(stderr, "xu4: warning: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}

#endif
