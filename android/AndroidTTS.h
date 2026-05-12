#include <jni.h>
#include <QString>

class AndroidTTS
{
	public:
		AndroidTTS();
		~AndroidTTS();
		void say(QString text);

	private:
		jobject m_ttsHelperObject;
};

