//
//  ViewController+AudioController.h
//  tone_barrier
//
//  An Objective-C class extension that declares properties related to
//  audio configuration and playback
//  used by the ViewController class
//
//  Created by Xcode Developer on 8/3/22.
//

#import "ViewController.h"


NS_ASSUME_NONNULL_BEGIN

static AVAudioSourceNodeRenderBlock (^AudioRendererRef)(void) = ^ AVAudioSourceNodeRenderBlock {
    __block Float32 theta = 0.f;
    const Float32 frequency = 440.f;
    const Float32 sampleRate = 48000.f;
    const Float32 amplitude = 0.25f;
    const Float32 M_PI_SQR = 2.f * M_PI;
    
    return ^OSStatus(BOOL * _Nonnull isSilence, const AudioTimeStamp * _Nonnull timestamp, AVAudioFrameCount frameCount, AudioBufferList * _Nonnull outputData) {
                Float32 theta_increment = M_PI_SQR * frequency / sampleRate;
                Float32 * buffer = (Float32 *)outputData->mBuffers[0].mData;
                for (AVAudioFrameCount frame = 0; frame < frameCount; frame++)
                {
                    buffer[frame] = sin(theta) * amplitude;
                    theta += theta_increment;
                    !(theta > M_PI_SQR) ?: (theta -= M_PI_SQR);
                }
                return (OSStatus)noErr;
            };
};

static AVAudioSourceNode * (^AudioSourceRef)(AVAudioSourceNodeRenderBlock) = ^ AVAudioSourceNode * (AVAudioSourceNodeRenderBlock audio_renderer) {
    AVAudioSourceNode * source_node = [[AVAudioSourceNode alloc] initWithRenderBlock:audio_renderer];

    return source_node;
};

//static NSArray<typeof(void(^)(NSNotification *))> * (^observe_audio_session_notifications)...

// 1. create an array in the first block that takes an NSNotificationCenter as its only parameter
// 2. return a callback block that takes a notification observer block and stores a new block similar to id(^object)(void) that invokes it when callerd
// 3.

typedef typeof(void(^(^)(NSNotificationCenter *))(id(^(^(^)(NSNotificationCenter *))(NSNotification *, void(^)(NSNotification *)))(void)))audio_session_notification_observer;
static AVAudioSession * (^AudioSessionRef)(NSArray<typeof(void(^)(NSNotification *))> *) = ^ AVAudioSession * (NSArray<typeof(void(^)(NSNotification *))> * audio_session_notification_observers) {
    AVAudioSession * session = [AVAudioSession sharedInstance];
    
    @try {
        __autoreleasing NSError *error = nil;
        [session setCategory:AVAudioSessionCategoryPlayAndRecord mode:AVAudioSessionModeDefault options:AVAudioSessionCategoryOptionDefaultToSpeaker error:&error];
        [session setSupportsMultichannelContent:TRUE  error:&error];
        [session setPreferredInputNumberOfChannels:2  error:&error];
        [session setPreferredOutputNumberOfChannels:2 error:&error];
        [session setPrefersNoInterruptionsFromSystemAlerts:TRUE error:&error];
        
        typedef typeof(void (^(^(^)(NSNotificationCenter * _Nonnull __strong))(NSNotification * _Nonnull __strong, typeof(void (^)(NSNotification * _Nonnull __strong))  _Nonnull __strong))(void)) notification_observer_ref;
        notification_observer_ref notification_observer = ^ (NSNotificationCenter * notification_center) {
            return ^ (NSNotification * observed_notification, typeof(void(^)(NSNotification *))notification_handler) {
                return ^{
                    [notification_center addObserverForName:observed_notification.name object:observed_notification.object queue:[NSOperationQueue mainQueue] usingBlock:notification_handler];
                };
            };
        };
        
        void (^observe_audio_session_interruption_notification)(void) = notification_observer([NSNotificationCenter defaultCenter])([NSNotification notificationWithName:(NSNotificationName)AVAudioSessionInterruptionNotification object:session], ^(NSNotification * notification) {
            UInt8 theInterruptionType = [[notification.userInfo valueForKey:AVAudioSessionInterruptionTypeKey] intValue];
            NSLog(@"Session interrupted > --- %s ---\n", theInterruptionType == AVAudioSessionInterruptionTypeBegan ? "Begin Interruption" : "End Interruption");
            static BOOL _isSessionInterrupted;
            switch (theInterruptionType) {
                case AVAudioSessionInterruptionTypeBegan: {
                    //                _isSessionInterrupted = [_engine isRunning];
                    //                !(_isSessionInterrupted) ?: [self toggleAudioEngineRunningStatus:((ViewController *)[[[[UIApplication sharedApplication] delegate] window] rootViewController]).playPauseButton];
                    break;
                }
                    
                case AVAudioSessionInterruptionTypeEnded: {
                    //                !(_isSessionInterrupted) ?: [self toggleAudioEngineRunningStatus:((ViewController *)[[[[UIApplication sharedApplication] delegate] window] rootViewController]).playPauseButton];
                    //                _isSessionInterrupted = FALSE;
                    break;
                }
                    
                default:
                    break;
            }
        });
        
        void (^observe_audio_route_change_notification)(void) = notification_observer([NSNotificationCenter defaultCenter])([NSNotification notificationWithName:(NSNotificationName)AVAudioSessionRouteChangeNotification object:session], ^(NSNotification * notification) {
            UInt8 reasonValue = [[notification.userInfo valueForKey:AVAudioSessionRouteChangeReasonKey] intValue];
            AVAudioSessionRouteDescription *routeDescription = [notification.userInfo valueForKey:AVAudioSessionRouteChangePreviousRouteKey];
            
            NSLog(@"Route change:");
            switch (reasonValue) {
                case AVAudioSessionRouteChangeReasonNewDeviceAvailable:
                    NSLog(@"     NewDeviceAvailable");
                    break;
                case AVAudioSessionRouteChangeReasonOldDeviceUnavailable:
                    NSLog(@"     OldDeviceUnavailable");
                    break;
                case AVAudioSessionRouteChangeReasonCategoryChange:
                    NSLog(@"     CategoryChange");
                    NSLog(@"     New Category: %@", [[AVAudioSession sharedInstance] category]);
                    break;
                case AVAudioSessionRouteChangeReasonOverride:
                    NSLog(@"     Override");
                    break;
                case AVAudioSessionRouteChangeReasonWakeFromSleep:
                    NSLog(@"     WakeFromSleep");
                    break;
                case AVAudioSessionRouteChangeReasonNoSuitableRouteForCategory:
                    NSLog(@"     NoSuitableRouteForCategory");
                    break;
                default:
                    NSLog(@"     ReasonUnknown");
            }
            
            NSLog(@"Previous route:\n");
            NSLog(@"%@", routeDescription);
        });

        for (typeof(void(^)(void)) observe_notification in @[observe_audio_session_interruption_notification, observe_audio_route_change_notification]) observe_notification();
        
        !(!error) ?: ^ (NSError ** error_t) {
            printf("Error configuring audio session:\n\t%s\n", [[*error_t debugDescription] UTF8String]);
            NSException* exception = [NSException
                                      exceptionWithName:(*error_t).domain
                                      reason:(*error_t).localizedDescription
                                      userInfo:@{@"Error Code" : @((*error_t).code)}];
            @throw exception;
        }(&error);
    } @catch (NSException *exception) {
        printf("Exception configuring audio session:\n\t%s\n\t%s\n\t%lu",
               [exception.name UTF8String],
               [exception.reason UTF8String],
               ((NSNumber *)[exception.userInfo valueForKey:@"Error Code"]).unsignedIntegerValue);
    }
    
    return session;
};

static AVAudioEngine * (^AudioEngineRef)(AVAudioSourceNode *) = ^(AVAudioSourceNode * audio_source) {
    AVAudioEngine * audio_engine = [[AVAudioEngine alloc] init];
    [audio_engine attachNode:audio_source];
    [audio_engine connect:audio_source to:audio_engine.mainMixerNode format:nil];
    [audio_engine prepare];
    
    return audio_engine;
};

typedef typeof(bool(^)(void)) audio_state;
typedef typeof(audio_state(^)(AVAudioEngine *, AVAudioSession *)) audio_state_ref;
audio_state_ref audio_state_ref_init = ^ (AVAudioEngine * audio_engine, AVAudioSession * audio_session) {
    __block NSError * error = nil;
    return ^ bool {
        return ([audio_session setActive:(((![audio_engine isRunning]) && ^ BOOL { [audio_engine startAndReturnError:&error]; return (!error) ? ([audio_engine isRunning]) : FALSE; }()) || ^ BOOL { [audio_engine stop]; return ([audio_engine isRunning]); }()) error:&error]) & [audio_engine isRunning];
    };
};

typedef typeof(void(^)(audio_state, ...)) audio_control;
audio_control play_pause_button_audio_control = ^ (audio_state audio_state_play_pause_button, ...) {
    
};

audio_control_ref lock_screen_audio_control = ^ (audio_state_ref * audio_state_t, ...) {
    va_list argp;
    va_start(argp, audio_state_t);
    MPNowPlayingInfoCenter * nowPlayingInfoCenter = va_arg(argp, MPNowPlayingInfoCenter *);
    MPRemoteCommandCenter  * remoteCommandCenter  = va_arg(argp, MPRemoteCommandCenter  *);
    
    NSMutableDictionary<NSString *, id> * nowPlayingInfo = [[NSMutableDictionary alloc] initWithCapacity:2];
    [nowPlayingInfo setObject:@"ToneBarrier" forKey:MPMediaItemPropertyTitle];
    [nowPlayingInfo setObject:(NSString *)@"James Alan Bush" forKey:MPMediaItemPropertyArtist];
    [nowPlayingInfo setObject:(NSString *)@"The Life of a Demoniac" forKey:MPMediaItemPropertyAlbumTitle];
    
    static UIImage * image;
    MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithImage:(image = [UIImage imageNamed:@"WaveIcon"])];
    [nowPlayingInfo setObject:(MPMediaItemArtwork *)artwork forKey:MPMediaItemPropertyArtwork];
    
    [(nowPlayingInfoCenter = [MPNowPlayingInfoCenter defaultCenter]) setNowPlayingInfo:(NSDictionary<NSString *,id> * _Nullable)nowPlayingInfo];
    
    MPRemoteCommandHandlerStatus (^remote_command_handler)(MPRemoteCommandEvent * _Nonnull) = ^ MPRemoteCommandHandlerStatus (MPRemoteCommandEvent * _Nonnull event) {
        __block NSError * error = nil;
        [nowPlayingInfoCenter setPlaybackState:(*audio_state_t)()];
        return (!error) ? MPRemoteCommandHandlerStatusSuccess : MPRemoteCommandHandlerStatusCommandFailed;
    };
    
    [[(remoteCommandCenter = [MPRemoteCommandCenter sharedCommandCenter]) playCommand] addTargetWithHandler:remote_command_handler];
    [[remoteCommandCenter stopCommand] addTargetWithHandler:remote_command_handler];
    [[remoteCommandCenter pauseCommand] addTargetWithHandler:remote_command_handler];
    [[remoteCommandCenter togglePlayPauseCommand] addTargetWithHandler:remote_command_handler];
    
    [[UIApplication sharedApplication] beginReceivingRemoteControlEvents];
};

static void (^(^(^update_audio_controls)(MPNowPlayingInfoCenter *, MPRemoteCommandCenter *))(UIButton *))(bool(^)(void)) = ^ (MPNowPlayingInfoCenter * now_playing_info_center, MPRemoteCommandCenter * remote_command_center) {
    return ^ (UIButton * playback_button) {
        return ^ (bool(^audio_state)(void)) {
        };
    };
};
//
//    return ^ (bool(^update_playback_interface)(void)) {
//        dispatch_async(dispatch_get_main_queue(), ^{
//            [playback_button setSelected:update_playback_interface()];
//        });
//    };
//};

@interface ViewController ()



@end

NS_ASSUME_NONNULL_END
