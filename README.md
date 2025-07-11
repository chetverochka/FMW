# FMW
FMW (FMOD wrapper) my audio player wrapper, compatible with cocos2d-x v4.x / axmol v2.x

## First of all, you need to put the following line in the main loop of axmol:
```
FMW::AudioPlayer::getInstance()->update();
```
Or add the following piece of code inside AppDelegate.cpp:
```
// FMOD update
Director::getInstance()->getScheduler()->schedule([&](float dt){
    FMW::AudioPlayer::getInstance()->update();
}, this, 0, false, "FMW_Update");
```

## and in AppDelegate.cpp:
```
// This function will be called when the app is inactive. Note, when receiving a phone call it is invoked.
void AppDelegate::applicationDidEnterBackground()
{
    Director::getInstance()->stopAnimation();
    FMW::AudioPlayer::getInstance()->forcePauseAll();

#if USE_AUDIO_ENGINE
    AudioEngine::pauseAll();
#endif
}

// this function will be called when the app is active again
void AppDelegate::applicationWillEnterForeground()
{
    Director::getInstance()->startAnimation();
    FMW::AudioPlayer::getInstance()->forceResumeAll();

#if USE_AUDIO_ENGINE
    AudioEngine::resumeAll();
#endif
}
```

## Create sound with FMW::AudioPlayer:
```
FMW::Sound* sound = FMW::AudioPlayer::getInstance()->createSound("music.mp3");
```
