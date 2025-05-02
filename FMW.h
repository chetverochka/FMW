#ifndef __FMW_H__
#define __FMW_H__

#include "cocos2d.h"
#include "fmod.hpp"
#include "fmod_errors.h"
#include <map>
#include <vector>
// FMOD Wrapper (FMW)

#define FMOD_SOUND_PATH(__PATH__) FMW::AudioPlayer::getInstance()->fmodSoundPath(__PATH__).c_str()
//#define FMW_DEBUG_ENABLED 1

namespace FMW {
    enum class SpectrumChannel { kMonoLeft, kMonoRight, kStereo };

    class AudioPlayer;

    class Sound {
    protected:
        FMOD::Channel* m_fmChannel;
        FMOD::Sound* m_fmSound;
        FMOD::System* m_fmSystem;
        FMOD::DSP* m_fmDSPFFT;

        bool m_userPaused;
        bool m_enginePaused; // force pause
        bool m_isDSPFFTinitialised;
        bool m_loopEnabled;
        int  m_id;
        void setEnginePaused(bool value);
        void update();
    public:
        Sound();
        ~Sound() {
            stop();
            m_fmSound->release();
        }
        void initDSPFFT(FMOD::System* system);
        void play();
        void stop();
        void pause();
        void resume();
        bool isPaused();
        bool isEnginePaused();
        void setVolume(float value) { m_fmChannel->setVolume(value); }
        float getVolume() { float volume = 0.f; m_fmChannel->getVolume(&volume); return volume; }
        void setPitch(float value) { m_fmChannel->setPitch(value); }
        float getPitch() { float pitch; m_fmChannel->getPitch(&pitch); return pitch; }
        std::vector<float> getFFTSpectrum(SpectrumChannel channel = SpectrumChannel::kStereo);
        float getAverageSpectrumAmplitude(SpectrumChannel channel = SpectrumChannel::kStereo);
        void setCurrentTime(float timeInSeconds) {
            unsigned int ms = timeInSeconds * 1000.f;
            m_fmChannel->setPosition(ms, FMOD_TIMEUNIT_MS);
        }
        float getCurrentTime() {
            unsigned int ms = 0;
            m_fmChannel->getPosition(&ms, FMOD_TIMEUNIT_MS);
            return (float)ms / 1000.f;
        }
        void setLoopEnabled(bool value) {
            m_loopEnabled = value;
            if (value)
            {
                m_fmChannel->setMode(FMOD_LOOP_NORMAL);
                m_fmChannel->setLoopCount(-1);
            }
            else
            {
                m_fmChannel->setMode(FMOD_LOOP_OFF);
                m_fmChannel->setLoopCount(1);
            }
        }
        bool isLoopEnabled() { return m_loopEnabled; }
        friend class AudioPlayer;
    };

    class AudioPlayer {
    protected:
        static AudioPlayer* m_instance;
        AudioPlayer();

        FMOD::System* m_fmSystem;
        int m_idCounter;
        std::map<int, FMW::Sound*> m_soundsMap;
        std::vector<FMW::Sound*> m_sounds;
    public:
        static AudioPlayer* getInstance() { if (!m_instance) m_instance = new AudioPlayer(); return m_instance; }
        FMOD::System* getFMODSystem() { return m_fmSystem; }

        std::string fmodSoundPath(std::string path);
        FMW::Sound* createSound(std::string songPath, FMOD_MODE fmMode = FMOD_DEFAULT);
        void destroySound(FMW::Sound* target);
        void update();

        void forcePause(FMW::Sound* target);
        void forceResume(FMW::Sound* target);
        void forcePauseAll();
        void forceResumeAll();
    };
}

#endif // !__FMW_H__
