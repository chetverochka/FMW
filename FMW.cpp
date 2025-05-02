#include "FMW.h"
#include "cocos2d.h"

FMW::AudioPlayer* FMW::AudioPlayer::m_instance = nullptr;

FMW::AudioPlayer::AudioPlayer() {
    m_idCounter = 0;

    FMOD_RESULT fmResult = FMOD::System_Create(&m_fmSystem);
    if (fmResult != FMOD_OK) {
        CCLOG("FMOD error! (%d) %s", fmResult, FMOD_ErrorString(fmResult));
    }

    fmResult = m_fmSystem->init(512, FMOD_INIT_NORMAL, nullptr);
    if (fmResult != FMOD_OK) {
        CCLOG("FMOD init error! (%d) %s", fmResult, FMOD_ErrorString(fmResult));
    }
}

std::string FMW::AudioPlayer::fmodSoundPath(std::string path)
{
    bool isAbsolutePath = cocos2d::FileUtils::getInstance()->isAbsolutePath(path);
    if (!isAbsolutePath) {
        // file:///android_asset/ + path wrapper for android
        if (cocos2d::Application::getInstance()->getTargetPlatform() == cocos2d::ApplicationBase::Platform::Android) {
            return "file:///android_asset/" + path;
        }
        else {
            return cocos2d::FileUtils::getInstance()->getDefaultResourceRootPath() + path;
        }
    }
    return path;
}

FMW::Sound* FMW::AudioPlayer::createSound(std::string songPath, FMOD_MODE fmMode) {
    FMW::Sound* fmwSound = new FMW::Sound();
    FMOD_RESULT fmResult = m_fmSystem->createSound(FMOD_SOUND_PATH(songPath), fmMode, nullptr, &fmwSound->m_fmSound);
    if (fmResult != FMOD_OK) {
        CCLOG("FMOD sound load error! (%d) %s", fmResult, FMOD_ErrorString(fmResult));
        AX_SAFE_DELETE(fmwSound);
        return nullptr;
    }
#ifdef FMW_DEBUG_ENABLED
    CCLOG("FMW::AudioPlayer::createSound: sound created successfully! ID = %d;", m_idCounter);
#endif // FMW_DEBUG_ENABLED

    fmwSound->m_fmSystem = m_fmSystem;
    fmwSound->m_id = m_idCounter;
    m_idCounter++;
    m_soundsMap[fmwSound->m_id] = fmwSound;
    m_sounds.push_back(fmwSound);
    return fmwSound;
}

void FMW::AudioPlayer::destroySound(FMW::Sound* target) {
    for (int i = 0; i < m_sounds.size(); i++) {
        if (m_sounds[i] == target) {
            m_sounds.erase(m_sounds.begin() + i);
        }
    }
    delete target;
    //AX_SAFE_DELETE(target);
}

void FMW::AudioPlayer::update() {
    if (m_fmSystem) m_fmSystem->update();
}

FMW::Sound::Sound() {
    m_userPaused = m_enginePaused = m_isDSPFFTinitialised = m_loopEnabled = false;
}

void FMW::Sound::play() {
    FMOD_RESULT result = m_fmSystem->playSound(m_fmSound, nullptr, false, &m_fmChannel);
    if (result != FMOD_OK) {
        CCLOG("FMOD play error! (%d) %s", result, FMOD_ErrorString(result));

    }
    if (!m_isDSPFFTinitialised) initDSPFFT(m_fmSystem);
}

void FMW::Sound::stop() {
    m_fmChannel->stop();
}

void FMW::Sound::pause() {
    if (!m_userPaused) {
        m_userPaused = true;
        m_fmChannel->setPaused(true);
    }
}

void FMW::Sound::resume() {
    if (m_userPaused || !m_enginePaused) {
        m_userPaused = false;
        if (!m_enginePaused) m_fmChannel->setPaused(false);
    }
}

bool FMW::Sound::isPaused() {
    return m_userPaused;
}

bool FMW::Sound::isEnginePaused() {
    return m_enginePaused;
}

void FMW::Sound::setEnginePaused(bool value) {
    if (m_enginePaused != value) {
        m_enginePaused = value;

        if (value) {
            pause();
        }
        else {
            resume();
        }
    }
}

void FMW::AudioPlayer::forcePause(FMW::Sound* target)
{
    target->setEnginePaused(true);
}

void FMW::AudioPlayer::forceResume(FMW::Sound* target)
{
    target->setEnginePaused(false);
}

void FMW::AudioPlayer::forcePauseAll() {
    for (Sound* target : m_sounds)
        forcePause(target);
}

void FMW::AudioPlayer::forceResumeAll() {
    for (Sound* target : m_sounds)
        forceResume(target);
}

void FMW::Sound::initDSPFFT(FMOD::System* system) {
    bool successInit = true;
    FMOD_RESULT result = system->createDSPByType(FMOD_DSP_TYPE_FFT, &m_fmDSPFFT);

    if (result == FMOD_OK) {
        
        result = system->createDSPByType(FMOD_DSP_TYPE_FFT, &m_fmDSPFFT);
        if (result == FMOD_OK)
        {
            m_fmChannel->addDSP(0, m_fmDSPFFT);
        }
        else
        {
            CCLOG("FMOD createDSP error! (%d) %s", result, FMOD_ErrorString(result));
            successInit = false;
        }
    }
    else
    {
        successInit = false;
        CCLOG("FMOD createDSP error! (%d) %s", result, FMOD_ErrorString(result));
    }
    if (successInit) m_isDSPFFTinitialised = true;
}

std::vector<float> FMW::Sound::getFFTSpectrum(SpectrumChannel channel) {
    FMOD::DSP* fft = m_fmDSPFFT;
    std::vector<float> spectrum = {};
    if (fft && m_isDSPFFTinitialised)
    {
        FMOD_DSP_PARAMETER_FFT* fftData = nullptr;
        FMOD_RESULT result = fft->getParameterData(FMOD_DSP_FFT_SPECTRUMDATA, (void**)&fftData, nullptr, nullptr, 0);
        if (result == FMOD_OK && fftData && fftData->length > 0)
        {
            unsigned short binCount = fftData->length;
            unsigned short maxChannelIndex = fftData->numchannels - 1;

#ifdef FMW_DEBUG_ENABLED
            //CCLOG("FMW::Sound::getFFTSpectrum: requested channel = %d; max channels = %d", channel, maxChannelIndex + 1);
#endif

            for (int i = 0; i < binCount; ++i)
            {
                float leftMagnitude = fftData->spectrum[0][i];
                float rightMagnitude = fftData->spectrum[1][i];
                float stereoMagnitude = leftMagnitude + rightMagnitude / 2.f;

                if (channel == SpectrumChannel::kMonoLeft)
                    spectrum.push_back(leftMagnitude);
                else if (channel == SpectrumChannel::kMonoRight)
                    spectrum.push_back(rightMagnitude);
                else if (channel == SpectrumChannel::kStereo)
                    spectrum.push_back(stereoMagnitude);
            }
        }
    }

    return spectrum;
}

float FMW::Sound::getAverageSpectrumAmplitude(SpectrumChannel channel) {
    std::vector<float> spectrum = getFFTSpectrum(channel);
    if (!spectrum.empty())
    {
        float sum = 0.f;
        for (float& n : spectrum) sum += n;
        return (float)sum / (float)spectrum.size();
    }
    return 0.f;
}

void FMW::Sound::update() {
    if (m_isDSPFFTinitialised) {

    }
}
