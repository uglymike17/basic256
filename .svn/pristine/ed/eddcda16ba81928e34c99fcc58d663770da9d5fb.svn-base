#include "AndroidTTS.h"
#include <QDebug>

// thanks to the great example from http://community.kde.org/Necessitas/JNI
// 2014-01-01 jmr

static JavaVM* s_javaVM = 0;
static jclass s_ttsHelperClassID = 0;
static jmethodID s_ttsHelperConstructorMethodID=0;
static jmethodID s_ttsHelperSayMethodID=0;


AndroidTTS::AndroidTTS()
{
	JNIEnv* env;
	// Qt is running in a different thread than Java UI, so you always Java VM *MUST* be attached to current thread
	if (s_javaVM->AttachCurrentThread(&env, NULL)<0) {
		qCritical()<<"JNI AttachCurrentThread failed";
		return;
	}

	// Create a new instance of AndroidTTSHelper
    m_ttsHelperObject = env->NewGlobalRef(env->NewObject(s_ttsHelperClassID, s_ttsHelperConstructorMethodID));
    if (!m_ttsHelperObject) {
        qCritical()<<"JNI Can't create the AndroidTTSHelper Object";
		return;
	}

	// Don't forget to detach from current thread
	s_javaVM->DetachCurrentThread();
}

AndroidTTS::~AndroidTTS()
{
}

void AndroidTTS::say(QString text) {
    if (!m_ttsHelperObject) return;
 
    JNIEnv* env;
    if (s_javaVM->AttachCurrentThread(&env, NULL)<0)
    {
        qCritical()<<"JNI AttachCurrentThread failed";
        return;
    }
    jstring str = env->NewString(reinterpret_cast<const jchar*>(text.constData()), text.length());
    jboolean res = env->CallBooleanMethod(m_ttsHelperObject, s_ttsHelperSayMethodID, str);
    (void) res;
    env->DeleteLocalRef(str);
    s_javaVM->DetachCurrentThread();
    return;
}


// this method is called immediately after the module is load
JNIEXPORT jint JNI_OnLoad(JavaVM* vm, void* /*reserved*/)
{
	JNIEnv* env;
	if (vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) != JNI_OK) {
		qCritical()<<"Can't get the JNI enviroument";
		return -1;
	}

	s_javaVM = vm;
	// search for our class
    jclass clazz=env->FindClass("org/basic256/AndroidTTSHelper");
	if (!clazz) {
        qCritical()<<"Can't find AndroidTTSHelper class";
		return -1;
	}
	// keep a global reference to it
    s_ttsHelperClassID = (jclass)env->NewGlobalRef(clazz);

	// search for its contructor
    s_ttsHelperConstructorMethodID = env->GetMethodID(s_ttsHelperClassID, "<init>", "()V");
    if (!s_ttsHelperConstructorMethodID) {
        qCritical()<<"Can't find AndroidTTSHelper class contructor";
		return -1;
	}

	// search for say method
    s_ttsHelperSayMethodID = env->GetMethodID(s_ttsHelperClassID, "say", "(Ljava/lang/String;)V");
    if (!s_ttsHelperSayMethodID) {
        qCritical()<<"Can't find AndroidTTSHelper say method";
		return -1;
	}

	return JNI_VERSION_1_6;
} 
 
